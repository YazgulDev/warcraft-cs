using System;
using System.Diagnostics;
using System.Drawing;
using System.Collections.Generic;
using System.IO;
using System.Windows.Forms;

namespace WarcraftCSLauncher {
    public sealed class UpdateAvailableForm : Form {
        private readonly bool preview;
        protected override bool ShowWithoutActivation { get { return preview; } }
        public UpdateAvailableForm(ReleaseUpdate release,string mode,bool preview=false) {
            this.preview=preview;
            Text="Warcraft CS update available";ClientSize=new Size(680,470);Font=new Font("Segoe UI",10);
            StartPosition=FormStartPosition.CenterParent;FormBorderStyle=FormBorderStyle.FixedDialog;
            MaximizeBox=false;MinimizeBox=false;ShowInTaskbar=false;
            Controls.Add(new Label {Text="Warcraft CS "+release.Manifest.Version+" is available",Left=20,Top=18,Width=640,Height=34,
                Font=new Font(Font.FontFamily,16,FontStyle.Bold)});
            var notes=new TextBox {Name="ReleaseNotes",Text=ChangeSummary(release.Notes),
                Left=20,Top=62,Width=640,Height=222,Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical};
            Controls.Add(notes);
            var page=new LinkLabel {Text="Open release page",Left=20,Top=293,Width=640,Height=25};
            page.LinkClicked+=(s,e)=>Process.Start(new ProcessStartInfo(release.PageUrl) {UseShellExecute=true});Controls.Add(page);
            string details=mode=="Developer" ? "Developer: builds DLLs locally. Missing Build Tools / SDK will be downloaded (several GB)." :
                "Player: installs bundled DLLs. No Build Tools or Windows SDK are downloaded.";
            // Update is an explicit consent action; declining this dialog performs no installation.
            Controls.Add(new Label {Name="ModeDetails",Text=details+"\r\nPython / NumPy may be prepared for local asset conversion. Close Warcraft first.\r\nChoosing Update accepts the download/dependency terms shown in the launcher.",
                Left=20,Top=326,Width=640,Height=78});
            var update=new Button {Name="ConfirmUpdate",Text="Update",Left=412,Top=418,Width=118,Height=34,DialogResult=DialogResult.OK};
            var decline=new Button {Name="DeclineUpdate",Text="Not now",Left=542,Top=418,Width=118,Height=34,DialogResult=DialogResult.Cancel};
            if (mode=="Player" && !release.Manifest.SupportsPlayer) {
                // Older source-only releases can still be installed with the explicit Developer action in the launcher.
                update.Enabled=false;
                ((Label)Controls["ModeDetails"]).Text="This older release has no Player DLL bundle. Choose Developer in the launcher to build it locally.\r\nDeveloper mode may download Build Tools / Windows SDK. Your existing game remains playable.";
            }
            Controls.Add(update);Controls.Add(decline);AcceptButton=update;CancelButton=decline;
        }
        private static string ChangeSummary(string body) {
            // Only the opening change list belongs in the updater; full notes and links remain on the release page.
            var changes=new List<string>();
            using (var reader=new StringReader((body ?? String.Empty).Trim())) {
                string line;
                while ((line=reader.ReadLine())!=null && line.Trim().StartsWith("- ",StringComparison.Ordinal))
                    changes.Add(line.Trim());
            }
            // Windows multiline text boxes need CRLF to keep GitHub's LF-separated bullets on separate lines.
            return changes.Count==0 ? "Open the release page to see what changed." : String.Join("\r\n",changes);
        }
    }
}
