using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace WarcraftCSLauncher {
    // Discover known journals without recursively scanning game assets or following directory links.
    public static class LogCatalog {
        public static List<KeyValuePair<string, string>> List(string installation, string launcherDirectory) {
            var result = new List<KeyValuePair<string, string>>();
            AddDirectory(result, launcherDirectory, "launcher*.log", "Launcher");
            if (String.IsNullOrWhiteSpace(installation)) return result;
            try {
                var root = Path.GetFullPath(installation);
                AddDirectory(result, root, "install.log", "Installation");
                AddDirectory(result, Path.Combine(root, "Game", "WarcraftCS"), "WarcraftCS*.log", "Game");
                // A selected source runtime can also be inspected directly without pretending it is a client install.
                AddDirectory(result, Path.Combine(root, "WarcraftCS"), "WarcraftCS*.log", "Source game");
                var updates = Path.Combine(root, "updates");
                if (SafeDirectory(updates)) foreach (var revision in Directory.GetDirectories(updates))
                    AddDirectory(result, revision, "launcher-replacement.log", "Replacement " + Path.GetFileName(revision));
            } catch (IOException) { } catch (UnauthorizedAccessException) { } catch (ArgumentException) { }
            result.Sort((left, right) => StringComparer.OrdinalIgnoreCase.Compare(left.Value, right.Value));
            return result;
        }
        private static bool SafeDirectory(string path) {
            if (String.IsNullOrEmpty(path) || !Directory.Exists(path)) return false;
            for (var directory = Path.GetFullPath(path); directory != null; directory = Path.GetDirectoryName(directory))
                if ((File.GetAttributes(directory) & FileAttributes.ReparsePoint) != 0) return false;
            return true;
        }
        private static void AddDirectory(List<KeyValuePair<string, string>> result, string directory, string pattern, string label) {
            try {
                if (!SafeDirectory(directory)) return;
                foreach (var path in Directory.GetFiles(directory, pattern, SearchOption.TopDirectoryOnly))
                    if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) == 0)
                        result.Add(new KeyValuePair<string, string>(label + " — " + Path.GetFileName(path), path));
            } catch (IOException) { } catch (UnauthorizedAccessException) { } catch (ArgumentException) { }
        }
        public static string ReadTail(string path) {
            // Share rotation/writes and snapshot a bounded tail so an active writer cannot grow a UI read indefinitely.
            using (var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete)) {
                const int maximum = 512 * 1024;
                long length = stream.Length; bool truncated = length > maximum;
                stream.Seek(Math.Max(0, length - maximum), SeekOrigin.Begin);
                var buffer = new byte[(int)Math.Min(maximum, length)]; int read = 0;
                while (read < buffer.Length) { int count = stream.Read(buffer, read, buffer.Length - read); if (count == 0) break; read += count; }
                using (var memory = new MemoryStream(buffer, 0, read))
                using (var reader = new StreamReader(memory, Encoding.UTF8, true)) {
                    if (truncated) reader.ReadLine(); // Skip the partial first UTF-8 line.
                    // WinForms text boxes need CRLF; accept LF-only or legacy CR journals without joining lines.
                    var text = reader.ReadToEnd().Replace("\r\n", "\n").Replace("\r", "\n").Replace("\n", "\r\n");
                    return (truncated ? "[Showing the last 512 KiB. Save full log to export everything.]\r\n" : "") + text;
                }
            }
        }
        public static void Export(string source, string destination) {
            if (String.Equals(Path.GetFullPath(source), Path.GetFullPath(destination), StringComparison.OrdinalIgnoreCase))
                throw new IOException("Choose a different file; the live log cannot be overwritten.");
            using (var input = new FileStream(source, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
            using (var output = new FileStream(destination, FileMode.Create, FileAccess.Write, FileShare.None)) {
                // Export the complete snapshot, including lines omitted from the viewer's bounded tail.
                long remaining = input.Length; var buffer = new byte[65536];
                while (remaining > 0) {
                    int count = input.Read(buffer, 0, (int)Math.Min(buffer.Length, remaining)); if (count == 0) break;
                    output.Write(buffer, 0, count); remaining -= count;
                }
            }
        }
    }
}
