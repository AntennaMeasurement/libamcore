---
name: amcore-development
description: Develop and document libamcore, the portable C antenna-measurement library. Use for C API changes, calculation modules, CLI behavior, CMake builds, tests, or project documentation.
---

# AMCore Development

Use this skill when implementing or reviewing changes in this repository. For
project context, read [the LLM project context](../../../docs/public/LLM_CONTEXT.md)
and verify details against the current source.

## Workflow

1. Read the relevant public header, implementation, and tests before editing.
   Check `README.md` and `CMakeLists.txt` when public behavior or build wiring
   is involved.
2. Keep calculations portable C99, with no OS-specific dependencies or dynamic
   allocation by default.
3. For numeric APIs, validate pointers and input domains, detect non-finite or
   unrepresentable results, return the appropriate `amcore_status`, and write
   output parameters only on success.
4. Add or update focused tests for normal values, invalid inputs, range
   boundaries, and unchanged outputs on error, as appropriate.
5. Update user-facing documentation when behavior, supported modules, or build
   instructions change. Keep CLI help and build options consistent, and record
   notable features in `CHANGELOG.md`.
6. Configure, build, and test with the repository's CMake workflow:

   ```sh
   cmake -S . -B build -DBUILD_TESTING=ON
   cmake --build build --config Release
   ctest --test-dir build -C Release --output-on-failure
   ```

   For a narrow change, run the smallest relevant validation and state what
   was and was not tested.

## API and module conventions

- Public headers live in `include/amcore/`; `<amcore/amcore.h>` is the umbrella
  header and includes the module headers.
- Public functions use the `amcore_` prefix, shared declarations use
  `AMCORE_API`, and C++ consumers are supported through `extern "C"`.
- Keep units explicit in names and documentation (for example, `_hz` and `_m`).
- Return `amcore_status` and use existing status values.
- The CLI is a separate consumer of the C API. Preserve its module/subcommand
  structure, numeric output, and error-to-exit-status behavior.
- Do not describe planned antenna-measurement areas as implemented without
  code and a public interface.
