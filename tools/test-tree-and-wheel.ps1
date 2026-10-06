$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
New-Item -ItemType Directory -Path $build -Force | Out-Null
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$script=Join-Path $build 'test-tree-and-wheel.cmd'
# Compile geometry/input rules independently of Warcraft; retail model fixtures remain optional and private.
@('@echo off',('call "'+$environment+'" >nul'),
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "'+(Join-Path $root 'tests/TreeTrunkMeshTests.cpp')+'" "'+(Join-Path $root 'src/TreeTrunkMesh.cpp')+'" /Fe:tree-trunk-tests.exe'),
    'if errorlevel 1 exit /b 1','tree-trunk-tests.exe','if errorlevel 1 exit /b 1',
    ('cl /nologo /std:c++17 /EHsc /O2 /W4 "'+(Join-Path $root 'tests/WeaponWheelTests.cpp')+'" /Fe:weapon-wheel-tests.exe'),
    'if errorlevel 1 exit /b 1','weapon-wheel-tests.exe') | Set-Content -LiteralPath $script -Encoding ASCII
Push-Location $build
try { & $env:COMSPEC /d /c test-tree-and-wheel.cmd; if ($LASTEXITCODE) { throw 'Tree/wheel regressions failed.' } }
finally { Pop-Location }
