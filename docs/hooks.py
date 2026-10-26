import json
from pathlib import Path


def on_page_markdown(markdown, *, page, config, files):
    if page.file.src_uri != "index.md":
        return markdown

    manifest_path = Path(config.config_file_path).parent / "docs" / "public" / "api.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    version = manifest.get("library_version")
    if not isinstance(version, str) or not version:
        raise ValueError(f"Missing library_version in {manifest_path}")

    return markdown.replace("{{ library_version }}", version)
