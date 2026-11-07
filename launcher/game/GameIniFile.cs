using System;
using System.IO;
using System.Linq;
using System.Text;

namespace WarcraftCSLauncher {
    public sealed class GameIniFile {
        private readonly Encoding encoding;
        private byte[] saved;
        public string Filename { get; private set; }
        public string Text { get; private set; }

        public static string Location(string installation) {
            return Path.Combine(installation,"Game","WarcraftCS","WarcraftCS.ini");
        }

        public GameIniFile(string installation) {
            Filename=Path.GetFullPath(Location(installation));
            CheckPath(Filename);
            if (!File.Exists(Filename)) throw new FileNotFoundException("Install Warcraft CS to create its INI file.",Filename);
            if (new FileInfo(Filename).Length>1024*1024) throw new IOException("The INI is too large to edit here.");
            saved=File.ReadAllBytes(Filename);
            // Keep Unicode BOMs and older local-ANSI INIs in their original encoding.
            using (var reader=new StreamReader(new MemoryStream(saved),new UTF8Encoding(false,true),true)) {
                try { Text=reader.ReadToEnd();encoding=reader.CurrentEncoding; }
                catch (DecoderFallbackException) { encoding=Encoding.Default;Text=encoding.GetString(saved); }
            }
        }

        public void Save(string text) {
            CheckPath(Filename);
            string backup=Filename+".bak";
            CheckPath(backup);
            if (!File.ReadAllBytes(Filename).SequenceEqual(saved))
                throw new IOException("The INI changed outside the launcher. Reopen it before saving.");
            // Publish the edited file and its previous copy together, preserving the installed INI on failure.
            string temporary=Path.Combine(Path.GetDirectoryName(Filename),Guid.NewGuid().ToString("N")+".tmp");
            try {
                using (var stream=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None))
                using (var writer=new StreamWriter(stream,encoding)) writer.Write(text);
                File.Replace(temporary,Filename,backup);
                saved=File.ReadAllBytes(Filename);
                Text=text;
            } finally {
                if (File.Exists(temporary)) File.Delete(temporary);
            }
        }

        private static void CheckPath(string path) {
            // Follow the same private-runtime boundary as Play, including redirected files and backup paths.
            for (string part=path;part!=null;part=Path.GetDirectoryName(part)) {
                if ((File.Exists(part) || Directory.Exists(part)) &&
                    (File.GetAttributes(part)&FileAttributes.ReparsePoint)!=0)
                    throw new IOException("The INI path must not use links or junctions.");
            }
        }
    }
}
