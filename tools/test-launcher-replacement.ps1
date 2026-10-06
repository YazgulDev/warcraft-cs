$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$fixture=Join-Path $root ('build/replacement-tests-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
$shell=Join-Path $env:WINDIR 'System32/WindowsPowerShell/v1.0/powershell.exe'
$target=Join-Path $fixture 'Launcher.exe'
$replacement=Join-Path $fixture 'Next.exe'
# Windowless disposable executables exercise the real delayed replacement and restart, without opening the launcher or a game.
foreach ($version in @('old','new')) {
    $source=Join-Path $fixture ($version+'.cs')
    ('using System; using System.IO; using System.Reflection; using System.Threading; class Fixture { static void Main(string[] args) {'+
     'if (args.Length>0) { Thread.Sleep(1500); return; } File.WriteAllText(Assembly.GetExecutingAssembly().Location+".started","'+$version+'"); }}') |
        Set-Content -LiteralPath $source -Encoding UTF8
    $output=if ($version -eq 'old') { $target } else { $replacement }
    & $compiler /nologo /target:winexe "/out:$output" $source
    if ($LASTEXITCODE) { throw 'Replacement fixture compilation failed.' }
}
$oldHash=(Get-FileHash -LiteralPath $target).Hash
$newHash=(Get-FileHash -LiteralPath $replacement).Hash
$requestFile=Join-Path $fixture 'request.json'
$parent=Start-Process -FilePath $target -ArgumentList 'wait' -WindowStyle Hidden -PassThru
@{ParentId=$parent.Id;Replacement=$replacement;Target=$target;Sha256=$newHash} |
    ConvertTo-Json | Set-Content -LiteralPath $requestFile -Encoding UTF8
& $shell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'launcher/replace-launcher.ps1') -RequestFile $requestFile
if ($LASTEXITCODE) { throw 'Delayed launcher replacement failed.' }
for ($attempt=0;$attempt -lt 100 -and !(Test-Path -LiteralPath ($target+'.started'));$attempt++) { Start-Sleep -Milliseconds 50 }
if ((Get-FileHash -LiteralPath $target).Hash -ne $newHash -or
    (Get-FileHash -LiteralPath ($target+'.previous')).Hash -ne $oldHash -or
    (Get-Content -LiteralPath ($target+'.started') -Raw) -ne 'new') { throw 'Launcher replacement, backup or restart failed.' }
# A corrupt cached executable must neither overwrite nor restart the installed launcher.
Remove-Item -LiteralPath ($target+'.started')
@{ParentId=$parent.Id;Replacement=$replacement;Target=$target;Sha256=('0'*64)} |
    ConvertTo-Json | Set-Content -LiteralPath $requestFile -Encoding UTF8
& $shell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'launcher/replace-launcher.ps1') -RequestFile $requestFile
if ($LASTEXITCODE -ne 1 -or (Get-FileHash -LiteralPath $target).Hash -ne $newHash -or
    (Test-Path -LiteralPath ($target+'.started'))) { throw 'Corrupt replacement was not rejected safely.' }
$global:LASTEXITCODE=0
Write-Output 'PASS delayed launcher replacement, previous-version backup, restart and corrupt executable rejection'
