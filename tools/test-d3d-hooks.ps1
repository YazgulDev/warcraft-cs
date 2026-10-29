$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build/d3d-hook-tests';New-Item -ItemType Directory -Path $build -Force | Out-Null
$library=Join-Path $root 'build/native/minhook/libMinHook.x86.lib'
if(!(Test-Path -LiteralPath $library)){throw 'Build the native module first to prepare its pinned MinHook library.'}
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$script=Join-Path $build 'test.cmd'
# The test includes the actual hook implementation once, with no engine install or game process.
$command='cl /nologo /MT /std:c++17 /EHsc /O2 /DNOMINMAX /I"'+(Join-Path $root '.local/dependencies/minhook/include')+'" "'+
    (Join-Path $root 'tests/D3DHooksTests.cpp')+'" "'+(Join-Path $root 'src/presentation/FullscreenView.cpp')+
    '" /Fe:hooks.exe /link "'+$library+'" user32.lib gdi32.lib'
[IO.File]::WriteAllLines($script,@('@echo off',('call "'+$environment+'" >nul'),$command,'if errorlevel 1 exit /b 1','hooks.exe'),[Text.UTF8Encoding]::new($false))
Push-Location $build
try{& $env:COMSPEC /d /c test.cmd;if($LASTEXITCODE){throw 'D3D probe safety tests failed.'}}
finally{Pop-Location}
