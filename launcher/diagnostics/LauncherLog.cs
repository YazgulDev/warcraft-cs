using System;
using System.IO;
using System.Text;

namespace WarcraftCSLauncher {
    // One persistent launcher journal includes failures before a client installation exists.
    public static class LauncherLog {
        private static readonly object gate = new object();
        public static string DirectoryPath { get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WarcraftCSLauncher"); } }
        public static void Write(string message) { Append(DirectoryPath, message); }
        public static void Append(string directory, string message) {
            lock (gate) { try {
                Directory.CreateDirectory(directory);
                var path = Path.Combine(directory, "launcher.log");
                // Bound a single malformed backend line as well as the total segment, preserving a useful prefix.
                if (message.Length > 16384) message = message.Substring(0,16384) + "...[truncated]";
                var record = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ss.fffZ") + " [LAUNCHER] " + message + Environment.NewLine;
                // Bound repetitive setup/API failures and retain three previous segments.
                if (File.Exists(path) && new FileInfo(path).Length + Encoding.UTF8.GetByteCount(record) > 4 * 1024 * 1024) {
                    for (int index = 3; index >= 1; --index) {
                        var target = Path.Combine(directory, "launcher." + index + ".log");
                        var source = index == 1 ? path : Path.Combine(directory, "launcher." + (index - 1) + ".log");
                        if (File.Exists(source)) File.Copy(source, target, true);
                    }
                    File.WriteAllText(path, "", new UTF8Encoding(false));
                }
                using (var stream = new FileStream(path, FileMode.Append, FileAccess.Write, FileShare.ReadWrite | FileShare.Delete))
                using (var writer = new StreamWriter(stream, new UTF8Encoding(false)))
                    writer.Write(record);
            } catch (IOException) { } catch (UnauthorizedAccessException) { } catch (ArgumentException) { }
            }
            // An unavailable log folder must not prevent Play/Update.
        }
    }
}
