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
            Require(!agreement.Checked && !install.Enabled,"UI grants consent by default");
            form.Controls["WarcraftDirectory"].Text=wc; form.Controls["CounterStrikeDirectory"].Text=cs;
            form.Controls["InstallDirectory"].Text=destination;
            Require(!install.Enabled,"Folders alone enable install");
            agreement.Checked=true; Require(install.Enabled,"Valid agreement/folders do not enable install");
            agreement.Checked=false; Require(!install.Enabled,"Revoked agreement did not disable install");
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
        Require(!Directory.Exists(Path.Combine(source,".local")),"Private assets embedded");
        Console.WriteLine("PASS launcher consent, original-directory protection, special-character paths, ZIP traversal and source payload");
        return 0;
    }
}
