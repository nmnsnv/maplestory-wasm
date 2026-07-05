#!/usr/bin/env python3
"""Summarize quest automation reports for quick offline triage."""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path
from typing import Any


def load_report(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{path} must contain a JSON object")
    return data


def _int_or_none(value: Any) -> int | None:
    if isinstance(value, bool):
        return None
    if isinstance(value, int):
        return value
    return None


def _str_or_empty(value: Any) -> str:
    if value is None:
        return ""
    return str(value)


def _quest_id(failure: dict[str, Any]) -> str:
    return _str_or_empty(failure.get("questId"))


def _status_value(value: Any) -> str:
    if isinstance(value, bool):
        return "?"
    if isinstance(value, int):
        return str(value)
    return "?"


def _quest_status_transition(failure: dict[str, Any]) -> str:
    before = _status_value(failure.get("questStatusBefore"))
    after_open = _status_value(failure.get("questStatusAfterOpen"))
    after_dispose = _status_value(failure.get("questStatusAfterDispose"))
    return f"{before}->{after_open}->{after_dispose}"


def _failure_rows(report: dict[str, Any]) -> list[dict[str, Any]]:
    failures = report.get("failures")
    if not isinstance(failures, list):
        return []
    return [failure for failure in failures if isinstance(failure, dict)]


def _group_failures(
    failures: list[dict[str, Any]],
    key_name: str,
    output_key: str,
    top: int,
) -> list[dict[str, Any]]:
    grouped: dict[Any, list[dict[str, Any]]] = defaultdict(list)
    for failure in failures:
        grouped[failure.get(key_name)].append(failure)

    rows: list[dict[str, Any]] = []
    for key, group in grouped.items():
        if key_name == "mapId":
            value = key if isinstance(key, int) and not isinstance(key, bool) else None
        else:
            value = _str_or_empty(key)
        rows.append(
            {
                output_key: value,
                "count": len(group),
                "examples": [_quest_id(failure) for failure in group[:top]],
            }
        )

    return sorted(rows, key=lambda row: (-row["count"], str(row[output_key])))[:top]


def _quest_status_groups(failures: list[dict[str, Any]], top: int) -> list[dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for failure in failures:
        grouped[_quest_status_transition(failure)].append(failure)

    rows: list[dict[str, Any]] = []
    for transition, group in grouped.items():
        rows.append(
            {
                "transition": transition,
                "count": len(group),
                "examples": [_quest_id(failure) for failure in group[:top]],
            }
        )

    return sorted(rows, key=lambda row: (-row["count"], row["transition"]))[:top]


def summarize_report(report: dict[str, Any], top: int) -> dict[str, Any]:
    failures = _failure_rows(report)
    missing_map_quests: list[dict[str, str]] = []

    for failure in failures:
        error = _str_or_empty(failure.get("error"))
        map_id = failure.get("mapId")
        if map_id is None or error == "Quest start NPC has no indexed map":
            missing_map_quests.append(
                {
                    "questId": _quest_id(failure),
                    "questName": _str_or_empty(failure.get("questName")),
                    "startNpc": _str_or_empty(failure.get("startNpc")),
                    "error": error,
                }
            )

    generated_at = report.get("generatedAt")
    return {
        "summary": {
            "generatedAt": generated_at if isinstance(generated_at, str) else None,
            "totalQuests": _int_or_none(report.get("totalQuests")),
            "planned": _int_or_none(report.get("planned")),
            "tested": _int_or_none(report.get("tested")),
            "passed": _int_or_none(report.get("passed")),
            "failed": _int_or_none(report.get("failed")),
        },
        "errorGroups": _group_failures(failures, "error", "error", top),
        "npcGroups": _group_failures(failures, "startNpc", "startNpc", top),
        "mapGroups": _group_failures(failures, "mapId", "mapId", top),
        "questStatusGroups": _quest_status_groups(failures, top),
        "missingMapQuests": missing_map_quests,
    }


def print_text_summary(summary: dict[str, Any]) -> None:
    totals = summary["summary"]
    print(f"Generated: {totals['generatedAt'] or 'n/a'}")
    print(
        "Totals: "
        f"totalQuests={totals['totalQuests']} "
        f"planned={totals['planned']} "
        f"tested={totals['tested']} "
        f"passed={totals['passed']} "
        f"failed={totals['failed']}"
    )

    def print_groups(title: str, groups: list[dict[str, Any]], key: str) -> None:
        print()
        print(title)
        if not groups:
            print("  none")
            return
        for group in groups:
            examples = ", ".join(group["examples"])
            print(f"  {group['count']:4d}  {group[key]}  examples=[{examples}]")

    print_groups("Top error groups:", summary["errorGroups"], "error")
    print_groups("Top NPC groups:", summary["npcGroups"], "startNpc")
    print_groups("Top map groups:", summary["mapGroups"], "mapId")
    print_groups("Top quest status transitions:", summary["questStatusGroups"], "transition")

    print()
    print(f"Missing-map quests: {len(summary['missingMapQuests'])}")
    for quest in summary["missingMapQuests"][:10]:
        print(
            "  "
            f"{quest['questId']} | {quest['questName']} | "
            f"npc={quest['startNpc']} | {quest['error']}"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Summarize quest automation JSON reports.")
    parser.add_argument(
        "report_path",
        nargs="?",
        type=Path,
        default=Path("scripts/quest_test_report.json"),
        help="Report JSON path. Defaults to scripts/quest_test_report.json.",
    )
    parser.add_argument("--json", action="store_true", help="Print machine-readable JSON.")
    parser.add_argument("--top", type=int, default=10, help="Maximum groups and examples to include.")
    args = parser.parse_args()

    if args.top < 1:
        parser.error("--top must be positive")

    try:
        report = load_report(args.report_path)
    except FileNotFoundError:
        print(f"error: report file not found: {args.report_path}", file=sys.stderr)
        return 1
    except json.JSONDecodeError as exc:
        print(f"error: invalid JSON in {args.report_path}: {exc}", file=sys.stderr)
        return 1
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    except OSError as exc:
        print(f"error: unable to read {args.report_path}: {exc}", file=sys.stderr)
        return 1

    summary = summarize_report(report, args.top)
    if args.json:
        print(json.dumps(summary, indent=2, sort_keys=True))
    else:
        print_text_summary(summary)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
