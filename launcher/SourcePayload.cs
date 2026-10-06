using System;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Security.Cryptography;

namespace WarcraftCSLauncher {
    public static class SourcePayload {
        public static string Revision {
            get {
                using (var resource=Assembly.GetExecutingAssembly().GetManifestResourceStream("WarcraftCS.Source.zip"))
                using (var memory=new MemoryStream()) {
                    if (resource==null) throw new InvalidOperationException("Source payload is missing.");
                    resource.CopyTo(memory); return ReleaseManifest.Hash(memory.ToArray());
                }
            }
        }
        public static string Extract(string installDirectory, bool consent) {
            if (!consent) throw new InvalidOperationException("Download/install consent is required.");
            using (var resource = Assembly.GetExecutingAssembly().GetManifestResourceStream("WarcraftCS.Source.zip")) {
                if (resource == null) throw new InvalidOperationException("The launcher source payload is missing.");
                using (var memory = new MemoryStream()) {
                    resource.CopyTo(memory);
                    var data = memory.ToArray();
                    string hash;
                    using (var sha = SHA256.Create()) hash = BitConverter.ToString(sha.ComputeHash(data)).Replace("-", "").ToLowerInvariant();
                    var root = Path.Combine(installDirectory, "sources", hash);
                    using (var zip = new ZipArchive(new MemoryStream(data), ZipArchiveMode.Read)) ExtractArchive(zip, root);
                    return root;
                }
            }
        }

        public static void ExtractArchive(ZipArchive zip, string destination) {
            var root = Path.GetFullPath(destination).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            long total = 0;
            // Bound extraction and reject traversal even though the archive is produced by our source audit.
            if (zip.Entries.Count > 1000) throw new InvalidDataException("Source archive contains too many files.");
            foreach (var entry in zip.Entries) {
                var output = Path.GetFullPath(Path.Combine(root, entry.FullName));
                if (!output.StartsWith(root, StringComparison.OrdinalIgnoreCase) || entry.FullName.Contains(":"))
                    throw new InvalidDataException("Unsafe source archive path.");
                // An existing directory junction must not redirect extraction into an original game or other files.
                RequireOrdinaryPath(output);
                total += entry.Length;
                if (total > 10000000) throw new InvalidDataException("Source archive is too large.");
            }
            Directory.CreateDirectory(root);
            foreach (var entry in zip.Entries) {
                var output = Path.GetFullPath(Path.Combine(root, entry.FullName));
                if (String.IsNullOrEmpty(entry.Name)) { Directory.CreateDirectory(output); continue; }
                Directory.CreateDirectory(Path.GetDirectoryName(output));
                using (var input = entry.Open()) using (var file = File.Create(output)) input.CopyTo(file);
            }
        }
        public static void RequireOrdinaryPath(string path) {
            // Apply the same junction protection to extracted sources and executable update caches.
            for (var component=Path.GetFullPath(path);component!=null;component=Path.GetDirectoryName(component))
                if ((File.Exists(component) || Directory.Exists(component)) &&
                    (File.GetAttributes(component)&FileAttributes.ReparsePoint)!=0)
                    throw new InvalidDataException("Writing through a junction is not allowed.");
        }
    }
}
