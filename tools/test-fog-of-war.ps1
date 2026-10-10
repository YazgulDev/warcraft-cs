$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise the actual adapter against independent fog/mask switches without requiring game files.
$script=Join-Path $build 'test-fog-of-war.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 "'+(Join-Path $root 'tests/NativeFogOfWarTests.cpp')+'" "'+(Join-Path $root 'src/platform/NativeFogOfWar.cpp')+'" /Fe:fog-of-war-tests.exe'),
'if errorlevel 1 exit /b 1','fog-of-war-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) {throw 'Fog of war adapter verification failed.'} }
finally {Pop-Location}
