# Validation of the 0.1.0 source baseline

Windows, native x86 Warcraft III 1.26a / Game.dll 1.26.0.6401, OpenGL, offline private runtime.
No game footage, game content, raw memory dumps or personal campaign saves are included here.

Seven C++ suites independently validate movement, look-angle wrapping, model-bound ray contact,
melee timing/range, friendly/C4/AWP damage policy, INI/ammo calculations and classic recoil.
The recoil oracles check published coefficients, burst timers and the known ran1 seed-one sequence.

Native private stock-map checks confirmed AK first kick:1/0.9/1.5 degrees standing/crouched/moving;
M4:0.65/0.6/1; USP/AWP:2, sustained AK vertical cap5.75 and lateral cap1.75.
O/H recruitment and J release were tested in both modes with real owned footmen and native orders.
No further formation orders followed release, and FPS remained enabled.
One harness wrongly expected exactly two recruits after moving closer to starting workers;
repositioning the fixture confirmed the two-footman oracle without changing ownership filtering.

Earlier native checks covered FPS/RTS attack acquisition, real rune healing and one-time ammo reward,
ordinary equipment/full Backpack handling, H/O/H combat restoration, destructable gate leaves,
hero-model hiding, fullscreen/raw mouse rotation and OpenGL context recreation after Alt-Tab.
Native Backpack rejects auto-use runes even after research; the mod preserves that rule.
Friendly damage coefficient is0.5; C4 base damage2500 is still processed by native Warcraft damage rules.

The normal installed baseline loaded a preserved night-elf campaign with Maiev650HP,
correct physical camera position, J/F8 and no private-fixture/controller-fault output.
All campaign missions, map triggers and every custom map have not been verified.
Fresh source-archive setup was also checked outside the original development layout:
it created a separate owned Warcraft runtime, installed NumPy in a private venv,
verified the pinned MinHook checksum, built MinHook and the mod with MSVC x86,
generated Miles forwarding code locally and converted the owner's six CS models.
No prebuilt dependency from the private research checkouts was required.

The resulting normal build started a real Lost Temple map with all native hooks
reporting successful creation/enabling, raw mouse input and FPS rendering active.
All seven viewmodels loaded, including the procedural sword fallback; J and F8
worked without a controller fault. This confirms standalone setup and a native
smoke check; it does not establish complete campaign/custom-map compatibility.

The complete staged tree passed the source allowlist/credential/binary/path audit.
Adversarial audit checks rejected fake credentials, renamed binaries, game assets,
private paths and decompiler headers. An early audit pattern matched its own pattern
text; anchoring the header check fixed that false positive before publication.

## 0.2.0 original sword and English README

The release converter always generates the project's original straight silver blade,
gold guard and leather grip. An existing private Grudge model is not selected or read.
External sword parameters/imports were removed from both setup and conversion.
An owned-CS conversion produced the original procedural sword cache successfully;
its rig/animation data and audio timeline come from the owner's knife as before.
The sword geometry itself is unchanged from the previously verified original sword.
Runtime C++ and combat behavior are unchanged in this update.

PowerShell/Python syntax checks and source-tree audits were run for the changed tooling.
The complete English README includes requirements, setup, controls, settings, limitations
and legal references. Linked relative documents remain in the release tree.
No fresh automated input was sent to the user's active game for this documentation/tooling update.

## 0.3.0 standalone Windows client launcher

The WinForms EXE was built with the Windows .NET Framework compiler. Its embedded ZIP
contains the audited project source, without game assets, downloaded dependencies or build output.
An offscreen preview verified the complete folder-selection, consent and progress layout.
UI tests confirm that valid folders alone cannot enable Install: explicit agreement is required,
and revoking agreement disables it again. Runner/extractor and PowerShell no-consent tests
reject installation before creating files. Path/JSON tests include archive traversal rejection
and protection against installing into the original game folders or a drive root.

The real pinned Python installer passed SHA256 and PSF signature verification, and the real
Microsoft bootstrapper passed Microsoft signature verification. Negative checksum/publisher
checks rejected untrusted downloads. Existing Python and Visual Studio/SDK were reused for
the complete installation tests; fresh vendor installation on a clean Windows machine has
not been tested.

A complete C# runner installation extracted its embedded source into a separate client folder,
created an owned Warcraft copy, installed NumPy in a private venv, verified/downloaded MinHook,
converted the owner's CS models and built the normal 372736-byte native mod. The selected
installation path included Cyrillic characters, spaces, an ampersand and an apostrophe.
The completion marker and Play configuration point inside that client folder.
This test exposed and verified fixes for inherited PowerShell Core module paths, Windows
PowerShell UTF-8 JSON decoding and Unicode paths in the native build command file.

The user's active Warcraft session was left running and received no automated input.
Play was checked through its installed configuration; a fresh game launch was not performed
for this installer feature. Native gameplay behavior is unchanged by the launcher.
All seven independent C++ logic suites and launcher regression checks passed again
when preparing 0.3.0. The release EXE reports assembly version 0.3.0.0 and includes
the English installation guide and source-only release tree.
