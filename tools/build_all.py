#!/usr/bin/env python3
"""Build the four exercises and final game in all solution configurations."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


WORKSPACE = Path(__file__).resolve().parents[1]
SOLUTIONS = [
    WORKSPACE / "homework1" / "EasyXExercises.sln",
    WORKSPACE / "final_project" / "MoeBubbleBattle" / "MoeBubbleBattle.sln",
]
CONFIGURATIONS = ("Debug", "Release")
PLATFORMS = ("x86", "x64")


def normalized_environment() -> dict[str, str]:
    """Return a case-normalized block so MSBuild never receives Path and PATH."""
    result: dict[str, str] = {}
    for key, value in os.environ.items():
        result[key.upper()] = value
    return result


def find_msbuild() -> Path:
    command = shutil.which("MSBuild.exe")
    if command:
        return Path(command)
    program_files = Path(os.environ.get("ProgramFiles", "C:/Program Files"))
    candidates = sorted(
        program_files.glob("Microsoft Visual Studio/*/*/MSBuild/Current/Bin/MSBuild.exe"),
        reverse=True,
    )
    if not candidates:
        raise FileNotFoundError("MSBuild.exe not found; install Visual Studio C++ desktop tools")
    return candidates[0]


def main() -> int:
    msbuild = find_msbuild()
    environment = normalized_environment()
    results: list[tuple[str, str, str, int]] = []
    print(f"MSBUILD {msbuild}")
    for solution in SOLUTIONS:
        for configuration in CONFIGURATIONS:
            for platform in PLATFORMS:
                relative = solution.relative_to(WORKSPACE)
                print(f"BUILD {relative} {configuration}|{platform}")
                completed = subprocess.run(
                    [
                        str(msbuild),
                        str(solution),
                        "/m",
                        "/nologo",
                        "/v:minimal",
                        "/t:Build",
                        f"/p:Configuration={configuration}",
                        f"/p:Platform={platform}",
                    ],
                    cwd=WORKSPACE,
                    env=environment,
                    check=False,
                )
                results.append((str(relative), configuration, platform, completed.returncode))
                if completed.returncode:
                    break
            if results[-1][3]:
                break
        if results[-1][3]:
            break

    print("BUILD_SUMMARY")
    for solution, configuration, platform, code in results:
        print(f"{solution}\t{configuration}|{platform}\texit={code}")
    return 1 if any(row[3] for row in results) else 0


if __name__ == "__main__":
    sys.exit(main())
