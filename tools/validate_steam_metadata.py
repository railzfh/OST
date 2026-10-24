"""Check bundled Steam metadata against the function names used by this source tree."""
import argparse
import pathlib
import re
import sys
import tomllib

ROOT = pathlib.Path(__file__).resolve().parents[1]
HASH = re.compile(r"[0-9a-f]{64}")


def fnv1a(name):
    result = 0x811C9DC5
    for byte in name.encode():
        result = ((result ^ byte) * 0x1000193) & 0xFFFFFFFF
    return result


def validate(root):
    required = {"steamui": set(), "steamclient": set()}
    calls = re.compile(
        r"(?m)^\s*(INSTALL_HOOK_[CUD]|RESOLVE_[CU]|ARM_CAPTURE_[CU]|ARM_INT3_[CU])"
        r"\((\w+)[,)]"
    )
    for source in (ROOT / "src").rglob("*.cpp"):
        for macro, name in calls.findall(source.read_text(encoding="utf-8-sig")):
            required["steamui" if macro.endswith("_U") else "steamclient"].add(name)
    count = 0
    client_hashes = set()
    for component, names in required.items():
        files = sorted((root / "pattern" / component).glob("*.toml"))
        assert files, f"No bundled pattern files for {component}"
        for path in files:
            assert HASH.fullmatch(path.stem), f"Invalid DLL hash filename: {path}"
            entries = tomllib.loads(path.read_text(encoding="utf-8"))
            found = set()
            for key, entry in entries.items():
                name = entry["name"]
                assert name not in found, f"Duplicate function {name}: {path}"
                found.add(name)
                assert int(key, 16) == fnv1a(name), f"Incorrect function hash: {name}"
                assert int(entry["rva"], 16) > 0, f"Missing function RVA: {name}"
                assert len(bytes.fromhex(entry["sig"])) >= 16, f"Short signature: {name}"
            assert names <= found, f"Missing {component} functions: {sorted(names - found)}"
            if component == "steamclient":
                client_hashes.add(path.stem)
            count += len(entries)
    method_count = 0
    expected = {
        "IClientUser": (1, {"GetSteamID", "GetAppOwnershipTicketExtendedData",
                           "RequestEncryptedAppTicket", "GetEncryptedAppTicket"}),
        "IClientUtils": (4, {"GetAppID", "GetAPICallResult"}),
    }
    for dll_hash in sorted(client_hashes):
        path = root / "ipc" / "steamclient" / f"{dll_hash}.toml"
        assert path.is_file(), f"Missing matching IPC file: {path}"
        entries = tomllib.loads(path.read_text(encoding="utf-8"))
        for iface, (interface_id, methods) in expected.items():
            table = entries[iface]
            assert table["interface_id"] == interface_id, f"Incorrect interface ID: {iface}"
            assert int(table["vtable_rva"], 16) > 0, f"Missing vtable: {iface}"
            hashes = set()
            for method in methods:
                entry = table[method]
                func_hash = int(entry["funcHash"], 16)
                assert 0 < func_hash <= 0xFFFFFFFF and func_hash not in hashes, method
                hashes.add(func_hash)
                assert 0 < int(entry["fencepost"], 16) <= 0xFFFFFFFF, method
                assert int(entry["wrapper_rva"], 16) > 0, method
                assert entry["method_index"] >= 0 and entry["argc"] >= 0, method
                method_count += 1
    return count, method_count


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path,
                        default=ROOT / "compatibility" / "opensteamtool")
    args = parser.parse_args()
    try:
        patterns, methods = validate(args.root)
    except (AssertionError, KeyError, ValueError, OSError) as error:
        print(f"Metadata validation failed: {error}", file=sys.stderr)
        sys.exit(1)
    print(f"Validated {patterns} patterns and {methods} IPC methods in {args.root}")
