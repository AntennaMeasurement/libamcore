---
name: amcore-development
description: "Develop and document libamcore, the portable C antenna-measurement library. Use for C API changes, computational modules, CLI behavior, CMake builds, tests, or project documentation."
---

# AMCore Development

Use this skill when implementing or reviewing changes in this repository. For
the current architecture, implemented features, build options, and historical
project intent, read [the LLM project context](../../../doc/LLM_CONTEXT.md).
Confirm details against the current source; the context document is a guide,
not a substitute for code.

## Workflow

1. Read the relevant public header, implementation, and tests before editing.
   Check `README.md` and `CMakeLists.txt` when public behavior or build wiring
   is involved.
2. Keep the library portable C99, with no OS-specific dependencies or dynamic
   allocation in computational APIs. Follow the existing module naming and
   public-header patterns.
3. For numeric APIs, validate pointers and input domains, detect non-finite or
   unrepresentable results, return the appropriate `amcore_status`, and write
   output parameters only on success. Avoid overflow-prone intermediate
   calculations.
4. Add or update focused tests for normal values, invalid inputs, range
   boundaries, and unchanged outputs on error, as appropriate.
5. Update `README.md` and `doc/LLM_CONTEXT.md` when user-visible behavior,
   supported modules, or build instructions change.
6. Configure, build, and run tests using the repository's CMake workflow:

   ```sh
   cmake -S . -B build -DBUILD_TESTING=ON
   cmake --build build --config Release
   ctest --test-dir build -C Release --output-on-failure
   ```

   If only a narrow surface changed, run the smallest relevant validation and
   state clearly what was and was not tested.

## API and Module Conventions

- Public headers live in `include/amcore/`; `<amcore/amcore.h>` is the umbrella
  header and includes the module headers.
- Public functions use the `amcore_` prefix, shared public declarations use
  `AMCORE_API`, and C++ consumers are supported through `extern "C"`.
- Keep units explicit in names and documentation (for example, `_hz` and
  `_m`).
- Return `amcore_status`; use the existing status values rather than inventing
  undocumented error handling.
- The CLI is a separate consumer of the C API. Preserve its module/subcommand
  structure, numeric output and error-to-exit-status behavior.
- Do not describe a planned antenna-measurement area as implemented unless
  there is code and a public interface for it.
