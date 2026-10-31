# Add newly introduced settings without replacing the player's prices, controls or damage preferences.
function Update-GameplayConfig([string]$Template,[string]$Destination) {
    if (!(Test-Path -LiteralPath $Destination)) { Copy-Item -LiteralPath $Template -Destination $Destination;return }
    $defaults=[Collections.Generic.List[object]]::new()
    $section=$null
    foreach ($line in Get-Content -LiteralPath $Template) {
        if ($line -match '^\s*\[([^\]]+)\]\s*$') {
            $section=@{name=$Matches[1];lines=[Collections.Generic.List[string]]::new()}
            $defaults.Add($section)
        }
        if ($null -ne $section) { $section.lines.Add($line) }
    }
    $lines=[Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(Get-Content -LiteralPath $Destination))
    $changed=$false
    # Upgrade former nuclear stock tuning once, while retaining deliberate v3 and customized values.
    $nuclearSection=$false;$nuclearVersion=1;$nuclearVersionIndex=-1;$nuclearValues=@{}
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$nuclearSection=$Matches[1] -eq 'NuclearBomb';continue}
        if(!$nuclearSection -or $lines[$i] -notmatch '^\s*([^=;]+?)\s*=\s*(.*?)\s*$'){continue}
        $key=$Matches[1];$value=$Matches[2].Trim();$nuclearValues[$key]=@($i,$value)
        if($key -eq 'BehaviorVersion'){$nuclearVersionIndex=$i;if(![int]::TryParse($value,[ref]$nuclearVersion)){$nuclearVersion=1}}
    }
    if($nuclearVersion -lt 3) {
        foreach($upgrade in @(@('Radius',1600,8000),@('VisualRadius',1200,3200),@('ExplosionScale',8,12),@('FlameScale',5,7),@('SmokeScale',6,14),@('SmokeSeconds',8,18),@('ScreenEffectRadius',2400,10000))) {
            $saved=$nuclearValues[$upgrade[0]];[double]$oldNuclear=0
            if($saved -and [double]::TryParse($saved[1],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldNuclear) -and
                ($oldNuclear -eq $upgrade[1] -or ($upgrade[0] -eq 'Radius' -and $nuclearVersion -lt 2 -and $oldNuclear -eq 0))) {
                $lines[$saved[0]]=$upgrade[0]+'='+([double]$upgrade[2]).ToString([Globalization.CultureInfo]::InvariantCulture);$changed=$true
            }
        }
        if($nuclearVersionIndex -ge 0){$lines[$nuclearVersionIndex]='BehaviorVersion=3';$changed=$true}
    }
    # The requested enemy-building default upgrades separately from radius/smoke tuning.
    $destructionVersion=1;$destructionVersionIndex=-1
    $saved=$nuclearValues['DestructionVersion']
    if($saved){$destructionVersionIndex=$saved[0];if(![int]::TryParse($saved[1],[ref]$destructionVersion)){$destructionVersion=1}}
    if($destructionVersion -lt 2) {
        $saved=$nuclearValues['DestroyBuildings']
        if($saved -and $saved[1] -in @('false','0')){$lines[$saved[0]]='DestroyBuildings=true';$changed=$true}
        if($destructionVersionIndex -ge 0){$lines[$destructionVersionIndex]='DestructionVersion=2';$changed=$true}
    }
    # Upgrade only the old stock explosion cue; saved city/artillery/off and versioned sapper choices survive.
    $soundVersion=1;$soundVersionIndex=-1;$saved=$nuclearValues['SoundVersion']
    if($saved){$soundVersionIndex=$saved[0];if(![int]::TryParse($saved[1],[ref]$soundVersion)){$soundVersion=1}}
    if($soundVersion -lt 2){
        $saved=$nuclearValues['ExplosionSound']
        if($saved -and $saved[1] -eq 'sapper'){$lines[$saved[0]]='ExplosionSound=layered';$changed=$true}
        if($soundVersionIndex -ge 0){$lines[$soundVersionIndex]='SoundVersion=2';$changed=$true}
    }
    # Upgrade the former stock head spray together once; custom values and versioned opt-backs remain intact.
    $bloodSection=$false;$bloodVersion=1;$bloodVersionIndex=-1;$bloodValues=@{}
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$bloodSection=$Matches[1] -eq 'CombatVisuals';continue}
        if(!$bloodSection -or $lines[$i] -notmatch '^\s*([^=;]+?)\s*=\s*(.*?)\s*$'){continue}
        $key=$Matches[1];$value=$Matches[2].Trim()
        if($key -eq 'BloodVersion'){$bloodVersionIndex=$i;if(![int]::TryParse($value,[ref]$bloodVersion)){$bloodVersion=1}}
        $bloodValues[$key]=@($i,$value)
    }
    if($bloodVersion -lt 2) {
        foreach($upgrade in @(@('BloodCount',16,48),@('BloodSeconds',0.5,0.8),@('BloodScale',1,1.75),@('BloodSpeed',180,220))) {
            $saved=$bloodValues[$upgrade[0]];[double]$oldBlood=0
            if($saved -and [double]::TryParse($saved[1],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldBlood) -and $oldBlood -eq $upgrade[1]) {
                $lines[$saved[0]]=$upgrade[0]+'='+([double]$upgrade[2]).ToString([Globalization.CultureInfo]::InvariantCulture);$changed=$true
            }
        }
        if($bloodVersionIndex -ge 0){$lines[$bloodVersionIndex]='BloodVersion=2';$changed=$true}
    }
    # Upgrade only the former HE stock peak once; keep custom damage, explicit zero and versioned stock choices.
    $heSection=$false;$heDamageIndex=-1;$heVersionIndex=-1;$heVersion='1'
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$heSection=$Matches[1] -eq 'HEGrenade';continue}
        if($heSection -and $lines[$i] -match '^\s*(Damage|DamageVersion)\s*=\s*(.*?)\s*$') {
            if($Matches[1] -eq 'Damage'){$heDamageIndex=$i}else{$heVersionIndex=$i;$heVersion=$Matches[2].Trim()}
        }
    }
    if($heVersion -in @('','1')) {
        [double]$oldHE=0
        if($heDamageIndex -ge 0 -and [double]::TryParse(($lines[$heDamageIndex] -split '=',2)[1].Trim(),[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldHE) -and $oldHE -eq 100) {
            $lines[$heDamageIndex]='Damage=300';$changed=$true
        }
        if($heVersionIndex -ge 0){$lines[$heVersionIndex]='DamageVersion=2';$changed=$true}
    }
    # Upgrade only previous stock energy values once. User tuning, including explicit zero, remains authoritative.
    $energySection='';$energyVersion='1';$energyVersionIndex=-1;$energyValues=@{}
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$energySection=$Matches[1];continue}
        if($lines[$i] -match '^\s*(Damage|ChargedDamage|DamageVersion)\s*=\s*(.*?)\s*$') {
            $key=$Matches[1];$value=$Matches[2].Trim()
            if($energySection -eq 'HalfLife' -and $key -eq 'DamageVersion'){$energyVersion=$value;$energyVersionIndex=$i}
            if($energySection -in @('TauCannon','GluonGun')){$energyValues[$energySection+'.'+$key]=@($i,$value)}
        }
    }
    if($energyVersion -in @('','1')) {
        foreach($upgrade in @(@('TauCannon.Damage',20,100),@('TauCannon.ChargedDamage',200,1000),@('GluonGun.Damage',14,70))) {
            $saved=$energyValues[$upgrade[0]];[double]$oldValue=0
            if($saved -and [double]::TryParse($saved[1],[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldValue) -and $oldValue -eq $upgrade[1]) {
                $lines[$saved[0]]=($upgrade[0] -split '\.')[1]+'='+$upgrade[2];$changed=$true
            }
        }
        if($energyVersionIndex -ge 0){$lines[$energyVersionIndex]='DamageVersion=2';$changed=$true}
    }
    # Migrate only the former RPG stock radius once; preserve every other custom blast and damage value.
    $rpgSection=$false;$radiusIndex=-1;$blastIndex=-1;$blastVersion='1'
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$rpgSection=$Matches[1] -eq 'RPG';continue}
        if($rpgSection -and $lines[$i] -match '^\s*(Radius|BlastVersion)\s*=\s*(.*?)\s*$') {
            if($Matches[1] -eq 'Radius'){$radiusIndex=$i}else{$blastIndex=$i;$blastVersion=$Matches[2].Trim()}
        }
    }
    if($blastVersion -in @('','1')) {
        if($radiusIndex -ge 0) {
            [double]$oldRadius=0
            if([double]::TryParse(($lines[$radiusIndex] -split '=',2)[1].Trim(),[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldRadius) -and $oldRadius -eq 375) {
                $lines[$radiusIndex]='Radius=525';$changed=$true
            }
        }
        if($blastIndex -ge 0){$lines[$blastIndex]='BlastVersion=2';$changed=$true}
    }
    # Apply the latest balance after historical upgrades; versioned opt-backs and non-stock values stay intact.
    foreach($upgrade in @(
        @('RPG','Damage','DamageVersion',2,100,1800),
        @('RPG','Radius','BlastVersion',3,525,600),
        @('HEGrenade','Damage','DamageVersion',3,300,600),
        @('M3','HeroMultiplier','MultiplierVersion',2,3,6),
        @('XM1014','HeroMultiplier','MultiplierVersion',2,2.4,4.8))) {
        $balanceSection='';$valueIndex=-1;$versionIndex=-1;$savedVersion=1
        for($i=0;$i -lt $lines.Count;$i++) {
            if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$balanceSection=$Matches[1];continue}
            if($balanceSection -ne $upgrade[0] -or $lines[$i] -notmatch '^\s*([^=;]+?)\s*=\s*(.*?)\s*$'){continue}
            if($Matches[1] -eq $upgrade[1]){$valueIndex=$i}
            if($Matches[1] -eq $upgrade[2]){
                $versionIndex=$i
                if(![int]::TryParse($Matches[2].Trim(),[ref]$savedVersion)){$savedVersion=0}
            }
        }
        if($savedVersion -ge $upgrade[3]){continue}
        [double]$savedValue=0
        if($valueIndex -ge 0 -and [double]::TryParse(($lines[$valueIndex] -split '=',2)[1].Trim(),[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$savedValue) -and $savedValue -eq $upgrade[4]) {
            $lines[$valueIndex]=$upgrade[1]+'='+([double]$upgrade[5]).ToString([Globalization.CultureInfo]::InvariantCulture);$changed=$true
        }
        if($versionIndex -ge 0){$lines[$versionIndex]=$upgrade[2]+'='+$upgrade[3];$changed=$true}
    }
    # Upgrade the complete old stock trajectory once; every custom speed/gravity pair remains intact.
    $grenadeSection=$false;$throwIndex=-1;$gravityIndex=-1;$trajectoryIndex=-1;$trajectoryVersion='1'
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$grenadeSection=$Matches[1] -eq 'Grenades';continue}
        if($grenadeSection -and $lines[$i] -match '^\s*(ThrowSpeed|Gravity|TrajectoryVersion)\s*=\s*(.*?)\s*$') {
            switch($Matches[1]) {
                'ThrowSpeed' {$throwIndex=$i}
                'Gravity' {$gravityIndex=$i}
                'TrajectoryVersion' {$trajectoryIndex=$i;$trajectoryVersion=$Matches[2].Trim()}
            }
        }
    }
    if($trajectoryVersion -in @('','1')) {
        if($throwIndex -ge 0 -and $gravityIndex -ge 0) {
            [double]$oldThrow=0;[double]$oldGravity=0
            $parseThrow=[double]::TryParse(($lines[$throwIndex] -split '=',2)[1].Trim(),[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldThrow)
            $parseGravity=[double]::TryParse(($lines[$gravityIndex] -split '=',2)[1].Trim(),[Globalization.NumberStyles]::Float,[Globalization.CultureInfo]::InvariantCulture,[ref]$oldGravity)
            if($parseThrow -and $parseGravity -and $oldThrow -eq 750 -and $oldGravity -eq 800) {
                $lines[$throwIndex]='ThrowSpeed=1125';$lines[$gravityIndex]='Gravity=660';$changed=$true
            }
        }
        if($trajectoryIndex -ge 0){$lines[$trajectoryIndex]='TrajectoryVersion=2';$changed=$true}
    }
    $oldSection='';$customStride=$false
    foreach($existing in $lines) {
        if($existing -match '^\s*\[([^\]]+)\]\s*$'){$oldSection=$Matches[1];continue}
        if($oldSection -eq 'ThirdPerson' -and $existing -match '^\s*RunCycleDistance\s*=\s*(\d+(?:\.\d+)?)\s*$') {
            $stride=[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)
            if($stride -ge 40 -and $stride -le 1000 -and $stride -ne 210){$customStride=$true}
        }
    }
    # Only the old default changes: F7 is now TPS entry. Preserve custom refill keys and the rest of the layout.
    $shortcutSection=$false;$refillIndex=-1;$thirdBinding=$false;$unitViewBinding=$false;$allWeaponsIndex=-1;$oldSquadIndex=-1
    $usedShortcuts=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$shortcutSection=$Matches[1] -eq 'Shortcuts';continue}
        if($shortcutSection -and $lines[$i] -match '^\s*([^;#=]+?)\s*=\s*(.*?)\s*$') {
            $name=$Matches[1].Trim();$value=$Matches[2].Trim();[void]$usedShortcuts.Add($value)
            if($name -eq 'ToggleThirdPerson'){$thirdBinding=$true}
            if($name -eq 'ToggleUnitView'){$unitViewBinding=$true}
            if($name -eq 'AllWeapons' -and $value -eq 'F9'){$allWeaponsIndex=$i}
            if($name -eq 'Refill' -and $value -eq 'F7'){$refillIndex=$i}
            if($name -eq 'SquadOrders' -and $value -in @('SEMICOLON',';')){$oldSquadIndex=$i}
        }
    }
    if(!$thirdBinding -and $refillIndex -ge 0) {
        $replacement='NONE'
        foreach($candidate in @('F11','F12','F5','F4')) {if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
        $lines[$refillIndex]='Refill='+$replacement;$changed=$true
        # A later F9 migration must see the newly reserved refill key, rather than assign it twice.
        [void]$usedShortcuts.Remove('F7');[void]$usedShortcuts.Add($replacement)
    }
    # F9 is the requested independent unit camera. Only its old all-weapons default is relocated.
    if(!$unitViewBinding -and $allWeaponsIndex -ge 0) {
        $replacement='NONE'
        foreach($candidate in @('F12','F5','F4')){if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
        $lines[$allWeaponsIndex]='AllWeapons='+$replacement;$changed=$true
        [void]$usedShortcuts.Remove('F9');[void]$usedShortcuts.Add($replacement)
    }
    # Move only the former squad default to Y; a saved Y action and custom squad keys remain authoritative.
    if($oldSquadIndex -ge 0) {
        $replacement=if($usedShortcuts.Contains('Y')){'NONE'}else{'Y'}
        $lines[$oldSquadIndex]='SquadOrders='+$replacement;$changed=$true
        [void]$usedShortcuts.Add($replacement)
    }
    foreach ($entry in $defaults) {
        $start=-1;$end=$lines.Count
        for ($i=0;$i -lt $lines.Count;$i++) {
            if ($lines[$i] -match '^\s*\[([^\]]+)\]\s*$') {
                if ($start -ge 0) { $end=$i;break }
                if ($Matches[1] -eq $entry.name) { $start=$i }
            }
        }
        if ($start -lt 0) {
            $lines.Add('');$lines.AddRange([string[]]$entry.lines);$changed=$true;continue
        }
        $keys=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        for ($i=$start+1;$i -lt $end;$i++) {
            if ($lines[$i] -match '^\s*([^;#=]+?)\s*=') { [void]$keys.Add($Matches[1].Trim()) }
        }
        $comments=[Collections.Generic.List[string]]::new()
        foreach ($line in $entry.lines) {
            if ($line -match '^\s*;') { $comments.Add($line);continue }
            if ($line -match '^\s*([^;#=]+?)\s*=') {
                # Insert into the original section, never append a duplicate INI section ignored by Win32.
                if ($keys.Add($Matches[1].Trim())) {
                    # Preserve an existing chosen manual stride rather than hiding it behind the new automatic default.
                    if($entry.name -eq 'ThirdPerson' -and $Matches[1].Trim() -eq 'AutoAnimationSpeed' -and $customStride){$line='AutoAnimationSpeed=false'}
                    # Adding Q must not conflict with a custom action that already uses it.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'ToggleAllyTargeting' -and $usedShortcuts.Contains('Q')){$line='ToggleAllyTargeting=NONE'}
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'ToggleUnitView' -and $usedShortcuts.Contains('F9')){$line='ToggleUnitView=NONE'}
                    # Existing custom Tab/I controls win over newly introduced inventory defaults.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'InventoryNumbers' -and $usedShortcuts.Contains('TAB')){$line='InventoryNumbers=NONE'}
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'InventoryPanel' -and $usedShortcuts.Contains('I')){$line='InventoryPanel=NONE'}
                    # Adding the new Y default must not overwrite another saved action.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'SquadOrders' -and $usedShortcuts.Contains('Y')){$line='SquadOrders=NONE'}
                    # U remains opt-in for AI; another saved action using it takes priority during migration.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'SettingsMenu') {
                        # A new settings shortcut must yield to existing custom function keys.
                        $settingsKey='NONE'
                        foreach($candidate in @('F2','F3','F4','F5')) {if(!$usedShortcuts.Contains($candidate)){$settingsKey=$candidate;break}}
                        $line='SettingsMenu='+$settingsKey;[void]$usedShortcuts.Add($settingsKey)
                    }
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'ArmyAI'  -and $usedShortcuts.Contains('U')){$line='ArmyAI=NONE'}
                    # A migrated/custom refill may already own the new F12 default in a partial old layout.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'AllWeapons' -and $usedShortcuts.Contains('F12')) {
                        $replacement='NONE'
                        foreach($candidate in @('F5','F4')){if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
                        $line='AllWeapons='+$replacement
                    }
                    foreach ($comment in $comments) { $lines.Insert($end,$comment);$end++ }
                    $lines.Insert($end,$line);$end++;$changed=$true
                }
            }
            $comments.Clear()
        }
    }
    if ($changed) {
        # Atomic replacement keeps interrupted updates from leaving a partial settings file.
        $temporary=$Destination+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
        try {
            [IO.File]::WriteAllLines($temporary,$lines,[Text.UTF8Encoding]::new($false))
            [IO.File]::Replace($temporary,$Destination,[NullString]::Value)
        } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary } }
    }
}
