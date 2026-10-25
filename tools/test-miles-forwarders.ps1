$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/prebuilt-runtime.ps1')
$build=Join-Path $root 'build/miles-forwarder-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
@('LIBRARY WarcraftOriginalMss','EXPORTS','Named=_Named@0 @11','Alias=_Named@0 @13','Hidden=_Hidden@0 @17 NONAME','_Decorated@0=_Decorated@0 @19') | Set-Content (Join-Path $build 'first.def') -Encoding ascii
@('LIBRARY WarcraftOriginalMss','EXPORTS','Other=_Named@0 @42','Hidden=_Hidden@0 @44 NONAME') | Set-Content (Join-Path $build 'second.def') -Encoding ascii
$script=Join-Path $build 'test-miles.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /LD /MT /O2 "'+(Join-Path $root 'src/runtime/MilesLoader.cpp')+'" /link /OUT:loader-template.dll'),
    'if errorlevel 1 exit /b 1',
    ('cl /nologo /LD /MT /O2 "'+(Join-Path $root 'tests/MilesForwarderFixture.cpp')+'" /link /DEF:first.def /OUT:first.dll'),
    'if errorlevel 1 exit /b 1',
    ('cl /nologo /LD /MT /O2 "'+(Join-Path $root 'tests/MilesForwarderFixture.cpp')+'" /link /DEF:second.def /OUT:second.dll'),
    'if errorlevel 1 exit /b 1',
    ('cl /nologo /MT /EHsc "'+(Join-Path $root 'tests/MilesForwarderTests.cpp')+'" /Fe:miles-consumer.exe')) | Set-Content $script -Encoding ascii
