# App-local compatibility files are independent of game assets and never installed into Windows.
function Get-Direct3DHashes {
    return @{
        'd3dx9_43.dll'='2fea401698db2764ec5943de6634bf0eb5dbe191a963b1ef3620a027730b83e9'
        'd3dcompiler_43.dll'='aaca0bac2e1e94966f4546b6234333f9cdc1a4599024e30cd20aef4c1fd1b4f1'
    }
}
function Assert-Direct3DFiles([string]$Directory) {
    $metadata=Join-Path $Directory 'direct3d-runtime.json'
    if(!(Test-Path -LiteralPath $metadata -PathType Leaf)){throw 'Direct3D build metadata is missing.'}
    $built=Get-Content -LiteralPath $metadata -Raw -Encoding UTF8 | ConvertFrom-Json
    # Local compiler output varies by toolchain; bind its checksum to the reviewed upstream source and /MT build.
    if($built.FormatVersion -ne 1 -or $built.BridgeRevision -cne '255338f698c8270b537f0a91a13f795f4f988250' -or
        $built.Runtime -cne 'static-msvc' -or $built.BridgeSha256 -cnotmatch '^[0-9a-f]{64}$') {throw 'Unsupported Direct3D bridge build.'}
    $hashes=Get-Direct3DHashes;$hashes['d3d8.dll']=$built.BridgeSha256
    foreach($name in $hashes.Keys) {
        $path=Join-Path $Directory $name
        if(!(Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -cne $hashes[$name]) {
            throw "Missing or modified Direct3D compatibility component: $name"
        }
    }
}
function Install-Direct3DRuntime([string]$Directory,[string]$Runtime) {
    Assert-Direct3DFiles $Directory
    Assert-OrdinaryRuntimePath $Runtime
    if(!(Test-Path -LiteralPath (Join-Path $Runtime '.warcraft-cs-private-runtime') -PathType Leaf)) {
        throw 'Direct3D components require a marked private Warcraft CS runtime.'
    }
    $names=@('d3d8.dll','d3dx9_43.dll','d3dcompiler_43.dll')
    $notices=Join-Path $Runtime 'WarcraftCS/notices'
    foreach($name in $names){Assert-OrdinaryRuntimePath (Join-Path $Runtime $name)}
    Assert-OrdinaryRuntimePath $notices
    Assert-OrdinaryRuntimePath (Join-Path $Runtime 'direct3d-runtime.json')
    New-Item -ItemType Directory -Path $notices -Force | Out-Null
    foreach($name in $names) {
        $target=Join-Path $Runtime $name
        # Preserve a pre-existing private graphics wrapper before setup replaces it with the reviewed version.
        if((Test-Path -LiteralPath $target -PathType Leaf) -and
            (Get-FileHash -LiteralPath $target).Hash -ne (Get-FileHash -LiteralPath (Join-Path $Directory $name)).Hash) {
            $backup=Join-Path $Runtime ('WarcraftCS/backups/graphics/'+[guid]::NewGuid().ToString('N')+'/'+$name)
            Assert-OrdinaryRuntimePath $backup
            New-Item -ItemType Directory -Path (Split-Path -Parent $backup) -Force | Out-Null
            Copy-Item -LiteralPath $target -Destination $backup
        }
        Copy-Item -LiteralPath (Join-Path $Directory $name) -Destination $target -Force
    }
    Copy-Item -LiteralPath (Join-Path $Directory 'direct3d-runtime.json') -Destination (Join-Path $Runtime 'direct3d-runtime.json') -Force
    foreach($name in @('d3d8to9-BSD-2-Clause.txt','Microsoft-DXSDK-D3DX.txt')) {
        $target=Join-Path $notices $name;Assert-OrdinaryRuntimePath $target
        Copy-Item -LiteralPath (Join-Path $Directory ('licenses/'+$name)) -Destination $target -Force
    }
    Write-Output 'Prepared Direct3D 8 to 9 and D3DX locally; OpenGL remains available. No system installation required.'
}
