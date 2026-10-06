param([string]$OutputDirectory = '',[string]$MinHookDirectory='',[string]$PythonExecutable='', [switch]$TestStatusEffects,[switch]$TestGameplay)
$ErrorActionPreference = 'Stop'
$modRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
if (!$OutputDirectory) { $OutputDirectory = Get-WarcraftCsRuntime $modRoot }
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$compilerLocator = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$compilerRoot = & $compilerLocator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$compilerRoot) { throw 'Visual Studio x86 C++ tools are required.' }
$compilerEnvironment = Join-Path $compilerRoot 'VC/Auxiliary/Build/vcvars32.bat'
# Build the pinned upstream library locally instead of depending on a bundled third-party binary.
if (!$MinHookDirectory) { $MinHookDirectory=Join-Path $modRoot '.local/dependencies/minhook' }
if (!(Test-Path -LiteralPath (Join-Path $MinHookDirectory 'include/MinHook.h'))) { & (Join-Path $PSScriptRoot 'fetch-dependencies.ps1') -Destination $MinHookDirectory }
$hookRoot=Join-Path $MinHookDirectory 'include'
# Keep audio mixing and classic viewport layout separate from gameplay and mesh rendering.
# Map camera interception is separate from physical eye positioning and gameplay coordination.
# Share model decoding while keeping unit and destructable geometry separate from the controller.
# Actor mesh filtering owns rendering only, independent of the unit's simulation visibility.
# Relative Windows input is separate from look-angle math, physical movement and native camera ownership.
$sources = @('WarcraftApi.cpp', 'MouseLook.cpp', 'MovementPhysics.cpp', 'WarcraftCollision.cpp', 'FirstPersonCamera.cpp', 'MapCameraGuard.cpp', 'ActorRenderFilter.cpp', 'GameAudio.cpp', 'FullscreenView.cpp', 'HitFeedback.cpp', 'ScopeView.cpp', 'UnitStatus.cpp', 'ModelBounds.cpp', 'SpriteTransform.cpp', 'UnitHitboxes.cpp', 'DestructableHitboxes.cpp', 'PlantedBomb.cpp', 'WeaponRecoil.cpp', 'ShooterController.cpp', 'GunMesh.cpp', 'Overlay.cpp', 'Plugin.cpp') | ForEach-Object { '"' + (Join-Path $modRoot "src/$_") + '"' }
# Native spell fixtures are opt-in and are excluded from the installed normal build.
# Item pickup and temporary native squad policies remain independent of shooter input/rendering.
$sources += @('GameplaySettings.cpp','ItemPickup.cpp','SquadController.cpp','FpsCombatGuard.cpp') | ForEach-Object { '"' + (Join-Path $modRoot "src/$_") + '"' }
# Install defaults only once so rebuilds preserve the player's customized settings.
$configDirectory=Join-Path $OutputDirectory 'WarcraftCS'
New-Item -ItemType Directory -Path $configDirectory -Force | Out-Null
$configFile=Join-Path $configDirectory 'WarcraftCS.ini'
if (!(Test-Path -LiteralPath $configFile)) { Copy-Item -LiteralPath (Join-Path $modRoot 'config/WarcraftCS.ini') -Destination $configFile }
$testDefine = ''
if ($TestStatusEffects) {
    $sources += '"' + (Join-Path $modRoot 'tests/StatusEffectScene.cpp') + '"'
    $testDefine = ' /DWCS_STATUS_TEST'
}
# Rune/AI native fixtures stay outside the installed ordinary build.
if ($TestGameplay) {
    $sources += '"' + (Join-Path $modRoot 'tests/RuneCombatScene.cpp') + '"'
    $testDefine += ' /DWCS_GAMEPLAY_TEST'
}
$buildDirectory = Join-Path $modRoot 'build'
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
# Preserve Miles byte-for-byte and forward its complete export table from a tiny loader.
$originalSound = Join-Path $OutputDirectory 'WarcraftOriginalMss.dll'
if (!(Test-Path -LiteralPath $originalSound)) {
    $ownedSound=Join-Path $OutputDirectory 'Mss32.dll'
    if (!(Test-Path -LiteralPath $ownedSound)) { throw 'Run setup with your own Warcraft install before building the sound proxy.' }
    Copy-Item -LiteralPath $ownedSound -Destination $originalSound
}
$python = if ($PythonExecutable) { $PythonExecutable } else { Get-WarcraftCsPython $modRoot }
$exports = Join-Path $buildDirectory 'MilesExports.def'
& $python (Join-Path $PSScriptRoot 'miles_exports.py') $originalSound $exports
if ($LASTEXITCODE) { throw 'Could not generate sound forwarders.' }
# Keep compiler output and the mod binary in project directories, targeting classic x86.
$hookBuild=Join-Path $buildDirectory 'minhook'
New-Item -ItemType Directory -Path $hookBuild -Force | Out-Null
$hookLibrary=Join-Path $hookBuild 'libMinHook.x86.lib'
$hookSources=@('src/buffer.c','src/hook.c','src/trampoline.c','src/hde/hde32.c') | ForEach-Object { '"'+(Join-Path $MinHookDirectory $_)+'"' }
$hookScript=Join-Path $hookBuild 'compile.cmd'
@('@echo off',('call "'+$compilerEnvironment+'" >nul'),'if errorlevel 1 exit /b 1',
    ('cl /nologo /c /MT /O2 /W3 /DWIN32_LEAN_AND_MEAN /I"'+$hookRoot+'" '+($hookSources -join ' ')),
    'if errorlevel 1 exit /b 1',('lib /nologo /OUT:"'+$hookLibrary+'" buffer.obj hook.obj trampoline.obj hde32.obj')) |
    Set-Content -LiteralPath $hookScript -Encoding ascii
