<!-- Last content update: Unreleased (latest tag: v0.2.0) -->
# libamcore

`libamcore` is a portable C library for core antenna-measurement calculations.
This branch contains the calculation API and the `amcore` command-line tool;
operation logging, call history, and telemetry are not included.

## Calculations

| Module | Function | Description |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength` | Computes free-space wavelength from frequency. |
| `farfield` | `amcore_far_field_distance` | Computes minimum far-field distance from frequency and aperture length. |

Far-field distance follows IEEE Std 149-2021, section 7.1.2:
`R_ff = max(5D, 1.6 lambda, 2D^2/lambda)`.

The public API is available through `<amcore/amcore.h>`. It uses C99, supports
C++ consumers, and does not require platform-specific runtime dependencies.
Inputs must be positive and finite. Output parameters are changed only on
success.

## Build

Requirements: CMake 3.15 or newer and a C99-capable compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
```

The default build produces a static library and the CLI. To build a shared
library:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON
cmake --build build-shared --config Release
```

To configure and run tests:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

## Command-line interface

The CLI is enabled by default. Its calculation output is numeric-only on
standard output; invalid arguments and out-of-range calculations produce an
error on standard error and a nonzero exit status.

```sh
build/amcore help
build/amcore version
build/amcore utility wavelength 1e9
build/amcore farfield distance 1e9 1
```

Use `-DAMCORE_BUILD_CLI=OFF` to omit the CLI.

## WebAssembly

An optional standalone WebAssembly module can be built with an activated
Emscripten SDK:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-wasm --target amcore_wasm
```

The output `amcore.wasm` exports the two calculation functions.

## Install and consume

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build --prefix install
```

Consumers can link the installed package with:

```cmake
find_package(amcore CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

## Documentation

The public documentation is built with MkDocs. From the repository root,
install the documentation dependencies and build the site:

```sh
python -m pip install -r requirements-docs.txt
python -m mkdocs build --strict
```

The generated site is written to `site/`. To preview it locally while editing:

```sh
python -m mkdocs serve
```

Then open the local URL printed by MkDocs. See [build and install](docs/public/getting-started.md),
[the C API](docs/public/api.md), [the CLI](docs/public/cli.md),
[CMake integration](docs/public/cmake-integration.md), and
[WebAssembly](docs/public/webassembly.md).

## Releases

To release a new version, update `project(libamcore VERSION ...)` in
`CMakeLists.txt` and push to `main`. CI moves the `CHANGELOG.md` Unreleased
details into the new version's release notes, leaving an empty Unreleased
section. If there are no pending details, it uses commit subjects instead.
It also synchronizes `vcpkg.json` (`version-string`) and `docs/public/api.json`
(`library_version`) with the CMake version, then commits these files before
creating the release tag. The release workflow packages that tagged commit
and publishes its versioned changelog entry as the GitHub release notes.

Release preparation regression tests run with Node.js:

```sh
node --test tests/test_release_workflow.cjs
```
