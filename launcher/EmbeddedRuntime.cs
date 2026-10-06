using System;
using System.IO;
using System.Reflection;

namespace WarcraftCSLauncher {
    public static class EmbeddedRuntime {
        public static void Verify(byte[] launcher, ReleaseManifest manifest) {
            if (!manifest.SupportsPlayer) throw new InvalidDataException("This older release has no bundled Player DLLs. Choose Developer mode or a newer Player release.");
            // Read resources only: a downloaded launcher is not executed to inspect its native payload.
            Assembly assembly;
            try { assembly=Assembly.Load(launcher); } catch (BadImageFormatException) { throw new InvalidDataException("Invalid Player launcher assembly."); }
            VerifyResource(assembly,"WarcraftCS.Runtime.zip",manifest.RuntimeSha256,5000000);
            VerifyResource(assembly,"WarcraftCS.Source.zip",manifest.SourceSha256,10000000);
        }
        private static void VerifyResource(Assembly assembly,string name,string hash,long limit) {
            using (var resource=assembly.GetManifestResourceStream(name)) using (var memory=new MemoryStream()) {
                if (resource==null || resource.Length>limit) throw new InvalidDataException("Player launcher is missing a valid embedded package.");
                resource.CopyTo(memory);ReleaseManifest.Verify(memory.ToArray(),hash);
            }
        }
    }
}
