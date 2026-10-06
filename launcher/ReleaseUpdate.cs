namespace WarcraftCSLauncher {
    public sealed class ReleaseUpdate {
        public string Notes { get; set; }
        public string PageUrl { get { return "https://github.com/YazgulDev/warcraft-cs/releases/tag/v"+Manifest.Version; } }
        public ReleaseManifest Manifest { get; set; }
        public string SourceUrl { get; set; }
        public string LauncherUrl { get; set; }
    }
}
