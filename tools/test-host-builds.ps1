$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/host-build.ps1')
$build=Join-Path $root 'build/host-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$commands=@('@echo off',('call "'+$environment+'" >nul'),'if errorlevel 1 exit /b 1')
# Version-only PE fixtures are project-authored data: no game code/assets are loaded or redistributed.
foreach($fixture in @(@{Name='host126';Minor=26;Private=6401},@{Name='host127';Minor=27;Private=52240},@{Name='host128';Minor=28;Private=7205})) {
    $name=$fixture.Name
    $resource=@('#include <winres.h>','1 VERSIONINFO',("FILEVERSION 1,{0},0,{1}" -f $fixture.Minor,$fixture.Private),
        ("PRODUCTVERSION 1,{0},0,{1}" -f $fixture.Minor,$fixture.Private),'FILEOS VOS_NT_WINDOWS32','FILETYPE VFT_DLL','BEGIN','END')
    $resource | Set-Content -LiteralPath (Join-Path $build ($name+'.rc')) -Encoding ascii
    $commands+=('rc /nologo /fo '+$name+'.res '+$name+'.rc')
    $commands+='if errorlevel 1 exit /b 1'
    $commands+=('link /nologo /dll /noentry /machine:x86 /out:'+ $name+'.dll '+$name+'.res')
    $commands+='if errorlevel 1 exit /b 1'
}
$commands+=('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/GameBuildTests.cpp')+'" "'+(Join-Path $root 'src/platform/GameBuild.cpp')+'" /Fe:host-tests.exe /link version.lib')
$commands+='if errorlevel 1 exit /b 1'
$commands+='host-tests.exe host126.dll host127.dll host128.dll'
$script=Join-Path $build 'test-hosts.cmd';$commands | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {
    & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Native host selection regression failed.'}
    foreach($name in @('host126','host127')) {
        $hostInfo=Find-WarcraftHostBuild ((Get-Item -LiteralPath ($name+'.dll')).VersionInfo)
        if(!$hostInfo){throw ('Installer rejected supported fixture '+$name)}
    }
    if(Find-WarcraftHostBuild ((Get-Item -LiteralPath 'host128.dll').VersionInfo)){throw 'Installer accepted unsupported host.'}
    Write-Output 'PASS installer accepts exact 1.26a/1.27a versions and rejects unsupported builds.'
} finally {Pop-Location}
# A rejected ABI/name match must leave a previously generated native table intact.
. (Join-Path $PSScriptRoot 'paths.ps1')
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_generate_native_offsets.py')
if($LASTEXITCODE){throw 'Native table generation checks failed.'}
