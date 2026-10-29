param([string]$Destination='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/direct3d-runtime.ps1')
if(!$Destination){$Destination=Join-Path $root '.local/dependencies/direct3d'}
New-Item -ItemType Directory -Path (Join-Path $Destination 'licenses') -Force | Out-Null
function Get-PinnedFile([string]$Uri,[string]$Path,[string]$Hash) {
    # Verify every cache hit and download; a mutable upstream URL cannot silently replace reviewed code.
    if(!(Test-Path -LiteralPath $Path)) {
        [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
        $temporary=$Path+'.download-'+[guid]::NewGuid().ToString('N')
        Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $temporary
        if((Get-FileHash -LiteralPath $temporary).Hash -ne $Hash){throw 'Direct3D dependency download checksum mismatch.'}
        Move-Item -LiteralPath $temporary -Destination $Path
    }
    if((Get-FileHash -LiteralPath $Path).Hash -ne $Hash){throw "Direct3D dependency cache checksum mismatch: $Path"}
}
$archive=Join-Path $Destination 'microsoft.dxsdk.d3dx.9.29.952.8.nupkg'
Get-PinnedFile 'https://api.nuget.org/v3-flatcontainer/microsoft.dxsdk.d3dx/9.29.952.8/microsoft.dxsdk.d3dx.9.29.952.8.nupkg' $archive 'ead0906ae8a26c18a7525da7490127a2110f7c58f18293738283e30e97c6ea4b'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip=[IO.Compression.ZipFile]::OpenRead($archive)
try {
    # Only the official x86 redistributable is needed; no legacy SDK or elevated installer is run.
    foreach($name in @('d3dx9_43.dll','d3dcompiler_43.dll')) {
        $entry=@($zip.Entries | Where-Object {$_.FullName -ieq ('build/native/release/bin/x86/'+$name)})[0]
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry,(Join-Path $Destination $name),$true)
    }
} finally {$zip.Dispose()}
$bridgeReady=$false;try{Assert-Direct3DFiles $Destination;$bridgeReady=$true}catch{}
if(!$bridgeReady) {
    # Build the pinned BSD bridge instead of shipping an upstream binary that requires a global VC runtime.
    $bridgeArchive=Join-Path $Destination 'd3d8to9-v1.16.0.zip'
    Get-PinnedFile 'https://codeload.github.com/crosire/d3d8to9/zip/refs/tags/v1.16.0' $bridgeArchive '3b4e8263f8792cc839cc3bd2576d5f74082a664997a2f8a4766e08cfd881b13f'
    $staging=Join-Path $Destination ('source-'+[guid]::NewGuid().ToString('N'))
    Expand-Archive -LiteralPath $bridgeArchive -DestinationPath $staging
    & (Join-Path $PSScriptRoot 'build-direct3d-bridge.ps1') -SourceDirectory (Join-Path $staging 'd3d8to9-1.16.0') -OutputDirectory $Destination
}
Assert-Direct3DFiles $Destination
foreach($name in @('d3d8to9-BSD-2-Clause.txt','Microsoft-DXSDK-D3DX.txt')) {
    Copy-Item -LiteralPath (Join-Path $root ('licenses/'+$name)) -Destination (Join-Path $Destination ('licenses/'+$name)) -Force
}
Write-Output "Verified Direct3D compatibility runtime: $Destination"
