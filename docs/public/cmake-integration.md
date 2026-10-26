# CMake integration

## Add the project as a subdirectory

When `libamcore` is included in a larger CMake project with `add_subdirectory`,
link the build-tree alias:

```cmake
add_subdirectory(path/to/libamcore)

add_executable(my_app main.c)
target_link_libraries(my_app PRIVATE amcore::amcore)
```

The target supplies the public include directory for headers such as
`<amcore/amcore.h>`.

## Use an installed package

After [installing](getting-started.md#install) the library:

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
| `AMCORE_BUILD_WASM` | `OFF` | Build the standalone WebAssembly target; requires Emscripten. |

Operation logging, telemetry, and CLI call history are not included in this
branch.

## WebAssembly

The optional standalone WebAssembly module requires the Emscripten toolchain.
See [the WebAssembly guide](webassembly.md) for a build example and exported
functions.
