Verify that a recently implemented feature is correct, safe, and documented.

Run this after `/implement` completes, or anytime you want to validate the current state of the codebase.

## Input
- If $ARGUMENTS points to a `.spec.md` file, read it as the feature spec. Also look for a matching plan file in the same directory (same name without `.spec` suffix) — read both. The spec provides requirements and test scenarios for traceability; the plan provides the implementation design.
- If $ARGUMENTS points to a plan `.md` file (non-spec), read it. Also look for a matching `.spec.md` — read both if found.
- If $ARGUMENTS is free text, treat it as a feature name/description. Search `docs/plans/` for a matching spec and plan.
- If $ARGUMENTS is empty, look at recent git changes (`git diff`, `git status`) to determine what was changed. Search `docs/plans/` for the most recently modified spec that matches the changed files. If no spec is found, skip requirements traceability checks (Phase 5 items that require the spec) and note this in the output.

## Phase 1: Build Verification
Configure and build with warnings treated as errors, for every configuration the project's `CLAUDE.md` specifies (e.g. Debug and Release, or multiple compilers if the project supports both GCC and Clang).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror"
cmake --build build
```
(Adjust flags to the project's actual `CLAUDE.md`/`CMakeLists.txt` convention — e.g. `/W4 /WX` on MSVC.)

- [ ] Project builds with zero warnings and zero errors, in every configuration the project supports locally.
- [ ] No new warning suppressions (`#pragma ... diagnostic ignored`, `// NOLINT`, blanket `[[maybe_unused]]`) added without explicit user approval.
- [ ] No new compiler/CMake minimum-version bump without explicit user approval, if one was introduced.

If warnings exist: report each warning with file, line number, and message. Do NOT suppress them — fix the root cause. If a warning cannot be fixed (e.g., third-party header issue), report it and ask the user for explicit approval before suppressing.

## Phase 2: Dead Code & Unused Dependencies
Scan for dead code and unused dependencies in changed and related files:

- [ ] No unused `#include` directives in changed files (check with `clang-tidy`'s `misc-include-cleaner` or equivalent if configured, otherwise inspect manually).
- [ ] No unreferenced classes, functions, or templates introduced by the changes (declared but never called/instantiated outside tests).
- [ ] No new third-party dependencies added to `CMakeLists.txt`/`conanfile`/`vcpkg.json` that are not actually used in application code.
- [ ] No orphaned forward declarations or stale `friend` declarations left over from refactoring.

If dead code or unused dependencies are found: report each item. Do NOT remove them automatically — present the findings and let the user decide (some may be intentionally kept for future use).

## Phase 3: Test Suite
Run ALL tests across all affected modules, locally:
```
ctest --test-dir build --output-on-failure
```
(Or the project's direct test-binary invocation if it doesn't use CTest.)

- [ ] All unit and integration tests pass — zero failures.
- [ ] No skipped or disabled tests (`DISABLED_*` in GoogleTest, `[!hide]`/`[!shouldfail]` in Catch2) that should be active.
- [ ] Test coverage exists for new logic — every new public function/class has at least one test exercising it.
- [ ] Boundary and error-path cases from the spec's Test Scenarios are covered, not just the happy path.

### Sanitizer & Static Analysis Pass (if the project supports it locally)
- [ ] Test suite passes under AddressSanitizer/UndefinedBehaviorSanitizer for any module with manual memory management, pointer arithmetic, or new unsafe casts.
- [ ] Test suite passes under ThreadSanitizer for any module with new concurrency.
- [ ] `clang-tidy` (or the project's configured static analyzer) reports no new findings on changed files beyond what the project's baseline already accepts.

If tests fail: report the failures with root cause analysis. Do NOT fix them — that is `/implement`'s job. If fixes are needed, include them in the verdict.

## Phase 4: Parallel Review
Launch the following review passes **in parallel** — they are independent and read-only. Use the project's configured review agents/plugins if available; otherwise perform each pass directly:

1. **Code review** — check all changed files against project conventions in `CLAUDE.md` files (naming, ownership style, error-handling style).
2. **Resource/memory-safety review** — find resource leaks (acquire without RAII-guaranteed release), use-after-move, use-after-free risk, dangling references/iterators, missing `const`, unnecessary copies.
3. **Exception-safety review** — for any function that can throw, confirm it leaves objects in a valid state on the throwing path (basic guarantee at minimum; note where strong guarantee is expected but not met).
4. **Test coverage analysis** — evaluate whether new tests actually exercise the contracts in the plan, or just restate the implementation; identify critical gaps.
5. **API/type design review** — review any new public types/functions for proper encapsulation, minimal public surface, and consistent ownership semantics with the rest of the module.
6. **Comment accuracy review** — verify that new/changed comments (especially Doxygen-style `///` or `/** */` API docs) are accurate and not stale relative to the code they describe.

Collect all findings before proceeding.

## Phase 5: Requirements Check
Compare the implementation against the plan/requirements:
- [ ] Every requirement is addressed — nothing was skipped.
- [ ] No extra scope — nothing was added beyond what was requested.
- [ ] Contracts match what was implemented (signatures, preconditions/postconditions, error behavior) for every affected public function.
- [ ] Every scenario from the spec's "Test Scenarios" section has a corresponding test.

## Phase 6: Safety & Security Review
Verify the following checklist, adapted to what the feature touches:
- [ ] No undefined behavior introduced: no signed integer overflow on attacker/user-controlled input, no out-of-bounds access, no use of uninitialized memory.
- [ ] All array/container indexing that comes from external/user input is bounds-checked (`.at()` or explicit check) rather than unchecked `operator[]`.
- [ ] No raw `new`/`delete` pairs that could be replaced by RAII; if present, ownership and exception-safety are explicitly justified.
- [ ] Input validation at module/API boundaries — a public function does not trust its caller to have validated arguments unless documented as a precondition (and violating a documented precondition is intentionally UB, not silently exploitable).
- [ ] No sensitive data (keys, credentials, paths with PII) written to logs or left in debug output.
- [ ] If parsing untrusted/external input (files, network, user-provided strings), parsing is fuzz-test-friendly and does not assume well-formed input.
- [ ] Concurrency: no new data races (revisit Phase 3's sanitizer results here if TSan was run).

## Phase 7: Documentation Review
Check whether documentation needs updating — **report** what is stale or missing, but do NOT update docs directly. Documentation changes are a separate concern and should be committed intentionally (via `/document` or manually), not bundled into an implementation commit.

Check the following for drift:
- [ ] `CLAUDE.md` (root) — stale if project-wide structure, build instructions, or supported platforms changed.
- [ ] `<module>/CLAUDE.md` for each affected module — stale if new public API, architecture, or conventions were added.
- [ ] `tests/CLAUDE.md` — stale if new test patterns, fixtures, or conventions were introduced.
- [ ] Public header API docs (Doxygen comments) — stale or missing for new/changed public functions.
- [ ] `README.md` build/usage instructions — stale if new dependencies or build steps were introduced.
- Do NOT check documentation unrelated to the changes.

## Output
Present a summary:
- **Build:** clean (zero warnings, zero errors, all configurations) or list of warnings/errors per configuration.
- **Dead code / unused deps:** clean or list of findings.
- **Tests:** pass/fail count (unit + integration); sanitizer/static-analysis results if run.
- **Code review:** issues from each review pass (grouped by severity).
- **Requirements:** covered / gaps found.
- **Safety/security:** issues found or all clear.
- **Docs:** stale/missing sections found (if any) — suggest `/document` to fix.
- **Verdict:** ready to commit, or list of items to fix first (suggest re-running `/implement` to address them).