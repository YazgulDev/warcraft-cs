# Player setup adapts export data locally with Windows' existing .NET compiler; no C++ toolchain or retail DLL is shipped.
function Convert-MilesForwarders([byte[]]$Template,[byte[]]$Original) {
    if(!('WarcraftCS.Setup.MilesForwarderBuilder' -as [type])) {
        Add-Type -Path (Join-Path $PSScriptRoot 'MilesForwarderBuilder.cs')
    }
    return ,([WarcraftCS.Setup.MilesForwarderBuilder]::Build($Template,$Original))
}
