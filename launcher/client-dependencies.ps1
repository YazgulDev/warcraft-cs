# Download only after the caller's consent gate. Pinned Python and signed Microsoft installers are checked before execution.
function Get-ClientDownload([string]$Url,[string]$Destination,[string]$Publisher,[string]$Hash='') {
    if (!(Test-Path -LiteralPath $Destination)) {
        [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
        Write-Output "Downloading $([IO.Path]::GetFileName($Destination))..." | Out-Host
        $temporary=$Destination+'.partial'
        Invoke-WebRequest -UseBasicParsing -Uri $Url -OutFile $temporary
        Move-Item -LiteralPath $temporary -Destination $Destination
    }
    if ($Hash -and (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash -ne $Hash) { throw 'Dependency checksum mismatch.' }
    $signature=Get-AuthenticodeSignature -LiteralPath $Destination
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch $Publisher) { throw 'Dependency publisher signature verification failed.' }
    return $Destination
}
function Test-ClientPython([string]$Executable) {
    if (!$Executable -or !(Test-Path -LiteralPath $Executable -PathType Leaf) -or $Executable -match '\\WindowsApps\\') { return $false }
    # Verify venv/pip support as well as version; store aliases must not trigger an interactive download.
    try {
        & $Executable -c 'import sys,venv,ensurepip; sys.exit(0 if sys.version_info >= (3,10) and sys.version_info < (3,15) else 1)' *> $null
        return $LASTEXITCODE -eq 0
    } catch { return $false }
}
function Get-ClientPython([string]$Install,[string]$Explicit='') {
    $local=Join-Path $Install 'dependencies/python/python.exe'
    $command=Get-Command python.exe -ErrorAction SilentlyContinue
    $candidates=@($Explicit,$local,$command.Source)
    # Reuse registered Python installations even when the user did not put Python on PATH.
    foreach ($registry in @('HKCU:/Software/Python/PythonCore','HKLM:/Software/Python/PythonCore')) {
        foreach ($version in @(Get-ChildItem -LiteralPath $registry -ErrorAction SilentlyContinue)) {
            $location=Get-Item -LiteralPath (Join-Path $version.PSPath 'InstallPath') -ErrorAction SilentlyContinue
            if ($location -and $location.GetValue('')) { $candidates+=Join-Path $location.GetValue('') 'python.exe' }
        }
    }
    foreach ($candidate in $candidates) {
        if (Test-ClientPython $candidate) { Write-Output "Using Python: $candidate" | Out-Host; return $candidate }
    }
    $cache=Join-Path $Install 'downloads';New-Item -ItemType Directory -Path $cache -Force | Out-Null
    $installer=Get-ClientDownload 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe' (Join-Path $cache 'python-3.12.10-amd64.exe') 'CN=Python Software Foundation' '67B5635E80EA51072B87941312D00EC8927C4DB9BA18938F7AD2D27B328B95FB'
    $target=Split-Path -Parent $local
    Write-Output 'Installing private Python (no PATH changes)...' | Out-Host
    $arguments='/quiet InstallAllUsers=0 TargetDir="'+$target+'" PrependPath=0 AssociateFiles=0 Include_launcher=0 Include_test=0 Include_doc=0 Include_tcltk=0 Shortcuts=0'
    $process=Start-Process -FilePath $installer -ArgumentList $arguments -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -notin @(0,3010) -or !(Test-ClientPython $local)) { throw 'Python installation did not complete. See the vendor installer logs and try again.' }
    return $local
}
function Get-ClientCompiler {
    $locator=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (!(Test-Path -LiteralPath $locator)) { return '' }
    $root=& $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $sdk=(Get-ItemProperty -LiteralPath 'HKLM:/SOFTWARE/Microsoft/Windows Kits/Installed Roots' -ErrorAction SilentlyContinue).KitsRoot10
    $sdkReady=$false
    if ($sdk) {
        foreach ($headers in @(Get-ChildItem -LiteralPath (Join-Path $sdk 'Include') -Directory -ErrorAction SilentlyContinue)) {
            if ((Test-Path -LiteralPath (Join-Path $headers.FullName 'um/windows.h')) -and (Test-Path -LiteralPath (Join-Path $headers.FullName 'ucrt/stdio.h'))) { $sdkReady=$true }
        }
    }
    if ($root -and $sdkReady -and (Test-Path -LiteralPath (Join-Path $root 'VC/Auxiliary/Build/vcvars32.bat'))) { return $root }
    return ''
}
function Install-ClientBuildTools([string]$Install) {
    if (Get-ClientCompiler) { Write-Output 'Using installed C++ build tools.';return }
    $cache=Join-Path $Install 'downloads';New-Item -ItemType Directory -Path $cache -Force | Out-Null
    $installer=Get-ClientDownload 'https://aka.ms/vs/17/release/vs_BuildTools.exe' (Join-Path $cache 'vs_buildtools.exe') 'CN=Microsoft Corporation'
    Write-Output 'Installing Microsoft C++ tools and Windows SDK. Approve the Windows administrator prompt if shown.'
    # Microsoft's supported interactive installer owns UAC/progress; never bypass elevation or force a reboot.
    $process=Start-Process -FilePath $installer -ArgumentList '--wait --passive --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended' -WindowStyle Normal -Wait -PassThru
    if ($process.ExitCode -eq 3010) { throw 'Microsoft tools require a restart. Restart Windows, then run Install again.' }
    if ($process.ExitCode -ne 0 -or !(Get-ClientCompiler)) { throw 'Microsoft C++ tools/Windows SDK installation did not complete. Re-run Install after resolving the vendor installer message.' }
}
