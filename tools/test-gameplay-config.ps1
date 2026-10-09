$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/gameplay-config.ps1')
$folder=Join-Path $root ('.local/config-merge-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$template=Join-Path $root 'config/WarcraftCS.ini'
$config=Join-Path $folder 'legacy.ini'
@('[Runes]','AmmoPercent=37','[AK47]','Price=0','Damage=81','[Damage]','FriendlyFirePercent=25') | Set-Content -LiteralPath $config -Encoding ascii
# Updates append missing keys inside existing sections while retaining all custom values.
Update-GameplayConfig $template $config
$text=Get-Content -LiteralPath $config -Raw
foreach ($required in @('AmmoPercent=37','Price=0','Damage=81','FriendlyFirePercent=25','BombCount=20','Access=anywhere','AmmoPrice=80')) {
    if (!$text.Contains($required)) { throw "Missing/patched preference: $required" }
}
if ([regex]::Matches($text,'(?m)^\[AK47\]').Count -ne 1) { throw 'Duplicate section can hide new INI keys.' }
$before=(Get-FileHash -LiteralPath $config).Hash
Update-GameplayConfig $template $config
if ((Get-FileHash -LiteralPath $config).Hash -ne $before) { throw 'Repeated config update changed an already merged file.' }
$empty=Join-Path $folder 'empty.ini'
[IO.File]::WriteAllText($empty,'');Update-GameplayConfig $template $empty
if (!(Get-Content -LiteralPath $empty -Raw).Contains('BombCount=20')) { throw 'Empty config repair failed.' }
$fresh=Join-Path $folder 'fresh.ini';Update-GameplayConfig $template $fresh
if ((Get-FileHash -LiteralPath $fresh).Hash -ne (Get-FileHash -LiteralPath $template).Hash) { throw 'Fresh config differs from defaults.' }
Write-Output 'PASS fresh/legacy/empty/repeated config updates preserve custom values and expose new settings.'
