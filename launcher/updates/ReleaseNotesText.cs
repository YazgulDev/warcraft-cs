using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace WarcraftCSLauncher {
    // Version-specific reviewed lists survive GitHub rate limits without inventing changes for unknown releases.
    public static class ReleaseNotesText {
        public static string Resolve(ReleaseManifest manifest,string body) {
            if (!String.IsNullOrWhiteSpace(manifest.ReleaseNotes)) return Format(manifest.ReleaseNotes);
            if (!String.IsNullOrWhiteSpace(body)) return Format(body);
            using (var stream=typeof(ReleaseNotesText).Assembly.GetManifestResourceStream("WarcraftCS.ReleaseNotes."+manifest.Version+".txt")) {
                if (stream!=null) using (var reader=new StreamReader(stream)) return Format(reader.ReadToEnd());
            }
            return "Список изменений для этой версии не опубликован. Подробности доступны на странице выпуска.";
        }
        public static string Format(string text) {
            var lines=new List<string>();bool previousBullet=false;
            // WinForms needs CRLF. Join wrapped Markdown bullets so each change remains one logical line.
            foreach (string raw in text.Replace("\r\n","\n").Replace('\r','\n').Split('\n')) {
                string line=raw.Trim();
                if (line.Length==0) { previousBullet=false;continue; }
                bool bullet=Regex.IsMatch(line,@"^(?:[-*+]\s+|\d+[.)]\s+)");
                bool heading=Regex.IsMatch(line,@"^#{1,6}\s+");
                line=Regex.Replace(line,@"^(?:[-*+]\s+|\d+[.)]\s+|#{1,6}\s+)","");
                line=Regex.Replace(line,@"\[([^\]]+)\]\([^)]+\)","$1").Replace("**","").Replace("__","").Replace("`","");
                if (!bullet && !heading && previousBullet) lines[lines.Count-1]+=" "+line;
                else lines.Add("- "+line);
                previousBullet=bullet || (!heading && previousBullet);
            }
            return String.Join("\r\n",lines);
        }
    }
}
