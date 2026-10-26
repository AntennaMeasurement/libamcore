# WebAssembly

The optional standalone WebAssembly target requires an installed and activated
Emscripten SDK. Configure with Emscripten's CMake wrapper and disable the
native CLI:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-wasm --target amcore_wasm
```

The build produces `build-wasm/amcore.wasm`, which exports
`amcore_free_space_wavelength` and `amcore_far_field_distance` without a
`main` entry point. The C functions keep their normal signatures; result
pointers refer to locations in WebAssembly linear memory. When
`AMCORE_BUILD_WASM` is enabled, the module is installed into the library
directory.
