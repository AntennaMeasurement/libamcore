# WebAssembly

The optional standalone WebAssembly target requires an installed and
activated Emscripten SDK. Configure with Emscripten's CMake wrapper and disable
the native CLI and tests:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DBUILD_TESTING=OFF
cmake --build build-wasm --target amcore_wasm
```

The build produces `build-wasm/amcore.wasm`, which exports
`amcore_free_space_wavelength` and `amcore_far_field_distance` without a
`main` entry point. The C functions keep their normal signatures: the result
pointer refers to a location in WebAssembly linear memory. When
`AMCORE_BUILD_WASM` is enabled, the module is installed into the library
directory.

Desktop SQLite operation logging is unavailable for WebAssembly. Configuring
with `AMCORE_ENABLE_OPERATION_LOGGING=ON` and either Emscripten or
`AMCORE_BUILD_WASM=ON` fails explicitly. Keep logging disabled (the default);
the WebAssembly module has no logging, database, or native filesystem dependency.

## Telemetry

`AMCORE_ENABLE_TELEMETRY=ON` selects Emscripten Fetch instead of libwebsockets.
Unlike the default standalone module, this build produces `amcore.js` plus
`amcore.wasm` and any toolchain-generated worker script. Deploy these files
together. The JavaScript module factory is `createAmcore`; calculation exports
and `malloc`/`free` and the `HEAPF64` memory view are available. The installed CMake library target propagates
`-pthread` and `-sFETCH=1` to applications linking it.

Telemetry builds fetch coreJSON v3.3.1 during CMake configuration and compile
the validator for WebAssembly. For offline builds, point
`FETCHCONTENT_SOURCE_DIR_COREJSON` at a local coreJSON checkout. The serialized
request body is capped at 1 MiB by default. Set
`AMCORE_TELEMETRY_MAX_BODY_SIZE_KB` at CMake configuration time to change the
limit in KB (1 KB = 1024 bytes).

```sh
emcmake cmake -S . -B build-wasm-telemetry \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF \
  -DBUILD_TESTING=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DAMCORE_ENABLE_TELEMETRY=ON \
  -DAMCORE_TELEMETRY_ENDPOINT=https://api.signalregistry.net
cmake --build build-wasm-telemetry --target amcore_wasm
```

The URL is an example, not a verified service or default destination.
Calculation APIs remain synchronous. **Invoke them from a pthread worker
thread, not the browser main thread**: Emscripten synchronous Fetch requires
worker execution. Main-browser-thread calls still calculate correctly but
emit an explicit telemetry error rather than attempting an unsupported
blocking request. Host applications must create their pthread worker and
dispatch calculations there; this module does not create an application worker
or silently queue events. A generic JavaScript Web Worker is not a substitute
for an Emscripten pthread.

For an Emscripten application with its own C `main`, link `amcore::amcore`
and move that application main to a pthread, for example:

```cmake
target_link_libraries(my_app PRIVATE amcore::amcore)
target_link_options(my_app PRIVATE
    "-sPROXY_TO_PTHREAD=1"
    "-sPTHREAD_POOL_SIZE=1"
)
```

Calculations invoked by that C main can use synchronous telemetry without
blocking the browser's UI thread. Applications managing their own pthreads
can instead call the calculation APIs from those workers.

Serve the application with cross-origin isolation headers:

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

The browser must support `SharedArrayBuffer` and pthread workers. The endpoint
must permit the application's origin and JSON POSTs via CORS (including the
`Content-Type` preflight). HTTPS pages cannot send to HTTP endpoints due to
mixed-content rules. Browser certificate verification is used; the desktop
`AMCORE_TELEMETRY_CA_FILE` option has no effect.

Each event uses synchronous Fetch with a five-second request timeout. No
local persistence, retries or authentication headers are added. Browser XHR
redirect handling follows browser policy, unlike the desktop transport's
explicit no-redirect setting; use a direct, trusted endpoint with no redirects.
Network, CORS and non-2xx errors go to stderr without changing calculations.
See [payload and privacy details](cmake-integration.md#telemetry).
