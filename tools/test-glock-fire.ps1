$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Verify bounded secondary bursts without native input or weapon assets.
$script=Join-Path $build 'test-glock-fire.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/GlockFireTests.cpp')+'" /Fe:glock-fire-tests.exe'),
    'if errorlevel 1 exit /b 1','glock-fire-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Glock fire verification failed.' };Write-Output 'PASS Glock burst spacing, partial ammo, cooldown, cancellation, late frames and clock wraparound.' }
finally { Pop-Location }
