using System;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

namespace WarcraftCSLauncher {
    public sealed class LogViewerForm : Form {
        private readonly string installation;
        private readonly bool preview;
        private readonly ComboBox files = new ComboBox();
        private readonly TextBox contents = new TextBox();
        private readonly Label status = new Label();
        private readonly CheckBox live = new CheckBox();
        private readonly Timer timer = new Timer();
        protected override bool ShowWithoutActivation { get { return preview; } }
        public LogViewerForm(string installation, bool preview = false) {
            this.installation = installation; this.preview = preview;
            Text = "Warcraft CS — Logs"; ClientSize = new Size(960, 600); MinimumSize = new Size(640, 400);
            Font = new Font("Segoe UI", 10); AutoScaleMode = AutoScaleMode.Dpi;
            var actions = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 80, Padding = new Padding(8), WrapContents = true };
            files.Name = "LogFiles"; files.Width = 420; files.DropDownStyle = ComboBoxStyle.DropDownList;
            files.DisplayMember = "Key"; files.ValueMember = "Value";
            files.SelectedIndexChanged += (s, e) => ReadSelected(); actions.Controls.Add(files);
            var refresh = new Button { Name = "RefreshLogs", Text = "Refresh", AutoSize = true };
            refresh.Click += (s, e) => RefreshFiles(); actions.Controls.Add(refresh);
            var copy = new Button { Name = "CopyLog", Text = "Copy text", AutoSize = true };
            copy.Click += (s, e) => { try { if (contents.TextLength != 0) Clipboard.SetText(contents.Text); } catch (Exception error) { status.Text = error.Message; } };
            actions.Controls.Add(copy);
            var save = new Button { Name = "SaveLog", Text = "Save full log...", AutoSize = true };
            save.Click += (s, e) => ExportSelected(); actions.Controls.Add(save);
            live.Name = "LiveLogs"; live.Text = "Auto refresh (2s)"; live.Checked = true; live.AutoSize = true; actions.Controls.Add(live);
            contents.Name = "LogContents"; contents.Dock = DockStyle.Fill; contents.Multiline = true; contents.ReadOnly = true;
            contents.WordWrap = false; contents.ScrollBars = ScrollBars.Both; contents.Font = new Font("Consolas", 10);
            // Two footer lines need room for both the path and refresh time at the launcher font size.
            status.Dock = DockStyle.Bottom; status.Height = 64; status.Padding = new Padding(8);
            status.AutoEllipsis = true;
            Controls.Add(contents); Controls.Add(actions); Controls.Add(status);
            // Reading requires no installation/download consent and can run alongside the game.
            RefreshFiles(); timer.Interval = 2000; timer.Tick += (s, e) => { if (live.Checked) ReadSelected(); };
            if (!preview) timer.Start();
            FormClosed += (s, e) => timer.Dispose();
        }
        public void RefreshFiles() {
            string previous = files.SelectedValue as string;
            var entries = LogCatalog.List(installation, preview ? null : LauncherLog.DirectoryPath);
            files.DataSource = entries;
            if (previous != null) files.SelectedValue = previous;
            else {
                // Open the current game session by default; launcher logs still work before the first game launch.
                int current = entries.FindIndex(entry => Path.GetFileName(entry.Value) == "WarcraftCS.log");
                if (current >= 0) files.SelectedIndex = current;
            }
            if (files.SelectedIndex < 0 && entries.Count > 0) files.SelectedIndex = 0;
            if (entries.Count == 0) { contents.Clear(); status.Text = "No logs yet. Install or start the game, then press Refresh."; }
            else ReadSelected();
        }
        private void ReadSelected() {
            string path = files.SelectedValue as string; if (path == null) return;
            try {
                var text = LogCatalog.ReadTail(path);
                if (contents.Text != text) {
                    // Keep selected text; follow appended lines only when already at the end.
                    int start = contents.SelectionStart, length = contents.SelectionLength;
                    bool atEnd = start == contents.TextLength && length == 0;
                    contents.Text = text;
                    contents.Select(atEnd ? contents.TextLength : Math.Min(start, contents.TextLength), Math.Min(length, Math.Max(0, contents.TextLength - start)));
                    if (atEnd) contents.ScrollToCaret();
                }
                status.Text = Path.GetFileName(path) + " — Updated " + DateTime.Now.ToString("HH:mm:ss") + "\r\n" + path;
            } catch (IOException error) { status.Text = "Log unavailable (it may have rotated): " + error.Message; }
              catch (UnauthorizedAccessException error) { status.Text = error.Message; }
        }
        private void ExportSelected() {
            string path = files.SelectedValue as string; if (path == null) return;
            using (var dialog = new SaveFileDialog { Filter = "Log files (*.log)|*.log|Text files (*.txt)|*.txt", FileName = Path.GetFileName(path) }) {
                if (dialog.ShowDialog(this) != DialogResult.OK) return;
                try { LogCatalog.Export(path, dialog.FileName); status.Text = "Saved full log: " + dialog.FileName; }
                catch (Exception error) { status.Text = error.Message; }
            }
        }
    }
}
