import re
from pathlib import Path


def on_page_markdown(markdown, *, page, config, files):
    if page.file.src_uri != "index.md":
        return markdown
    
    # Read version from the VERSION file
    version_path = Path(config.config_file_path).parent / "VERSION"
    version = version_path.read_text(encoding="utf-8").strip()

    return markdown.replace("{{ library_version }}", version)
