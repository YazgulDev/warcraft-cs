using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace WarcraftCSLauncher {
    // Compact English summaries are separate from the full release documentation.
    public static class ReleaseNotesText {
        public static string Resolve(ReleaseManifest manifest,string body) {
            string summary=Format(manifest.ReleaseNotes);
            if (summary.Length!=0) return summary;
            // Only the explicit metadata block may override a reviewed version-specific fallback.
            var metadata=Regex.Match(body ?? "",@"<!--\s*launcher-summary\s*\r?\n(?<summary>[\s\S]*?)-->");
            if (metadata.Success) {
                summary=Format(metadata.Groups["summary"].Value);
                if (summary.Length!=0) return summary;
            }
            using (var stream=typeof(ReleaseNotesText).Assembly.GetManifestResourceStream("WarcraftCS.ReleaseNotes."+manifest.Version+".txt")) {
                if (stream!=null) using (var reader=new StreamReader(stream)) {
                    summary=Format(reader.ReadToEnd());
                    if (summary.Length!=0) return summary;
                }
            }
            // Legacy bodies are accepted only if the entire body is already a concise change list.
            summary=Format(body);
            return summary.Length!=0 ? summary : "A short change list is not available for this version. See the release page for details.";
        }
        public static string Format(string text) {
            if (String.IsNullOrWhiteSpace(text)) return String.Empty;
            var lines=new List<string>();
            // Reject prose, headings, code and non-English scripts instead of turning documentation into changes.
            foreach (string raw in text.Replace("\r\n","\n").Replace('\r','\n').Split('\n')) {
                string line=raw.Trim();
                if (line.Length==0) continue;
                if (Regex.IsMatch(line,@"^(?:#{1,6}\s|```|~~~)")) return String.Empty;
                bool bullet=Regex.IsMatch(line,@"^(?:[-*+]\s+|\d+[.)]\s+)");
                line=Regex.Replace(line,@"^(?:[-*+]\s+|\d+[.)]\s+)","");
                line=Regex.Replace(line,@"\[([^\]]+)\]\([^)]+\)","$1").Replace("**","").Replace("__","").Replace("`","");
                if (Regex.IsMatch(line,@"[\p{L}-[A-Za-z]]|[<>]|[\x00-\x08\x0b\x0c\x0e-\x1f]")) return String.Empty;
                if (bullet && Regex.IsMatch(line,@"^(?:Added|Fixed|Changed|Updated|Improved|Removed|Enabled|Disabled|Preserved)\s+")) lines.Add("- "+line);
                else if (!bullet && lines.Count!=0 && Char.IsWhiteSpace(raw[0])) lines[lines.Count-1]+=" "+line;
                else return String.Empty;
            }
            // WinForms needs CRLF; wrapped Markdown continuations remain one logical change.
            return String.Join("\r\n",lines);
        }
    }
}
