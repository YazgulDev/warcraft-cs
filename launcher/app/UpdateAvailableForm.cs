using System;
using System.Diagnostics;
using System.Drawing;
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
            // Display each reviewed change separately, including legacy manifests with no GitHub release body.
            var notes=new TextBox {Name="ReleaseNotes",Text=ReleaseNotesText.Resolve(release.Manifest,release.Notes),
                Left=20,Top=62,Width=640,Height=222,Multiline=true,ReadOnly=true,ScrollBars=ScrollBars.Vertical};
            Controls.Add(notes);
            var page=new LinkLabel {Text="Open release page",Left=20,Top=293,Width=640,Height=25};
            page.LinkClicked+=(s,e)=>Process.Start(new ProcessStartInfo(release.PageUrl) {UseShellExecute=true});Controls.Add(page);
            string details=mode=="Developer" ? "Developer: builds DLLs locally. Missing Build Tools / SDK will be downloaded (several GB)." :
                "Player: installs bundled DLLs. No Build Tools or Windows SDK are downloaded.";
            // Update is an explicit consent action; declining this dialog performs no installation.
            // Confirmation covers full runtime repair as well as the selected mode's dependencies.
            Controls.Add(new Label {Name="ModeDetails",Text=details+"\r\nReinstalls the private game; keeps saves and settings. Close Warcraft first.\r\nPython / NumPy may be prepared. Update accepts the launcher download/dependency terms.",
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
    }
}
