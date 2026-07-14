#!/usr/bin/env python3
"""Create a clean, named course-submission ZIP after the personal video exists."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path
from pathlib import PurePosixPath


WORKSPACE = Path(__file__).resolve().parents[1]
PROJECT = WORKSPACE / "final_project" / "MoeBubbleBattle"
DIST = WORKSPACE / "dist"
GENERATOR = PROJECT / "tools" / "generate_report.py"
AUDITOR = PROJECT / "tools" / "audit_report.py"
WORD_CONVERTER = WORKSPACE / "tools" / "word_save_as_doc.vbs"

IGNORED_NAMES = {
    ".git", ".vs", "build", "dist", "Debug", "Release", "x64",
    "__pycache__", ".pytest_cache", ".idea", ".agents", ".codex", ".firecrawl",
}


def nonempty(value: str) -> str:
    cleaned = value.strip()
    if not cleaned:
        raise argparse.ArgumentTypeError("此字段不能为空")
    return cleaned


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="生成课程提交目录和 ZIP；必须先准备本人讲解的 FLV 视频。"
    )
    parser.add_argument("--sequence", required=True, type=nonempty, help="序号")
    parser.add_argument("--student-id", required=True, type=nonempty, help="学号")
    parser.add_argument("--student-name", required=True, type=nonempty, help="姓名")
    parser.add_argument("--teacher", required=True, type=nonempty, help="任课教师")
    parser.add_argument("--material-link", required=True, type=nonempty, help="百度网盘链接或老师认可的材料链接")
    parser.add_argument("--video", required=True, type=Path, help="本人讲解的 720P FLV 视频")
    parser.add_argument(
        "--keep-docx",
        action="store_true",
        help="除老师要求的 .doc 外，额外保留一份同名 .docx 备份",
    )
    parser.add_argument("--force", action="store_true", help="覆盖 dist 中同名的旧提交目录和 ZIP")
    return parser.parse_args()


def safe_remove(path: Path) -> None:
    resolved_dist = DIST.resolve()
    resolved = path.resolve()
    if resolved == resolved_dist or resolved_dist not in resolved.parents:
        raise RuntimeError(f"refuse to remove path outside dist: {resolved}")
    if resolved.is_dir():
        shutil.rmtree(resolved)
    elif resolved.exists():
        resolved.unlink()


def is_ignored_name(name: str) -> bool:
    lowered = name.casefold()
    return lowered in {item.casefold() for item in IGNORED_NAMES} or lowered.startswith("build")


def ignore_generated(_directory: str, names: list[str]) -> set[str]:
    ignored = {name for name in names if is_ignored_name(name)}
    ignored.update(name for name in names if name.endswith((".user", ".suo", ".VC.db", ".opendb")))
    return ignored


def copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination, ignore=ignore_generated)


def ensure_clean_tree(root: Path) -> None:
    bad_paths: list[Path] = []
    for path in root.rglob("*"):
        if any(is_ignored_name(part) for part in path.relative_to(root).parts):
            bad_paths.append(path)
        if path.is_file() and path.suffix.casefold() in {".obj", ".pdb", ".ilk", ".tlog", ".idb"}:
            bad_paths.append(path)
    if bad_paths:
        sample = ", ".join(str(path.relative_to(root)) for path in bad_paths[:10])
        raise RuntimeError(f"generated files leaked into submission: {sample}")


def run_checked(arguments: list[str]) -> None:
    completed = subprocess.run(arguments, cwd=WORKSPACE, check=False)
    if completed.returncode:
        raise RuntimeError(f"command failed ({completed.returncode}): {' '.join(arguments)}")


def convert_docx_to_doc(source: Path, destination: Path) -> None:
    """Use Microsoft Word to create a real Word 97-2003 .doc file."""
    cscript = shutil.which("cscript.exe") or shutil.which("cscript")
    if cscript is None:
        raise RuntimeError("未找到 cscript，无法按老师要求生成真正的 .doc 报告")
    if not WORD_CONVERTER.is_file():
        raise FileNotFoundError(WORD_CONVERTER)

    run_checked([
        cscript,
        "//nologo",
        str(WORD_CONVERTER),
        str(source.resolve()),
        str(destination.resolve()),
    ])
    if not destination.is_file():
        raise RuntimeError("Word 转换结束，但没有生成 .doc 文件")

    # Legacy binary Word documents are OLE compound files.  This prevents an
    # accidental extension-only rename from passing the packaging gate.
    ole_signature = bytes.fromhex("D0 CF 11 E0 A1 B1 1A E1")
    if destination.read_bytes()[:8] != ole_signature:
        raise RuntimeError("生成文件不是真正的 Word 97-2003 .doc 格式")


def audit_zip(archive: Path, folder_name: str, report_name: str, video_name: str) -> None:
    """Verify archive integrity, root naming, required files and clean contents."""
    with zipfile.ZipFile(archive) as zipped:
        damaged = zipped.testzip()
        if damaged is not None:
            raise RuntimeError(f"ZIP CRC 校验失败: {damaged}")

        infos = zipped.infolist()
        names = [info.filename for info in infos]
        roots = {PurePosixPath(name).parts[0] for name in names if name}
        if roots != {folder_name}:
            raise RuntimeError(f"ZIP 顶层目录错误: {sorted(roots)}")

        prefix = f"{folder_name}/"
        required = {prefix + report_name, prefix + video_name}
        missing = required.difference(names)
        if missing:
            raise RuntimeError(f"ZIP 缺少必交文件: {sorted(missing)}")

        bad_entries: list[str] = []
        for info in infos:
            parts = PurePosixPath(info.filename).parts
            if any(is_ignored_name(part) for part in parts):
                bad_entries.append(info.filename)
            if info.filename.casefold().endswith((".obj", ".pdb", ".ilk", ".tlog", ".idb", ".user", ".suo")):
                bad_entries.append(info.filename)
            if any(ord(char) > 127 for char in info.filename) and not info.flag_bits & 0x800:
                bad_entries.append(f"{info.filename} (中文文件名未标记 UTF-8)")
        if bad_entries:
            raise RuntimeError(f"ZIP 内容不合规: {bad_entries[:10]}")


def main() -> int:
    args = parse_args()
    video = args.video.resolve()
    if not video.is_file():
        raise FileNotFoundError(video)
    if video.suffix.casefold() != ".flv":
        raise ValueError("视频必须先按课程要求转换为 .flv")

    folder_name = f"{args.sequence}-{args.student_id}-{args.student_name}-萌泡大作战"
    archive_name = f"{args.sequence}-{args.student_id}-{args.student_name}"
    submission = DIST / folder_name
    archive_base = DIST / archive_name
    archive_path = archive_base.with_suffix(".zip")

    DIST.mkdir(parents=True, exist_ok=True)
    if submission.exists() or archive_path.exists():
        if not args.force:
            raise FileExistsError("dist 中已有同名提交；检查后使用 --force 覆盖")
        safe_remove(submission)
        safe_remove(archive_path)

    submission.mkdir()
    exercises_destination = submission / "1.1与1.2-EasyX四个练习"
    game_destination = submission / "萌泡大作战-游戏工程"
    copy_tree(WORKSPACE / "homework1", exercises_destination)
    copy_tree(PROJECT, game_destination)
    safe_remove(game_destination / "report")

    report_docx = submission / f"程序设计课程实践报告-{args.student_name}.docx"
    report_doc = submission / f"程序设计课程实践报告-{args.student_name}.doc"
    run_checked([
        sys.executable, str(GENERATOR),
        "--output", str(report_docx),
        "--sequence", args.sequence,
        "--student-id", args.student_id,
        "--student-name", args.student_name,
        "--teacher", args.teacher,
        "--material-link", args.material_link,
    ])
    run_checked([sys.executable, str(AUDITOR), "--report", str(report_docx), "--expect-filled"])
    convert_docx_to_doc(report_docx, report_doc)
    if not args.keep_docx:
        report_docx.unlink()

    video_name = f"项目汇报视频-{args.student_name}.flv"
    shutil.copy2(video, submission / video_name)
    ensure_clean_tree(submission)
    shutil.make_archive(str(archive_base), "zip", root_dir=DIST, base_dir=folder_name)
    audit_zip(archive_path, folder_name, report_doc.name, video_name)

    print(f"SUBMISSION_FOLDER {submission}")
    print(f"ARCHIVE {archive_path}")
    print("REMINDER 检查 .doc 打印预览，并从 ZIP 解压后完整试运行和播放视频。")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as error:
        print(f"ERROR {error}", file=sys.stderr)
        sys.exit(1)
