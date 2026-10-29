$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/head-gore-tests';New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise actual native palette ownership with authored memory/rig fixtures, never retail data.
$script=Join-Path $build 'test.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/NativeHeadGoreTests.cpp')+'" /Fe:head-gore-tests.exe'),
    'if not "%errorlevel%"=="0" exit /b 1','head-gore-tests.exe') | Set-Content $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Native head gore regression failed.'}}
finally {Pop-Location}
