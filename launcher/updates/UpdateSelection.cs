namespace WarcraftCSLauncher {
    public static class UpdateSelection {
        public static ReleaseUpdate Select(ReleaseUpdate candidate, string currentVersion, string currentRevision,
            string installedRevision, bool includesRuntime, bool reinstall) {
            if (candidate == null) return null;
            // Explicit Update repairs the latest release even when its version and hashes already match.
            // Startup checks still offer only a different release/revision, never a repeated installation.
            if (reinstall) return candidate;
            if (!candidate.Manifest.IsNewer(currentVersion, currentRevision, installedRevision)) return null;
            if (includesRuntime && candidate.Manifest.Version == currentVersion && !candidate.Manifest.SupportsPlayer) return null;
            return candidate;
        }
    }
}
