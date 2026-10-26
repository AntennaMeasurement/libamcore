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

## Install

Install the library, public headers, CLI (when enabled), and CMake package:

```sh
cmake --install build --prefix install
```

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

## Publish versioned documentation with Read the Docs

The repository includes `.readthedocs.yaml`.
Import the repository in Read the Docs and enable Git integration to build and
publish documentation from the repository.

To make earlier releases available in the Read the Docs version selector,
activate their Git tags in the project's **Versions** settings and leave them
visible. The release workflow creates version tags such as `v0.5.0`; Read the
Docs builds and publishes each active tag. Its version selector is provided
when viewing the site on Read the Docs. The overview page displays the library
version from the API manifest for each build.

## WebAssembly

See [the WebAssembly guide](webassembly.md) for the optional standalone module.
