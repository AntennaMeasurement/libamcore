# Build and install

## Requirements

- CMake 3.15 or newer
- A C99-capable compiler
- An activated Emscripten SDK only when building WebAssembly

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

To build and run the tests:

```sh
cmake -S . -B build-tests -DBUILD_TESTING=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

The `amcore_tests` executable prints a PASS/FAIL list and a final summary.
All tests run even if a test fails, and any failure returns a nonzero exit
status. For example, on Windows with a Visual Studio Release build:

```powershell
.\build-tests\Release\amcore_tests.exe
```

Output includes:

```text
Running 20 tests:
  [PASS] test_wavelength
  ...
  [PASS] test_near_field_spherical

Summary: 20 passed, 0 failed, 20 total
```

CTest normally hides output from successful tests. To display the list through
CTest, use:

```sh
ctest --test-dir build-tests -C Release --verbose
```

## Install

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build
```

On Windows, the default install location is
`%LOCALAPPDATA%\Programs\libamcore`, so installation does not require
administrator permissions. You can still choose a different location when
configuring with `-DCMAKE_INSTALL_PREFIX`.

An installed CMake consumer can locate and link the library with
`find_package(amcore CONFIG REQUIRED)` and `amcore::amcore`.

## Build the documentation

From the repository root, install the MkDocs dependencies listed in
`requirements-docs.txt` and build the public documentation:

```sh
python -m pip install -r requirements-docs.txt
python -m mkdocs build --strict
```

MkDocs writes the generated site to `site/`. To serve a live preview while
editing the documentation:

```sh
python -m mkdocs serve
```

Open the local URL printed by MkDocs to view the preview.

## WebAssembly

See [the WebAssembly guide](webassembly.md) for the optional standalone module.
