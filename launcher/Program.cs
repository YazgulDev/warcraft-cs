using System;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;

namespace WarcraftCSLauncher {
    public static class Program {
        [STAThread]
        public static void Main(string[] args) {
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            bool preview = args.Length == 2 && args[0] == "--preview";
            using (var form = new LauncherForm(preview)) {
                // Render the real controls for offline UI verification; this mode never starts setup/downloads.
                if (preview) {
                    form.StartPosition=FormStartPosition.Manual; form.Location=new Point(-10000,-10000);
                    form.Show(); Application.DoEvents();
                    using (var bitmap = new Bitmap(form.Width, form.Height)) {
                        form.DrawToBitmap(bitmap, new Rectangle(0,0,form.Width,form.Height)); bitmap.Save(args[1]);
                    }
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
    }
}
