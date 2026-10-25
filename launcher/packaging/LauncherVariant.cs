using System.Reflection;

namespace WarcraftCSLauncher {
    public static class LauncherVariant {
        public const string SourceFile = "WarcraftCSLauncher.exe";
        public const string IncludedFile = "WarcraftCSLauncher_DLL_Included.exe";
        // Local preview bundles keep stable version metadata without confusing the published build with this source snapshot.
        public static bool IsPreview {get {return Assembly.GetExecutingAssembly().GetManifestResourceInfo("WarcraftCS.Preview.txt")!=null;}}
        // Detect capability from the payload itself, never from a user-renamed executable or saved install mode.
        public static bool IncludesRuntime {
            get { return Assembly.GetExecutingAssembly().GetManifestResourceInfo("WarcraftCS.Runtime.zip") != null; }
        }
        public static string InstallationMode(bool included, string savedMode) {
            return included && savedMode != "Developer" ? "Player" : "Developer";
        }
    }
}
