# Installation profile ownership is separate from additive gameplay-default migrations.
function Set-InstallationProfile([string]$Config,[bool]$WarcraftOnly) {
    $lines=[Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]](Get-Content -LiteralPath $Config -Encoding UTF8))
    $value='WarcraftOnly='+$WarcraftOnly.ToString().ToLowerInvariant()
    $inside=$false;$found=$false;$section=-1
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$inside=$Matches[1] -eq 'Installation';if($inside){$section=$i};continue}
        if($inside -and $lines[$i] -match '^\s*WarcraftOnly\s*='){$lines[$i]=$value;$found=$true}
    }
    if(!$found) {
        if($section -ge 0){$lines.Insert($section+1,$value)}
        else{$lines.Add('');$lines.Add('[Installation]');$lines.Add($value)}
    }
    [IO.File]::WriteAllLines($Config,$lines,[Text.UTF8Encoding]::new($false))
}
