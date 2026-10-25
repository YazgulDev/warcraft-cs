$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Production wallet, transaction and native loot lifecycle run together against synthetic Warcraft functions.
$script=Join-Path $build 'test-dollar-loot.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/DollarLootTests.cpp')+'" "'+(Join-Path $root 'src/platform/DroppedWeapons.cpp')+'" "'+(Join-Path $root 'src/input/ShortcutBindings.cpp')+'" /Fe:dollar-loot-tests.exe'),
    'if errorlevel 1 exit /b 1','dollar-loot-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Dollar economy and loot tests failed.'};Write-Output 'PASS dollar purchases, reward filtering, caps, drop odds, pickups and native marker lifecycle.'}
finally {Pop-Location}
