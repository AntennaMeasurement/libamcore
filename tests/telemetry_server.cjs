const assert = require('node:assert/strict');
const http = require('node:http');
const https = require('node:https');
const { readFileSync } = require('node:fs');
const { spawn } = require('node:child_process');

const [endpoint, mode, executable, ...args] = process.argv.slice(2);
const url = new URL(endpoint);
assert.ok(url.protocol === 'http:' || url.protocol === 'https:');
assert.equal(url.hostname, '127.0.0.1');
const events = [];
const failures = [];
const sockets = new Set();
function filterTransportNotices(text) {
    return text.replace(
        /\[[^\r\n]*\] [NW]: (lws_plat_vhost_tls_client_ctx_init: Imported \d+ certs from plat store|lws_plat_set_socket_options_ip: not implemented on windows platform|lws_gate_accepts: on = [01])\r?\n/g,
        '');
}

function handleRequest(request, response) {
    let body = '';
    request.setEncoding('utf8');
    request.on('data', chunk => { body += chunk; });
    request.on('end', () => {
        try {
            assert.equal(request.method, 'POST');
            assert.equal(request.url, url.pathname + url.search);
            assert.equal(request.headers['content-type'], 'application/json');
            assert.equal(Number(request.headers['content-length']), Buffer.byteLength(body));
            events.push(JSON.parse(body));
        } catch (error) {
            failures.push(error);
        }
        if (mode === 'timeout') {
            return;
        }
        if (mode === 'disconnect') {
            request.socket.destroy();
            return;
        }
        if (mode === 'redirect') {
            response.writeHead(302, { Location: '/must-not-follow' });
        } else {
            response.writeHead(mode === 'status' ? 503 : 204);
        }
        response.end();
    });
}
const server = url.protocol === 'https:'
    ? https.createServer({
        cert: readFileSync(process.env.AMCORE_TEST_TLS_CERT),
        key: readFileSync(process.env.AMCORE_TEST_TLS_KEY)
    }, handleRequest)
    : http.createServer(handleRequest);
server.on('connection', socket => {
    sockets.add(socket);
    socket.on('close', () => sockets.delete(socket));
});

function verifyEvents(started, finished) {
    assert.equal(events.length, 15, 'all successful, failed and nested calls must be sent');
    for (const event of events) {
        assert.deepEqual(Object.keys(event).sort(),
            ['version', 'function', 'inputs', 'output_is_null', 'return_value', 'output',
                'executed_at_us', 'duration_ns'].sort());
        assert.match(event.version, /^\d+\.\d+\.\d+$/);
        assert.ok(Number.isInteger(event.executed_at_us));
        assert.ok(event.executed_at_us >= started * 1000);
        assert.ok(event.executed_at_us <= finished * 1000);
        assert.ok(Number.isInteger(event.duration_ns) && event.duration_ns >= 0);
        assert.ok(event.duration_ns < 60e9);
        assert.equal(typeof event.output_is_null, 'boolean');
        if (event.return_value !== 0) {
            assert.equal(event.output, null);
        }
    }
    const wavelength = events.filter(event => event.function === 'amcore_free_space_wavelength');
    const farfield = events.filter(event => event.function === 'amcore_far_field_distance');
    assert.equal(wavelength.length, 10);
    assert.equal(farfield.length, 5);
    assert.equal(wavelength.filter(event => event.return_value === 0).length, 3);
    assert.equal(wavelength.filter(event => event.return_value === 1).length, 6);
    assert.equal(wavelength.filter(event => event.return_value === 2).length, 1);
    assert.deepEqual(wavelength.slice(1, 5).map(event => event.inputs.frequency_hz),
        ['NaN', '+Infinity', '-Infinity', 0]);
    assert.equal(wavelength[5].output_is_null, true);
    assert.deepEqual(wavelength[0].inputs, { frequency_hz: 1e9 });
    assert.deepEqual(wavelength[0].output, { wavelength_m: 0.299792458 });
    assert.deepEqual(farfield[0].inputs, { frequency_hz: 1e9, aperture_length_m: 1 });
    assert.ok(Math.abs(farfield[0].output.distance_m - 2e9 / 299792458) < 1e-12);
    assert.deepEqual(farfield.map(event => event.return_value), [0, 1, 1, 2, 1]);
    assert.equal(farfield[4].output_is_null, true);
    assert.ok(farfield[0].executed_at_us <= wavelength[7].executed_at_us);
    assert.ok(farfield[0].duration_ns >= wavelength[7].duration_ns);
    assert.ok(events.indexOf(wavelength[7]) < events.indexOf(farfield[0]));
}

async function main() {
    await new Promise((resolve, reject) => {
        server.once('error', reject);
        server.listen(Number(url.port), '127.0.0.1', resolve);
    });
    if (mode === 'refused') {
        await new Promise(resolve => server.close(resolve));
    }
    const started = Date.now();
    let stdout = '';
    let stderr = '';
    const child = spawn(executable, args, { stdio: ['ignore', 'pipe', 'pipe'] });
    child.stdout.on('data', chunk => { stdout += chunk; });
    child.stderr.on('data', chunk => { stderr += chunk; });
    let result;
    try {
        result = await new Promise((resolve, reject) => {
            child.once('error', reject);
            child.once('close', (code, signal) => resolve({ code, signal }));
        });
    } finally {
        for (const socket of sockets) {
            socket.destroy();
        }
        await new Promise(resolve => server.close(resolve));
    }
    const finished = Date.now();
    stderr = filterTransportNotices(stderr);
    assert.equal(failures.length, 0, failures.map(String).join('\n'));
    if (mode === 'default') {
        process.stdout.write(stdout);
        process.stderr.write(stderr);
        if (args.some(argument => /^(help|--help|-h|version|--version)$/.test(argument))) {
            assert.equal(events.length, 0, 'help/version must not send telemetry');
        }
        process.exitCode = result.code ?? 1;
        return;
    }
    assert.equal(result.code, 0, `calculation failed: ${stderr}`);
    if (mode === 'verify') {
        assert.equal(stderr, '');
        verifyEvents(started, finished);
    } else if (mode === 'locale') {
        assert.equal(stderr, '');
        if (stdout.startsWith('SKIP:')) {
            process.stdout.write(stdout);
        } else {
            assert.equal(events.length, 1);
            assert.deepEqual(events[0].output, { wavelength_m: 0.299792458 });
        }
    } else if (mode === 'tls') {
        assert.equal(stderr, '');
        assert.equal(events.length, 1);
        assert.match(stdout, /^0\.29979245[0-9]*[\r\n]+$/);
        assert.equal(Number(stdout), 0.299792458);
    } else {
        assert.match(stderr, /amcore telemetry:/);
        assert.equal(events.length, ['refused', 'tls-reject'].includes(mode) ? 0 : 1);
        if (args[0] === 'utility') {
            assert.match(stdout, /^0\.29979245[0-9]*[\r\n]+$/);
            assert.equal(Number(stdout), 0.299792458);
        }
        if (mode === 'status') assert.match(stderr, /503/);
        if (mode === 'redirect') assert.match(stderr, /302/);
        if (mode === 'timeout') {
            assert.match(stderr, /timed out/);
            assert.ok(finished - started >= 4500 && finished - started < 10000,
                `timeout duration ${finished - started}ms`);
        }
    }
}

main().catch(error => {
    console.error(error);
    for (const socket of sockets) socket.destroy();
    server.close();
    process.exitCode = 1;
});
