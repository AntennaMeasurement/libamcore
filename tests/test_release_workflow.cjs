const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

function workflowScript(name) {
  const workflow = fs.readFileSync(
    path.join(__dirname, '..', '.github', 'workflows', name), 'utf8');
  const lines = workflow.replace(/\r\n/g, '\n').split('\n');
  const start = lines.indexOf('          script: |');
  assert.notEqual(start, -1);
  const script = [];
  for (const line of lines.slice(start + 1)) {
    if (line.trim() && !line.startsWith('            ')) break;
    script.push(line.slice(12));
  }
  return `(async () => {\n${script.join('\n')}\n})()`;
}

const prepareScript = workflowScript('ci.yml');
const publishScript = workflowScript('release.yml');
const history = '## [v1.2.2] - 2026-10-01\n\n- Historical change.\n';

async function prepare({
  changelog = `# Changelog\n\n## [Unreleased]\n\n### Changed\n\n- Pending change.\n\n${history}`,
  tags = 'v1.2.2',
  ref = 'refs/heads/main',
  overrides = {},
} = {}) {
  const files = {
    'CMakeLists.txt': 'project(libamcore\n VERSION 1.2.3\n LANGUAGES C)\n',
    'CHANGELOG.md': changelog,
    'docs/public/api.json': '{\n  "library_version": "0.1.0",\n  "functions": []\n}\n',
    ...overrides,
  };
  const original = { ...files };
  const calls = [];
  const outputs = {};
  let committed;
  const fileSystem = {
    readFileSync: name => {
      assert.ok(Object.hasOwn(files, name), `Unexpected file: ${name}`);
      return files[name];
    },
    writeFileSync: (name, content) => { files[name] = content; },
  };
  await vm.runInNewContext(prepareScript, {
    require: name => {
      assert.equal(name, 'fs');
      return fileSystem;
    },
    core: { setOutput: (name, value) => { outputs[name] = value; } },
    context: {
      eventName: 'push', ref, sha: 'source-sha', repo: {}, runId: 1,
      payload: { ref },
    },
    exec: {
      getExecOutput: async (command, args) => {
        assert.equal(command, 'git');
        calls.push(Array.from(args));
        let stdout = '';
        if (args[0] === 'tag' && args[1] === '--list') stdout = tags;
        if (args[0] === 'log') stdout = 'Commit subject\nAnother subject\n';
        if (args[0] === 'diff') {
          stdout = Object.keys(original).filter(name => files[name] !== original[name]).join('\n');
        }
        if (args[0] === 'commit') committed = { ...files };
        return { stdout };
      },
    },
    github: { paginate: async () => [], rest: { actions: { listWorkflowRunsForRepo: {} } } },
  });
  return { files, original, calls, outputs, committed, fileSystem };
}

test('moves pending notes and synchronizes manifests before committing and tagging', async () => {
  const result = await prepare();
  const { files, calls, committed, outputs } = result;
  assert.match(files['CHANGELOG.md'], /## \[Unreleased\]\n\n## \[v1\.2\.3\] - \d{4}-\d{2}-\d{2}/);
  assert.equal(files['CHANGELOG.md'].split('- Pending change.').length, 2);
  assert.ok(files['CHANGELOG.md'].endsWith(history));
  assert.equal(files['docs/public/api.json'],
    result.original['docs/public/api.json'].replace('0.1.0', '1.2.3'));
  assert.deepEqual(committed, files);
  assert.deepEqual(outputs, { tag: 'v1.2.3', changed: 'true' });
  assert.equal(calls.some(args => args[0] === 'log'), false);
  assert.deepEqual(calls.find(args => args[0] === 'add'),
    ['add', '--', 'CHANGELOG.md', 'docs/public/api.json']);
  const commitIndex = calls.findIndex(args => args[0] === 'commit');
  const tagIndex = calls.findIndex(args => args[0] === 'tag' && args[1] === '-a');
  assert.ok(commitIndex < tagIndex);
  assert.match(calls[tagIndex][4], /Pending change/);
  assert.doesNotMatch(calls[tagIndex][4], /Unreleased/);
  assert.equal(calls[tagIndex].at(-1), 'HEAD');
  assert.ok(calls.findIndex(args => args[0] === 'push') > tagIndex);

  await vm.runInNewContext(publishScript, {
    require: () => result.fileSystem,
    process: { env: { RELEASE_TAG: 'v1.2.3' } },
  });
  assert.match(files['release-notes.md'], /Pending change/);
  assert.doesNotMatch(files['release-notes.md'], /Unreleased|Historical change/);
});

test('uses commit subjects with empty or absent Unreleased sections', async () => {
  for (const pending of ['## [Unreleased]\n\n', '']) {
    const { files, calls } = await prepare({ changelog: `# Changelog\n\n${pending}${history}` });
    assert.match(files['CHANGELOG.md'], /### Changes\n\n- Commit subject\n- Another subject/);
    assert.equal(calls.filter(args => args[0] === 'log').length, 1);
  }
});

test('handles the first release and preserves CRLF newlines', async () => {
  const { files } = await prepare({
    tags: '',
    changelog: '# Changelog\r\n\r\n## [Unreleased]\r\n\r\n- First change.\r\n',
  });
  assert.match(files['CHANGELOG.md'], /## \[v1\.2\.3\]/);
  assert.match(files['CHANGELOG.md'], /- First change\./);
  assert.doesNotMatch(files['CHANGELOG.md'], /(?<!\r)\n/);
});

test('merges pending notes into a prewritten release without duplicating its heading', async () => {
  const { files } = await prepare({
    changelog: `# Changelog\n\n## [Unreleased]\n\n- Pending.\n\n## [v1.2.3]\n\n- Written notes.\n\n${history}`,
  });
  assert.equal(files['CHANGELOG.md'].split('## [v1.2.3]').length, 2);
  assert.match(files['CHANGELOG.md'], /## \[Unreleased\]\n\n## \[v1\.2\.3\]\n\n- Pending\./);
  assert.match(files['CHANGELOG.md'], /- Written notes\./);
  assert.equal(JSON.parse(files['docs/public/api.json']).library_version, '1.2.3');
});

test('does not create an empty preparation commit', async () => {
  const { calls } = await prepare({
    changelog: `# Changelog\n\n## [Unreleased]\n\n## [v1.2.3]\n\n- Ready.\n\n${history}`,
    overrides: {
      'docs/public/api.json': '{"library_version":"1.2.3"}',
    },
  });
  assert.equal(calls.some(args => args[0] === 'commit'), false);
  assert.ok(calls.some(args => args[0] === 'tag' && args[1] === '-a'));
});

test('leaves files unchanged when no new release is needed or on tag pushes', async () => {
  for (const options of [{ tags: 'v1.2.3' }, { ref: 'refs/tags/v1.2.3' }]) {
    const { files, original, outputs, calls } = await prepare(options);
    assert.deepEqual(files, original);
    assert.equal(outputs.changed, 'false');
    assert.equal(calls.some(args => args[0] === 'push'), false);
  }
});

test('rejects reused version tags and invalid version metadata', async () => {
  await assert.rejects(prepare({ tags: 'v1.2.4\nv1.2.3' }), /already exists/);
  await assert.rejects(prepare({ overrides: { 'CMakeLists.txt': 'project(libamcore)' } }),
    /Cannot read libamcore VERSION/);
  await assert.rejects(prepare({ overrides: { 'docs/public/api.json': '{}' } }),
    /Cannot read library_version from docs\/public\/api.json/);
});
