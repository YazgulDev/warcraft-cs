$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Visual Studio x86 C++ tools are required.' }
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$fixture=Join-Path $build ('cs-audio-'+[guid]::NewGuid().ToString('N'))
# Verify actual CS mixer gain with synthetic silence; Warcraft, its saves and original assets are never opened.
$script=Join-Path $build 'test-cs-audio.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/GameAudioVolumeTests.cpp')+'" "'+(Join-Path $root 'src/audio/GameAudio.cpp')+'" "'+(Join-Path $root 'src/config/GameplaySettings.cpp')+'" /Fe:cs-audio-tests.exe /link ole32.lib'),
    'if errorlevel 1 exit /b 1',('cs-audio-tests.exe "'+$fixture+'"')) | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'Native CS audio volume verification failed.' } }
finally { Pop-Location }
