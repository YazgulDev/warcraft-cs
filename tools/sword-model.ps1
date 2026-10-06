# Resolve only private owner-supplied models; a remembered choice must never silently become a generated blade.
function Get-WarcraftCsSwordModel([string]$Root, [string]$Requested='') {
    $selected=$Requested
    $settings=Join-Path $Root '.local/setup.json'
    if (!$selected -and (Test-Path -LiteralPath $settings)) {
        $selected=(Get-Content -LiteralPath $settings -Raw | ConvertFrom-Json).sword_model
    }
    $preferred=Join-Path $Root '.local/models/v_grudge_sword.mdl'
    if (!$selected -and (Test-Path -LiteralPath $preferred)) { $selected=$preferred }
    if (!$selected) { return '' }
    if (!(Test-Path -LiteralPath $selected -PathType Leaf)) {
        throw 'The selected private sword model is missing. Restore it or pass -SwordModel with its new path.'
    }
    return (Resolve-Path -LiteralPath $selected).Path
}
