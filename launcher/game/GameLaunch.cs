using System;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace WarcraftCSLauncher {
    public static class GameLaunch {
        public static ProcessStartInfo CreateStartInfo(string installation,string edition) {
            string root=Path.GetFullPath(installation);
            string runtime=Path.Combine(root,"Game");
            // A private game's directory must not redirect Play into an original installation via a junction.
            for (string directory=runtime;directory!=null;directory=Path.GetDirectoryName(directory)) {
                if (Directory.Exists(directory) && (File.GetAttributes(directory)&FileAttributes.ReparsePoint)!=0)
                    throw new InvalidOperationException("The private game path must not use a junction.");
            }
            var start=new ProcessStartInfo(Path.Combine(runtime,"war3.exe"),GameEdition.LaunchArguments(edition));
            start.WorkingDirectory=runtime;start.UseShellExecute=false;
            // Apply DPI compatibility before window creation, including when playing an older installed source revision.
            string layers=start.EnvironmentVariables["__COMPAT_LAYER"] ?? "";
            if (!Regex.IsMatch(layers,@"\bHIGHDPIAWARE\b",RegexOptions.IgnoreCase))
                start.EnvironmentVariables["__COMPAT_LAYER"]=(layers+" HIGHDPIAWARE").Trim();
            return start;
        }
        public static void Start(string installation,string edition) {
            // Reuse an existing private runtime without setup/downloads or dependence on an older launch.ps1.
            var start=CreateStartInfo(installation,edition);
            if (!File.Exists(Path.Combine(installation,"client-installed.json")) || !File.Exists(start.FileName))
                throw new InvalidOperationException("Install Warcraft CS before playing.");
            var processes=Process.GetProcessesByName("war3");
            bool running=processes.Length!=0;
            foreach(var process in processes) process.Dispose();
            if (running) throw new InvalidOperationException("Close the existing Warcraft III window before starting Warcraft CS.");
            using (var game=Process.Start(start)) {
                if (game==null) throw new InvalidOperationException("Warcraft could not be started.");
            }
        }
    }
}
