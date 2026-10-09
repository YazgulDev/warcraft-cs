using System;
using System.IO;
using System.IO.Compression;
using System.Windows.Forms;
using WarcraftCSLauncher;

public static class LauncherTests {
    private static void Require(bool value,string message) { if (!value) throw new Exception(message); }
    private static void Reject(Action action,string message) {
        try { action(); } catch (InvalidOperationException) { return; } catch (InvalidDataException) { return; }
        throw new Exception(message);
    }
    [STAThread]
    public static int Main(string[] args) {
        var root=Path.GetFullPath(args[0]);Directory.CreateDirectory(root);
        var wc=Path.Combine(root,"Warcraft's folder & other");var cs=Path.Combine(root,"CS folder");
        Directory.CreateDirectory(wc);Directory.CreateDirectory(cs);
        var destination=Path.Combine(root,"NoConsent");
        var request=new ClientRequest {WarcraftDirectory=wc,CounterStrikeDirectory=cs,InstallDirectory=destination};
        foreach (bool included in new[] {false,true}) using (var form=new LauncherForm(true,included)) {
            var agreement=(CheckBox)form.Controls["DownloadConsent"];
            var install=(Button)form.Controls["InstallButton"];
            var developer=(Button)form.Controls["DeveloperInstallButton"];
            var update=(Button)form.Controls["UpdateButton"];
            var edition=(ComboBox)form.Controls["GameEdition"];
            // Reading is independent of installation consent and is present in both actual launcher variants.
            var logs=(Button)form.Controls["ReadLogsButton"];
            Require(logs!=null && logs.Enabled && logs.Text=="Read logs","Log-reader action missing or consent-gated");
            // Both variants expose the same editions and preserve the previous TFT default.
            Require(edition!=null && edition.DropDownStyle==ComboBoxStyle.DropDownList && edition.Items.Count==2,
                "Edition selector missing or allows arbitrary arguments");
            Require(edition.SelectedIndex==0 && edition.Items[1].ToString()=="Reign of Chaos","Wrong edition default/options");
            edition.SelectedIndex=1;
            Require(edition.SelectedItem.ToString()=="Reign of Chaos","RoC cannot be selected");
            // Separate installation and GitHub update actions must exist in both actual launcher layouts.
            Require(install.Text==(included ? "Install — Player" : "Install"),"Wrong Install action for variant");
            Require((developer!=null)==included,"Source-only UI offers bundled Player/Developer modes");
            Require(update!=null && update.Text=="Update","Dedicated Update action missing");
            // Neither installation mode may inherit download permission from valid folders alone.
            Require(!agreement.Checked && !install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"UI grants consent by default");
            form.Controls["WarcraftDirectory"].Text=wc; form.Controls["CounterStrikeDirectory"].Text=cs;
            form.Controls["InstallDirectory"].Text=destination;
            Require(!install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"Folders alone enable install");
            agreement.Checked=true; Require(install.Enabled && update.Enabled && (developer==null || developer.Enabled),"Valid agreement/folders do not enable actions");
            agreement.Checked=false; Require(!install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"Revoked agreement did not disable actions");
        }
        Require(LauncherVariant.InstallationMode(false,"Player")=="Developer","Source-only updater selects Player");
        Require(LauncherVariant.InstallationMode(true,"Player")=="Player","Included updater changes Player mode");
        Require(LauncherVariant.InstallationMode(true,"Developer")=="Developer","Included updater forgets explicit source mode");
        // Consent must prevent even extraction/requests, not merely disable a visual control.
        Reject(() => SetupRunner.Install(request,false,Console.WriteLine),"No-consent runner started work");
        Reject(() => SourcePayload.Extract(destination,false),"No-consent payload was extracted");
        Require(!Directory.Exists(destination),"No-consent install wrote files");
        request.InstallDirectory=Path.Combine(wc,"child");Reject(request.ValidateDestination,"Warcraft child allowed");
        request.InstallDirectory=cs;Reject(request.ValidateDestination,"Original CS folder allowed");
        request.InstallDirectory=Path.GetPathRoot(root);Reject(request.ValidateDestination,"Drive root allowed");
        request.InstallDirectory=destination;request.ValidateDestination();
        // Legacy JSON keeps TFT; serialization round-trips RoC without treating data as shell arguments.
        var serializer=new System.Web.Script.Serialization.JavaScriptSerializer();
        Require(serializer.Deserialize<ClientRequest>("{}").GameEdition==GameEdition.FrozenThrone,"Legacy preferences change edition");
        request.GameEdition=GameEdition.ReignOfChaos;
        request.Save(Path.Combine(root,"quoted-paths.json"));
        Require(serializer.Deserialize<ClientRequest>(File.ReadAllText(Path.Combine(root,"quoted-paths.json"))).GameEdition==GameEdition.ReignOfChaos,
            "RoC preference not saved");
        Require(GameEdition.LaunchArguments(GameEdition.ReignOfChaos)=="-opengl -classic","RoC launch argument missing");
        Require(GameEdition.LaunchArguments(null)=="-opengl","Legacy launch default wrong");
        Reject(()=>GameEdition.LaunchArguments("ReignOfChaos; arbitrary command"),"Unvalidated edition reaches launch arguments");
        // Launching an older installed revision still selects RoC and opts out of DPI bitmap scaling.
        foreach (string selected in new[] {GameEdition.FrozenThrone,GameEdition.ReignOfChaos}) {
            var start=GameLaunch.CreateStartInfo(destination,selected);
            Require(start.FileName==Path.Combine(destination,"Game","war3.exe") && !start.UseShellExecute,"Play escapes private game");
            Require(start.WorkingDirectory==Path.Combine(destination,"Game"),"Wrong game working directory");
            Require(start.Arguments==GameEdition.LaunchArguments(selected),"Selected edition lost during Play");
            Require(start.EnvironmentVariables["__COMPAT_LAYER"].Contains("HIGHDPIAWARE"),"Fullscreen Play permits DPI bitmap scaling");
        }
        Require(File.ReadAllText(Path.Combine(root,"quoted-paths.json")).Contains("Warcraft"),"Request serialization failed");
        using (var memory=new MemoryStream()) {
            using (var zip=new ZipArchive(memory,ZipArchiveMode.Create,true)) { zip.CreateEntry("../escaped.txt"); }
            memory.Position=0;
            using (var zip=new ZipArchive(memory,ZipArchiveMode.Read)) Reject(() => SourcePayload.ExtractArchive(zip,Path.Combine(root,"traversal")),"Zip traversal allowed");
        }
        Require(!File.Exists(Path.Combine(root,"escaped.txt")),"Archive escaped extraction root");
        var source=SourcePayload.Extract(destination,true);
        Require(File.Exists(Path.Combine(source,"launcher","install-client.ps1")),"Embedded installer missing");
        Require(File.Exists(Path.Combine(source,"src","Plugin.cpp")),"Embedded owned sources missing");
        // Clients must receive the audio-copy policy, while the codecs themselves stay owner-supplied.
        Require(File.Exists(Path.Combine(source,"setup","audio-runtime.ps1")),"Embedded Miles setup missing");
        Require(!Directory.Exists(Path.Combine(source,".local")),"Private assets embedded");
        // Cancel/close declines updates; the prompt exposes both the release information and mode-specific downloads.
        var release=new ReleaseUpdate {Manifest=new ReleaseManifest {Version="0.5.0",RuntimeSha256=new string('a',64)},Notes="New tree and weapon fixes."};
        using(var prompt=new UpdateAvailableForm(release,"Player",true)) {
            Require(((TextBox)prompt.Controls["ReleaseNotes"]).Text==release.Notes,"Release notes missing");
            Require(((Label)prompt.Controls["ModeDetails"]).Text.Contains("No Build Tools"),"Player prompt hides dependencies");
            Require(((Button)prompt.Controls["DeclineUpdate"]).DialogResult==DialogResult.Cancel,"Decline confirms update");
            Require(((Button)prompt.Controls["ConfirmUpdate"]).DialogResult==DialogResult.OK,"Confirm does not accept update");
        }
        release.Manifest.RuntimeSha256=null;
        using(var legacy=new UpdateAvailableForm(release,"Player",true)) Require(!((Button)legacy.Controls["ConfirmUpdate"]).Enabled,"Player prompt offers legacy source build");
        using(var dev=new UpdateAvailableForm(release,"Developer",true)) Require(((Label)dev.Controls["ModeDetails"]).Text.Contains("several GB"),"Developer prompt hides SDK download");
        // Real journal discovery/reader/export work with Unicode, live writers and retained sessions.
        var logRoot=Path.Combine(root,"log-reader");var gameLogs=Path.Combine(logRoot,"Game","WarcraftCS");
        Directory.CreateDirectory(gameLogs);
        var runtimeLog=Path.Combine(gameLogs,"WarcraftCS.log");
        File.WriteAllText(runtimeLog,"Прицел: hip-fire\r\n",new System.Text.UTF8Encoding(false));
        File.WriteAllText(Path.Combine(gameLogs,"WarcraftCS.1.log"),"previous session");
        File.WriteAllText(Path.Combine(logRoot,"install.log"),"installer");
        var replacement=Path.Combine(logRoot,"updates","revision-test");Directory.CreateDirectory(replacement);
        File.WriteAllText(Path.Combine(replacement,"launcher-replacement.log"),"replacement");
        var launcherLogs=Path.Combine(logRoot,"launcher-journal");LauncherLog.Append(launcherLogs,"session-start");
        Require(LogCatalog.List(logRoot,launcherLogs).Count==5,"Known log sessions/installer/launcher/replacement missing");
        Require(LogCatalog.List(Path.Combine(root,"no-log-installation"),null).Count==0,"Empty installation cannot be read safely");
        using(var writer=new FileStream(runtimeLog,FileMode.Append,FileAccess.Write,FileShare.ReadWrite|FileShare.Delete)) {
            byte[] line=System.Text.Encoding.UTF8.GetBytes("live-append\r\n");writer.Write(line,0,line.Length);writer.Flush();
            Require(LogCatalog.ReadTail(runtimeLog).Contains("Прицел") && LogCatalog.ReadTail(runtimeLog).Contains("live-append"),"Live UTF-8 journal is unreadable");
        }
        var large=Path.Combine(gameLogs,"WarcraftCS.2.log");File.WriteAllText(large,new string('x',600000)+"\nlast-complete-line\n");
        var tail=LogCatalog.ReadTail(large);Require(tail.Length<513000 && tail.Contains("last-complete-line") && tail.Contains("last 512 KiB"),"Viewer tail is not bounded");
        Require(tail.Contains("last-complete-line\r\n"),"LF-only log lines will join together in the WinForms reader");
        var exported=Path.Combine(logRoot,"full-export.log");LogCatalog.Export(large,exported);
        Require(File.ReadAllText(exported)==File.ReadAllText(large),"Full export omitted the hidden log prefix");
        bool exportRejected=false;try { LogCatalog.Export(runtimeLog,runtimeLog); } catch(IOException) { exportRejected=true; }
        Require(exportRejected,"Export overwrote the live log");
        using(var viewer=new LogViewerForm(logRoot,true)) {
            viewer.StartPosition=FormStartPosition.Manual;viewer.Location=new System.Drawing.Point(-10000,-10000);viewer.Show();Application.DoEvents();
            viewer.RefreshFiles();
            var text=(TextBox)viewer.Controls["LogContents"];
            Require(text.ReadOnly && text.Text.Contains("live-append"),"Actual reader does not open the current game log");
            File.AppendAllText(runtimeLog,"next-refresh\n");viewer.RefreshFiles();
            Require(text.Text.Contains("next-refresh"),"Refresh did not reload a changed log");viewer.Close();
        }
        // Launcher rotation is shared with live readers and never mixes diagnostics with player configuration.
        var launcherFile=Path.Combine(launcherLogs,"launcher.log");File.WriteAllText(launcherFile,new string('l',4*1024*1024));
        using(var reader=new FileStream(launcherFile,FileMode.Open,FileAccess.Read,FileShare.ReadWrite|FileShare.Delete)) LauncherLog.Append(launcherLogs,"rotated-launcher");
        Require(File.Exists(Path.Combine(launcherLogs,"launcher.1.log")) && File.ReadAllText(launcherFile).Contains("rotated-launcher"),"Launcher rotation failed with a live reader");
        Console.WriteLine("PASS launcher consent, original-directory protection, special-character paths, ZIP traversal, source payload and live log viewer/export");
        return 0;
    }
}
