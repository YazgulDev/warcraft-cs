$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Real gesture/coordinator/native dispatch code runs against authored engine functions, without game files or focus.
$sources=@('tests/InteractiveInventoryTests.cpp','src/application/InventoryController.cpp','src/platform/NativeItemActions.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$script=Join-Path $build 'test-interactive-inventory.cmd'
@('@echo off',('call "'+$environment+'" >nul'),('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' user32.lib /Fe:interactive-inventory-tests.exe'),
'if errorlevel 1 exit /b 1','interactive-inventory-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Interactive inventory regression failed.'};Write-Output 'PASS item mode, gestures, stable item identity, native target forms, charges, drop/reorder and lifecycle.'}
finally {Pop-Location}
