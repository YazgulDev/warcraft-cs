$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build';New-Item -ItemType Directory -Force $build | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$script=Join-Path $build 'test-nuclear-chat.cmd'
$sources=@('tests/NuclearChatTests.cpp','src/config/GameplaySettings.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
# Real native kills are verified only in the disposable game; this suite checks portable policy and INI behavior.
@('@echo off',('call "'+$environment+'" >nul'),('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:nuclear-chat-tests.exe'),'if errorlevel 1 exit /b 1','nuclear-chat-tests.exe') | Set-Content $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Nuclear chat regression failed.'}}
finally {Pop-Location}

$sources=@('tests/NativeNuclearTests.cpp','src/platform/NativeNuclearBomb.cpp','src/platform/NativeNuclearDestruction.cpp','src/input/ShortcutBindings.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
@('@echo off',('call "'+$environment+'" >nul'),('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:native-nuclear-tests.exe'),'if errorlevel 1 exit /b 1','native-nuclear-tests.exe') | Set-Content $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Native nuclear regression failed.'}}
finally {Pop-Location}
