#!/usr/bin/env python3
"""Merge bundle matrix includes from the current ref with extras on origin/custom."""

from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path


def git_show(path: str) -> str | None:
    result = subprocess.run(
        ["git", "show", f"origin/custom:{path}"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return None
    return result.stdout


def load_includes(text: str) -> list[dict]:
    data = json.loads(text)
    include = data.get("include")
    if not isinstance(include, list):
        raise ValueError("matrix JSON must contain an 'include' array")
    return include


def merge_includes(base: list[dict], extra: list[dict]) -> list[dict]:
    by_pinout = {entry["pinout"]: entry for entry in base if entry.get("pinout")}
    merged = list(base)
    for entry in extra:
        pinout = entry.get("pinout")
        if not pinout or pinout in by_pinout:
            continue
        by_pinout[pinout] = entry
        merged.append(entry)
    return merged


def main() -> int:
    matrix_path = Path(".github/bundle-modxo-matrix.json")
    if not matrix_path.is_file():
        print(f"Missing {matrix_path}", file=sys.stderr)
        return 1

    includes = load_includes(matrix_path.read_text(encoding="utf-8"))

    if (
        subprocess.run(
            ["git", "rev-parse", "--verify", "origin/custom"],
            capture_output=True,
        ).returncode
        == 0
    ):
        custom_text = git_show(".github/bundle-modxo-matrix.json")
        if custom_text:
            includes = merge_includes(includes, load_includes(custom_text))

    payload = json.dumps({"include": includes})

    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a", encoding="utf-8") as out:
            out.write(f"matrix={payload}\n")
    else:
        print(payload)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())