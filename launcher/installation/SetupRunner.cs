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
            if (request.InstallMode=="Player" && !LauncherVariant.IncludesRuntime)
                throw new InvalidOperationException("This source-only launcher builds locally. Use Install or download the DLL-included launcher for Player setup.");
            request.ValidateDestination();
            return Task.Run(() => {
                ReleaseUpdater.RequireIdle(request.InstallDirectory);
                var source = SourcePayload.Extract(request.InstallDirectory, consent);
                InstallSource(request,source,consent,report);
            });
        }

        public static void InstallSource(ClientRequest request, string source, bool consent, Action<string> report,string launcherFile=null) {
            // Both embedded setup and release updates share the same permission, idle-game and path checks.
            if (!consent) throw new InvalidOperationException("Download/install consent is required.");
            request.ValidateDestination(); ReleaseUpdater.RequireIdle(request.InstallDirectory);
            string allowed=Path.GetFullPath(Path.Combine(request.InstallDirectory,"sources"))+Path.DirectorySeparatorChar;
            if (!Path.GetFullPath(source).StartsWith(allowed,StringComparison.OrdinalIgnoreCase)) throw new InvalidOperationException("Invalid update source path.");
            // The Player installer reads embedded module data from this EXE or the verified update-cache EXE.
            request.LauncherExecutable=launcherFile ?? System.Reflection.Assembly.GetExecutingAssembly().Location;
            var requestFile=Path.Combine(request.InstallDirectory,"install-request.json"); request.Save(requestFile);
            Run(Path.Combine(source,"launcher","install-client.ps1"),"-RequestFile "+Quote(requestFile)+" -DownloadConsent",report);
        }

        public static void Run(string script, string arguments, Action<string> report) {
            // Record backend start and exit even when the installer fails before opening install.log.
            report("Setup process starting script="+script);
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
                report("Setup process exited code="+process.ExitCode);
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
