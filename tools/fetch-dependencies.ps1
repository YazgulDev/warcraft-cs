param([string]$Destination='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if (!$Destination) { $Destination=Join-Path $root '.local/dependencies/minhook' }
# Fetch upstream code only into the ignored private cache; immutable revision and archive checksum pin the dependency.
$revision='c3fcafdc10146beb5919319d0683e44e3c30d537'
$sha256='CDCB160F734D81BD4D235DFEA79E3F5A661C8EF0AB74FA814272AA5449069034'
$marker=Join-Path $Destination '.warcraft-cs-revision'
if (Test-Path -LiteralPath $Destination) {
    if ((Test-Path -LiteralPath $marker) -and (Get-Content -LiteralPath $marker -Raw).Trim() -eq $revision) { return }
    throw "Dependency directory already exists without the expected revision marker: $Destination"
}
$cache=Split-Path -Parent $Destination
New-Item -ItemType Directory -Path $cache -Force | Out-Null
$archive=Join-Path $cache "minhook-$revision.zip"
if (!(Test-Path -LiteralPath $archive)) {
    [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -UseBasicParsing -Uri "https://codeload.github.com/TsudaKageyu/minhook/zip/$revision" -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $sha256) { throw 'MinHook archive checksum mismatch.' }
$staging=Join-Path $cache ('unpack-'+[guid]::NewGuid().ToString('N'))
Expand-Archive -LiteralPath $archive -DestinationPath $staging
# Move only the known extracted dependency root; no original install or existing cache is removed.
$extracted=Join-Path $staging "minhook-$revision"
Move-Item -LiteralPath $extracted -Destination $Destination
Set-Content -LiteralPath $marker -Value $revision -Encoding ascii
Write-Output "MinHook v1.3.4 prepared at $Destination"
