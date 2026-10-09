using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Web.Script.Serialization;
using WarcraftCSLauncher;

public static class LauncherUpdateTests {
    private static void Require(bool condition,string message) { if (!condition) throw new Exception(message); }
    private static void Reject(Action action,string message) {
        try { action(); } catch (InvalidDataException) { return; } catch (InvalidOperationException) { return; }
        throw new Exception(message);
    }
    private static byte[] Archive(bool traversal=false) {
        using (var memory=new MemoryStream()) {
            using (var zip=new ZipArchive(memory,ZipArchiveMode.Create,true)) {
                foreach (string name in new[] {"VERSION","launcher/install-client.ps1","launcher/replace-launcher.ps1","src/Plugin.cpp"})
                    using (var writer=new StreamWriter(zip.CreateEntry(name).Open())) writer.Write(name=="VERSION" ? "0.3.0" : "owned source fixture");
                if (traversal) zip.CreateEntry("../escaped.txt");
            }
            return memory.ToArray();
        }
    }
    private static ReleaseUpdate Package(byte[] source,byte[] exe) {
        return new ReleaseUpdate {Manifest=new ReleaseManifest {Version="0.3.0",Revision=ReleaseManifest.Hash(source),
            SourceSha256=ReleaseManifest.Hash(source),LauncherSha256=ReleaseManifest.Hash(exe)},
            SourceUrl="https://github.com/YazgulDev/warcraft-cs/releases/download/v0.3.0/WarcraftCS-sources.zip",
            LauncherUrl="https://github.com/YazgulDev/warcraft-cs/releases/download/v0.3.0/WarcraftCSLauncher.exe"};
    }
    [STAThread]
    public static int Main(string[] args) {
        // A disposable process named war3 validates the idle guard without interacting with the real game.
        if (args.Length==2 && args[0]=="--idle-fixture") { File.WriteAllText(args[1],"ready"); Thread.Sleep(30000); return 0; }
        string root=Path.GetFullPath(args[0]); Directory.CreateDirectory(root);
        string wc=Path.Combine(root,"Original WC"),cs=Path.Combine(root,"Original CS");Directory.CreateDirectory(wc);Directory.CreateDirectory(cs);
        var request=new ClientRequest {WarcraftDirectory=wc,CounterStrikeDirectory=cs,InstallDirectory=Path.Combine(root,"Client"),InstallMode="Developer"};
        // Use a real source-only assembly: the updater now inspects the candidate for unexpected runtime resources.
        byte[] source=Archive(),exe=File.ReadAllBytes(Assembly.GetExecutingAssembly().Location); var update=Package(source,exe);
        // Manual Update must repair an already-current installation; automatic checks stay quiet.
        Require(UpdateSelection.Select(update,"0.3.0",update.Manifest.Revision,update.Manifest.Revision,false,false)==null,
            "Startup reoffers an identical release");
        Require(UpdateSelection.Select(update,"0.3.0",update.Manifest.Revision,update.Manifest.Revision,false,true)==update,
            "Manual Update cannot reinstall an identical release");
        Require(UpdateSelection.Select(null,"0.3.0",update.Manifest.Revision,null,false,true)==null,
            "Unavailable metadata selected an unverified reinstall");
        Require(UpdateSelection.Select(update,"0.3.0",update.Manifest.Revision,null,true,false)==null &&
            UpdateSelection.Select(update,"0.3.0",update.Manifest.Revision,null,true,true)==update,
            "Legacy same-version checks and explicit repair are not separated");
        update.Manifest.Validate("v0.3.0");
        Require(update.Manifest.IsNewer("0.2.0",update.Manifest.Revision,null),"New release skipped");
        Require(!update.Manifest.IsNewer("0.4.0","old",null),"Downgrade allowed");
        Require(!update.Manifest.IsNewer("0.3.0",update.Manifest.Revision,update.Manifest.Revision),"Same build loops");
        Require(update.Manifest.IsNewer("0.3.0",update.Manifest.Revision,"old"),"Old runtime with new launcher skipped");
        Require(update.Manifest.IsNewer("0.3.0","old",null),"Same-version repaired build skipped");
        Require(ReleaseManifest.ParseVersion("0.10.0")>ReleaseManifest.ParseVersion("0.9.0"),"Versions compared lexically");
        Reject(()=>ReleaseManifest.ParseVersion("0.3.0-beta"),"Prerelease accepted as stable");
        Reject(()=>update.Manifest.Validate("v0.4.0"),"Tag/manifest mismatch accepted");
        Reject(()=>ReleaseClient.ValidateAssetUrl("https://github.com/Other/project/releases/download/v0.3.0/WarcraftCSLauncher.exe","v0.3.0","WarcraftCSLauncher.exe"),"Foreign repository executable allowed");
        int downloads=0; Func<string,byte[]> fetch=url=> { downloads++; return url.EndsWith(".zip") ? source : exe; };
        Reject(()=>ReleaseUpdater.Prepare(update,request,false,fetch),"No-consent update allowed");
        Require(downloads==0 && !Directory.Exists(request.InstallDirectory),"No-consent update wrote/downloaded");
        // A legacy release must not silently turn Player installation into compiler/SDK downloads.
        request.InstallMode="Player";
        Reject(()=>ReleaseUpdater.Prepare(update,request,true,fetch),"Legacy release accepted as Player");
        Require(downloads==0 && !Directory.Exists(request.InstallDirectory),"Legacy Player rejection downloaded packages");
        request.InstallMode="Developer";
        Reject(()=>ReleaseUpdater.Prepare(update,request,true,url=>new byte[] {1,2,3}),"Corrupt package accepted");
        Require(!Directory.Exists(request.InstallDirectory),"Corrupt package touched the installation");
        var unsafeSource=Archive(true);var unsafeUpdate=Package(unsafeSource,exe);
        Reject(()=>ReleaseUpdater.Prepare(unsafeUpdate,request,true,url=>url.EndsWith(".zip") ? unsafeSource : exe),"Traversal archive accepted");
        Require(!File.Exists(Path.Combine(root,"escaped.txt")),"Archive escaped its source folder");
        var serializer=new JavaScriptSerializer();
        var assets=new List<object>();
        foreach (string name in new[] {"WarcraftCS-update.json","WarcraftCS-sources.zip","WarcraftCSLauncher.exe"})
            assets.Add(new {name=name,browser_download_url="https://github.com/YazgulDev/warcraft-cs/releases/download/v0.3.0/"+name});
        string metadata=serializer.Serialize(new {draft=false,prerelease=false,tag_name="v0.3.0",assets=assets});
        // Windows PowerShell emits a UTF-8 BOM; verify the real manifest format works in the client parser.
        var parsed=ReleaseClient.Parse(metadata,url=>Encoding.UTF8.GetBytes("\uFEFF"+serializer.Serialize(update.Manifest)));
        Require(parsed.Manifest.Revision==update.Manifest.Revision,"Release API/manifest parsing failed");
        // The rate-limit fallback must preserve the stable-version/checksum contract and canonical package URLs.
        var fallback=ReleaseClient.ParseLatestManifest(Encoding.UTF8.GetBytes("\uFEFF"+serializer.Serialize(update.Manifest)));
        Require(fallback.SourceUrl==update.SourceUrl && fallback.LauncherUrl==update.LauncherUrl,"Fallback changes package origin");
        // Markdown wraps and Unix newlines must not collapse separate change items in the Windows textbox.
        string markdown="* **Added** sound volume control\n  and F8 reload.\n- Fixed the update change list.";
        string formatted="- Added sound volume control and F8 reload.\r\n- Fixed the update change list.";
        Require(ReleaseNotesText.Format(markdown)==formatted,"Per-line Markdown formatting failed");
        Require(ReleaseNotesText.Format(formatted)==formatted,"Resolved notes must remain stable when displayed");
        string legacyMetadata=serializer.Serialize(new {draft=false,prerelease=false,tag_name="v0.3.0",assets=assets,body=markdown});
        Require(ReleaseClient.Parse(legacyMetadata,url=>Encoding.UTF8.GetBytes(serializer.Serialize(update.Manifest))).Notes==formatted,
            "Legacy GitHub body changes lost");
        var knownManifest=Package(source,exe).Manifest;knownManifest.Version="0.6.1";
        Require(ReleaseClient.ParseLatestManifest(Encoding.UTF8.GetBytes(serializer.Serialize(knownManifest))).Notes.Contains("CSVolumePercent"),
            "Legacy manifest fallback lacks the published version's changes");
        Require(!fallback.Notes.Contains("CSVolumePercent"),"Unknown version reuses another release's changes");
        // Reproduce the screenshot with the real full release body on a successful GitHub API response.
        string fullBody=File.ReadAllText(args[1]);
        var knownAssets=new List<object>();
        foreach (string name in new[] {"WarcraftCS-update.json","WarcraftCS-sources.zip",LauncherVariant.SourceFile,LauncherVariant.IncludedFile})
            knownAssets.Add(new {name=name,browser_download_url="https://github.com/YazgulDev/warcraft-cs/releases/download/v0.6.1/"+name});
        knownManifest.DllIncludedLauncherSha256=new string('b',64);knownManifest.DllIncludedRuntimeSha256=new string('c',64);
        string knownMetadata=serializer.Serialize(new {draft=false,prerelease=false,tag_name="v0.6.1",assets=knownAssets,body=fullBody});
        string cached=ReleaseNotesText.Resolve(knownManifest,null);
        foreach (bool included in new[] {false,true}) {
            var apiNotes=ReleaseClient.Parse(knownMetadata,url=>Encoding.UTF8.GetBytes(serializer.Serialize(knownManifest)),included).Notes;
            var fallbackNotes=ReleaseClient.ParseLatestManifest(Encoding.UTF8.GetBytes(serializer.Serialize(knownManifest)),included).Notes;
            Require(apiNotes==cached && fallbackNotes==cached && apiNotes.Split(new[] {"\r\n"},StringSplitOptions.None).Length==6,
                "Full GitHub body overrides compact changes in one variant/API path");
        }
        Require(ReleaseNotesText.Format(fullBody)=="" && ReleaseNotesText.Format("- Добавлен параметр звука.")=="" &&
            ReleaseNotesText.Format("- Added sound control.\n  ```ini\n  Volume=50\n  ```")=="", "Documentation or non-English summaries accepted");
        string markedBody=fullBody+"\n<!-- launcher-summary\n"+markdown+"\n-->\n";
        Require(ReleaseNotesText.Resolve(knownManifest,markedBody)==formatted,"Explicit launcher metadata was ignored");
        knownManifest.ReleaseNotes="- Добавлен параметр звука.";
        Require(ReleaseNotesText.Resolve(knownManifest,fullBody)==cached,"Old Russian manifest bypasses English fallback");
        knownManifest.ReleaseNotes=null;
        Require(ReleaseNotesText.Resolve(update.Manifest,fullBody)==fallback.Notes && fallback.Notes.StartsWith("A short change list"),
            "Unknown version exposes full docs or a non-English unavailable message");
        Require(ReleaseNotesText.Resolve(update.Manifest,markedBody)==formatted,"Unknown version loses explicit launcher metadata");
        Reject(()=>ReleaseClient.ParseLatestManifest(Encoding.UTF8.GetBytes("{\"Version\":\"0.3.0-beta\"}")),"Fallback permits unstable manifest");
        var runtimeDigest=update.Manifest.RuntimeSha256;
        update.Manifest.RuntimeSha256="invalid";Reject(()=>update.Manifest.Validate("v0.3.0"),"Malformed Player hash accepted");
        update.Manifest.RuntimeSha256=runtimeDigest;
        // Dual releases keep independent hashes and preserve source-only / DLL-included choices on both API paths.
        var dual=Package(source,exe);
        dual.Manifest.ReleaseNotes="- Added sound volume control.\n- Fixed the update change list.";
        dual.Manifest.DllIncludedLauncherSha256=new string('b',64);
        dual.Manifest.DllIncludedRuntimeSha256=new string('c',64);
        dual.Manifest.Validate("v0.3.0");
        var dualAssets=new List<object>(assets);
        dualAssets.Add(new {name=LauncherVariant.IncludedFile,browser_download_url="https://github.com/YazgulDev/warcraft-cs/releases/download/v0.3.0/"+LauncherVariant.IncludedFile});
        string dualMetadata=serializer.Serialize(new {draft=false,prerelease=false,tag_name="v0.3.0",assets=dualAssets});
        Func<string,byte[]> dualFetch=url=>Encoding.UTF8.GetBytes(serializer.Serialize(dual.Manifest));
        var standard=ReleaseClient.Parse(dualMetadata,dualFetch,false);
        var bundled=ReleaseClient.Parse(dualMetadata,dualFetch,true);
        Require(standard.LauncherName==LauncherVariant.SourceFile && !standard.Manifest.SupportsPlayer,"Source-only selection introduces DLLs");
        Require(bundled.LauncherName==LauncherVariant.IncludedFile && bundled.Manifest.SupportsPlayer,"Included selection loses DLLs");
        Require(bundled.Manifest.LauncherSha256==dual.Manifest.DllIncludedLauncherSha256 && bundled.Manifest.RuntimeSha256==dual.Manifest.DllIncludedRuntimeSha256,"Included selection uses standard checksums");
        var bundledFallback=ReleaseClient.ParseLatestManifest(dualFetch("manifest"),true);
        string expectedNotes=ReleaseNotesText.Format(dual.Manifest.ReleaseNotes);
        Require(standard.Notes==expectedNotes && bundled.Notes==expectedNotes && bundledFallback.Notes==expectedNotes &&
            bundled.Manifest.ReleaseNotes==dual.Manifest.ReleaseNotes,"Variant/API fallback lost manifest changes");
        Require(ReleaseClient.Parse(legacyMetadata,dualFetch).Notes==expectedNotes,"Manifest list must take priority over legacy body");
        Require(bundledFallback.LauncherUrl==bundled.LauncherUrl && bundledFallback.Manifest.LauncherSha256==bundled.Manifest.LauncherSha256,"Included fallback changes variant");
        Reject(()=>ReleaseClient.Parse(metadata,dualFetch,true),"Missing included asset accepted");
        dual.Manifest.DllIncludedRuntimeSha256=null;Reject(()=>dual.Manifest.Validate("v0.3.0"),"Incomplete included checksum pair accepted");
        dual.Manifest.DllIncludedRuntimeSha256=new string('c',64);
        EmbeddedRuntime.VerifySourceOnly(exe);
        // Invalid DLL bytes and unknown asset names cannot reach the update cache.
        Reject(()=>EmbeddedRuntime.VerifySourceOnly(Encoding.ASCII.GetBytes("MZinvalid")),"Invalid source-only assembly accepted");
        var wrongName=Package(source,exe);wrongName.LauncherName="unexpected.exe";
        Reject(()=>ReleaseUpdater.Prepare(wrongName,request,true,fetch),"Unknown launcher variant accepted");
        Require(ReleaseClient.Parse("{\"draft\":false,\"prerelease\":true}",fetch)==null,"Prerelease offered");
        Require(ReleaseClient.Parse("{\"draft\":false,\"prerelease\":false,\"tag_name\":\"v0.3.0\",\"assets\":[]}",fetch)==null,"Legacy release rejected");
        var prepared=ReleaseUpdater.Prepare(update,request,true,fetch);
        Require(File.Exists(Path.Combine(prepared.SourceDirectory,"src","Plugin.cpp")),"Verified sources not installed");
        Require(ReleaseManifest.Hash(File.ReadAllBytes(prepared.LauncherFile))==update.Manifest.LauncherSha256,"Verified executable changed");
        Directory.CreateDirectory(Path.Combine(request.InstallDirectory,"Game"));
        string saved=Path.Combine(request.InstallDirectory,"Game","progress.w3z");File.WriteAllText(saved,"untouched progress");
        string stub=Path.Combine(request.InstallDirectory,"Game","war3.exe"),ready=Path.Combine(root,"fixture-ready");
        File.Copy(Assembly.GetExecutingAssembly().Location,stub);
        using (var process=Process.Start(new ProcessStartInfo(stub,"--idle-fixture "+SetupRunner.Quote(ready)) {UseShellExecute=false,CreateNoWindow=true})) {
            try {
                for (int i=0;i<100 && !File.Exists(ready);i++) Thread.Sleep(50);
                Require(File.Exists(ready),"Idle-game fixture did not start"); downloads=0;
                Reject(()=>ReleaseUpdater.Prepare(update,request,true,fetch),"Updating a running game allowed");
                Require(downloads==0 && File.ReadAllText(saved)=="untouched progress","Running-game guard touched files");
            } finally { if (!process.HasExited) process.Kill(); process.WaitForExit(); }
        }
        Console.WriteLine("PASS updater release API/BOM, versions/build revisions, consent, checksums, archive traversal, source installation and running-game protection");
        return 0;
    }
}
