using System;
using System.Drawing;
using System.Windows.Forms;

namespace WarcraftCSLauncher {
    internal sealed class GameIniForm : Form {
        public GameIniForm(GameIniFile file) {
            Text="Warcraft CS INI";
            StartPosition=FormStartPosition.CenterParent;
            ClientSize=new Size(750,600);
            MinimumSize=new Size(480,320);
            AutoScaleMode=AutoScaleMode.Dpi;
            var path=new Label {Text=file.Filename,Dock=DockStyle.Top,Height=34,AutoEllipsis=true};
            var editor=new TextBox {Text=file.Text,Multiline=true,AcceptsTab=true,AcceptsReturn=true,
                WordWrap=false,ScrollBars=ScrollBars.Both,Dock=DockStyle.Fill,MaxLength=1024*1024,
                Font=new Font("Consolas",10)};
            var save=new Button {Text="Save",AutoSize=true,Enabled=false};
            var close=new Button {Text="Close",AutoSize=true,DialogResult=DialogResult.Cancel};
            var footer=new FlowLayoutPanel {Dock=DockStyle.Bottom,Height=48,Padding=new Padding(5)};
            footer.Controls.AddRange(new Control[] {save,close,new Label {AutoSize=true,Margin=new Padding(8,9,0,0),
                Text="Press F8 in FPS to reload supported settings."}});
            Controls.AddRange(new Control[] {editor,footer,path});
            CancelButton=close;
            // Preserve comments and custom keys by editing the same file the game reads.
            editor.TextChanged+=(sender,args)=>save.Enabled=editor.Text!=file.Text;
            save.Click+=(sender,args)=>{
                try { file.Save(editor.Text);save.Enabled=false; }
                catch (Exception error) { MessageBox.Show(this,error.Message,Text,MessageBoxButtons.OK,MessageBoxIcon.Error); }
            };
            FormClosing+=(sender,args)=>{
                if (save.Enabled && MessageBox.Show(this,"Discard unsaved changes?",Text,
                    MessageBoxButtons.YesNo,MessageBoxIcon.Question)!=DialogResult.Yes) args.Cancel=true;
            };
        }
    }
}
