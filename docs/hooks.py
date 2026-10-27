import re
from pathlib import Path


def on_page_markdown(markdown, *, page, config, files):
    if page.file.src_uri != "index.md":
        return markdown

    cmake_path = Path(config.config_file_path).parent / "CMakeLists.txt"
    cmake = cmake_path.read_text(encoding="utf-8")
    match = re.search(
        r"\bproject\s*\(\s*libamcore\s+VERSION\s+"
        r"([0-9]+(?:\.[0-9]+){0,3})(?=\s|\))",
        cmake,
        re.IGNORECASE,
    )
    if match is None:
        raise ValueError(f"Missing libamcore project VERSION in {cmake_path}")

    return markdown.replace("{{ library_version }}", match.group(1))
