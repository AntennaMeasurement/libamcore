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
| `AMCORE_ENABLE_OPERATION_LOGGING` | `OFF` | Record public calculation calls in SQLite on desktop Windows, Linux, or macOS. Not supported for WebAssembly or bare-metal targets; leave off for embedded Linux. |

## Desktop operation logging

Enable `AMCORE_ENABLE_OPERATION_LOGGING` to store successful, failed, and nested
calls in `libamcore.db`, with one table per public calculation function.
SQLite development files must be available to CMake (`find_package(SQLite3)`).
With vcpkg, use the repository's manifest feature so the dependency is installed
for the same triplet as the build. The manifest pins a `builtin-baseline` to
select registry dependency versions, including when using Visual Studio's
bundled vcpkg. CMake can install it during configuration;
in PowerShell, with `VCPKG_ROOT` set to the vcpkg checkout:

```powershell
cmake -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
cmake --build build-logging --config Release
```

The toolchain must be present on the first configuration of a build directory.
If that directory was previously configured without vcpkg and SQLite is not
found, reset the cached configuration with CMake 3.24 or newer:

```powershell
cmake --fresh -S . -B build-logging `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_MANIFEST_FEATURES=operation-logging `
  -DAMCORE_ENABLE_OPERATION_LOGGING=ON
```

`--fresh` resets cached options; pass any additional build options again.
For older CMake versions, use a new build directory instead. Confirm that
configuration reports `Running vcpkg install` and `Found SQLite3`.

Alternatively, from the repository root, install the manifest feature
explicitly with `vcpkg install --x-feature=operation-logging`. In manifest
mode, `vcpkg install` does not accept individual package arguments such as
`sqlite3`. The feature or CMake configuration installs the dependency declared
by `operation-logging`; use the matching vcpkg toolchain when configuring.
Installed packages discover SQLite automatically for consuming applications.
Shared SQLite runtime dependencies must be distributed separately.

The database directory is `%LOCALAPPDATA%\libamcore` on Windows,
`$XDG_DATA_HOME/libamcore` (or `~/.local/share/libamcore`) on Linux, and
`~/Library/Application Support/libamcore` on macOS. Directories are created
automatically; relative or missing Linux XDG locations fall back to HOME.

Tables `amcore_free_space_wavelength` and `amcore_far_field_distance` contain an
integer `id`, named numeric inputs (`frequency_hz`, plus `aperture_length_m` for
far-field calls), `output_is_null`, the status code `return_value`, and the
numeric result (`wavelength_m` or `distance_m`, NULL on failure). Non-finite
inputs are stored as text `NaN`, `+Infinity`, or `-Infinity`.
`executed_at_us` is the UTC start time in Unix-epoch microseconds;
`duration_ns` is monotonic elapsed time in nanoseconds. Duration excludes the
call's own write but includes nested calls and their logging.

Logging allocates memory and writes synchronously, with up to five seconds of
SQLite lock waiting per database operation. Concurrent calls use independent
connections. Logging errors go to stderr without changing API results.
Records are not buffered or retried, and there is no automatic retention.
Protect and manage these files as potentially sensitive user data.
Disabled builds have no logging or SQLite dependency.
The CLI automatically uses the library's logging setting and reports it in
general, module, and calculation help. There is no runtime toggle; CLI parse
errors and help/version requests do not invoke calculation functions and are
not recorded. See [CLI logging behavior](cli.md#desktop-operation-logging).

For embedded or cross-compiled builds, provide the relevant CMake toolchain
file and commonly disable both tests and the CLI:

```sh
cmake -S . -B build-firmware \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
  -DBUILD_TESTING=OFF \
  -DAMCORE_ENABLE_OPERATION_LOGGING=OFF \
  -DAMCORE_BUILD_CLI=OFF
```
