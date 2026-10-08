using System;
using System.IO;
using System.Web.Script.Serialization;

namespace WarcraftCSLauncher {
    public sealed class ClientRequest {
        public string WarcraftDirectory { get; set; }
        public string CounterStrikeDirectory { get; set; }
        public string InstallDirectory { get; set; }
        public string PythonExecutable { get; set; }
        // Retain the old JSON field for client compatibility; it no longer authorizes automatic installation.
        public bool AutomaticUpdates { get; set; }
        public string InstallMode { get; set; }
        public string LauncherExecutable { get; set; }
        // Edition preferences are optional in old clients and independent of download consent.
        public string GameEdition { get; set; }
        public ClientRequest() { InstallMode="Player"; GameEdition=WarcraftCSLauncher.GameEdition.FrozenThrone; }

        public void ValidateDestination() {
            GameEdition=WarcraftCSLauncher.GameEdition.Normalize(GameEdition);
            // Missing legacy mode selects bundled Player modules; only an explicit Developer choice builds locally.
            if (InstallMode!="Player" && InstallMode!="Developer") throw new InvalidOperationException("Choose Player or Developer mode.");
            if (!Directory.Exists(WarcraftDirectory) || !Directory.Exists(CounterStrikeDirectory))
                throw new InvalidOperationException("Select your installed Warcraft III and Counter-Strike folders.");
            var target = Path.GetFullPath(InstallDirectory).TrimEnd(Path.DirectorySeparatorChar);
            // Junctions must not redirect a selected update destination into the original games.
            for (var parent=target; parent!=null; parent=Path.GetDirectoryName(parent)) {
                if ((File.Exists(parent) || Directory.Exists(parent)) && (File.GetAttributes(parent)&FileAttributes.ReparsePoint)!=0)
                    throw new InvalidOperationException("Install into a real directory, not through a junction.");
            }
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
