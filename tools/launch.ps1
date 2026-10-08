param([string]$Map = '', [switch]$Menu, [switch]$Windowed,
    [ValidateSet('FrozenThrone','ReignOfChaos')][string]$Edition='FrozenThrone')
$ErrorActionPreference = 'Stop'
$modRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
# Standalone setup owns its runtime path; the legacy workspace lab remains available for existing local saves.
$labRoot=Get-WarcraftCsRuntime $modRoot
$gameExecutable = Join-Path $labRoot 'war3.exe'
if (!(Test-Path -LiteralPath $gameExecutable)) { throw 'The isolated Warcraft-CS game copy has not been prepared.' }
if (Get-Process war3 -ErrorAction SilentlyContinue) { throw 'Close the existing Warcraft III window before starting Warcraft-CS.' }
# Open Warcraft's map/campaign menu by default; an explicit -Map still launches that map directly.
# Native fullscreen is the default; retain an explicit windowed mode for capture/debugging.
$arguments = '-opengl'
# RoC and TFT share war3.exe and the verified native ABI; the campaign menu is selected before startup.
if ($Edition -eq 'ReignOfChaos') { $arguments += ' -classic' }
if ($Windowed) { $arguments += ' -window' }
if ($Map) {
    $mapPath = (Resolve-Path -LiteralPath $Map).Path
    if ([System.IO.Path]::GetExtension($mapPath) -notin @('.w3m', '.w3x')) { throw 'Choose a Warcraft III .w3m/.w3x map.' }
    # Classic Warcraft expects a map under its game directory, addressed relatively.
    $labPrefix = [System.IO.Path]::GetFullPath($labRoot).TrimEnd('\') + '\'
    if (!$mapPath.StartsWith($labPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        $importDirectory = Join-Path $labRoot 'Maps/WarcraftCS'
        New-Item -ItemType Directory -Path $importDirectory -Force | Out-Null
        $mapHash = (Get-FileHash -LiteralPath $mapPath -Algorithm SHA256).Hash.Substring(0,16)
        $importPath = Join-Path $importDirectory ($mapHash + [System.IO.Path]::GetExtension($mapPath))
        Copy-Item -LiteralPath $mapPath -Destination $importPath -Force
        $mapPath = $importPath
    }
    $relativeMap = $mapPath.Substring($labPrefix.Length)
    $arguments += ' -loadfile "' + $relativeMap + '"'
}
$launchInfo = [System.Diagnostics.ProcessStartInfo]::new($gameExecutable, $arguments)
$launchInfo.WorkingDirectory = $labRoot
$launchInfo.UseShellExecute = $false
if (!$Windowed) {
    # Disable Windows bitmap scaling for this launch so fullscreen GL pixels and HUD coordinates agree.
    $inheritedLayers = $launchInfo.EnvironmentVariables['__COMPAT_LAYER']
    if ($inheritedLayers -notmatch '\bHIGHDPIAWARE\b') {
        $launchInfo.EnvironmentVariables['__COMPAT_LAYER'] = ($inheritedLayers + ' HIGHDPIAWARE').Trim()
    }
}
[System.Diagnostics.Process]::Start($launchInfo) | Select-Object Id, ProcessName
