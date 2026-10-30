# WebAssembly

GitHub releases include `amcore.wasm` alongside the native binaries. This is
the standalone calculation library, not a WebAssembly build of the CLI.
CI builds it with Emscripten 4.0.10 and tests the exported API with Node.js 22
on Linux for pushes, pull requests, manual runs, and releases.

The optional standalone WebAssembly target requires an installed and activated
Emscripten SDK. Configure with Emscripten's CMake wrapper and disable the
native CLI:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-wasm --target amcore_wasm
```

The build produces `build-wasm/amcore.wasm`, which exports the calculation
functions, including `amcore_free_space_wavelength_m`,
`amcore_far_field_distance`, `amcore_far_field_line_of_sight_km`,
`amcore_far_field_radio_horizon_km`, and `amcore_far_field_path_loss_db`,
and the near-field functions
`amcore_near_field_separation_distance_m`,
`amcore_near_field_planar_ideal_step_size_m`,
`amcore_near_field_planar_ideal_scan_length_m`,
`amcore_near_field_planar_angle_of_view_deg`,
`amcore_near_field_planar_sampling_parameters`,
`amcore_near_field_cylindrical_ideal_y_scan_length_m`,
`amcore_near_field_cylindrical_ideal_y_step_size_m` and
`amcore_near_field_spherical_maximum_radius_extent_m`, without a `main` entry
point. It also exports `malloc` and `free` for allocating result storage.
The C functions keep their normal signatures;
result pointers refer to locations in WebAssembly linear memory. When
`AMCORE_BUILD_WASM` is enabled, the module is installed into the library
directory.

## Test the distribution

With Node.js 22 or newer, run the same API smoke test used by CI:

```sh
node tests/test_wasm.mjs build-wasm/amcore.wasm
```

The test initializes the module as a WASI preview 1 reactor, verifies all
calculation exports and the speed-of-light constant, and checks numeric results,
output pointers, and error statuses. The module has no CLI or `_start` entry
point. Hosts must provide any WASI preview 1 imports requested by the module and
call `_initialize` once before using the library (Node.js's `WASI.initialize`
does this).

Output pointers are byte offsets into the exported `memory`; doubles occupy
8 bytes and `size_t` occupies 4 bytes in this wasm32 build. Use properly aligned
storage allocated with the exported `malloc` and release it with `free` when
finished. Check for a zero allocation result before passing pointers. The
smoke test shows how to allocate output storage and read results with a
little-endian `DataView`. Function return values are the normal `amcore_status` codes:
0 for success, 1 for invalid arguments, and 2 for out-of-range results.
