Implement a feature according to the provided plan.

Prerequisite: the plan should already define scope (which modules), contracts (signatures, preconditions/postconditions, error behavior), steps, and test design. If the plan is missing these, stop and ask the user to run `/design` first.

## Input
- If $ARGUMENTS points to a file, read it as the implementation plan.
- If $ARGUMENTS contains `--module <name>`, implement only that module from the plan. This is useful for large features or when continuing across conversations.
- Otherwise treat $ARGUMENTS as a reference to a plan discussed in conversation.
- If no plan is identifiable, stop and ask the user.

## Before You Start
1. Read the plan and identify affected modules/targets, in dependency order (leaf modules first).
2. Read the relevant `CLAUDE.md` files for each affected module — these are the source of truth for conventions:
   - `CLAUDE.md` (root) — build system, C++ standard, compiler flags, dependency management.
   - `<module>/CLAUDE.md` — architecture, ownership conventions, error-handling style.
   - `tests/CLAUDE.md` — test framework (GoogleTest/Catch2), fixtures, naming, how to run unit vs. integration tests.
3. Validate that the plan includes:
   - Contracts (signatures, preconditions/postconditions, error behavior) — required for any new/changed public function. If missing, define them before writing implementation code.
   - Test design (test cases, fixtures, boundary conditions) — required for any module with logic changes. If missing, stop and ask the user to run `/design` to add it.

## Implementation Loop
Execute one module at a time, in the dependency order given by the plan (leaf modules before the modules that depend on them). Skip modules not in scope.

Follow the red/green/refactor loop below for every module.

### Step 1: Write tests first
- Write tests (GoogleTest/Catch2, matching the project's existing convention) that verify the contracts and expected behavior from the plan, including boundary/error cases.
- Build the test target and run it — tests MUST fail or fail to compile against the not-yet-implemented interface (red phase). If they pass, the test is not testing anything new.

### Step 2: Implement
- Write the minimum code to make tests pass.
- Follow conventions from the module's `CLAUDE.md` — do not reinvent patterns (ownership model, error-handling style, naming).
- Default to value semantics and RAII; reach for `unique_ptr` before `shared_ptr`, and `shared_ptr` only with a stated reason. Avoid raw `new`/`delete` outside of low-level allocator code.
- Keep the public header surface minimal — implementation details that don't need to be public belong in a `detail`/`impl` namespace or in the `.cpp` file, not in the public header.
- When using unfamiliar or recently updated standard library or third-party APIs, check current documentation (cppreference, the library's own docs) rather than relying on possibly-stale knowledge of the API.
- After each significant change:
  1. Predict which tests should now pass or break.
  2. Rebuild and run tests.
  3. Compare predicted vs actual results.
  4. If mismatch: find root cause and fix before moving on.

### Step 3: Module checkpoint
- All new tests for this module pass.
- No previously passing tests are broken (rebuild and run the full test suite for modules that depend on this one too, since header/ABI changes can ripple).
- Review the module's changes for: correctness against the plan's contracts, exception-safety (does any operation leave an object in a half-constructed/half-destroyed state on throw?), resource leaks (every acquire has a matching release, ideally via RAII rather than explicit cleanup), and unnecessary copies (missing `std::move`, pass-by-value where a reference would do, missing `const`).
- If issues are found, fix them before proceeding.
- Inform the user of progress before moving to the next module.

### Build & Compiler Hygiene
- Build with warnings enabled as the project's `CLAUDE.md` specifies (typically at least `-Wall -Wextra`, or `/W4` on MSVC); treat new warnings in changed files as build-blocking.
- Do not silence a warning (`#pragma GCC diagnostic ignored`, `// NOLINT`, `[[maybe_unused]]` used to hide an actually-unused variable) without explicit user approval — fix the root cause first.
- If the project enables sanitizers (ASan/UBSan/TSan) for local test runs, run the test suite under them for any module with new pointer arithmetic, manual memory management, or concurrency.
- If the project uses `clang-tidy`/`clang-format`, run them on changed files before considering a module done; follow existing suppressions only where the project's `CLAUDE.md` already documents that exception.

### Concurrency-specific (if the module involves multithreading)
- Identify shared mutable state and confirm it's protected by the synchronization primitive the plan specifies.
- Write a test that exercises the concurrent path (e.g. multiple threads calling the new function), not just the single-threaded happy path — even a basic stress test catches obvious races.
- If TSan is available locally, run the new tests under it before marking the module done.

### CMake-specific
- Add new source files to the correct target via `target_sources`, and new dependencies via `target_link_libraries` with the correct visibility (`PUBLIC`/`PRIVATE`/`INTERFACE`) — getting this wrong silently leaks dependencies to consumers or breaks the build for them.
- If a new third-party dependency was introduced in the plan, wire it via the project's existing convention (FetchContent/Conan/vcpkg/system find_package) — don't introduce a second dependency-management mechanism.
- After CMake changes, reconfigure and do a full rebuild (not just incremental) at least once to catch stale-cache or generator issues.

## Cross-Module Wiring
When multiple modules are involved, ensure they connect correctly:
- **Header dependencies:** a module only `#include`s the public headers of modules it's allowed to depend on per the project's dependency graph — no circular dependencies between targets.
- **ABI/API consistency:** if a public signature changed, every caller in the codebase (not just the ones in the plan) is updated; grep for the old signature before declaring a module done.
- **Error handling:** if module A returns `std::expected`/error codes and module B expects exceptions (or vice versa), the boundary between them must explicitly convert — don't let the inconsistency leak through.
- **Build graph:** the dependency order in `CMakeLists.txt`/`target_link_libraries` matches the actual `#include` dependency order — no module links against something that creates a cycle.

## When Done
Inform the user that implementation is complete and suggest running `/verify` with the spec file for full traceability:
```
/verify docs/plans/{feature-name}.spec.md
```