import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

assert.ok(process.argv[2], 'Usage: node tests/test_wasm.mjs <amcore.wasm>');
const wasi = new WASI({ version: 'preview1', returnOnExit: true });
const bytes = await readFile(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {
    wasi_snapshot_preview1: wasi.wasiImport,
});
const api = instance.exports;
assert.equal(api.main, undefined, 'The library must not export the CLI entry point');
assert.equal(api._start, undefined, 'The library must not be a WASI command');
wasi.initialize(instance);

assert.ok(api.memory instanceof WebAssembly.Memory);
assert.equal(typeof api.malloc, 'function');
assert.equal(typeof api.free, 'function');
const output = api.malloc(24);
assert.notEqual(output, 0, 'Result storage allocation must succeed');
assert.equal(output % 8, 0, 'Result storage must be aligned for doubles');
const memory = new DataView(api.memory.buffer);
const lightSpeed = 299792458;
assert.ok(api.speed_of_light_m_per_s instanceof WebAssembly.Global);
assert.equal(memory.getFloat64(api.speed_of_light_m_per_s.value, true), lightSpeed);

function close(actual, expected) {
    assert.ok(
        Number.isFinite(actual) &&
            Math.abs(actual - expected) <= 1e-12 * Math.max(1, Math.abs(expected)),
        `Expected ${expected}, got ${actual}`,
    );
}

const cases = [
    ['amcore_free_space_wavelength_m', [1e9], lightSpeed / 1e9],
    ['amcore_far_field_distance', [1e9, 1], 2e9 / lightSpeed],
    ['amcore_far_field_line_of_sight_km', [100], 35.7],
    ['amcore_far_field_radio_horizon_km', [100], 41.2],
    ['amcore_far_field_path_loss_db', [100, 1, 2, 3], 67.45],
    ['amcore_near_field_separation_distance_m', [1e9], 5 * lightSpeed / 1e9],
    ['amcore_near_field_planar_sampling_spacing_m', [1e9], lightSpeed / 2e9],
    ['amcore_near_field_planar_scan_length_m', [1, 2, 45], 5],
    ['amcore_near_field_planar_angle_of_view_deg', [1, 2, 5], 45],
    ['amcore_near_field_cylindrical_ideal_y_scan_length_m', [1, 0.5, 2, 1, 0, 45], 3.5],
    ['amcore_near_field_spherical_maximum_radius_extent_m', [3, 4, 12], 13],
];

for (const [name, args, expected] of cases) {
    assert.equal(typeof api[name], 'function', `Missing export: ${name}`);
    assert.equal(api[name](...args, output), 0, `${name} must succeed`);
    close(memory.getFloat64(output, true), expected);
    assert.equal(api[name](...args, 0), 1, `${name} must reject null output`);
}

assert.equal(
    api.amcore_near_field_planar_sampling_parameters(
        1e9, 1, output, output + 8, output + 16, output + 20,
    ),
    0,
);
close(memory.getFloat64(output, true), -0.596);
close(memory.getFloat64(output + 8, true), 0.596);
assert.equal(memory.getUint32(output + 16, true), 9);
assert.equal(memory.getUint32(output + 20, true), 149);
assert.equal(
    api.amcore_near_field_planar_sampling_parameters(
        1e9, 1, 0, output + 8, output + 16, output + 20,
    ),
    1,
);

memory.setFloat64(output, 123, true);
for (const frequency of [0, -1, NaN, Infinity]) {
    assert.equal(api.amcore_free_space_wavelength_m(frequency, output), 1);
    assert.equal(memory.getFloat64(output, true), 123);
}
assert.equal(api.amcore_free_space_wavelength_m(Number.MIN_VALUE, output), 2);
assert.equal(memory.getFloat64(output, true), 123);

api.free(output);
console.log('PASS: standalone WASM library exports, calculations, and error handling');
