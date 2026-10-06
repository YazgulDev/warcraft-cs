using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Text;
using System.Web.Script.Serialization;

namespace WarcraftCSLauncher {
    public static class ReleaseClient {
        public const string LatestUrl = "https://api.github.com/repos/YazgulDev/warcraft-cs/releases/latest";
        public static ReleaseUpdate Latest() {
            return Parse(Encoding.UTF8.GetString(Download(LatestUrl)), Download);
        }
        public static ReleaseUpdate Parse(string json, Func<string, byte[]> download) {
            var serializer = new JavaScriptSerializer();
            var release = serializer.Deserialize<Dictionary<string, object>>(json);
            if ((bool)release["draft"] || (bool)release["prerelease"]) return null;
            string tag = (string)release["tag_name"];
            var assets = new Dictionary<string, string>();
            foreach (var item in (System.Collections.IEnumerable)release["assets"]) {
                var asset = (Dictionary<string, object>)item;
                string name = (string)asset["name"];
                if (name == "WarcraftCS-update.json" || name == "WarcraftCS-sources.zip" || name == "WarcraftCSLauncher.exe") {
                    string url = (string)asset["browser_download_url"];
                    ValidateAssetUrl(url, tag, name);
                    assets.Add(name, url);
                }
            }
            // Older releases without an updater manifest remain playable and usable for embedded setup.
            if (!assets.ContainsKey("WarcraftCS-update.json")) return null;
            if (!assets.ContainsKey("WarcraftCS-sources.zip") || !assets.ContainsKey("WarcraftCSLauncher.exe"))
                throw new InvalidDataException("Release upload is incomplete. Retry the update check later.");
            var manifest = serializer.Deserialize<ReleaseManifest>(Encoding.UTF8.GetString(download(assets["WarcraftCS-update.json"])).TrimStart('\uFEFF'));
            manifest.Validate(tag);
            return new ReleaseUpdate {Manifest=manifest, SourceUrl=assets["WarcraftCS-sources.zip"], LauncherUrl=assets["WarcraftCSLauncher.exe"]};
        }
        public static void ValidateAssetUrl(string url, string tag, string name) {
            // Restrict executable/source downloads to this repository's canonical GitHub release assets.
            string expected = "https://github.com/YazgulDev/warcraft-cs/releases/download/" + tag + "/" + name;
            if (url != expected || tag == null || !tag.StartsWith("v", StringComparison.Ordinal))
                throw new InvalidDataException("Untrusted release asset URL.");
            ReleaseManifest.ParseVersion(tag.Substring(1));
        }
        public static byte[] Download(string url) {
            ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
            var request = (HttpWebRequest)WebRequest.Create(url);
            request.UserAgent="WarcraftCSLauncher"; request.Timeout=15000; request.ReadWriteTimeout=30000;
            using (var response = (HttpWebResponse)request.GetResponse()) using (var stream=response.GetResponseStream())
            using (var memory = new MemoryStream()) {
                // Bound download memory and reject insecure redirects before consuming package data.
                if (response.ResponseUri.Scheme != "https") throw new InvalidDataException("Insecure release redirect.");
                var buffer = new byte[65536]; int count;
                while ((count=stream.Read(buffer,0,buffer.Length)) > 0) {
                    if (memory.Length + count > 20000000) throw new InvalidDataException("Release asset is too large.");
                    memory.Write(buffer,0,count);
                }
                return memory.ToArray();
            }
        }
    }
}
