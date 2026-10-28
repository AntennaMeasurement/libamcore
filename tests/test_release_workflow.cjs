const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');

const workflowsPath = path.join(__dirname, '..', '.github', 'workflows');
const ci = fs.readFileSync(path.join(workflowsPath, 'ci.yml'), 'utf8');

test('CI builds main and developer-pushed version tags without a version gate', () => {
  assert.match(ci, /push:\s+branches:\s+- main\s+tags:\s+- "v\*"/);
  assert.doesNotMatch(ci, /^\s+(?:needs|if|ref):/m);
  assert.match(ci, /os: \[ubuntu-latest, windows-latest, macos-latest\]/);
  assert.match(ci, /cmake -S \. -B build -DBUILD_TESTING=ON/);
  assert.match(ci, /cmake --build build --config Release/);
  assert.match(ci, /ctest --test-dir build -C Release --output-on-failure/);
});

test('CI runs the release policy tests', () => {
  assert.match(ci, /run: node --test tests\/test_release_workflow\.cjs/);
});

test('workflows leave tags, releases, and changelog updates to developers', () => {
  assert.equal(fs.existsSync(path.join(workflowsPath, 'release.yml')), false);
  for (const name of fs.readdirSync(workflowsPath).filter(name => /\.ya?ml$/.test(name))) {
    const workflow = fs.readFileSync(path.join(workflowsPath, name), 'utf8');
    assert.doesNotMatch(workflow, /contents:\s*write/, name);
    assert.doesNotMatch(workflow, /git\s+(?:tag|push|commit)\b/, name);
    assert.doesNotMatch(workflow, /action-gh-release|gh release create|createRelease/, name);
    assert.doesNotMatch(workflow, /writeFileSync|CHANGELOG\.md/, name);
  }
});
