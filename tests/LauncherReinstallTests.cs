using System;
using System.IO;
using System.Collections.Generic;
using WarcraftCSLauncher;

public static class LauncherReinstallTests {
    private static void Require(bool condition, string message) { if (!condition) throw new Exception(message); }
    private static void Write(string path, string value) {
        Directory.CreateDirectory(Path.GetDirectoryName(path)); File.WriteAllText(path,value);
    }
    public static int Main(string[] args) {
        string root=Path.GetFullPath(args[0]), versionedDll=Path.GetFullPath(args[1]);
        foreach (bool legacy in new[] {true,false}) foreach (string mode in new[] {"Player","Developer"}) {
            string trial=Path.Combine(root,(legacy ? "legacy-" : "modern-")+mode);
            string wc=Path.Combine(trial,"Owned Warcraft"),cs=Path.Combine(trial,"Owned CS");
            Directory.CreateDirectory(wc);Directory.CreateDirectory(cs);
            foreach (string name in new[] {"war3.exe","Mss32.dll","Storm.dll","War3.mpq","War3x.mpq","War3Patch.mpq","War3xlocal.mpq"}) Write(Path.Combine(wc,name),"owned "+name);
            File.Copy(versionedDll,Path.Combine(wc,"Game.dll"));
            foreach (string name in new[] {"Mssfast.m3d","Mp3dec.asi","Reverb3.flt"}) Write(Path.Combine(wc,"redist","miles",name),"owned provider");
            Write(Path.Combine(wc,"Maps","official.w3x"),"owned map");
            foreach (string weapon in new[] {"ak47","m4a1","usp","awp","knife","c4"}) Write(Path.Combine(cs,"models","v_"+weapon+".mdl"),"synthetic model");
            Write(Path.Combine(cs,"sound","weapons","knife_slash1.wav"),"synthetic sound");
            Write(Path.Combine(cs,"sound","player","pl_step1.wav"),"synthetic sound");
            var request=new ClientRequest {WarcraftDirectory=wc,CounterStrikeDirectory=cs,InstallDirectory=Path.Combine(trial,"Client"),InstallMode=mode};
            string runtime=Path.Combine(request.InstallDirectory,"Game");
            Write(Path.Combine(runtime,".warcraft-cs-private-runtime"),"private fixture");
            Write(Path.Combine(runtime,"Game.dll"),"damaged");
            Write(Path.Combine(runtime,"Mss32.dll"),"previous project proxy");
            Write(Path.Combine(runtime,"save","profile.w3z"),"saved progress");
            Write(Path.Combine(runtime,"WarcraftCS","WarcraftCS.ini"),"[Movement]\r\nJumpBoostPercent=15\r\n");
            Write(Path.Combine(runtime,"Maps","private.w3x"),"private map");
            Write(Path.Combine(runtime,"WarcraftCS","assets","test.wcg"),"stale asset");
            Write(Path.Combine(runtime,"WarcraftCS.mix"),"stale project DLL");
            Write(Path.Combine(request.InstallDirectory,"client-installed.json"),"{\"sword_model\":\"private-choice.mdl\"}");
            string current=SourcePayload.Extract(request.InstallDirectory,true);
            string selected=Path.Combine(request.InstallDirectory,"sources",legacy ? "legacy-verified-release" : "modern-verified-release");
            Directory.CreateDirectory(Path.Combine(selected,"launcher"));
            if (!legacy) {
                Directory.CreateDirectory(Path.Combine(selected,"setup"));
                foreach (string name in new[] {"warcraft-runtime.ps1","prebuilt-runtime.ps1"})
                    File.Copy(Path.Combine(current,"setup",name),Path.Combine(selected,"setup",name));
            }
            // Stub only conversion/module installation. Actual SetupRunner and the production copy adapter
            // must repair game files first, including an older release that has no repair policy.
            Write(Path.Combine(selected,"launcher","install-client.ps1"), @"param($RequestFile,[switch]$DownloadConsent)
$ErrorActionPreference='Stop'
if (!$DownloadConsent) { throw 'No consent' }
$request=Get-Content -LiteralPath $RequestFile -Raw -Encoding UTF8 | ConvertFrom-Json
$runtime=Join-Path $request.InstallDirectory 'Game'
$adapter=Join-Path (Split-Path -Parent $PSScriptRoot) 'setup/warcraft-runtime.ps1'
if (Test-Path -LiteralPath $adapter) { . $adapter; Copy-WarcraftRuntime $request.WarcraftDirectory $runtime }
if ((Get-FileHash -LiteralPath (Join-Path $runtime 'Game.dll')).Hash -ne (Get-FileHash -LiteralPath (Join-Path $request.WarcraftDirectory 'Game.dll')).Hash) { throw 'Game DLL was not reinstalled before backend' }
if (!(Test-Path -LiteralPath (Join-Path $runtime 'Storm.dll')) -or !(Test-Path -LiteralPath (Join-Path $runtime 'Maps/official.w3x'))) { throw 'Game resources were not repaired' }
[IO.File]::WriteAllText((Join-Path $runtime 'WarcraftCS.mix'),('installed '+$request.InstallMode))
[IO.File]::WriteAllText((Join-Path $runtime 'Mss32.dll'),'new project proxy')
[IO.File]::WriteAllText((Join-Path $runtime 'WarcraftCS/assets/test.wcg'),'converted asset')
[IO.File]::WriteAllText((Join-Path $request.InstallDirectory 'client-installed.json'),'ready')
");
            var messages=new List<string>();
            SetupRunner.InstallSource(request,selected,true,line=>messages.Add(line),"verified-launcher.exe");
            Require(File.ReadAllText(Path.Combine(runtime,"WarcraftCS.mix"))=="installed "+mode,"Mode/module reinstall lost");
            Require(File.ReadAllText(Path.Combine(runtime,"WarcraftCS","assets","test.wcg"))=="converted asset","Assets were not reconverted");
            Require(File.ReadAllText(Path.Combine(runtime,"save","profile.w3z"))=="saved progress","Progress changed");
            Require(File.ReadAllText(Path.Combine(runtime,"WarcraftCS","WarcraftCS.ini")).Contains("JumpBoostPercent=15"),"Custom config changed");
            Require(File.ReadAllText(Path.Combine(runtime,"Maps","private.w3x"))=="private map","Private map changed");
            Require(messages.Exists(line=>line.Contains("Refreshed")),"Runtime repair did not execute");
            if (legacy) Require(File.ReadAllText(Path.Combine(request.InstallDirectory,"client-installed.previous.json")).Contains("private-choice.mdl"),"Legacy repair discarded retry preferences");
        }
        Console.WriteLine("PASS actual setup runner: legacy/modern release reinstall, both modes, corrupt/missing game resources, module/asset replacement and progress/config preservation.");
        return 0;
    }
}
