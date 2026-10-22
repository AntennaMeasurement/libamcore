# libamcore

`libamcore` is a C library for core computational routines used in antenna
measurements. The initial API provides free-space wavelength calculation; the
library is intended to grow with additional documented, tested routines.

## Build and test

Requirements: CMake 3.21 or newer and a C99-capable C compiler.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

To install the library and its CMake package:

```sh
cmake --install build --prefix install
```

Consumers can then use `find_package(amcore CONFIG REQUIRED)` and link
`amcore::amcore`.

## HTML documentation

Install Doxygen, then configure and build the documentation target:

```sh
cmake -S . -B build -DAMCORE_BUILD_DOCS=ON
cmake --build build --target docs
```

The generated site is in `build/docs/html/index.html`.

## vcpkg

The repository includes a vcpkg manifest. With vcpkg available, configure the
project using its toolchain file:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

## Packaging

CPack packages the installed library, headers, and CMake package configuration.
Run `cpack` from the configured build directory to create a ZIP archive on
Windows or a tarball on other platforms.
