param([string[]]$OwnedModel=@())
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Portable narrow-phase fixtures contain no retail geometry; optional owned inputs stay private.
$script=Join-Path $build 'test-static-collision.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/StaticCollisionTests.cpp')+'" /Fe:static-collision-tests.exe'),
'exit /b %errorlevel%') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {
    & $env:COMSPEC /d /c $script
    if($LASTEXITCODE){throw 'Static collision build failed.'}
    & ./static-collision-tests.exe @OwnedModel
    if($LASTEXITCODE){throw 'Static collision verification failed.'}
} finally {Pop-Location}
