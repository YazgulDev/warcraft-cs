using System.Reflection;

namespace WarcraftCSLauncher {
    public static class LauncherVariant {
        public const string SourceFile = "WarcraftCSLauncher.exe";
        public const string IncludedFile = "WarcraftCSLauncher_DLL_Included.exe";
        // Detect capability from the payload itself, never from a user-renamed executable or saved install mode.
        public static bool IncludesRuntime {
            get { return Assembly.GetExecutingAssembly().GetManifestResourceInfo("WarcraftCS.Runtime.zip") != null; }
        }
        public static string InstallationMode(bool included, string savedMode) {
            return included && savedMode != "Developer" ? "Player" : "Developer";
        }
    }
}
