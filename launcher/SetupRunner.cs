using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading.Tasks;

namespace WarcraftCSLauncher {
    public static class SetupRunner {
        public static Task Install(ClientRequest request, bool consent, Action<string> report) {
            // Defense in depth: a disabled button alone must never be the download permission gate.
            if (!consent) throw new InvalidOperationException("Agree to downloads and installation before continuing.");
            request.ValidateDestination();
            return Task.Run(() => {
                var source = SourcePayload.Extract(request.InstallDirectory, consent);
                var requestFile = Path.Combine(request.InstallDirectory, "install-request.json");
                request.Save(requestFile);
                Run(Path.Combine(source, "launcher", "install-client.ps1"),
                    "-RequestFile " + Quote(requestFile) + " -DownloadConsent", report);
            });
        }

        public static void Run(string script, string arguments, Action<string> report) {
            var start = new ProcessStartInfo(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),
                "WindowsPowerShell", "v1.0", "powershell.exe"),
                "-NoProfile -ExecutionPolicy Bypass -File " + Quote(script) + " " + arguments);
            start.UseShellExecute = false; start.CreateNoWindow = true;
            start.RedirectStandardOutput = true; start.RedirectStandardError = true;
            start.StandardOutputEncoding = Encoding.UTF8; start.StandardErrorEncoding = Encoding.UTF8;
            // PowerShell 7 can pass Core-only module paths to this child; Windows PowerShell needs its Desktop modules.
            start.EnvironmentVariables["PSModulePath"] = String.Join(";",new[] {
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System),"WindowsPowerShell","v1.0","Modules"),
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles),"WindowsPowerShell","Modules"),
                Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments),"WindowsPowerShell","Modules")
            });
            using (var process = new Process {StartInfo = start}) {
                process.OutputDataReceived += (s, e) => { if (e.Data != null) report(e.Data); };
                process.ErrorDataReceived += (s, e) => { if (e.Data != null) report(e.Data); };
                process.Start(); process.BeginOutputReadLine(); process.BeginErrorReadLine(); process.WaitForExit();
                if (process.ExitCode != 0) throw new InvalidOperationException("Setup failed. Read the log above, resolve the reported issue, then try again.");
            }
        }

        public static string Quote(string value) {
            // These arguments are Windows paths; reject embedded quotes instead of passing shell syntax.
            if (value.Contains("\"") || value.Contains("\r") || value.Contains("\n"))
                throw new ArgumentException("Invalid file path.");
            return "\"" + value.TrimEnd('\\') + "\"";
        }
    }
}