Push-Location $hookBuild
try { & $env:COMSPEC /d /c $hookScript; if ($LASTEXITCODE) { throw 'MinHook x86 build failed.' } } finally { Pop-Location }
$compileCommand = 'cl /nologo /LD /MT /std:c++17 /EHsc /W4 /O2 /DWIN32_LEAN_AND_MEAN /DNOMINMAX' + $testDefine + ' /I"' + $hookRoot + '" ' + ($sources -join ' ') + ' /link /OUT:"' + (Join-Path $OutputDirectory 'WarcraftCS.mix') + '" "' + $hookLibrary + '" user32.lib gdi32.lib opengl32.lib version.lib winmm.lib ole32.lib'
# A saved batch file avoids nested cmd/PowerShell quoting around Visual Studio paths.
$buildScript = Join-Path $buildDirectory 'compile.cmd'
$loaderCommand = 'cl /nologo /LD /MT /O2 /W4 /DWIN32_LEAN_AND_MEAN "' + (Join-Path $modRoot 'src/MilesLoader.cpp') + '" "' + [System.IO.Path]::ChangeExtension($exports, '.cpp') + '" /link /DEF:"' + $exports + '" /OUT:"' + (Join-Path $OutputDirectory 'Mss32.dll') + '"'
@('@echo off', 'rem Initialize the x86 compiler, then build the project-owned mod.', ('call "' + $compilerEnvironment + '" >nul'), 'if errorlevel 1 exit /b 1', $compileCommand, 'if errorlevel 1 exit /b 1', $loaderCommand) | Set-Content -LiteralPath $buildScript -Encoding ascii
Push-Location $buildDirectory
try {
    & $env:COMSPEC /d /c $buildScript
    if ($LASTEXITCODE) { throw "Build failed: $LASTEXITCODE" }
} finally { Pop-Location }
Get-Item -LiteralPath (Join-Path $OutputDirectory 'WarcraftCS.mix') | Select-Object FullName, Length
