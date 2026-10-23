# Build and install

## Requirements

- CMake 3.15 or newer
- A C99-capable compiler
- SQLite development headers and libraries only when desktop operation logging
  is enabled (not required by the default build)

## Build

The default build produces a static library and the `amcore` command-line
tool:

```sh
cmake -S . -B build
cmake --build build --config Release
```

To build a shared library instead:

```sh
cmake -S . -B build-shared -DBUILD_SHARED_LIBS=ON
cmake --build build-shared --config Release
```

To build and run the test suite:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

## Optional desktop operation logging

On desktop Windows, Linux, or macOS, enable SQLite recording at build time:

```sh
cmake -S . -B build-logging \
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON \
  -DBUILD_TESTING=ON
cmake --build build-logging --config Release
ctest --test-dir build-logging -C Release --output-on-failure
```

CMake must be able to find the SQLite development files. To have vcpkg provide
them, configure with the vcpkg toolchain file and enable the manifest feature
that declares SQLite. The manifest pins a `builtin-baseline`, as required by
Visual Studio's bundled vcpkg. In manifest mode, do not append a package name
such as `sqlite3` to `vcpkg install`. To install the feature's dependencies manually,
run this from the repository root:

```powershell
vcpkg install --x-feature=operation-logging
```

Or let CMake install the feature's dependencies during configuration. In
PowerShell, with `VCPKG_ROOT` set to the vcpkg checkout:

```powershell
cmake -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON `
  -DBUILD_TESTING=ON
```

This configuration installs the feature's `sqlite3` dependency for the
selected triplet.

Supply the toolchain on the build directory's first configuration. If
`build-logging` was already configured without it, use a new build directory
or add `--fresh` to the configure command (CMake 3.24 or newer). `--fresh`
resets cached options; pass all required options again. See
[CMake integration](cmake-integration.md#desktop-operation-logging) for the
recovery command.

Run `amcore --help` from the resulting build to see the logging setting and,
when enabled, the database location template.

Logging is off by default and must remain off for WebAssembly and embedded
builds, including embedded Linux. See
[desktop operation logging](cmake-integration.md#desktop-operation-logging)
for database locations, schema, timing, error handling, and data-retention
considerations.

## Install

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build --prefix install
```

An installed CMake consumer can then locate and link the library with
`find_package(amcore CONFIG REQUIRED)` and `amcore::amcore`.

See [CMake integration](cmake-integration.md) for a consumer example and
available build options.

Logging-enabled installed packages also discover SQLite for CMake consumers.
If SQLite is dynamically linked, its runtime library must be distributed
separately; it is not bundled by libamcore's CPack packages.