Push-Location $build
try {
    & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Miles fixtures failed to compile.'}
    $template=[IO.File]::ReadAllBytes((Join-Path $build 'loader-template.dll'))
    foreach($case in @(@{Name='first';Mode='1'},@{Name='second';Mode='2'})) {
        $directory=Join-Path $build $case.Name;New-Item -ItemType Directory -Path $directory -Force | Out-Null
        $original=[IO.File]::ReadAllBytes((Join-Path $build ($case.Name+'.dll')))
        [IO.File]::WriteAllBytes((Join-Path $directory 'WarcraftOriginalMss.dll'),$original)
        [IO.File]::WriteAllBytes((Join-Path $directory 'Mss32.dll'),(Convert-MilesForwarders $template $original))
        & (Join-Path $build 'miles-consumer.exe') $directory $case.Mode
        if($LASTEXITCODE){throw 'Windows could not resolve an adapted sound ABI.'}
    }
    # Bad input never mutates the package template or silently produces a partial DLL.
    $before=Get-RuntimeHash $template
    foreach($bad in @([byte[]]@(1,2,3),[byte[]]::new(256))) {
        $rejected=$false;try {$null=Convert-MilesForwarders $template $bad}catch{$rejected=$true}
        if(!$rejected){throw 'Malformed Miles input was accepted.'}
    }
    $original=[IO.File]::ReadAllBytes((Join-Path $build 'first.dll'))
    $rejected=$false;try {$null=Convert-MilesForwarders $original $original}catch{$rejected=$true}
    if(!$rejected -or (Get-RuntimeHash $template) -ne $before){throw 'Unreserved template accepted or input template mutated.'}

    # Exercise the real Player installer for both exact host identities and different local sound export tables.
    $package=Join-Path $build 'package';New-Item -ItemType Directory -Path (Join-Path $package 'licenses') -Force | Out-Null
    $files=@('WarcraftCS.mix','Mss32.dll','NOTICE','LICENSE-MIT','LICENSE-APACHE','licenses/MinHook-BSD-2-Clause.txt','licenses/ReGameDLL_CS-MIT.txt','licenses/ReHLDS-MIT.txt')
    $hashes=@{}
    foreach($file in $files) {
        $bytes=if($file -in @('WarcraftCS.mix','Mss32.dll')){$template}else{[Text.Encoding]::ASCII.GetBytes('Synthetic project-authored notice.')}
        [IO.File]::WriteAllBytes((Join-Path $package $file),$bytes);$hashes[$file]=Get-RuntimeHash $bytes
    }
    $revision='a'*64
    $manifest=@{Version='0.11.0';SourceRevision=$revision;HostVersion='1.26.0.6401';OriginalSoundSha256=(Get-RuntimeHash $original);FormatVersion=2;SoundProxyKind='local-export-table-v1';HostVersions=@('1.26.0.6401','1.27.0.52240');Files=$hashes}
    $manifest | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $package 'runtime.json') -Encoding utf8
    $compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
    foreach($case in @(@{Name='host126';Version='1.26.0.6401';Sound='first';Mode='1'},@{Name='host127';Version='1.27.0.52240';Sound='second';Mode='2'})) {
        $game=Join-Path $build $case.Name;$runtime=Join-Path $build ($case.Name+'-runtime')
        New-Item -ItemType Directory -Path $game,$runtime -Force | Out-Null
        $source=Join-Path $game 'Version.cs'
        ('[assembly: System.Reflection.AssemblyFileVersion("'+$case.Version+'")] public sealed class HostFixture {}') | Set-Content $source -Encoding ascii
        & $compiler /nologo /target:library /platform:x86 ('/out:'+(Join-Path $game 'Game.dll')) $source
        if($LASTEXITCODE){throw 'Host fixture compilation failed.'}
        Copy-Item (Join-Path $build ($case.Sound+'.dll')) (Join-Path $game 'Mss32.dll') -Force
        Copy-Item (Join-Path $game 'Mss32.dll') (Join-Path $runtime 'WarcraftOriginalMss.dll') -Force
        Set-Content (Join-Path $runtime '.warcraft-cs-private-runtime') 'Synthetic fixture runtime.'
        Install-PrebuiltRuntime $package $runtime $game
        & (Join-Path $build 'miles-consumer.exe') $runtime $case.Mode
        if($LASTEXITCODE){throw 'Installed Player proxy uses the wrong local ABI.'}
    }
    # Format-2 validation runs on the actual ZIP bytes before extraction; unknown policies/hosts remain rejected.
    function Test-PackageBytes($Metadata) {
        $Metadata | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $package 'runtime.json') -Encoding utf8
        $memory=[IO.MemoryStream]::new()
        $archive=[IO.Compression.ZipArchive]::new($memory,[IO.Compression.ZipArchiveMode]::Create,$true)
        try {
            foreach($file in @('runtime.json')+$files) {
                $entry=$archive.CreateEntry($file);$stream=$entry.Open()
                try {$bytes=[IO.File]::ReadAllBytes((Join-Path $package $file));$stream.Write($bytes,0,$bytes.Length)}finally{$stream.Dispose()}
            }
        } finally {$archive.Dispose()}
        try {$null=Read-RuntimePackage $memory.ToArray() $revision '0.11.0'}finally{$memory.Dispose()}
    }
    Test-PackageBytes $manifest
    foreach($policy in @('unknown-format','unknown-host','unknown-proxy')) {
        $bad=$manifest.Clone()
        switch($policy) {
            'unknown-format' {$bad.FormatVersion=3}
            'unknown-host' {$bad.HostVersions=@('1.26.0.6401','1.28.0.7205')}
            'unknown-proxy' {$bad.SoundProxyKind='unverified'}
        }
        $rejected=$false;try {Test-PackageBytes $bad}catch{$rejected=$true}
        if(!$rejected){throw ('Invalid Player package accepted: '+$policy)}
    }
    $legacy=$manifest.Clone();$legacy.Remove('FormatVersion');$legacy | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $package 'runtime.json') -Encoding utf8
    $rejected=$false;try {Assert-PrebuiltHost $package (Join-Path $build 'host127')}catch{$rejected=$true}
    if(!$rejected){throw 'Legacy package lost its exact-host restriction.'}
    Write-Output 'PASS native forwarders, aliases/holes, malformed input, format policy and adaptive Player installs for 1.26a/1.27a; legacy host checks retained.'
} finally {Pop-Location}
