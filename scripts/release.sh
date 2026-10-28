#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: bash scripts/release.sh [--dry-run]

Update the CHANGELOG.md while preserving existing entries and format.
Release the version declared in CMakeLists.txt to origin and GitHub.
Requires a clean, committed working tree, Git, GitHub CLI (gh), and
authenticated push/release access. Run from any working directory.

--dry-run  Preview the tag and release notes without changing files or publishing.
--help     Show this help.
EOF
}

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

if [[ "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi
if [[ "${1:-}" == "--dry-run" ]]; then
    DRY_RUN=true
else
    DRY_RUN=false
fi
# Other arguments are not supported
if [[ "${1:-}" != "" && "${1:-}" != "--dry-run" ]]; then
    fail "Unknown argument: $1"
    usage
    exit 1
fi

VERSION=$(cat VERSION)
# Read commits from the Git history from last previous tag to HEAD.
previous_tag=$(git describe --tags --abbrev=0 --match 'v[0-9]*' HEAD 2>/dev/null) || previous_tag=''
if [[ -n $previous_tag ]]; then
    CHANGELOG=$(git log --no-merges --format='- %s' "$previous_tag..HEAD")
else
    CHANGELOG=$(git log --no-merges --format='- %s' HEAD)
fi

echo "Releasing version $VERSION"

if [[ "$DRY_RUN" == true ]]; then
    echo "Dry run: would tag v$VERSION and push to origin"
    echo "Dry run: would generate release notes:"
    echo "$CHANGELOG"
else
    # Update CHANGELOG.md with the new version and release notes before tagging and pushing.
    line=8
    tmp=$(mktemp CHANGELOG.md.XXXXXX) || exit 1
    {
        head -n "$((line - 1))" -- CHANGELOG.md
        printf '## [v%s] - %s\n%s\n\n' "$VERSION" "$(date +%Y-%m-%d)" "$CHANGELOG"
        tail -n "+$line" -- CHANGELOG.md
    } > "$tmp" && mv -- "$tmp" CHANGELOG.md
    git add .
    git commit -m "Update CHANGELOG for v$VERSION"
    git push origin HEAD
    git tag "v$VERSION"
    git push origin "v$VERSION"
    gh release create "v$VERSION" --notes "$CHANGELOG"
    echo "Released version $VERSION"
fi