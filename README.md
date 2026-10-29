<!-- Last content update: Unreleased (latest tag: v0.2.0) -->
# libamcore

`libamcore` is a portable C library for core antenna-measurement calculations.
Project contains the calculation API and the `amcore` command-line tool.

The public API is available through `<amcore/amcore.h>`. It uses C99, supports
C++ consumers, and does not require nonstandard runtime dependencies. CMake
links the system math library where it is provided separately (such as `libm`
on Linux).

## Calculations

| Module | Function | Description |
| --- | --- | --- |
| `utility` | `amcore_free_space_wavelength_m` | Computes free-space wavelength from frequency. |
| `farfield` | `amcore_far_field_distance` | Computes minimum far-field distance from frequency and aperture length. |
| `farfield` | `amcore_far_field_line_of_sight_km` | Estimates line-of-sight range from height. |
| `farfield` | `amcore_far_field_radio_horizon_km` | Estimates radio horizon from height. |
| `farfield` | `amcore_far_field_path_loss_db` | Computes free-space path loss from frequency, range, and antenna gains. |
| `nearfield` | `amcore_near_field_separation_distance_m` | Computes the recommended near-field measurement separation. |
| `nearfield` | `amcore_near_field_planar_sampling_spacing_m` | Computes the maximum planar sampling spacing. |
| `nearfield` | `amcore_near_field_planar_scan_length_m` | Computes planar scan length for a desired angle of view. |
| `nearfield` | `amcore_near_field_planar_angle_of_view_deg` | Computes planar angle of view for a centered scan. |
| `nearfield` | `amcore_near_field_planar_sampling_parameters` | Computes centered planar scan endpoints, sample count, and spacing. |
| `nearfield` | `amcore_near_field_cylindrical_y_scan_length_m` | Computes cylindrical Y scan length. |
| `nearfield` | `amcore_near_field_spherical_maximum_radius_extent_m` | Computes spherical maximum radius extent. |

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

Running `amcore_tests` directly prints a PASS/FAIL list and a final summary.
All tests run even if one fails; any failure produces a nonzero exit status.
Use CTest's `--verbose` option to also see this list for successful runs.

## Releases

Releases are published explicitly with Bash (including Git Bash on Windows),
Git, and an authenticated [GitHub CLI](https://cli.github.com/):

1. Update the project version in `CMakeLists.txt`. Optionally add release notes
   under `## [Unreleased]` in `CHANGELOG.md`.
2. Commit all changes and check out the branch to publish. The working tree
   must be clean, including untracked files.
3. Preview, then publish:

   ```sh
   bash scripts/release.sh --dry-run
   bash scripts/release.sh
   ```

The script uses the CMake project version as the tag (for example, `v0.7.4`).
It preserves an existing changelog entry for that version. Otherwise, it moves
`Unreleased` notes into a dated release entry, leaving `Unreleased` empty.
If there are no notes, it uses non-merge commit subjects since the nearest
reachable version tag (or all commits for the first release).

Before tagging, it commits the changelog update if needed. It then atomically
pushes the current branch and annotated tag to `origin` and creates a GitHub
release using that entry as the release notes. The CI release workflow then
builds and uploads the native binaries and the standalone `amcore.wasm` library.
Existing tags are rejected; changing the CMake version alone does
not publish anything. Dry runs require no GitHub authentication or network
access and do not modify the repository.

If publishing fails, the script reports the error and retains local commits
and tags. After fixing the cause, push them manually if necessary and create
the GitHub release using the matching changelog entry; rerunning the script
will reject the existing tag.

Run the release-script integration tests with `bash tests/test_release.sh`.
They use temporary local Git repositories and a stub GitHub CLI; no GitHub
releases are published.

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

The output `amcore.wasm` exports the library's calculation functions, not the
CLI. GitHub releases include this module alongside the native binaries.
See the [WebAssembly documentation](docs/public/webassembly.md) for runtime
requirements and the API smoke test.

## Install and consume

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build
```

On Windows, the default install location is
`%LOCALAPPDATA%\Programs\libamcore`, so installation does not require
administrator permissions. You can still choose a different location when
configuring with `-DCMAKE_INSTALL_PREFIX`.

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

The documentation reads the library version from the `project()` declaration
in `CMakeLists.txt`.

The generated site is written to `site/`. To preview it locally while editing:

```sh
python -m mkdocs serve
```

Then open the local URL printed by MkDocs. See [build and install](docs/public/getting-started.md),
[the C API](docs/public/api.md), [the CLI](docs/public/cli.md),
[CMake integration](docs/public/cmake-integration.md), and
[WebAssembly](docs/public/webassembly.md).

## References

[1] [IEEE Std 149-2021](https://standards.ieee.org/standard/149-2021.html) - IEEE Recommended Practice for Antenna Measurements

[2] [Line-of-sight propagation](https://en.wikipedia.org/wiki/Line-of-sight_propagation) - Approximate line-of-sight range and radio horizon formulas

[3] [IEEE Std 1720-2012](https://standards.ieee.org/standard/1720-2012.html) - IEEE Recommended Practice for Near-Field Antenna Measurements

[4] Warren L. Stutzman and Gary A. Thiele, *Antenna Theory and Design*, 3rd edition

[5] NSI 2000 (Near-field Edition), Version 4, Software Operating Manual