using System;
using System.IO;
using System.Web.Script.Serialization;

namespace WarcraftCSLauncher {
    public sealed class ClientRequest {
        public string WarcraftDirectory { get; set; }
        public string CounterStrikeDirectory { get; set; }
        public string InstallDirectory { get; set; }
        public string PythonExecutable { get; set; }

        public void ValidateDestination() {
            if (!Directory.Exists(WarcraftDirectory) || !Directory.Exists(CounterStrikeDirectory))
                throw new InvalidOperationException("Select your installed Warcraft III and Counter-Strike folders.");
            var target = Path.GetFullPath(InstallDirectory).TrimEnd(Path.DirectorySeparatorChar);
            if (target == Path.GetPathRoot(target).TrimEnd(Path.DirectorySeparatorChar))
                throw new InvalidOperationException("Select a dedicated client folder, not a drive root.");
            // Extracting source must not modify an original game before the PowerShell validator can run.
            foreach (var game in new[] {WarcraftDirectory, CounterStrikeDirectory}) {
                var original = Path.GetFullPath(game).TrimEnd(Path.DirectorySeparatorChar);
                if (target.Equals(original, StringComparison.OrdinalIgnoreCase) ||
                    target.StartsWith(original + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException("Install Warcraft CS outside the original game folders.");
            }
        }

        public void Save(string path) {
            // JSON carries folder names as data, never as interpolated shell commands.
            File.WriteAllText(path, new JavaScriptSerializer().Serialize(this), new System.Text.UTF8Encoding(false));
        }
    }
}
