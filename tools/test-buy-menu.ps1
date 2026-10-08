$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Test economic invariants and CS category navigation without native Warcraft handles or private game art.
$script=Join-Path $build 'test-buy-menu.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/BuyMenuTests.cpp')+'" /Fe:buy-menu-tests.exe'),
    'if errorlevel 1 exit /b 1','buy-menu-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/BuyAccessTests.cpp')+'" "'+(Join-Path $root 'src/BuyAccess.cpp')+'" /Fe:buy-access-tests.exe'),
    'if errorlevel 1 exit /b 1','buy-access-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Buy-menu tests failed.' };Write-Output 'PASS buy economy, loadouts, navigation and mouse row geometry.' }
finally { Pop-Location }
. (Join-Path $PSScriptRoot 'paths.ps1')
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_export_skies.py')
if ($LASTEXITCODE) { throw 'Sky conversion tests failed.' }
# Synthetic preview fixtures prove arm filtering without including any copyrighted game data.
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/test_export_buy_previews.py')
if ($LASTEXITCODE) { throw 'Buy preview conversion tests failed.' }
& (Join-Path $PSScriptRoot 'test-gameplay-config.ps1')
