using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace WarcraftCSLauncher {
    public sealed class LauncherForm : Form {
        private readonly TextBox warcraft = new TextBox(), cs = new TextBox(), destination = new TextBox();
        private readonly CheckBox consent = new CheckBox();
        private readonly Button install = new Button(), developer = new Button(), update = new Button(), play = new Button(), check = new Button();
        private readonly TextBox log = new TextBox();
        private readonly Label status = new Label();
        private readonly ProgressBar progress = new ProgressBar();
        private bool busy;
        private bool checking;
        private ReleaseUpdate latest;
        private readonly bool preview;
        private readonly bool includesRuntime;
        protected override bool ShowWithoutActivation { get { return preview; } }

        public LauncherForm(bool preview=false,bool? runtimeIncluded=null) {
            this.preview=preview;
            includesRuntime=runtimeIncluded ?? LauncherVariant.IncludesRuntime;
            Text = "Warcraft CS by Yazgul - Launcher";
            ClientSize = new Size(780, 855); MinimumSize = Size; MaximizeBox = false;
            Font = new Font("Segoe UI", 10); AutoScaleMode = AutoScaleMode.Dpi;
            StartPosition = FormStartPosition.CenterScreen;
            // Keep the agreement/actions reachable on small displays and Windows display scaling.
            AutoScroll=true;
            Shown += async (s,e) => {
                if (preview) return;
                var area=Screen.FromControl(this).WorkingArea;
                if (Height > area.Height-30) { MinimumSize=new Size(500,400); Height=area.Height-30; Top=area.Top+15; }
                // Every startup checks release metadata; installation remains an explicit button action.
                await CheckUpdates();
            };
            AddLabel("WARCRAFT CS", 22, 18, 730, 36, new Font("Segoe UI", 22, FontStyle.Bold));
            AddLabel("Version "+CurrentVersion+" — "+(includesRuntime ? "DLL included" : "source-only; DLLs built locally"), 24, 62, 730, 28, Font);
            FolderRow("Warcraft III 1.26a folder", warcraft, 100);
            FolderRow("Counter-Strike 1.6 folder (cstrike or Half-Life)", cs, 172);
            FolderRow("Install Warcraft CS here", destination, 244);
            warcraft.Name="WarcraftDirectory"; cs.Name="CounterStrikeDirectory"; destination.Name="InstallDirectory";
            destination.Text = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WarcraftCS");
            AddLabel("DOWNLOADS AND INSTALLATION", 24, 323, 730, 24, new Font(Font, FontStyle.Bold));
            var explanation = new TextBox {Left=24, Top=353, Width=732, Height=136, Multiline=true, ReadOnly=true,
                BackColor=SystemColors.Control, BorderStyle=BorderStyle.None, TabStop=false,
                Text=(includesRuntime ? "Player: bundled mod DLLs; no Microsoft Build Tools or Windows SDK downloads.\r\n"+
                     "Developer: builds DLLs locally; prepares missing Build Tools / SDK (several GB).\r\n" :
                     "This source-only EXE contains no prebuilt mod DLLs.\r\n"+
                     "Local installation and updates build DLLs; missing Build Tools / SDK may take several GB.\r\n")+
                     "Python (~26 MB if missing) and NumPy convert your local CS assets.\r\n"+
                     "Source builds also obtain MinHook. Existing compatible tools are reused.\r\n"+
                     "Microsoft tools may request administrator approval or a restart.\r\n"+
                     "You supply both games. Downloads and installation require your agreement."};
            Controls.Add(explanation);
            AddLink("Python terms", "https://docs.python.org/3/license.html", 24, 489);
            AddLink("Microsoft terms", "https://visualstudio.microsoft.com/license-terms/", 157, 489);
            // Match the distributed launcher's release when presenting dependency and project notices.
            // Keep notices tied to the version actually running after a launcher update.
            AddLink("Project notices", "https://github.com/YazgulDev/warcraft-cs/blob/v"+CurrentVersion+"/NOTICE", 310, 489);
            consent.SetBounds(24, 522, 732, 36);
            consent.Name="DownloadConsent";
            consent.Text="I agree to download/install project updates and the listed dependencies under their terms.";
            consent.Checked=false; consent.CheckedChanged += (s,e) => RefreshActions(); Controls.Add(consent);
            // Describe the unconditional check without presenting an automatic-install preference.
            AddLabel("New releases are checked at startup and require your confirmation.",24,565,732,30,Font);
            // Source-only builds expose a single Install action; ready DLLs remain an explicit separate download.
            install.Name="InstallButton"; install.Text=includesRuntime ? "Install — Player" : "Install";
            install.SetBounds(24,605,includesRuntime ? 352 : 732,42);
            install.Click += async (s,e) => await Install(includesRuntime ? "Player" : "Developer"); Controls.Add(install);
            developer.Name="DeveloperInstallButton"; developer.Text="Install — Developer"; developer.SetBounds(390,605,366,42);
            developer.Click += async (s,e) => await Install("Developer");
            if (includesRuntime) Controls.Add(developer);
            play.Name="PlayButton"; play.Text="Play"; play.SetBounds(24,665,125,38); play.Click += async (s,e) => await Play(); Controls.Add(play);
            update.Name="UpdateButton"; update.Text="Update"; update.SetBounds(163,665,125,38);
            update.Click += async (s,e) => await UpdateProject(); Controls.Add(update);
            check.Name="CheckUpdatesButton"; check.Text="Check for updates"; check.SetBounds(302,665,170,38); check.Click+=async(s,e)=>await CheckUpdates(); Controls.Add(check);
            progress.SetBounds(578,675,178,18); Controls.Add(progress);
            status.SetBounds(24,719,732,38); status.Text="Choose folders, agree to downloads, then click Install."; Controls.Add(status);
            log.SetBounds(24,761,732,80); log.Multiline=true; log.ReadOnly=true; log.ScrollBars=ScrollBars.Vertical;
            log.Font=new Font("Consolas", 9); Controls.Add(log);
            foreach (var box in new[] {warcraft,cs,destination}) box.TextChanged += (s,e) => RefreshActions();
            FormClosing += (s,e) => { if (busy) { e.Cancel=true; MessageBox.Show(this,"Wait for installation to finish. Vendor installers must not be interrupted."); } };
            if (!preview) LoadPrevious();
            RefreshActions();
        }

        private static string Preferences { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WarcraftCSLauncher", "last-install.json"); } }
        private static string CurrentVersion { get { return Assembly.GetExecutingAssembly().GetName().Version.ToString(3); } }
        private void LoadPrevious() {
            try {
                if (!File.Exists(Preferences)) return;
                var saved=new JavaScriptSerializer().Deserialize<ClientRequest>(File.ReadAllText(Preferences));
                // Restore saved folders only; legacy automatic-install preferences never grant permission.
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
            developer.Enabled=includesRuntime && install.Enabled;
            update.Enabled=install.Enabled && !checking;
            play.Enabled=!busy && File.Exists(Path.Combine(destination.Text,"client-installed.json"));
            check.Enabled=!busy && !checking;
        }
        private void SetBusy(bool value) {
            busy=value; foreach (var box in new[] {warcraft,cs,destination}) box.Enabled=!value; consent.Enabled=!value;
            progress.Style=value ? ProgressBarStyle.Marquee : ProgressBarStyle.Blocks; RefreshActions();
        }
        private void Report(string line) {
            if (InvokeRequired) { BeginInvoke(new Action<string>(Report), line); return; }
            log.AppendText(line+Environment.NewLine);
        }
        private async Task Install(string mode) {
            if (!consent.Checked) return;
            // Install always uses this EXE's reviewed payload; only Update fetches a GitHub release.
            SetBusy(true); log.Clear(); status.Text="Installing... Progress and errors appear below.";
            var request=CurrentRequest(mode);
            try {
                await SetupRunner.Install(request, consent.Checked, Report);
                Directory.CreateDirectory(Path.GetDirectoryName(Preferences)); request.Save(Preferences);
                status.Text="Ready. Press Play, select a living owned unit and press F6.";
            } catch (Exception error) { Report(error.Message); status.Text="Installation incomplete. Check the log and try again."; }
            finally { SetBusy(false); }
        }
        private ClientRequest CurrentRequest(string mode) {
            return new ClientRequest {WarcraftDirectory=warcraft.Text,CounterStrikeDirectory=cs.Text,
                InstallDirectory=destination.Text,InstallMode=mode};
        }
        private string InstalledMode() {
            var marker=Path.Combine(destination.Text,"client-installed.json");
            if (!File.Exists(marker)) return LauncherVariant.InstallationMode(includesRuntime,null);
            var saved=new JavaScriptSerializer().Deserialize<System.Collections.Generic.Dictionary<string,string>>(File.ReadAllText(marker));
            string mode;saved.TryGetValue("install_mode",out mode);
            return LauncherVariant.InstallationMode(includesRuntime,mode);
        }
        private string InstalledRevision() {
            var marker=Path.Combine(destination.Text,"client-installed.json");
            if (!File.Exists(marker)) return null;
            var saved=new JavaScriptSerializer().Deserialize<System.Collections.Generic.Dictionary<string,string>>(File.ReadAllText(marker));
            return saved.ContainsKey("revision") ? saved["revision"] : Path.GetFileName(saved["source"]);
        }
        private async Task CheckUpdates() {
            if (checking || busy || preview) return;
            checking=true; RefreshActions();
            try {
                var candidate=await Task.Run(()=>ReleaseClient.Latest());
                // The metadata request may finish after the user closes the window.
                if (IsDisposed) return;
                latest=candidate!=null && candidate.Manifest.IsNewer(CurrentVersion,SourcePayload.Revision,InstalledRevision()) ? candidate : null;
                // Preserve the same-version legacy guard only for a launcher that actually bundles native modules.
                if (includesRuntime && latest!=null && latest.Manifest.Version==CurrentVersion && !latest.Manifest.SupportsPlayer) latest=null;
                if (busy) return;
                status.Text=latest==null ? "Up to date. Play and offline installation remain available." :
                    "Update available: "+latest.Manifest.Version+". Choose whether to install it.";
                Report(status.Text);
            } catch (Exception error) {
                // API outages/rate limits never disable an existing installation or trigger an unverified download.
                if (IsDisposed) return;
                latest=null; Report("Update check unavailable: "+error.Message);
                if (!busy) status.Text="Update check unavailable. You can still Play or install the embedded version.";
            } finally { checking=false; if (!IsDisposed) RefreshActions(); }
            if (IsDisposed || busy || latest==null) return;
            await ConfirmUpdate();
        }
        private async Task UpdateProject() {
            if (busy || checking || !consent.Checked) return;
            // Update performs a fresh GitHub check and confirms the chosen package; Install never invokes this path.
            await CheckUpdates();
        }
        private async Task ConfirmUpdate() {
            if (latest==null || busy || IsDisposed) return;
            var mode=InstalledMode();
            using (var dialog=new UpdateAvailableForm(latest,mode)) {
                if (dialog.ShowDialog(this)!=DialogResult.OK) { Report("Update postponed. Use Update when ready.");return; }
            }
            consent.Checked=true;await ApplyUpdate(mode);
        }
        private async Task ApplyUpdate(string mode) {
            if (busy || !consent.Checked || latest==null) return;
            var selected=latest; var request=CurrentRequest(mode); SetBusy(true);
            status.Text="Downloading and verifying the release, then updating the private installation...";
            try {
                var package=await Task.Run(()=>ReleaseUpdater.Prepare(selected,request,true,ReleaseClient.Download));
                Report("Verified release "+selected.Manifest.Version+" source and launcher SHA256.");
                await Task.Run(()=>SetupRunner.InstallSource(request,package.SourceDirectory,true,Report,package.LauncherFile));
                Directory.CreateDirectory(Path.GetDirectoryName(Preferences)); request.Save(Preferences);
                latest=null; status.Text="Updated. Saves, settings and original games were preserved.";
                // Restart only after successful runtime setup; a failed setup never replaces the working launcher.
                if (LauncherSelfUpdate.Schedule(package)) { SetBusy(false); Close(); return; }
            } catch (Exception error) { Report(error.Message); status.Text="Update not completed. Check the log; retry when Warcraft is closed."; }
            finally { if (!IsDisposed) SetBusy(false); }
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
