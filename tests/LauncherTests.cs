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
        using (var form=new LauncherForm(true)) {
            var agreement=(CheckBox)form.Controls["DownloadConsent"];
            var install=(Button)form.Controls["InstallButton"];
            var developer=(Button)form.Controls["DeveloperInstallButton"];
            // Neither installation mode may inherit download permission from valid folders alone.
            Require(!agreement.Checked && !install.Enabled && !developer.Enabled,"UI grants consent by default");
            form.Controls["WarcraftDirectory"].Text=wc; form.Controls["CounterStrikeDirectory"].Text=cs;
            form.Controls["InstallDirectory"].Text=destination;
            Require(!install.Enabled && !developer.Enabled,"Folders alone enable install");
            agreement.Checked=true; Require(install.Enabled && developer.Enabled,"Valid agreement/folders do not enable install");
            agreement.Checked=false; Require(!install.Enabled && !developer.Enabled,"Revoked agreement did not disable install");
        }
        // Consent must prevent even extraction/requests, not merely disable a visual control.
        Reject(() => SetupRunner.Install(request,false,Console.WriteLine),"No-consent runner started work");
        Reject(() => SourcePayload.Extract(destination,false),"No-consent payload was extracted");
        Require(!Directory.Exists(destination),"No-consent install wrote files");
        request.InstallDirectory=Path.Combine(wc,"child");Reject(request.ValidateDestination,"Warcraft child allowed");
        request.InstallDirectory=cs;Reject(request.ValidateDestination,"Original CS folder allowed");
        request.InstallDirectory=Path.GetPathRoot(root);Reject(request.ValidateDestination,"Drive root allowed");
        request.InstallDirectory=destination;request.ValidateDestination();
        request.Save(Path.Combine(root,"quoted-paths.json"));
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
        Console.WriteLine("PASS launcher consent, original-directory protection, special-character paths, ZIP traversal and source payload");
        return 0;
    }
}
