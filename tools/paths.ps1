# Resolve a standalone checkout first, retaining the existing private gameMesh lab for local development.
function Get-WarcraftCsRuntime([string]$Root) {
    $setupFile=Join-Path $Root '.local/setup.json'
    if (Test-Path -LiteralPath $setupFile) { return (Get-Content -LiteralPath $setupFile -Raw | ConvertFrom-Json).runtime }
    $legacy=Split-Path -Parent (Split-Path -Parent $Root)
    if (Test-Path -LiteralPath (Join-Path $legacy '.tools/universal-modder/Scripts/python.exe')) { return (Join-Path $legacy '.local/warcraft-cs') }
    return (Join-Path $Root '.local/warcraft-cs')
}
function Get-WarcraftCsPython([string]$Root) {
    $localPython=Join-Path $Root '.local/venv/Scripts/python.exe'
    if (Test-Path -LiteralPath $localPython) { return $localPython }
    $legacyPython=Join-Path (Split-Path -Parent (Split-Path -Parent $Root)) '.tools/universal-modder/Scripts/python.exe'
    if (Test-Path -LiteralPath $legacyPython) { return $legacyPython }
    $command=Get-Command python -ErrorAction SilentlyContinue
    if (!$command) { throw 'Install Python 3.10+ or run setup first.' }
    return $command.Source
}
