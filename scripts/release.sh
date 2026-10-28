#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: bash scripts/release.sh [--dry-run]

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

dry_run=false
case "${1:-}" in
    '') ;;
    --dry-run) dry_run=true ;;
    --help|-h) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
esac
[[ $# -le 1 ]] || fail "Expected at most one option."

for tool in git awk mktemp; do
    command -v "$tool" >/dev/null || fail "Required command not found: $tool"
done
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd -- "$script_dir/.."
[[ -f CMakeLists.txt && -f CHANGELOG.md ]] ||
    fail "CMakeLists.txt and CHANGELOG.md must exist at the repository root."
[[ -z $(git status --porcelain) ]] ||
    fail "Commit or stash all changes, including untracked files, before releasing."
branch=$(git symbolic-ref --quiet --short HEAD) ||
    fail "Check out a branch before releasing (detached HEAD is not supported)."

# Read only the project() declaration, not cmake_minimum_required(VERSION ...).
version=$(awk '
    {
        sub(/\r$/, "")
        sub(/#.*/, "")
        if (tolower($0) ~ /^[[:space:]]*project[[:space:]]*\(/) in_project = 1
        end_project = index($0, ")")
        if (in_project) {
            gsub(/[()"]/, " ")
            for (i = 1; i <= NF; i++) {
                if (want_version) { print $i; exit }
                if (toupper($i) == "VERSION") want_version = 1
            }
        }
        if (in_project && end_project) exit
    }
' CMakeLists.txt)
[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+(\.[0-9]+)?$ ]] ||
    fail "Expected a literal numeric project VERSION in CMakeLists.txt."
tag="v$version"
git show-ref --verify --quiet "refs/tags/$tag" &&
    fail "Tag $tag already exists locally; choose a new CMake version."
remote_url=$(git remote get-url origin) || fail "The origin remote is required."

if ! $dry_run; then
    command -v gh >/dev/null || fail "Required command not found: gh"
    gh auth status || fail "Authenticate with 'gh auth login' before releasing."
    # Normalize the common SSH URL so gh always targets origin, not GH_REPO.
    repo_url=$remote_url
    if [[ $repo_url == git@*:* ]]; then
        ssh_host=${repo_url%%:*}
        repo_url="https://${ssh_host#git@}/${repo_url#*:}"
    fi
    repo_url=${repo_url%.git}
    repo=$(gh repo view "$repo_url" --json url --jq .url) ||
        fail "Cannot resolve origin as a GitHub repository."
    remote_tag=$(git ls-remote --tags origin "refs/tags/$tag") ||
        fail "Cannot check origin for existing tags."
    [[ -z $remote_tag ]] || fail "Tag $tag already exists on origin."
fi

tmp_dir=$(mktemp -d)
cleanup() {
    rm -f -- "$tmp_dir/notes" "$tmp_dir/changelog" "$tmp_dir/commits"
    rmdir -- "$tmp_dir"
}
trap cleanup EXIT

extract_notes() {
    awk -v heading="$1" '
        { sub(/\r$/, "") }
        /^## / {
            if (inside) exit
            inside = ($0 == heading || index($0, heading " - ") == 1)
            next
        }
        inside { print }
    ' CHANGELOG.md
}

existing=$(awk -v heading="## [$tag]" '
    { sub(/\r$/, "") }
    $0 == heading || index($0, heading " - ") == 1 { print "yes"; exit }
' CHANGELOG.md)
if [[ $existing == yes ]]; then
    extract_notes "## [$tag]" > "$tmp_dir/notes"
else
    [[ $(awk '{ sub(/\r$/, "") } $0 == "## [Unreleased]" { n++ } END { print n+0 }' CHANGELOG.md) == 1 ]] ||
        fail "CHANGELOG.md must contain exactly one '## [Unreleased]' heading."
    extract_notes "## [Unreleased]" > "$tmp_dir/notes"
    if ! grep -q '[^[:space:]]' "$tmp_dir/notes"; then
        previous_tag=$(git describe --tags --abbrev=0 --match 'v[0-9]*' HEAD 2>/dev/null) ||
            previous_tag=''
        if [[ -n $previous_tag ]]; then
            git log --no-merges --format='- %s' "$previous_tag..HEAD" > "$tmp_dir/commits"
        else
            git log --no-merges --format='- %s' HEAD > "$tmp_dir/commits"
        fi
        [[ -s $tmp_dir/commits ]] || fail "No unreleased notes or commits to release."
        printf '### Changes\n\n' > "$tmp_dir/notes"
        cat "$tmp_dir/commits" >> "$tmp_dir/notes"
    fi
fi
grep -q '[^[:space:]]' "$tmp_dir/notes" || fail "Release notes for $tag are empty."

printf 'Release: %s\nBranch: %s\nRemote: %s\n\n' "$tag" "$branch" "$remote_url"
cat "$tmp_dir/notes"
if $dry_run; then
    printf '\nDry run: no files, commits, tags, pushes, or releases were created.\n'
    exit 0
fi

if [[ $existing != yes ]]; then
    awk -v tag="$tag" -v date="$(date +%F)" -v notes="$tmp_dir/notes" '
        { sub(/\r$/, "") }
        $0 == "## [Unreleased]" {
            print $0 "\n\n## [" tag "] - " date "\n"
            while ((getline line < notes) > 0) print line
            close(notes)
            print ""
            skip = 1
            next
        }
        skip && /^## / { skip = 0 }
        !skip { print }
    ' CHANGELOG.md > "$tmp_dir/changelog"
    cat "$tmp_dir/changelog" > CHANGELOG.md
fi

if ! git diff --quiet -- CHANGELOG.md; then
    git add -- CHANGELOG.md
    git commit -m "chore: prepare release $tag" \
        -m "Co-authored-by: Copilot <223556219+Copilot@users.noreply.github.com>"
fi
git tag -a "$tag" -m "Release $tag"
printf '\nPublishing branch and tag; failures leave local state intact for recovery.\n'
git push --atomic origin "HEAD:refs/heads/$branch" "refs/tags/$tag" ||
    fail "Push failed. Resolve the cause and push the branch and $tag manually; no GitHub release was created."
gh release create "$tag" --repo "$repo" --verify-tag \
    --title "$tag" --notes-file "$tmp_dir/notes" ||
    fail "Branch and tag were pushed, but release creation failed. Retry with: gh release create $tag --repo $repo --verify-tag (use the changelog entry as notes)."
