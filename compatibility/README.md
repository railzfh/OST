# Steam build 1792618285 compatibility metadata

This fork follows upstream `OpenSteam001/OpenSteamTool` main at
`2a08b0bfdc96c9e3f75df20f3f37bc8d8c09ee3a`. The latest upstream release is
`1.4.8`; upstream main contains five subsequent commits. This compatibility
release does not import the experimental v1.5.2 fork or all pending upstream PRs.

## Installation

Close Steam. Merge the packaged `opensteamtool` directory into the Steam root
alongside `dwmapi.dll`, `xinput1_4.dll`, and `OpenSteamTool.dll`. Keep your existing
configuration and Lua files. Copying only the DLLs omits the compatibility fix.

The build automatically includes these files in both Release and Debug output.
Upstream's existing loader still tries its remote sources first and falls back
to the local cache when remote retrieval fails. Future successful upstream
responses for the same hash may replace the local file.

The three TOMLs apply only to these exact DLLs:

| Component | DLL SHA-256 | Contents |
| --- | --- | --- |
| steamui.dll | `cd9226db4e6c28942df70c22c0475d91fdd62cba5f100baae2d65af57cabb610` | 7 function patterns |
| steamclient64.dll | `253e0a11a4415b38c78a42f7bdd6ecc426455f06c1ba6453a48090df3142dbbc` | 21 function patterns, 6 IPC methods across 2 interfaces |

Do not rename the files for another Steam DLL hash. Steam updates can require
new metadata and internal structure changes; a matching file alone does not
establish complete runtime compatibility.

## Origin and validation

The original error was caused by three missing hash-specific files, also absent
from `OpenSteam001/steam-monitor` pattern commit `134c9b19` and IPC commit
`c61b16cc` when checked on 2026-10-24.

The metadata was derived from the exact installed DLLs using historical byte
signatures, PE exception records, Capstone disassembly, embedded function-name
strings, IPC message constants, and vtable positions. REA/Ghidra additionally
confirmed the UI RunFrame call relationships, FillInAppOverview,
BuildCompleteAppOverviewChange, and the 32-bit RepeatedField add operation.

All 28 function signatures were unique in the corresponding DLL's executable
sections. IPC hashes, fenceposts and all six vtable slots were checked against
the client DLL. Function keys use FNV-1a over the actual names requested by the
source, including `FindOrCreateKey`, `ReadAsBinary`, and `SpawnProcess`.
`LoadModuleWithPath` is also retained for the previously installed experimental
build; the current upstream base does not request that function.

Run `python tools/validate_steam_metadata.py` with Python 3.11 or newer to check
TOML syntax, source-required function coverage, FNV-1a keys, and IPC records.
CI runs this check before compilation, and the release workflow checks the
staged metadata in both build configurations.

Runtime evidence from 2026-10-24 covers the **previously installed
aitronz/OpenSteamTool-exp v1.5.2 Debug build**, source `36f6a82`: all 28 patterns
and six IPC methods loaded, initialization completed without startup warnings,
and the Steam library displayed. A shutdown-time `abort()` was observed and
remains unresolved. This does not constitute a runtime test of newly compiled
upstream-main binaries. Game launches, downloads, CloudRedirect, and older Steam
builds were not tested. Releases carrying this metadata default to prerelease.

## Related upstream work

There were 11 open upstream PRs when checked on 2026-10-24. The closest proposals
are [#212](https://github.com/OpenSteam001/OpenSteamTool/pull/212) and
[#218](https://github.com/OpenSteam001/OpenSteamTool/pull/218), both still open.

- #212 proposes selecting a CNetPacket field view using pointer readability,
  with the data pointer moving from offset 0x08 to 0x10. Its author reports tests
  on Steam build 1791918634, not this exact build.
- #218 changes the packet structure unconditionally, switches metadata sources,
  and changes cache handling. It supplies no exact-build verification for
  1792618285. It does not include these three missing TOMLs.

Neither proposal is merged in this release. In particular, the byte-pattern
repair locates RecvPkt but does not update its CNetPacket accesses. Packet layout
compatibility needs separate verification before claiming working downloads.

Recent upstream development included metadata tracking (#55), library refresh
and removals (#84, #86), manifest/ticket handling (#92), platform/IPC/injection
refactoring (#109), stats API (#125), CloudRedirect (#138), and cloud RPC/stats
timing (#153). The complete upstream history is available in the parent
repository's [commit log](https://github.com/OpenSteam001/OpenSteamTool/commits/main/).

## Rollback

Close Steam and remove only the three hash-specific TOMLs supplied by this
directory. No user configuration, Lua script, ownership ticket, or Valve DLL is
included or modified by this metadata package.
