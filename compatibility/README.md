# Steam build 1792618285 compatibility

This fork follows upstream `OpenSteam001/OpenSteamTool` main at
`2a08b0bfdc96c9e3f75df20f3f37bc8d8c09ee3a`. It does not import the experimental
v1.5.2 fork or all pending upstream PRs.

The `.1` compatibility release bundled missing metadata. The `.2` release also
fixes access to the network packet payload for the exact client DLL below.
Metadata loading and successful initialization alone do not demonstrate that
library, network, or ownership hooks are executing.

## Installation

Close Steam. Extract the Release archive into the Steam root, merging folders.
It contains `dwmapi.dll`, `xinput1_4.dll`, `OpenSteamTool.dll`, and the entire
`opensteamtool` directory. Keep your existing configuration and Lua files. Use
all files from the same release; copying only the DLLs omits the bundled
compatibility metadata. User configuration is not bundled. The Debug archive
contains the same installable files plus PDB/ILK/EXP/LIB development files.

The three TOMLs apply only to these exact DLLs:

| Component | DLL SHA-256 | Contents |
| --- | --- | --- |
| steamui.dll | `cd9226db4e6c28942df70c22c0475d91fdd62cba5f100baae2d65af57cabb610` | 7 function patterns |
| steamclient64.dll | `253e0a11a4415b38c78a42f7bdd6ecc426455f06c1ba6453a48090df3142dbbc` | 21 function patterns, 6 IPC methods across 2 interfaces |

Both Release and Debug builds include these files. The upstream loader tries
remote sources first and falls back to the local cache; a successful remote
response for the same hash may replace a local file. Do not rename a TOML for
another DLL hash. Future Steam updates can also change internal structures.

## Packet layout fix

For the documented client hash, the payload pointer is at `+0x10` and its size
is at `+0x18`. Upstream's legacy structure used `+0x08` and `+0x10` respectively.
Reading the current packet through that legacy structure caused an access
violation during `UnpackRaw`: the supposed payload pointer was `0xffffffff`.

`Steam/NetPacket.h` selects the verified layout only for this exact SHA-256.
Other hashes retain upstream's legacy layout; this preserves existing behavior
and is not a claim of compatibility with every other Steam version. A packet
view reads or writes only the payload pointer and size. Steam always receives
the original packet address.

All four consumers use the view: normal receive handling, packet replacement,
RichPresence carrier injection, and Cloud carrier injection. Both injection
paths restore the original fields after their call to Steam.

This repository already binds the original `steamclient64.dll`. The local
experimental fork also needed a separate repair because it hooked an unused
shadow module. That experimental diversion machinery is not imported here.

## Validation and its limits

The metadata was derived from the exact installed DLLs using historical byte
signatures, PE exception records, Capstone disassembly, embedded names, IPC
constants and vtable slots. REA/Ghidra additionally confirmed the UI RunFrame
relationships, FillInAppOverview, BuildCompleteAppOverviewChange, and the
32-bit repeated-field add operation. All 28 signatures were unique; IPC hashes,
fenceposts and six vtable slots were checked. The unused experimental
`LoadModuleWithPath` entry is retained in the metadata.

The packet offsets were verified from current `steamclient64.dll` helper RVA
`0xD250A0`: native code reads the pointer at `+0x10` and size at `+0x18`.
The original crash and affected tool access were located with the matching
v1.5.2 PDB. The local repair changed all 18 affected displacement operands.

Runtime evidence on 2026-10-24 is from that repaired experimental v1.5.2 Debug
binary, not a newly compiled binary from this repository:

- 52 Lua files supplied 103 unique AppID/Depot entries; these are not 103 games.
- The package hook added all 103 entries to its license list.
- A 12-second sample observed 1,203 IPC calls, 137 ownership checks, 18 receives
  and four current-pipe AppID queries in the actual client module.
- Configured AppID `2483190` appeared in the library with an Update button,
  replacing its earlier Purchase button. Steam remained running after repair.

Native regression tests exercise legacy/current payload reads, replacement,
adjacent-field preservation and restoration, including empty and different DLL
hashes. CI and release builds run these tests with MSVC before building DLLs.
Metadata validation also runs before compilation and against staged output.

Run from the repository root in an x64 MSVC developer shell:

```powershell
python tools/validate_steam_metadata.py
cl /nologo /std:c++20 /EHsc /W4 /WX /Isrc tools/tests/net_packet_test.cpp /Fenet_packet_test.exe
.\net_packet_test.exe
```

The source-built release DLLs have not been installed into a live Steam session.
Downloads and game launches are not validated end to end. Manifest provider
availability remains outside this project. The shutdown-time watcher `abort()` is
fixed in source by detaching watcher threads during process termination and was
verified in the local repair binary; CI compiles the source fix, but this newly
built DLL has not been installed into live Steam. CloudRedirect and RichPresence
were not exercised end to end. A pushed tag publishes both a formal Release
archive and a diagnostic Debug archive.

## Related upstream proposals

PRs [#212](https://github.com/OpenSteam001/OpenSteamTool/pull/212) and
[#218](https://github.com/OpenSteam001/OpenSteamTool/pull/218) also discuss packet
layout changes. This fix uses the exact verified hash rather than a pointer
readability heuristic or an unconditional change for all versions. It does not
change metadata providers or merge either entire proposal.

## Rollback

Close Steam and restore all three loader/tool DLLs from your previous release.
Remove only this package's three hash-specific TOMLs if also reverting the
metadata. The package contains no user configuration, game Lua scripts,
credentials or Valve DLLs.
