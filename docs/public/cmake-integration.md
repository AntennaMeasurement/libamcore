# CMake integration

## Add the project as a subdirectory

When `libamcore` is included in a larger CMake project with `add_subdirectory`,
link the exported build-tree alias:

```cmake
add_subdirectory(path/to/libamcore)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

The target supplies the public include directory, so application code can
include `<amcore/amcore.h>`.

## Use an installed package

After [installing](getting-started.md#install) the library, use its CMake
package:

```cmake
find_package(amcore CONFIG REQUIRED)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

## Configure options

| Option | Default | Effect |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | Build a shared library instead of a static library. |
| `AMCORE_BUILD_CLI` | `ON` | Build and install the `amcore` command-line tool. |
| `BUILD_TESTING` | `OFF` | Build and register the tests. |
| `AMCORE_BUILD_WASM` | `OFF` | Build the standalone WebAssembly target; requires the Emscripten toolchain. |

For embedded or cross-compiled builds, provide the relevant CMake toolchain
file and commonly disable both tests and the CLI:

```sh
cmake -S . -B build-firmware \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DBUILD_TESTING=OFF \
  -DAMCORE_BUILD_CLI=OFF
```
