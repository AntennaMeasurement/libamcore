# Build and install

## Requirements

- CMake 3.15 or newer
- A C99-capable compiler

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

## Install

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build --prefix install
```

An installed CMake consumer can then locate and link the library with
`find_package(amcore CONFIG REQUIRED)` and `amcore::amcore`.

See [CMake integration](cmake-integration.md) for a consumer example and
available build options.
