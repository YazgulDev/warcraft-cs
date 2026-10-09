param([string]$OutputDirectory = '',[string]$MinHookDirectory='',[string]$PythonExecutable='', [switch]$TestStatusEffects,[switch]$TestGameplay,[switch]$TestTreeAndWheel,[switch]$TestWorldSurfaces,[switch]$TestFloatingText)
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
$sources = @('platform/WarcraftApi.cpp', 'input/MouseLook.cpp', 'movement/MovementPhysics.cpp', 'platform/WarcraftCollision.cpp', 'presentation/FirstPersonCamera.cpp', 'platform/MapCameraGuard.cpp', 'platform/ActorRenderFilter.cpp', 'audio/GameAudio.cpp', 'presentation/FullscreenView.cpp', 'presentation/HitFeedback.cpp', 'presentation/ScopeView.cpp', 'combat/UnitStatus.cpp', 'geometry/ModelBounds.cpp', 'platform/SpriteTransform.cpp', 'combat/UnitHitboxes.cpp', 'combat/DestructableHitboxes.cpp', 'combat/PlantedBomb.cpp', 'combat/WeaponRecoil.cpp', 'application/ShooterController.cpp', 'presentation/GunMesh.cpp', 'presentation/Overlay.cpp', 'runtime/PluginRuntime.cpp', 'Plugin.cpp', 'input/InputDispatcher.cpp') | ForEach-Object { '"' + (Join-Path $modRoot "src/$_") + '"' }
# Native spell fixtures are opt-in and are excluded from the installed normal build.
# Item pickup and temporary native squad policies remain independent of shooter input/rendering.
$sources += @('config/GameplaySettings.cpp','inventory/ItemPickup.cpp','squad/SquadController.cpp','platform/FpsCombatGuard.cpp') | ForEach-Object { '"' + (Join-Path $modRoot "src/$_") + '"' }
# Tree narrow-phase geometry is decoded independently of native widget enumeration and the controller.
$sources += '"' + (Join-Path $modRoot 'src/geometry/TreeTrunkMesh.cpp') + '"'
# Shared filled aiming geometry is compiled into both ordinary Player builds and scope rendering.
$sources += '"' + (Join-Path $modRoot 'src/presentation/ReticleView.cpp') + '"'
# Thread-safe bounded diagnostics are a native platform adapter, not controller/file ownership.
$sources += '"' + (Join-Path $modRoot 'src/platform/DiagnosticLog.cpp') + '"'
# Shop access is native-world policy; inventory prices/navigation remain independently testable.
$sources += @('economy/BuyAccess.cpp','presentation/BuyMenuView.cpp','presentation/SkyView.cpp','platform/MapEnvironment.cpp','platform/NativeSky.cpp','platform/NativeFloatingText.cpp') | ForEach-Object { '"'+(Join-Path $modRoot "src/$_")+'"' }
# Install defaults only once so rebuilds preserve the player's customized settings.
$configDirectory=Join-Path $OutputDirectory 'WarcraftCS'
New-Item -ItemType Directory -Path $configDirectory -Force | Out-Null
$configFile=Join-Path $configDirectory 'WarcraftCS.ini'
. (Join-Path $modRoot 'setup/gameplay-config.ps1')
Update-GameplayConfig (Join-Path $modRoot 'config/WarcraftCS.ini') $configFile
$testDefine = ''
if ($TestFloatingText) {
    # Render oracle is explicit and absent from both ordinary client launchers.
    $sources += '"'+(Join-Path $modRoot 'tests/FloatingTextScene.cpp')+'"'
    $testDefine += ' /DWCS_FLOATING_TEXT_TEST'
}
if ($TestStatusEffects) {
    $sources += '"' + (Join-Path $modRoot 'tests/StatusEffectScene.cpp') + '"'
    $testDefine += ' /DWCS_STATUS_TEST'
}
# Rune/AI native fixtures stay outside the installed ordinary build.
if ($TestGameplay) {
    $sources += '"' + (Join-Path $modRoot 'tests/RuneCombatScene.cpp') + '"'
    $testDefine += ' /DWCS_GAMEPLAY_TEST'
}
if ($TestTreeAndWheel) {
    # Tree/input fixtures require an explicit test build and request; release binaries exclude them.
    $sources += '"' + (Join-Path $modRoot 'tests/TreeAndWheelScene.cpp') + '"'
    $testDefine += ' /DWCS_TREE_WHEEL_TEST'
}
# Surface fixtures require an explicit build flag; client builds cannot run these map probes.
if ($TestWorldSurfaces) {
    $sources += '"' + (Join-Path $modRoot 'tests/WorldSurfaceScene.cpp') + '"'
    $testDefine += ' /DWCS_SURFACE_TEST'
}
$buildDirectory = Join-Path $modRoot 'build'
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
# Fingerprint the compiled source/config inputs; support logs distinguish same-version test DLLs.
$fingerprints=@(Get-ChildItem -LiteralPath (Join-Path $modRoot 'src') -Recurse -File | Sort-Object FullName | ForEach-Object {
    $_.FullName.Substring($modRoot.Length)+':'+(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
})
$fingerprints+=(Get-FileHash -LiteralPath (Join-Path $modRoot 'config/WarcraftCS.ini')).Hash
$fingerprints+=$testDefine
$sourceHasher=[Security.Cryptography.SHA256]::Create()
try { $sourceDigest=([BitConverter]::ToString($sourceHasher.ComputeHash([Text.Encoding]::UTF8.GetBytes(($fingerprints -join "`n"))))).Replace('-','').ToLowerInvariant() }
finally { $sourceHasher.Dispose() }
$metadata=Join-Path $buildDirectory 'BuildDiagnostics.hpp'
$buildVersion=(Get-Content -LiteralPath (Join-Path $modRoot 'VERSION') -Raw).Trim()
[IO.File]::WriteAllLines($metadata,@('#pragma once',('#define WCS_BUILD_VERSION "'+$buildVersion+'"'),('#define WCS_BUILD_SOURCE "'+$sourceDigest+'"')),[Text.UTF8Encoding]::new($false))
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
$hookCommands=@('@echo off','chcp 65001 >nul',('call "'+$compilerEnvironment+'" >nul'),'if errorlevel 1 exit /b 1',
    ('cl /nologo /c /MT /O2 /W3 /DWIN32_LEAN_AND_MEAN /I"'+$hookRoot+'" '+($hookSources -join ' ')),
    'if errorlevel 1 exit /b 1',('lib /nologo /OUT:"'+$hookLibrary+'" buffer.obj hook.obj trampoline.obj hde32.obj'))
# Client folders can contain Unicode: write BOM-free UTF-8 and select the matching cmd code page.
[IO.File]::WriteAllLines($hookScript,$hookCommands,[Text.UTF8Encoding]::new($false))
Push-Location $hookBuild
# The working directory owns this fixed filename, avoiding cmd parsing of user-chosen path characters.
try { & $env:COMSPEC /d /c compile.cmd; if ($LASTEXITCODE) { throw 'MinHook x86 build failed.' } } finally { Pop-Location }
$compileCommand = 'cl /nologo /LD /MT /std:c++17 /EHsc /W4 /O2 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /FI"' + $metadata + '"' + $testDefine + ' /I"' + $hookRoot + '" ' + ($sources -join ' ') + ' /link /OUT:"' + (Join-Path $OutputDirectory 'WarcraftCS.mix') + '" "' + $hookLibrary + '" user32.lib gdi32.lib opengl32.lib version.lib winmm.lib ole32.lib'
# A saved batch file avoids nested cmd/PowerShell quoting around Visual Studio paths.
$buildScript = Join-Path $buildDirectory 'compile.cmd'
$loaderCommand = 'cl /nologo /LD /MT /O2 /W4 /DWIN32_LEAN_AND_MEAN "' + (Join-Path $modRoot 'src/runtime/MilesLoader.cpp') + '" "' + [System.IO.Path]::ChangeExtension($exports, '.cpp') + '" /link /DEF:"' + $exports + '" /OUT:"' + (Join-Path $OutputDirectory 'Mss32.dll') + '"'
$commands=@('@echo off','chcp 65001 >nul', 'rem Initialize the x86 compiler, then build the project-owned mod.', ('call "' + $compilerEnvironment + '" >nul'), 'if errorlevel 1 exit /b 1', $compileCommand, 'if errorlevel 1 exit /b 1', $loaderCommand)
[IO.File]::WriteAllLines($buildScript,$commands,[Text.UTF8Encoding]::new($false))
Push-Location $buildDirectory
try {
    & $env:COMSPEC /d /c compile.cmd
    if ($LASTEXITCODE) { throw "Build failed: $LASTEXITCODE" }
} finally { Pop-Location }
Get-Item -LiteralPath (Join-Path $OutputDirectory 'WarcraftCS.mix') | Select-Object FullName, Length
