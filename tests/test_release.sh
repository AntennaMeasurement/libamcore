#!/usr/bin/env bash
set -euo pipefail

# These tests publish only to disposable local remotes and a stub GitHub CLI.
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
test_dir=$(mktemp -d)
trap 'rm -rf -- "$test_dir"' EXIT
export GIT_CONFIG_NOSYSTEM=1
export GIT_CONFIG_GLOBAL=/dev/null
export GIT_AUTHOR_NAME='Release Test'
export GIT_AUTHOR_EMAIL='release-test@example.invalid'
export GIT_COMMITTER_NAME=$GIT_AUTHOR_NAME
export GIT_COMMITTER_EMAIL=$GIT_AUTHOR_EMAIL

mkdir "$test_dir/bin"
cat > "$test_dir/bin/gh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
case "$1 $2" in
    'auth status') exit 0 ;;
    'repo view') printf 'https://github.com/test/release\n' ;;
    'release create')
        printf '%s\n' "$@" > "$GH_TEST_ARGS"
        while [[ $# -gt 0 ]]; do
            if [[ $1 == --notes-file ]]; then
                cp "$2" "$GH_TEST_NOTES"
                break
            fi
            shift
        done
        [[ ${GH_TEST_FAIL:-false} != true ]]
        ;;
    *) printf 'Unexpected gh invocation\n' >&2; exit 1 ;;
esac
EOF
chmod +x "$test_dir/bin/gh"
export PATH="$test_dir/bin:$PATH"
export GH_TEST_ARGS="$test_dir/gh-args"
export GH_TEST_NOTES="$test_dir/gh-notes"

setup_repo() {
    local name=$1
    mkdir "$test_dir/$name"
    cd "$test_dir/$name"
    git init --quiet -b main
    git init --quiet --bare "$test_dir/$name.git"
    git remote add origin "$test_dir/$name.git"
    mkdir scripts
    cp "$source_dir/scripts/release.sh" scripts/release.sh
    printf 'cmake_minimum_required(VERSION 3.15)\nproject(example\n VERSION 1.2.3\n LANGUAGES C)\n' > CMakeLists.txt
    printf '# Changelog\n\n## [Unreleased]\n\n## [v1.2.2] - 2026-01-01\n\n- Historical note\n' > CHANGELOG.md
    git add .
    git commit --quiet -m 'Initial release'
    git tag v1.2.2
    printf 'new feature\n' > feature.txt
    git add .
    git commit --quiet -m 'Add feature'
    rm -f "$GH_TEST_ARGS" "$GH_TEST_NOTES"
}

expect_failure() {
    if bash scripts/release.sh "$@" > "$test_dir/output" 2>&1; then
        printf 'FAIL: expected release script to fail\n' >&2
        exit 1
    fi
}

assert_published() {
    [[ $(git rev-parse HEAD) == "$(git --git-dir="$PWD/../${PWD##*/}.git" rev-parse refs/heads/main)" ]]
    [[ $(git cat-file -t refs/tags/v1.2.3) == tag ]]
    [[ $(git rev-parse refs/tags/v1.2.3) == "$(git ls-remote origin refs/tags/v1.2.3 | cut -f1)" ]]
    [[ -z $(git status --porcelain) ]]
    grep -Fxq v1.2.3 "$GH_TEST_ARGS"
    grep -Fxq -- --verify-tag "$GH_TEST_ARGS"
}

setup_repo generated
before=$(git rev-parse HEAD)
(cd "$test_dir"; bash "$test_dir/generated/scripts/release.sh" --dry-run) > "$test_dir/output"
grep -Fq 'Release: v1.2.3' "$test_dir/output"
grep -Fq -- '- Add feature' "$test_dir/output"
[[ $(git rev-parse HEAD) == "$before" && -z $(git status --porcelain) ]]
[[ ! -e $GH_TEST_ARGS ]]
bash scripts/release.sh > "$test_dir/output" 2>&1
assert_published
grep -Fq "## [v1.2.3] - $(date +%F)" CHANGELOG.md
grep -Fxq -- '- Add feature' "$GH_TEST_NOTES"
! grep -Fq 'Initial release' "$GH_TEST_NOTES"
grep -Fq 'Co-authored-by: Copilot' <(git log -1 --format=%B)
expect_failure
grep -Fq 'already exists locally' "$test_dir/output"
printf 'PASS: generated notes, dry run, atomic push, annotated tag, duplicate rejection\n'

setup_repo curated
printf '# Changelog\n\n## [Unreleased]\n\n### Added\n\n- Curated note\n\n## [v1.2.2] - 2026-01-01\n\n- Historical note\n' > CHANGELOG.md
git add CHANGELOG.md
git commit --quiet -m 'Document feature'
bash scripts/release.sh > "$test_dir/output" 2>&1
assert_published
grep -Fxq -- '- Curated note' "$GH_TEST_NOTES"
! grep -Fq 'Document feature' "$GH_TEST_NOTES"
grep -Fq -- '- Historical note' CHANGELOG.md
[[ $(awk '/^## \[Unreleased\]/{inside=1; next} /^## /{inside=0} inside && /[^[:space:]]/{n++} END{print n+0}' CHANGELOG.md) == 0 ]]
printf 'PASS: promotes curated notes and preserves historical entries\n'

setup_repo existing
printf '# Changelog\n\n## [Unreleased]\n\n- Future work\n\n## [v1.2.3] - 2026-01-02\n\n- Existing release note\n\n## [v1.2.2] - 2026-01-01\n\n- Historical note\n' > CHANGELOG.md
git add CHANGELOG.md
git commit --quiet -m 'Prepare notes'
before=$(git rev-parse HEAD)
bash scripts/release.sh > "$test_dir/output" 2>&1
assert_published
[[ $(git rev-parse HEAD) == "$before" ]]
grep -Fxq -- '- Existing release note' "$GH_TEST_NOTES"
! grep -Fq 'Future work' "$GH_TEST_NOTES"
grep -Fq -- '- Future work' CHANGELOG.md
printf 'PASS: preserves existing entry without an unnecessary commit\n'

setup_repo guards
printf 'uncommitted\n' >> feature.txt
expect_failure --dry-run
grep -Fq 'Commit or stash' "$test_dir/output"
git add feature.txt
git commit --quiet -m 'Commit change'
printf 'project(example VERSION invalid LANGUAGES C)\n' > CMakeLists.txt
git add CMakeLists.txt
git commit --quiet -m 'Invalid version'
expect_failure --dry-run
grep -Fq 'literal numeric' "$test_dir/output"
printf 'cmake_minimum_required(VERSION 3.15)\nproject(example LANGUAGES C)\nset(OTHER_VERSION VERSION 9.9.9)\n' > CMakeLists.txt
git add CMakeLists.txt
git commit --quiet -m 'Missing project version'
expect_failure --dry-run
grep -Fq 'literal numeric' "$test_dir/output"
printf 'PASS: rejects dirty trees, invalid versions, and missing project versions\n'

setup_repo first
git tag -d v1.2.2 >/dev/null
bash scripts/release.sh > "$test_dir/output" 2>&1
assert_published
grep -Fxq -- '- Initial release' "$GH_TEST_NOTES"
printf 'PASS: first release uses all non-merge commits\n'

setup_repo remote-tag
git tag v1.2.3
git push --quiet origin refs/tags/v1.2.3
git tag -d v1.2.3 >/dev/null
before=$(git rev-parse HEAD)
expect_failure
grep -Fq 'already exists on origin' "$test_dir/output"
[[ $(git rev-parse HEAD) == "$before" && -z $(git status --porcelain) ]]
printf 'PASS: remote duplicate rejected before changing changelog\n'

setup_repo crlf
printf 'cmake_minimum_required(VERSION 3.15)\r\nproject(example VERSION "1.2.3" LANGUAGES C)\r\n' > CMakeLists.txt
printf '# Changelog\r\n\r\n## [Unreleased]\r\n\r\n- Windows note\r\n\r\n## [v1.2.2] - 2026-01-01\r\n\r\n- Historical note\r\n' > CHANGELOG.md
git add .
git commit --quiet -m 'Use Windows line endings'
bash scripts/release.sh > "$test_dir/output" 2>&1
assert_published
grep -Fxq -- '- Windows note' "$GH_TEST_NOTES"
printf 'PASS: CRLF input and quoted, single-line CMake version\n'

setup_repo push-failure
cat > "$test_dir/push-failure.git/hooks/pre-receive" <<'EOF'
#!/usr/bin/env bash
printf 'Reject test push\n' >&2
exit 1
EOF
chmod +x "$test_dir/push-failure.git/hooks/pre-receive"
expect_failure
grep -Fq 'Push failed' "$test_dir/output"
[[ $(git cat-file -t refs/tags/v1.2.3) == tag ]]
[[ -z $(git ls-remote origin) ]]
[[ ! -e $GH_TEST_ARGS ]]
printf 'PASS: rejected atomic push leaves local state but publishes nothing\n'

setup_repo github-failure
export GH_TEST_FAIL=true
expect_failure
grep -Fq 'release creation failed' "$test_dir/output"
assert_published
printf 'PASS: GitHub failure is explicit and retains published tag\n'

printf 'All release script tests passed.\n'
