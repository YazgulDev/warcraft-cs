using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace WarcraftCSLauncher {
    public static class ReleaseUpdater {
        public static void RequireIdle(string installDirectory) {
            string runtime=Path.GetFullPath(Path.Combine(installDirectory,"Game"));
            // An update must never rebuild files beneath a running match or terminate that match for the user.
            foreach (var process in Process.GetProcessesByName("war3")) using (process) {
                try {
                    if (Path.GetDirectoryName(process.MainModule.FileName).Equals(runtime,StringComparison.OrdinalIgnoreCase))
                        throw new InvalidOperationException("Save your match and close Warcraft before updating.");
                } catch (System.ComponentModel.Win32Exception) {
                    throw new InvalidOperationException("Cannot verify the running Warcraft folder. Close Warcraft before updating.");
                }
            }
        }
        public static PreparedUpdate Prepare(ReleaseUpdate update, ClientRequest request, bool consent, Func<string,byte[]> download) {
            // The backend permission gate precedes every package download or filesystem write.
            if (!consent) throw new InvalidOperationException("Download/install consent is required.");
            request.ValidateDestination(); RequireIdle(request.InstallDirectory);
            update.Manifest.Validate("v" + update.Manifest.Version);
            if (request.InstallMode=="Player" && !update.Manifest.SupportsPlayer)
                throw new InvalidDataException("This release requires a source build. Choose Developer explicitly; Player mode never installs Build Tools or Windows SDK.");
            ReleaseClient.ValidateAssetUrl(update.SourceUrl,"v"+update.Manifest.Version,"WarcraftCS-sources.zip");
            ReleaseClient.ValidateAssetUrl(update.LauncherUrl,"v"+update.Manifest.Version,"WarcraftCSLauncher.exe");
            byte[] source=download(update.SourceUrl), launcher=download(update.LauncherUrl);
            ReleaseManifest.Verify(source,update.Manifest.SourceSha256); ReleaseManifest.Verify(launcher,update.Manifest.LauncherSha256);
            if (launcher.Length < 2 || launcher[0] != 'M' || launcher[1] != 'Z') throw new InvalidDataException("Invalid launcher executable.");
            if (request.InstallMode=="Player") EmbeddedRuntime.Verify(launcher,update.Manifest);
            string cache=Path.Combine(request.InstallDirectory,"updates",update.Manifest.Revision);
            SourcePayload.RequireOrdinaryPath(Path.Combine(cache,"WarcraftCSLauncher.exe"));
            // Validate required source contracts before writing either the new sources or the executable.
            using (var zip=new ZipArchive(new MemoryStream(source),ZipArchiveMode.Read)) {
                foreach (string name in new[] {"VERSION","launcher/install-client.ps1","launcher/replace-launcher.ps1","src/Plugin.cpp"})
                    if (zip.GetEntry(name)==null) throw new InvalidDataException("Release source payload is incomplete.");
                if (request.InstallMode=="Player" && zip.GetEntry("setup/prebuilt-runtime.ps1")==null)
                    throw new InvalidDataException("Release is missing its Player runtime installer.");
                using (var reader=new StreamReader(zip.GetEntry("VERSION").Open(),Encoding.UTF8))
                    if (reader.ReadToEnd().Trim()!=update.Manifest.Version) throw new InvalidDataException("Source version does not match the release.");
                string root=Path.Combine(request.InstallDirectory,"sources",update.Manifest.Revision);
                SourcePayload.ExtractArchive(zip,root);
                Directory.CreateDirectory(cache);
                string exe=Path.Combine(cache,"WarcraftCSLauncher.exe"); File.WriteAllBytes(exe,launcher);
                return new PreparedUpdate {SourceDirectory=root,LauncherFile=exe,
                    HelperScript=Path.Combine(root,"launcher","replace-launcher.ps1"),Manifest=update.Manifest};
            }
        }
    }
}
