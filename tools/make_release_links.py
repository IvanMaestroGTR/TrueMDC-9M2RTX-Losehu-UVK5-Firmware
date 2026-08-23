#!/usr/bin/env python3
"""Create GitHub raw and UVTools links for a packed TrueMDC firmware."""

import argparse
from pathlib import Path
from urllib.parse import quote, urlparse


def repository_path(repository_url):
    parsed = urlparse(repository_url)
    path = parsed.path.strip("/")
    if path.endswith(".git"):
        path = path[:-4]
    if not path or "/" not in path:
        raise ValueError("repository must look like https://github.com/OWNER/REPOSITORY")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="version suffix, for example 7.1 or Gen7.1")
    parser.add_argument(
        "--repository",
        default="https://github.com/IvanMaestroGTR/TrueMDC-9M2RTX-Losehu-UVK5-Firmware",
        help="GitHub repository URL",
    )
    parser.add_argument("--branch", default="main", help="GitHub branch")
    parser.add_argument(
        "--output",
        default="release-link.md",
        help="Markdown output path (default: release-link.md)",
    )
    args = parser.parse_args()

    version = args.version if args.version.startswith("Gen") else f"Gen{args.version}"
    repository = repository_path(args.repository)
    filename = f"TrueMDC.{version}.packed.bin"
    raw_url = f"https://raw.githubusercontent.com/{repository}/{args.branch}/archive/{filename}"
    flasher_url = f"https://egzumer.github.io/uvtools/?firmwareURL={quote(raw_url, safe='')}"
    markdown = (
        f"[Flash TrueMDC {version} with UVTools]({flasher_url})\n\n"
        f"[Download {filename}]({raw_url})\n"
    )

    output_path = Path(args.output)
    output_path.write_text(markdown, encoding="utf-8")
    print(f"Raw URL: {raw_url}")
    print(f"UVTools URL: {flasher_url}")
    print(f"Markdown written to: {output_path}")


if __name__ == "__main__":
    main()
