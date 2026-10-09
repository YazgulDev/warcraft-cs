using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Web.Script.Serialization;

namespace WarcraftCSLauncher {
    public static class LauncherSelfUpdate {
        public static bool Schedule(PreparedUpdate update) {
            string current=Assembly.GetExecutingAssembly().Location;
            if (ReleaseManifest.Hash(File.ReadAllBytes(current))==update.Manifest.LauncherSha256) return false;
            var request=Path.Combine(Path.GetDirectoryName(update.LauncherFile),"replace-request.json");
            // Hand paths to the detached helper as JSON data; replacement waits until this process exits.
            File.WriteAllText(request,new JavaScriptSerializer().Serialize(new {
                ParentId=Process.GetCurrentProcess().Id, Replacement=update.LauncherFile,
                Target=current, Sha256=update.Manifest.LauncherSha256
            }),new UTF8Encoding(false));
            var start=new ProcessStartInfo(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
                "WindowsPowerShell","v1.0","powershell.exe"),"-NoProfile -ExecutionPolicy Bypass -File "+
                SetupRunner.Quote(update.HelperScript)+" -RequestFile "+SetupRunner.Quote(request));
            start.UseShellExecute=false; start.CreateNoWindow=true;
            using (var process=Process.Start(start)) { if (process==null) throw new InvalidOperationException("Could not restart the updated launcher."); }
            return true;
        }
    }
}
