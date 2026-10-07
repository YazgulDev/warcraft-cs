using System;
using System.IO;
using System.Security.Cryptography;
using System.Text.RegularExpressions;

namespace WarcraftCSLauncher {
    public sealed class ReleaseManifest {
        public string Version { get; set; }
        public string Revision { get; set; }
        public string SourceSha256 { get; set; }
        public string LauncherSha256 { get; set; }
        public string RuntimeSha256 { get; set; }
        public string DllIncludedLauncherSha256 { get; set; }
        public string DllIncludedRuntimeSha256 { get; set; }
        public bool SupportsPlayer { get { return !String.IsNullOrEmpty(RuntimeSha256); } }
        public bool HasDllIncludedVariant { get { return !String.IsNullOrEmpty(DllIncludedLauncherSha256); } }

        public void Validate(string tag) {
            // A published stable tag and its manifest must identify the same version and source payload.
            ParseVersion(Version);
            if (tag != "v" + Version || !IsHash(Revision) || Revision != SourceSha256 || !IsHash(LauncherSha256))
                throw new InvalidDataException("Invalid release version or package checksums.");
            // Legacy source-build releases remain valid for Developer mode; Player bundles require their own digest.
            if (SupportsPlayer && !IsHash(RuntimeSha256)) throw new InvalidDataException("Invalid native runtime checksum.");
            // The primary manifest describes the source-only EXE; bundled hashes are an explicit optional pair.
            if ((HasDllIncludedVariant || !String.IsNullOrEmpty(DllIncludedRuntimeSha256)) &&
                (!IsHash(DllIncludedLauncherSha256) || !IsHash(DllIncludedRuntimeSha256)))
                throw new InvalidDataException("Invalid DLL-included launcher checksums.");
        }
        public ReleaseManifest ForDllIncludedLauncher() {
            if (!HasDllIncludedVariant) return this; // Legacy releases used one bundled executable.
            return new ReleaseManifest {Version=Version,Revision=Revision,SourceSha256=SourceSha256,
                LauncherSha256=DllIncludedLauncherSha256,RuntimeSha256=DllIncludedRuntimeSha256};
        }
        public bool IsNewer(string currentVersion, string currentRevision, string installedRevision) {
            int comparison = ParseVersion(Version).CompareTo(ParseVersion(currentVersion));
            // Release assets may receive a repaired build without moving the immutable published tag.
            return comparison > 0 || (comparison == 0 && (Revision != currentRevision ||
                (!String.IsNullOrEmpty(installedRevision) && Revision != installedRevision)));
        }
        public static System.Version ParseVersion(string value) {
            if (value == null || !Regex.IsMatch(value, @"^\d+\.\d+\.\d+$"))
                throw new InvalidDataException("A stable major.minor.patch release is required.");
            return new System.Version(value);
        }
        private static bool IsHash(string value) { return value != null && Regex.IsMatch(value, "^[0-9a-f]{64}$"); }
        public static string Hash(byte[] bytes) {
            using (var sha = SHA256.Create()) return BitConverter.ToString(sha.ComputeHash(bytes)).Replace("-", "").ToLowerInvariant();
        }
        public static void Verify(byte[] bytes, string expected) {
            if (!IsHash(expected) || Hash(bytes) != expected) throw new InvalidDataException("Release checksum mismatch; no update was installed.");
        }
    }
}
