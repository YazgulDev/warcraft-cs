using System;
using System.IO;
using System.IO.Compression;
using System.Windows.Forms;
using WarcraftCSLauncher;

public static class LauncherTests {
    private static void Require(bool value,string message) { if (!value) throw new Exception(message); }
    private static void Reject(Action action,string message) {
        try { action(); } catch (InvalidOperationException) { return; } catch (InvalidDataException) { return; }
        throw new Exception(message);
    }
    [STAThread]
    public static int Main(string[] args) {
        var root=Path.GetFullPath(args[0]);Directory.CreateDirectory(root);
        var wc=Path.Combine(root,"Warcraft's folder & other");var cs=Path.Combine(root,"CS folder");
        Directory.CreateDirectory(wc);Directory.CreateDirectory(cs);
        var destination=Path.Combine(root,"NoConsent");
        var request=new ClientRequest {WarcraftDirectory=wc,CounterStrikeDirectory=cs,InstallDirectory=destination};
        foreach (bool included in new[] {false,true}) using (var form=new LauncherForm(true,included)) {
            var agreement=(CheckBox)form.Controls["DownloadConsent"];
            var install=(Button)form.Controls["InstallButton"];
            var developer=(Button)form.Controls["DeveloperInstallButton"];
            var update=(Button)form.Controls["UpdateButton"];
            var edition=(ComboBox)form.Controls["GameEdition"];
            // Both variants expose the same editions and preserve the previous TFT default.
            Require(edition!=null && edition.DropDownStyle==ComboBoxStyle.DropDownList && edition.Items.Count==2,
                "Edition selector missing or allows arbitrary arguments");
            Require(edition.SelectedIndex==0 && edition.Items[1].ToString()=="Reign of Chaos","Wrong edition default/options");
            edition.SelectedIndex=1;
            Require(edition.SelectedItem.ToString()=="Reign of Chaos","RoC cannot be selected");
            // Separate installation and GitHub update actions must exist in both actual launcher layouts.
            Require(install.Text==(included ? "Install — Player" : "Install"),"Wrong Install action for variant");
            Require((developer!=null)==included,"Source-only UI offers bundled Player/Developer modes");
            Require(update!=null && update.Text=="Update","Dedicated Update action missing");
            // Neither installation mode may inherit download permission from valid folders alone.
            Require(!agreement.Checked && !install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"UI grants consent by default");
            form.Controls["WarcraftDirectory"].Text=wc; form.Controls["CounterStrikeDirectory"].Text=cs;
            form.Controls["InstallDirectory"].Text=destination;
            Require(!install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"Folders alone enable install");
            agreement.Checked=true; Require(install.Enabled && update.Enabled && (developer==null || developer.Enabled),"Valid agreement/folders do not enable actions");
            agreement.Checked=false; Require(!install.Enabled && !update.Enabled && (developer==null || !developer.Enabled),"Revoked agreement did not disable actions");
        }
        Require(LauncherVariant.InstallationMode(false,"Player")=="Developer","Source-only updater selects Player");
        Require(LauncherVariant.InstallationMode(true,"Player")=="Player","Included updater changes Player mode");
        Require(LauncherVariant.InstallationMode(true,"Developer")=="Developer","Included updater forgets explicit source mode");
        // Consent must prevent even extraction/requests, not merely disable a visual control.
        Reject(() => SetupRunner.Install(request,false,Console.WriteLine),"No-consent runner started work");
        Reject(() => SourcePayload.Extract(destination,false),"No-consent payload was extracted");
        Require(!Directory.Exists(destination),"No-consent install wrote files");
        request.InstallDirectory=Path.Combine(wc,"child");Reject(request.ValidateDestination,"Warcraft child allowed");
        request.InstallDirectory=cs;Reject(request.ValidateDestination,"Original CS folder allowed");
        request.InstallDirectory=Path.GetPathRoot(root);Reject(request.ValidateDestination,"Drive root allowed");
        request.InstallDirectory=destination;request.ValidateDestination();
        // Legacy JSON keeps TFT; serialization round-trips RoC without treating data as shell arguments.
        var serializer=new System.Web.Script.Serialization.JavaScriptSerializer();
        Require(serializer.Deserialize<ClientRequest>("{}").GameEdition==GameEdition.FrozenThrone,"Legacy preferences change edition");
        request.GameEdition=GameEdition.ReignOfChaos;
        request.Save(Path.Combine(root,"quoted-paths.json"));
        Require(serializer.Deserialize<ClientRequest>(File.ReadAllText(Path.Combine(root,"quoted-paths.json"))).GameEdition==GameEdition.ReignOfChaos,
            "RoC preference not saved");
        Require(GameEdition.LaunchArguments(GameEdition.ReignOfChaos)=="-opengl -classic","RoC launch argument missing");
        Require(GameEdition.LaunchArguments(null)=="-opengl","Legacy launch default wrong");
        Reject(()=>GameEdition.LaunchArguments("ReignOfChaos; arbitrary command"),"Unvalidated edition reaches launch arguments");
        // Launching an older installed revision still selects RoC and opts out of DPI bitmap scaling.
        foreach (string selected in new[] {GameEdition.FrozenThrone,GameEdition.ReignOfChaos}) {
            var start=GameLaunch.CreateStartInfo(destination,selected);
            Require(start.FileName==Path.Combine(destination,"Game","war3.exe") && !start.UseShellExecute,"Play escapes private game");
            Require(start.WorkingDirectory==Path.Combine(destination,"Game"),"Wrong game working directory");
            Require(start.Arguments==GameEdition.LaunchArguments(selected),"Selected edition lost during Play");
            Require(start.EnvironmentVariables["__COMPAT_LAYER"].Contains("HIGHDPIAWARE"),"Fullscreen Play permits DPI bitmap scaling");
        }
        Require(File.ReadAllText(Path.Combine(root,"quoted-paths.json")).Contains("Warcraft"),"Request serialization failed");
        using (var memory=new MemoryStream()) {
            using (var zip=new ZipArchive(memory,ZipArchiveMode.Create,true)) { zip.CreateEntry("../escaped.txt"); }
            memory.Position=0;
            using (var zip=new ZipArchive(memory,ZipArchiveMode.Read)) Reject(() => SourcePayload.ExtractArchive(zip,Path.Combine(root,"traversal")),"Zip traversal allowed");
        }
        Require(!File.Exists(Path.Combine(root,"escaped.txt")),"Archive escaped extraction root");
        var source=SourcePayload.Extract(destination,true);
        Require(File.Exists(Path.Combine(source,"launcher","install-client.ps1")),"Embedded installer missing");
        Require(File.Exists(Path.Combine(source,"src","Plugin.cpp")),"Embedded owned sources missing");
        // Clients must receive the audio-copy policy, while the codecs themselves stay owner-supplied.
        Require(File.Exists(Path.Combine(source,"setup","audio-runtime.ps1")),"Embedded Miles setup missing");
        Require(!Directory.Exists(Path.Combine(source,".local")),"Private assets embedded");
        // Cancel/close declines updates; the prompt exposes both the release information and mode-specific downloads.
        var release=new ReleaseUpdate {Manifest=new ReleaseManifest {Version="0.5.0",RuntimeSha256=new string('a',64)},Notes="- Added tree and weapon fixes."};
        using(var prompt=new UpdateAvailableForm(release,"Player",true)) {
            Require(((TextBox)prompt.Controls["ReleaseNotes"]).Text==release.Notes,"Release notes missing");
            Require(((Label)prompt.Controls["ModeDetails"]).Text.Contains("No Build Tools"),"Player prompt hides dependencies");
            Require(((Button)prompt.Controls["DeclineUpdate"]).DialogResult==DialogResult.Cancel,"Decline confirms update");
            Require(((Button)prompt.Controls["ConfirmUpdate"]).DialogResult==DialogResult.OK,"Confirm does not accept update");
        }
        release.Manifest.RuntimeSha256=null;
        using(var legacy=new UpdateAvailableForm(release,"Player",true)) Require(!((Button)legacy.Controls["ConfirmUpdate"]).Enabled,"Player prompt offers legacy source build");
        using(var dev=new UpdateAvailableForm(release,"Developer",true)) Require(((Label)dev.Controls["ModeDetails"]).Text.Contains("several GB"),"Developer prompt hides SDK download");
        // Verify visible list layout using GitHub LF input; full-note links and long legacy prose stay out of the dialog.
        release.Notes="- Added CS sound volume parameter.\n- Updated configuration documentation.\n\nFull Release Notes: https://example.test/notes";
        using(var compact=new UpdateAvailableForm(release,"Developer",true)) {
            var notes=(TextBox)compact.Controls["ReleaseNotes"];
            Require(notes.Lines.Length==2 && notes.Lines[0]=="- Added CS sound volume parameter." &&
                notes.Lines[1]=="- Updated configuration documentation.","Updater change bullets lost their line breaks");
            Require(!notes.Text.Contains("Full Release Notes"),"Updater includes full-note footer");
        }
        release.Notes="# Detailed release notes\n\nLong installation instructions.";
        using(var legacyNotes=new UpdateAvailableForm(release,"Developer",true))
            Require(((TextBox)legacyNotes.Controls["ReleaseNotes"]).Text=="Open the release page to see what changed.","Updater displays legacy long-form notes");
        // Full public Markdown and its detailed bullets must not replace the separate hidden launcher summary.
        release.Notes="# Detailed release notes\n\n## Configuration\n- Long configuration instructions.\n\n"+
            "<!-- launcher-summary\n- Added CS sound volume parameter.\n- Updated Release Notes rules.\n-->\n\nMore full notes.";
        using(var separated=new UpdateAvailableForm(release,"Developer",true)) {
            var notes=(TextBox)separated.Controls["ReleaseNotes"];
            Require(notes.Lines.Length==2 && notes.Lines[0]=="- Added CS sound volume parameter." &&
                notes.Lines[1]=="- Updated Release Notes rules.","Hidden launcher summary missing or incorrectly rendered");
            Require(!notes.Text.Contains("configuration") && !notes.Text.Contains("<!--") && !notes.Text.Contains("More full"),
                "Full GitHub notes or metadata leaked into updater");
        }
        // Missing or incomplete metadata cannot spill the rest of the public release document into the dialog.
        foreach(string invalid in new[] {null,"<!-- launcher-summary\n-->","<!-- launcher-summary\n- Incomplete metadata."}) {
            release.Notes=invalid;
            using(var missing=new UpdateAvailableForm(release,"Developer",true))
                Require(((TextBox)missing.Controls["ReleaseNotes"]).Text=="Open the release page to see what changed.",
                    "Invalid summary displays full or incomplete notes");
        }
        Console.WriteLine("PASS launcher consent, original-directory protection, special-character paths, ZIP traversal and source payload");
        return 0;
    }
}
