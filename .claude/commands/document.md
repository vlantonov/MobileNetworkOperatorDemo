Review the codebase and update project documentation to match the current state of the code.

Codebase is the source of truth. Documentation follows code, not the other way around.

## Input
- If $ARGUMENTS specifies a scope (e.g., a module name, `tests`, `all`), focus on that area.
- If $ARGUMENTS is empty, review all areas.

## Phase 1: Parallel Analysis
Launch **parallel `Explore` agents** — one per module/area — to compare documentation against code:

1. **Per-module agent** (one per library/module with a `CLAUDE.md`): Compare `<module>/CLAUDE.md` against actual code in that module. Check:
   - Public API Catalog — are all public headers, classes, and free functions listed? Any new/removed/changed signatures?
   - **API contract accuracy** — for each public function, verify documented behavior matches actual behavior by tracing the call path:
     1. Read the function body to identify what it can throw, what error values/codes it can return, and under what conditions.
     2. Trace any functions it calls internally that can themselves throw or fail — exceptions propagate unless caught, and undocumented propagation is a common drift source.
     3. Check documented preconditions against actual behavior — does the function assert/validate them, or silently assume them (UB on violation)?
     4. Document ALL possible outcomes, not just the happy path. Phrases like "never throws" or "always returns a valid value" must be verified — if any code path can throw or return an error, document it. If `noexcept` is declared, confirm the implementation actually can't throw.
   - Ownership/lifetime conventions — do documented patterns (who owns what, smart pointer usage) match actual code?
   - Architecture/internal structure — does the description of how the module is organized match the actual file/class layout?
   - Test patterns — do documented conventions match actual tests in this module's `tests/`?
   - Dependencies — any new internal or third-party dependencies not mentioned?

2. **Tests agent:** Compare `tests/CLAUDE.md` against actual code in `tests/`. Check:
   - Test inventory — any new test files/suites not reflected in the structure?
   - Framework and conventions — still using the documented test framework (GoogleTest/Catch2) and naming/fixture conventions?
   - Test categorization — does the documented split between unit and integration tests match how tests are actually organized and run?
   - Test data/fixtures — any new shared fixtures or test data files not documented?

3. **Root agent:** Compare root `CLAUDE.md` against the full project. Check:
   - Module/Domain Model section — are all modules/libraries listed with correct dependency relationships?
   - Project Structure — does the tree match actual directories and `CMakeLists.txt` targets?
   - Build & Toolchain — C++ standard, supported compilers, dependency management method, and build instructions all current?
   - Build configurations — are documented build types/flags (Debug/Release, sanitizer builds) still accurate?

## Phase 2: Drift Report
Compile agent findings into a drift report:

For each documentation file, list:
- **Accurate:** sections that match the code (no action needed).
- **Stale:** sections where code has changed but docs haven't (needs update).
- **Missing:** new code patterns/features with no documentation (needs addition).
- **Orphaned:** documentation for code that no longer exists (needs removal).

Present the drift report to the user before making changes.

## Phase 3: Update
After user reviews the drift report:
- Update stale sections to match current code.
- Add missing sections following the existing documentation style.
- Remove orphaned sections.
- Do NOT add speculative documentation — only document what exists in code.
- Do NOT change documentation structure unless necessary to accommodate new content.
- Keep documentation concise — reference code rather than duplicating it (e.g. point to the header rather than restating every member).

## Rules
- Never generate standalone README or documentation files unless explicitly requested.
- Update existing `CLAUDE.md` files in place.
- If a `CLAUDE.md` file doesn't exist for a module that needs one, ask the user before creating it.
- Do not document implementation details that are obvious from reading the code — focus on conventions, contracts, and decisions that aren't self-evident (ownership, thread-safety, error-handling policy).
- **API documentation must reflect the full behavior, not just the obvious return path.** When documenting a public function's contract, trace what it calls internally and what those calls can throw or fail with. If an internal call can propagate an exception or error that isn't caught/converted, that must be documented as part of the function's contract. Never write "never throws" or "always succeeds" without verifying that no code path in the call chain can produce a different outcome.
- Where the project uses Doxygen-style comments (`///`, `/** */`) as the primary API documentation mechanism, prefer updating those in the header over duplicating the same information in `CLAUDE.md` — `CLAUDE.md` should describe conventions and architecture, not restate every function's docstring.