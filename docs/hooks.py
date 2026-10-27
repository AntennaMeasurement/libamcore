import json
from pathlib import Path


def on_page_markdown(markdown, *, page, config, files):
    if page.file.src_uri != "index.md":
        return markdown

    manifest_path = Path(config.config_file_path).parent / "vcpkg.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    version = manifest.get("version-string")
    if not isinstance(version, str) or not version:
        raise ValueError(f"Missing version-string in {manifest_path}")

    return markdown.replace("{{ library_version }}", version)
