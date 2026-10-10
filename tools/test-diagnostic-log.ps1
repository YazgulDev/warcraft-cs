$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$envScript=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
$testDirectory=Join-Path $root ('.local/diagnostics-tests-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testDirectory -Force | Out-Null
# Exercise production Windows file sharing, rotation and concurrent hook-style writers in an isolated folder.
$script=Join-Path $build 'test-diagnostic-log.cmd'
@('@echo off',('call "'+$envScript+'" >nul'),
('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'src/platform/DiagnosticLog.cpp')+'" "'+(Join-Path $root 'tests/DiagnosticLogTests.cpp')+'" /Fe:diagnostic-log-tests.exe'),
'if errorlevel 1 exit /b 1',('diagnostic-log-tests.exe "'+$testDirectory+'"')) | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Diagnostic journal verification failed.'} }
finally {Pop-Location}
