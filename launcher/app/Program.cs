using System;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;

namespace WarcraftCSLauncher {
    public static class Program {
        [STAThread]
        public static void Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            if (args.Length == 2 && args[0] == "--preview-logs") {
                // Exercise the real empty-state reader without reading personal journals or activating a window.
                using (var reader = new LogViewerForm(null,true)) RenderPreview(reader,args[1]);
                return;
            }
            bool updatePreview=args.Length==2 && (args[0]=="--preview-update" || args[0]=="--preview-update-developer");
            if (updatePreview) {
                // Preview a realistic prompt without network, focus changes or installation side effects.
                // Use the real embedded version's list, also exercising the legacy missing-body fallback.
                var release=new ReleaseUpdate {Manifest=new ReleaseManifest {Version=System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString(3),RuntimeSha256=new string('a',64)}};
                using (var prompt=new UpdateAvailableForm(release,args[0]=="--preview-update-developer" ? "Developer" : "Player",true)) RenderPreview(prompt,args[1]);
                return;
            }
            bool preview = args.Length == 2 && args[0] == "--preview";
            if (!preview) {
                LauncherLog.Write("Session start version="+System.Reflection.Assembly.GetExecutingAssembly().GetName().Version+
                    " runtimeIncluded="+LauncherVariant.IncludesRuntime+" windows="+Environment.OSVersion+" process64="+Environment.Is64BitProcess);
                // Retain managed stack traces for both UI and worker failures.
                Application.ThreadException += (s,e) => { LauncherLog.Write("UI exception: "+e.Exception);MessageBox.Show(e.Exception.Message+"\r\nUse Read logs for details."); };
                AppDomain.CurrentDomain.UnhandledException += (s,e) => LauncherLog.Write("Unhandled exception: "+e.ExceptionObject);
                Application.ApplicationExit += (s,e) => LauncherLog.Write("Session exit");
            }
            using (var form = new LauncherForm(preview)) {
                // Render the real controls for offline UI verification; this mode never starts setup/downloads.
                if (preview) {
                    RenderPreview(form,args[1]);
                    return;
                }
                // A second launcher must not race the same private files or vendor installers.
                bool created;
                using (var instance=new Mutex(true,"Local\\WarcraftCSLauncher",out created)) {
                    if (!created) { MessageBox.Show("Warcraft CS Launcher is already open."); return; }
                    try { Application.Run(form); } finally { instance.ReleaseMutex(); }
                }
            }
        }
        private static void RenderPreview(Form form,string path) {
            form.StartPosition=FormStartPosition.Manual;form.Location=new Point(-10000,-10000);
            form.Show();Application.DoEvents();
            using (var bitmap=new Bitmap(form.Width,form.Height)) {
                form.DrawToBitmap(bitmap,new Rectangle(0,0,form.Width,form.Height));bitmap.Save(path);
            }
        }
    }
}
