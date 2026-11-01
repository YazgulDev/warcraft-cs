$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Force $build | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Authored PE records and injected native callbacks require no Warcraft binaries or interactive game input.
$script=Join-Path $build 'test-portals-deaths.cmd'
$registry=@('tests/NativeRegistryTests.cpp','src/platform/NativeRegistry.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
$interactions=@('tests/NativeInteractionsTests.cpp','src/platform/NativePortals.cpp','src/platform/NativeExplosiveDeath.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
@('@echo off',('call "'+$environment+'" >nul'),
 ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($registry -join ' ')+' /Fe:native-registry-tests.exe'),
 'if not "%errorlevel%"=="0" exit /b 1','native-registry-tests.exe','if not "%errorlevel%"=="0" exit /b 1',
 ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX /DWCS_NATIVE_INTERACTIONS_TEST '+($interactions -join ' ')+' /Fe:native-interactions-tests.exe'),
 'if not "%errorlevel%"=="0" exit /b 1','native-interactions-tests.exe','if not "%errorlevel%"=="0" exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try{& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Native portal/death adapter regressions failed.'}}
finally{Pop-Location}
