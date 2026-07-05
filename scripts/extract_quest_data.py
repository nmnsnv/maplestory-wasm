#!/usr/bin/env python3
"""Build a quest manifest from Cosmic WZ XML and quest scripts.

The manifest is intentionally generated from server-side XML/script data instead
of from client assets so the automation targets the same quest/NPC definitions
that Cosmic will execute during tests.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import sys
import xml.etree.ElementTree as ET


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = REPO_ROOT / "scripts" / "quest_manifest.json"


def existing_path(candidates: list[Path]) -> Path | None:
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def default_wz_root() -> Path:
    from_env = os.environ.get("COSMIC_WZ_ROOT")
    if from_env:
        return Path(from_env).expanduser()

    return existing_path([
        Path.home() / "Cosmic" / "wz",
        REPO_ROOT.parent / "Cosmic" / "wz",
        REPO_ROOT / "wz",
    ]) or (Path.home() / "Cosmic" / "wz")


def default_quest_scripts_root() -> Path:
    from_env = os.environ.get("COSMIC_QUEST_SCRIPT_ROOT")
    if from_env:
        return Path(from_env).expanduser()

    return existing_path([
        Path.home() / "Cosmic" / "scripts" / "quest",
        REPO_ROOT.parent / "Cosmic" / "scripts" / "quest",
    ]) or (Path.home() / "Cosmic" / "scripts" / "quest")


def parse_xml(path: Path) -> ET.Element:
    try:
        return ET.parse(path).getroot()
    except ET.ParseError as exc:
        raise RuntimeError(f"failed to parse XML {path}: {exc}") from exc


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise FileNotFoundError(f"missing {label}: {path}")


def child_by_name(parent: ET.Element, name: str) -> ET.Element | None:
    for child in parent:
        if child.attrib.get("name") == name:
            return child
    return None


def value_by_name(parent: ET.Element, name: str) -> str | None:
    child = child_by_name(parent, name)
    if child is None:
        return None
    return child.attrib.get("value")


def int_value_by_name(parent: ET.Element, name: str) -> int | None:
    value = value_by_name(parent, name)
    if value is None or value == "":
        return None
    try:
        return int(value)
    except ValueError:
        return None


def direct_named_children(root: ET.Element) -> dict[str, ET.Element]:
    return {
        child.attrib["name"]: child
        for child in root
        if "name" in child.attrib
    }


def parse_quest_names(quest_info_xml: Path) -> dict[str, str]:
    root = parse_xml(quest_info_xml)
    names: dict[str, str] = {}
    for quest_id, node in direct_named_children(root).items():
        name = value_by_name(node, "name")
        if name:
            names[quest_id] = name
    return names


def parse_quest_npcs(check_xml: Path) -> dict[str, dict[str, int | None]]:
    root = parse_xml(check_xml)
    quest_npcs: dict[str, dict[str, int | None]] = {}
    for quest_id, node in direct_named_children(root).items():
        start_node = child_by_name(node, "0")
        end_node = child_by_name(node, "1")
        quest_npcs[quest_id] = {
            "startNpc": int_value_by_name(start_node, "npc") if start_node is not None else None,
            "endNpc": int_value_by_name(end_node, "npc") if end_node is not None else None,
        }
    return quest_npcs


def parse_child_ints(parent: ET.Element | None, name: str) -> list[int]:
    node = child_by_name(parent, name) if parent is not None else None
    if node is None:
        return []

    values: list[int] = []
    for child in node:
        value = child.attrib.get("value")
        if value is None or value == "":
            continue
        try:
            values.append(int(value))
        except ValueError:
            continue
    return values


def parse_quest_requirement_list(parent: ET.Element | None) -> list[dict[str, int]]:
    node = child_by_name(parent, "quest") if parent is not None else None
    if node is None:
        return []

    requirements: list[dict[str, int]] = []
    for entry in node:
        quest_id = int_value_by_name(entry, "id")
        if quest_id is None:
            continue
        # Cosmic's QuestRequirement defaults a missing state to 0, which means
        # NOT_STARTED. Mirroring that here lets the runner prepare the same state.
        requirements.append({"id": quest_id, "state": int_value_by_name(entry, "state") or 0})
    return requirements


def parse_infoex_values(parent: ET.Element | None) -> list[str]:
    node = child_by_name(parent, "infoex") if parent is not None else None
    if node is None:
        return []

    values: list[str] = []
    for entry in node:
        value = value_by_name(entry, "value")
        if value is not None:
            values.append(value)
    return values


def parse_start_requirements(check_xml: Path) -> dict[str, dict[str, object]]:
    root = parse_xml(check_xml)
    requirements: dict[str, dict[str, object]] = {}
    for quest_id, node in direct_named_children(root).items():
        start_node = child_by_name(node, "0")
        if start_node is None:
            requirements[quest_id] = {}
            continue

        entry: dict[str, object] = {}
        level_min = int_value_by_name(start_node, "lvmin")
        if level_min is not None:
            entry["levelMin"] = level_min
        jobs = parse_child_ints(start_node, "job")
        if jobs:
            entry["jobs"] = jobs
        quest_requirements = parse_quest_requirement_list(start_node)
        if quest_requirements:
            entry["quests"] = quest_requirements
        info_number = int_value_by_name(start_node, "infoNumber")
        if info_number is not None:
            entry["infoNumber"] = info_number
        infoex = parse_infoex_values(start_node)
        if infoex:
            entry["infoex"] = infoex
        requirements[quest_id] = entry
    return requirements


def parse_npc_names(npc_xml: Path) -> dict[int, str]:
    root = parse_xml(npc_xml)
    names: dict[int, str] = {}
    for npc_id_text, node in direct_named_children(root).items():
        try:
            npc_id = int(npc_id_text)
        except ValueError:
            continue
        name = value_by_name(node, "name")
        if name:
            names[npc_id] = name
    return names


def index_quest_scripts(scripts_root: Path) -> dict[str, list[Path]]:
    scripts: dict[str, list[Path]] = {}
    if not scripts_root.is_dir():
        return scripts

    for script in sorted(scripts_root.glob("*.js")):
        stem = script.stem
        quest_id: str | None = None
        if stem.isdigit():
            quest_id = stem
        elif len(stem) > 2 and stem[0] == "q" and stem[-1] in {"s", "e"} and stem[1:-1].isdigit():
            quest_id = stem[1:-1]

        if quest_id is not None:
            scripts.setdefault(quest_id, []).append(script)

    return scripts


def relative_script_path(script: Path, scripts_root: Path) -> str:
    cosmic_root = scripts_root.parent.parent
    try:
        return script.relative_to(cosmic_root).as_posix()
    except ValueError:
        return script.as_posix()


def map_xml_files(map_root: Path) -> list[Path]:
    if not map_root.is_dir():
        return []
    return sorted(path for path in map_root.glob("Map*/*.img.xml") if path.is_file())


def parse_map_id(path: Path) -> int | None:
    text = path.name.removesuffix(".img.xml")
    try:
        return int(text)
    except ValueError:
        return None


def index_npc_maps(map_root: Path) -> dict[int, list[int]]:
    by_npc: dict[int, set[int]] = {}
    for path in map_xml_files(map_root):
        map_id = parse_map_id(path)
        if map_id is None:
            continue

        try:
            root = parse_xml(path)
        except RuntimeError as exc:
            print(f"warning: {exc}", file=sys.stderr)
            continue

        life = child_by_name(root, "life")
        if life is None:
            continue

        for entry in life:
            if value_by_name(entry, "type") != "n":
                continue
            npc_id_text = value_by_name(entry, "id")
            if not npc_id_text:
                continue
            try:
                npc_id = int(npc_id_text)
            except ValueError:
                continue
            by_npc.setdefault(npc_id, set()).add(map_id)

    return {npc_id: sorted(map_ids) for npc_id, map_ids in by_npc.items()}


def numeric_sort_key(value: str) -> tuple[int, str]:
    try:
        return (0, f"{int(value):010d}")
    except ValueError:
        return (1, value)


def build_manifest(wz_root: Path, scripts_root: Path, skip_map_scan: bool) -> dict[str, dict[str, object]]:
    quest_root = wz_root / "Quest.wz"
    string_root = wz_root / "String.wz"
    map_root = wz_root / "Map.wz" / "Map"

    check_xml = quest_root / "Check.img.xml"
    quest_info_xml = quest_root / "QuestInfo.img.xml"
    npc_xml = string_root / "Npc.img.xml"

    require_file(check_xml, "quest check XML")
    require_file(quest_info_xml, "quest info XML")
    require_file(npc_xml, "NPC string XML")

    quest_npcs = parse_quest_npcs(check_xml)
    start_requirements = parse_start_requirements(check_xml)
    quest_names = parse_quest_names(quest_info_xml)
    npc_names = parse_npc_names(npc_xml)
    script_index = index_quest_scripts(scripts_root)
    npc_maps = {} if skip_map_scan else index_npc_maps(map_root)

    quest_ids = sorted(set(quest_npcs) | set(quest_names) | set(script_index), key=numeric_sort_key)
    manifest: dict[str, dict[str, object]] = {}

    for quest_id in quest_ids:
        npcs = quest_npcs.get(quest_id, {})
        start_npc = npcs.get("startNpc")
        end_npc = npcs.get("endNpc")
        scripts = script_index.get(quest_id, [])
        script_paths = [relative_script_path(script, scripts_root) for script in scripts]

        entry: dict[str, object] = {
            "name": quest_names.get(quest_id, f"Quest {quest_id}"),
            "startNpc": start_npc,
            "startNpcName": npc_names.get(start_npc, "") if start_npc is not None else "",
            "endNpc": end_npc,
            "endNpcName": npc_names.get(end_npc, "") if end_npc is not None else "",
            "startNpcMaps": npc_maps.get(start_npc, []) if start_npc is not None else [],
            "endNpcMaps": npc_maps.get(end_npc, []) if end_npc is not None else [],
            "hasScript": bool(scripts),
            "scriptPath": script_paths[0] if script_paths else "",
            "startRequirements": start_requirements.get(quest_id, {}),
        }
        if len(script_paths) > 1:
            entry["scriptPaths"] = script_paths

        manifest[quest_id] = entry

    return manifest


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate scripts/quest_manifest.json from Cosmic WZ XML.")
    parser.add_argument("--wz-root", type=Path, default=default_wz_root(), help="Cosmic wz directory")
    parser.add_argument("--quest-scripts-root", type=Path, default=default_quest_scripts_root(), help="Cosmic quest scripts directory")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT, help="manifest output path")
    parser.add_argument("--skip-map-scan", action="store_true", help="skip the slower NPC-to-map reverse index")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    wz_root = args.wz_root.expanduser().resolve()
    scripts_root = args.quest_scripts_root.expanduser().resolve()
    output = args.output.expanduser().resolve()

    try:
        manifest = build_manifest(wz_root, scripts_root, args.skip_map_scan)
    except (FileNotFoundError, RuntimeError) as exc:
        print(f"extract_quest_data.py: {exc}", file=sys.stderr)
        return 1

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as handle:
        json.dump(manifest, handle, indent=2, ensure_ascii=False)
        handle.write("\n")

    scripted = sum(1 for quest in manifest.values() if quest["hasScript"])
    with_maps = sum(1 for quest in manifest.values() if quest["startNpcMaps"] or quest["endNpcMaps"])
    print(f"wrote {output}")
    print(f"quests={len(manifest)} scripted={scripted} with_npc_maps={with_maps}")
    print(f"wz_root={wz_root}")
    print(f"quest_scripts_root={scripts_root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
