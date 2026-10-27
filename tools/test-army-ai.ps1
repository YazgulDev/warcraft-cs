$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build';New-Item -ItemType Directory -Path $build -Force | Out-Null
$vc=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vc 'VC/Auxiliary/Build/vcvars32.bat'
# Exercise actual AI orders against authored identities/ownership/resources without game files or a visible window.
$sources=@('tests/ArmyAITests.cpp','src/squad/ArmyAI.cpp') | ForEach-Object {'"'+(Join-Path $root $_)+'"'}
# A negative native abort status is still a failure; stop before a later executable can hide it.
$script=Join-Path $build 'test-army-ai.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX '+($sources -join ' ')+' /Fe:army-ai-tests.exe'),
    'if not "%errorlevel%"=="0" exit /b 1','army-ai-tests.exe','if not "%errorlevel%"=="0" exit /b 1') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try {& $env:COMSPEC /d /c $script;if($LASTEXITCODE){throw 'Army AI regression failed.'};Write-Output 'PASS AI opt-in, owned resource orders, construction leases, manual/combat exclusions, defense and shop access.'}
finally {Pop-Location}
