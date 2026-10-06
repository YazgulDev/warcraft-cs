using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace WarcraftCSLauncher {
    public sealed class LauncherForm : Form {
        private readonly TextBox warcraft = new TextBox(), cs = new TextBox(), destination = new TextBox();
        private readonly CheckBox consent = new CheckBox();
        private readonly Button install = new Button(), play = new Button();
        private readonly TextBox log = new TextBox();
        private readonly Label status = new Label();
        private readonly ProgressBar progress = new ProgressBar();
        private bool busy;
        private readonly bool preview;
        protected override bool ShowWithoutActivation { get { return preview; } }

        public LauncherForm(bool preview=false) {
            this.preview=preview;
            Text = "Warcraft CS by Yazgul - Launcher";
            ClientSize = new Size(780, 730); MinimumSize = Size; MaximizeBox = false;
            Font = new Font("Segoe UI", 10); AutoScaleMode = AutoScaleMode.Dpi;
            StartPosition = FormStartPosition.CenterScreen;
            // Keep the agreement/actions reachable on small displays and Windows display scaling.
            AutoScroll=true;
            Shown += (s,e) => {
                if (preview) return;
                var area=Screen.FromControl(this).WorkingArea;
                if (Height > area.Height-30) { MinimumSize=new Size(500,400); Height=area.Height-30; Top=area.Top+15; }
            };
            AddLabel("WARCRAFT CS", 22, 18, 730, 36, new Font("Segoe UI", 22, FontStyle.Bold));
            AddLabel("Select your own games. Setup creates a separate copy for offline play.", 24, 62, 730, 28, Font);
            FolderRow("Warcraft III 1.26a folder", warcraft, 100);
            FolderRow("Counter-Strike 1.6 folder (cstrike or Half-Life)", cs, 172);
            FolderRow("Install Warcraft CS here", destination, 244);
            warcraft.Name="WarcraftDirectory"; cs.Name="CounterStrikeDirectory"; destination.Name="InstallDirectory";
            destination.Text = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WarcraftCS");
            AddLabel("DOWNLOADS AND INSTALLATION", 24, 323, 730, 24, new Font(Font, FontStyle.Bold));
            var explanation = new TextBox {Left=24, Top=353, Width=732, Height=106, Multiline=true, ReadOnly=true,
                BackColor=SystemColors.Control, BorderStyle=BorderStyle.None, TabStop=false,
                Text="If missing: Python (~26 MB), Microsoft C++ Build Tools and Windows SDK (several GB).\r\n"+
                     "Also: NumPy from PyPI and MinHook source from GitHub. No games are downloaded.\r\n"+
                     "Microsoft tools may ask for administrator approval or a restart. Existing tools are reused.\r\n"+
                     "CS is required for local models/sounds. Dependency downloads start only after consent."};
            Controls.Add(explanation);
            AddLink("Python terms", "https://docs.python.org/3/license.html", 24, 456);
            AddLink("Microsoft terms", "https://visualstudio.microsoft.com/license-terms/", 157, 456);
            // Match the distributed launcher's release when presenting dependency and project notices.
            AddLink("Project notices", "https://github.com/YazgulDev/warcraft-cs/blob/release/0.3.0/NOTICE", 310, 456);
            consent.SetBounds(24, 489, 732, 36);
            consent.Name="DownloadConsent";
            consent.Text="I agree to download and install the listed dependencies under their license terms.";
            consent.Checked=false; consent.CheckedChanged += (s,e) => RefreshActions(); Controls.Add(consent);
            install.Name="InstallButton"; install.Text="Install / Update"; install.SetBounds(24, 538, 180, 38); install.Click += async (s,e) => await Install(); Controls.Add(install);
            play.Name="PlayButton"; play.Text="Play"; play.SetBounds(218, 538, 140, 38); play.Click += async (s,e) => await Play(); Controls.Add(play);
            progress.SetBounds(378, 548, 378, 18); Controls.Add(progress);
            status.SetBounds(24, 587, 732, 26); status.Text="Choose folders and agree to downloads to enable Install."; Controls.Add(status);
            log.SetBounds(24, 620, 732, 90); log.Multiline=true; log.ReadOnly=true; log.ScrollBars=ScrollBars.Vertical;
            log.Font=new Font("Consolas", 9); Controls.Add(log);
            foreach (var box in new[] {warcraft,cs,destination}) box.TextChanged += (s,e) => RefreshActions();
            FormClosing += (s,e) => { if (busy) { e.Cancel=true; MessageBox.Show(this,"Wait for installation to finish. Vendor installers must not be interrupted."); } };
            LoadPrevious(); RefreshActions();
        }

        private static string Preferences { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WarcraftCSLauncher", "last-install.json"); } }
        private void LoadPrevious() {
            try {
                if (!File.Exists(Preferences)) return;
                var saved=new JavaScriptSerializer().Deserialize<ClientRequest>(File.ReadAllText(Preferences));
                warcraft.Text=saved.WarcraftDirectory; cs.Text=saved.CounterStrikeDirectory; destination.Text=saved.InstallDirectory;
            } catch { status.Text="Previous folder settings could not be loaded. Select folders again."; }
        }
        private void FolderRow(string title, TextBox box, int y) {
            AddLabel(title,24,y,730,24,Font); box.SetBounds(24,y+27,619,30); Controls.Add(box);
            var browse=new Button {Text="Browse...", Left=656, Top=y+26, Width=100, Height=31};
            browse.Click += (s,e) => { if (busy) return; using (var dialog=new FolderBrowserDialog {Description=title, SelectedPath=box.Text})
                if (dialog.ShowDialog(this)==DialogResult.OK) box.Text=dialog.SelectedPath; };
            Controls.Add(browse);
        }
        private void AddLabel(string text,int x,int y,int width,int height,Font font) {
            Controls.Add(new Label {Text=text, Left=x, Top=y, Width=width, Height=height, Font=font});
        }
        private void AddLink(string text,string url,int x,int y) {
            var link=new LinkLabel {Text=text,Left=x,Top=y,Width=145,Height=25};
            link.LinkClicked += (s,e) => Process.Start(new ProcessStartInfo(url) {UseShellExecute=true}); Controls.Add(link);
        }
        private void RefreshActions() {
            install.Enabled=!busy && consent.Checked && Directory.Exists(warcraft.Text) && Directory.Exists(cs.Text) && !String.IsNullOrWhiteSpace(destination.Text);
            play.Enabled=!busy && File.Exists(Path.Combine(destination.Text,"client-installed.json"));
        }
        private void SetBusy(bool value) {
            busy=value; foreach (var box in new[] {warcraft,cs,destination}) box.Enabled=!value; consent.Enabled=!value;
            progress.Style=value ? ProgressBarStyle.Marquee : ProgressBarStyle.Blocks; RefreshActions();
        }
        private void Report(string line) {
            if (InvokeRequired) { BeginInvoke(new Action<string>(Report), line); return; }
            log.AppendText(line+Environment.NewLine);
        }
        private async Task Install() {
            if (!consent.Checked) return;
            SetBusy(true); log.Clear(); status.Text="Installing... Progress and errors appear below.";
            var request=new ClientRequest {WarcraftDirectory=warcraft.Text,CounterStrikeDirectory=cs.Text,InstallDirectory=destination.Text};
            try {
                await SetupRunner.Install(request, consent.Checked, Report);
                Directory.CreateDirectory(Path.GetDirectoryName(Preferences)); request.Save(Preferences);
                status.Text="Ready. Press Play, select a living owned unit and press F6.";
            } catch (Exception error) { Report(error.Message); status.Text="Installation incomplete. Check the log and try again."; }
            finally { SetBusy(false); }
        }
        private async Task Play() {
            SetBusy(true);
            try {
                var saved=new JavaScriptSerializer().Deserialize<System.Collections.Generic.Dictionary<string,string>>(File.ReadAllText(Path.Combine(destination.Text,"client-installed.json")));
                var source=Path.GetFullPath(saved["source"]);
                var allowed=Path.GetFullPath(Path.Combine(destination.Text,"sources"))+Path.DirectorySeparatorChar;
                if (!source.StartsWith(allowed,StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("Invalid installed source path. Run Install again.");
                // Play launches only the prepared private runtime; it does not install/download dependencies.
                await Task.Run(() => SetupRunner.Run(Path.Combine(source,"tools","launch.ps1"),"",Report));
                status.Text="Warcraft started. Select your unit and press F6.";
            } catch (Exception error) { Report(error.Message); status.Text="Could not start Warcraft. Read the log."; }
            finally { SetBusy(false); }
        }
    }
}
