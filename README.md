<!-- Last content update tag: v0.1.0 -->
# libamcore

`libamcore` is a portable C library for core computational routines used in
antenna measurements. It builds as a static library by default for embedded use, can
also be built as a shared library, performs no dynamic allocation, and has no
operating-system or platform-specific dependencies.

## Modules and Functions

<!-- Modules and Functions table -->

| Module     | Function                       | Description                                                                       |
| ---------- | ------------------------------ | --------------------------------------------------------------------------------- |
| `utility`  | `amcore_free_space_wavelength` | Computes the free-space wavelength for a given frequency                          |
| `farfield` | `amcore_far_field_distance`    | Computes the minimum far-field distance for a given frequency and aperture length |

## Programming API

The public API is available through `<amcore/amcore.h>`, which includes the
utility module's `<amcore/utility.h>` API and the far-field module's
`<amcore/farfield.h>` API. The utility module exports
`speed_of_light_m_per_s` as a shared constant and provides
`amcore_free_space_wavelength`. `AMCORE_API` exports symbols from shared builds
on Windows and platforms with GCC-compatible visibility attributes. Consumers
linking through the CMake target receive the matching import/export settings.



## Building

Requirements: CMake 3.15 or newer and a C99-capable C compiler.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Static builds are the default. To build a shared library instead, configure
with `-DBUILD_SHARED_LIBS=ON`:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON
cmake --build build-shared --config Release
```

## Testing

To run the tests, first configure and build the test target with CMake, then use CTest to execute them. Tests are opt-in so cross-compiling firmware does not also try to build or run a host test executable:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

## Command-line interface

The `amcore` command-line tool is built by default. Use `-DAMCORE_BUILD_CLI=OFF`
to omit it, for example in a firmware cross-build. Calculations are grouped
under their associated module, using lowercase command names and positional
arguments:

```sh
build/amcore help
build/amcore version
build/amcore utility wavelength 1e9
build/amcore farfield distance 1e9 1
build/amcore utility --help
build/amcore farfield distance --help
```



## WebAssembly

With the Emscripten SDK installed and activated, configure and build the
standalone WebAssembly module:

```sh
emcmake cmake -S . -B build-wasm \
  -DAMCORE_BUILD_WASM=ON \
  -DAMCORE_BUILD_CLI=OFF \
  -DBUILD_TESTING=OFF
cmake --build build-wasm --target amcore_wasm
```

This produces `build-wasm/amcore.wasm`, exporting
`amcore_free_space_wavelength` and `amcore_far_field_distance` without a `main`
entry point. Both functions retain the C API: their final argument is a pointer
into WebAssembly linear memory where the result is written. The module is installed to the library
directory when `AMCORE_BUILD_WASM` is enabled.


## AI-assisted development

Repository-specific guidance for AI coding agents is in
[`doc/LLM_CONTEXT.md`](doc/LLM_CONTEXT.md), with the reusable development
workflow in [`.github/skills/amcore-development/SKILL.md`](.github/skills/amcore-development/SKILL.md).

## Embedded toolchain

For an embedded toolchain, pass its CMake toolchain file when configuring:

```sh
cmake -S . -B build-firmware \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/your/toolchain.cmake \
  -DBUILD_TESTING=OFF \
  -DAMCORE_BUILD_CLI=OFF
cmake --build build-firmware
```

The resulting static library can also be included directly in a firmware
project with `add_subdirectory()` and linked through the `amcore::amcore`
target. Its public header does not impose a C language standard on consumers.

To install the library, header, and CMake package:

```sh
cmake --install build --prefix install
```

Installed CMake consumers can use `find_package(amcore CONFIG REQUIRED)` and
link `amcore::amcore`.

## Public documentation

The public documentation is built with MkDocs and published to
[GitHub Pages](https://antennameasurement.github.io/libamcore/). To preview it
locally, install the documentation dependencies and start the development
server:

```sh
python -m pip install -r requirements-docs.txt
python -m mkdocs serve
```

Build the static site with:

```sh
python -m mkdocs build --strict
```

## vcpkg

The repository includes a vcpkg manifest. With vcpkg available, configure the
project using its toolchain file:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Packaging

CPack packages the installed library, headers, CMake package configuration,
and CLI when enabled. Run `cpack` from the configured build directory to create
a ZIP archive on Windows or a tarball on other platforms.

## References

[1] [IEEE Std 149-2021, IEEE Recommended Practice for Antenna Measurements](https://ieeexplore.ieee.org/document/9714428)
