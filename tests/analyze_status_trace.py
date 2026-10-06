"""Check player-visible restrictions in traces from the opt-in real spell scene."""
import re
import sys
from pathlib import Path

text = Path(sys.argv[1]).read_text(errors="replace")
pattern = re.compile(r"status oracle unit=\w+ x=([\d.-]+) y=([\d.-]+) speed=([\d.-]+) stun=(\d) root=(\d) disarm=(\d) scale=([\d.-]+) ammo=(\d+) weapon=(\d+)")
states = [tuple(map(float, match.groups())) for match in pattern.finditer(text)]
assert states, "No native status trace found"
stunned = [s for s in states if s[3]]
rooted = [s for s in states if s[4] and not s[3]]
slowed = [s for s in states if not s[4] and 0 < s[6] < .99]
normal = [s for s in states if not s[4] and .99 <= s[6] <= 1.01]
assert stunned and rooted and slowed and normal, "Missing a required native spell phase"
# The scripted take presses movement, jump, fire, reload, switch and refill while stunned.
assert all(s[0:2] == stunned[0][0:2] and s[2] == 0 for s in stunned), "Stun allowed movement"
assert all(s[7:9] == (9, 3) for s in stunned), "Stun allowed reload/refill/fire/switch"
assert all(s[0:2] == rooted[0][0:2] and s[2] == 0 for s in rooted), "Ensnare allowed movement"
assert rooted[0][7] == 9 and min(s[7] for s in rooted) == 8, "Ensnare should still permit shooting"
base_speed = max(s[2] for s in normal)
slow_speed = max(s[2] for s in slowed)
modifier = max(s[6] for s in slowed)
# Require meaningful movement near the expected cap so collisions cannot falsely pass a slow check.
assert base_speed > 250 and base_speed * modifier * .98 <= slow_speed <= base_speed * modifier + .2
recovery = text.find("scale=1.000", text.find("status fixture phase=slow"))
assert recovery >= 0, "Native slow never recovered"
assert "stunCount=1" in text and "native=0.0" in text and "accepted=1" in text
print(f"PASS: native stun/action lock, ensnare with shooting, speed {base_speed:.2f} -> {slow_speed:.2f}, modifier {modifier:.3f}, recovery")
