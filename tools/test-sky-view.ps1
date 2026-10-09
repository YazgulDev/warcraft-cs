$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
# Hidden standalone GL test uses synthetic colors; no Warcraft map, retail texture or desktop input is involved.
$script=Join-Path $build 'test-sky-view.cmd'
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 /DNOMINMAX "'+(Join-Path $root 'tests/SkyViewTests.cpp')+'" "'+(Join-Path $root 'src/presentation/SkyView.cpp')+'" /Fe:sky-view-tests.exe /link user32.lib gdi32.lib opengl32.lib'),
    'if errorlevel 1 exit /b 1','sky-view-tests.exe') | Set-Content -LiteralPath $script -Encoding ascii
Push-Location $build
try { & $env:COMSPEC /d /c $script;if ($LASTEXITCODE) { throw 'OpenGL sky tests failed.' };Write-Output 'PASS six cube directions, inherited pixel strides, three context replacements and foreground depth preservation.' }
finally { Pop-Location }
