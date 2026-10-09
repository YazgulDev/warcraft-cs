using System;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;

namespace WarcraftCSLauncher {
    public static class Program {
        [STAThread]
        public static void Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            bool updatePreview=args.Length==2 && (args[0]=="--preview-update" || args[0]=="--preview-update-developer");
            if (updatePreview) {
                // Preview a realistic prompt without network, focus changes or installation side effects.
                var release=new ReleaseUpdate {Manifest=new ReleaseManifest {Version=System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString(3),RuntimeSha256=new string('a',64)},
                    // Preview full public notes with separate hidden updater text, without setup or network actions.
                    Notes="# Warcraft CS release\n\nFull installation details remain on GitHub.\n\n<!-- launcher-summary\n"+
                        "- Added CS sound volume parameter.\n- Added configurable game settings skill.\n- Updated Release Notes rules.\n-->"};
                using (var prompt=new UpdateAvailableForm(release,args[0]=="--preview-update-developer" ? "Developer" : "Player",true)) RenderPreview(prompt,args[1]);
                return;
            }
            bool preview = args.Length == 2 && args[0] == "--preview";
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
