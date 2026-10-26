<!-- Last content update: Unreleased (latest tag: v0.2.0) -->
# libamcore

`libamcore` is a portable C library for core antenna-measurement calculations.
Project contains the calculation API and the `amcore` command-line tool.

The public API is available through `<amcore/amcore.h>`. It uses C99, supports
C++ consumers, and does not require platform-specific runtime dependencies.

## Calculations

| Module | Function | Description |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength` | Computes free-space wavelength from frequency. |
| `farfield` | `amcore_far_field_distance` | Computes minimum far-field distance from frequency and aperture length. |
| `farfield` | `amcore_far_field_line_of_sight_km` | Estimates line-of-sight range from height. |
| `farfield` | `amcore_far_field_radio_horizon_km` | Estimates radio horizon from height. |
| `farfield` | `amcore_far_field_path_loss_db` | Computes free-space path loss from frequency, range, and antenna gains. |

All calculations are referenced from the relevant standards and literature. See the [References](#references) section for details.


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
build/amcore farfield line-of-sight 100
build/amcore farfield radio-horizon 100
build/amcore farfield path-loss 100 1 0 0
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

The output `amcore.wasm` exports the library's calculation functions.

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

## References

[1] [IEEE Std 149-2021](https://standards.ieee.org/standard/149-2021.html) - Standard for Antenna Measurements

[2] [Line-of-sight propagation](https://en.wikipedia.org/wiki/Line-of-sight_propagation) - Approximate line-of-sight range and radio horizon formulas

[3] IEEE Std 1720-2012, section B.2.2, and Warren L. Stutzman and Gary A. Thiele, *Antenna Theory and Design*, 3rd edition, page 43 - Free-space path-loss formula
