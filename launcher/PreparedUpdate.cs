namespace WarcraftCSLauncher {
    public sealed class PreparedUpdate {
        public string SourceDirectory { get; set; }
        public string LauncherFile { get; set; }
        public string HelperScript { get; set; }
        public ReleaseManifest Manifest { get; set; }
    }
}
