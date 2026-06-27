Design a feature implementation and produce a step-by-step execution plan that `/implement` can consume directly.

## Input
- If $ARGUMENTS points to an .md file, read it as the feature specification.
- Otherwise treat $ARGUMENTS as a brief feature description.
- If neither is provided, ask the user what feature to design.

## Phase 1: Understand
1. Read the feature spec/description carefully.
2. Read the relevant `CLAUDE.md` files — these define conventions and current architecture:
   - `CLAUDE.md` (root) — project structure, target list, C++ standard, dependency management (vendored/Conan/vcpkg/FetchContent), build/toolchain conventions.
   - `<module>/CLAUDE.md` for each affected module/library — public API design (headers, namespaces), internal architecture, ownership/lifetime conventions (raw vs. smart pointers, RAII patterns), error-handling style (exceptions vs. error codes vs. `std::expected`).
   - `tests/CLAUDE.md` — test framework (GoogleTest/Catch2), fixture conventions, naming, how unit vs. integration tests are organized and run.
3. Check `docs/plans/` for existing feature specs (`*.spec.md`) and prior plans that may be relevant.
4. **Use parallel `Explore` agents** to analyze the modules affected by the feature — one agent per module. Each agent should investigate the current state of that module relevant to the feature (existing classes, headers, CMake targets, dependency graph between targets). Run them simultaneously to save time.
5. If the feature extends existing functionality, deep-trace the existing code path (call graph, header dependencies, which translation units would need to recompile) before proposing changes.
6. Use dedicated doc tools when the feature involves library/standard APIs: a C++ reference lookup tool/MCP if available, or web search for current cppreference/Boost/library documentation when working with an unfamiliar or recently updated API.
7. Codebase is the source of truth — if `CLAUDE.md` and code disagree, trust the code.
8. Identify which modules/libraries/targets are affected. Only include targets that actually need changes. New/changed public headers ripple to every consumer of that target — flag this explicitly.

## Phase 2: Clarify
- If requirements are ambiguous or incomplete, ask the user targeted questions before proceeding.
- Do NOT guess or assume design decisions with long-lived consequences — ask. This especially includes: API/ABI stability guarantees, ownership model for new types, exception vs. error-code policy, and whether a change may break existing consumers.

## Phase 3: Design
Produce an architecture sketch before writing the step-by-step plan: what new/changed types, free functions, and CMake targets are needed, and how they fit into the existing dependency graph between modules.

Produce a design for each affected module. Skip modules that don't need changes.

### Per-Module Design (for each affected module/library)
- New/modified classes, functions, templates — public API surface vs. internal (`detail`/`impl` namespace or `.cpp`-only) surface.
- Header placement: does this belong in a public include directory or stay implementation-private?
- Memory/ownership model: who owns what, value vs. reference semantics, smart pointer choice (`unique_ptr` default, `shared_ptr` only with a stated reason).
- Error handling: exceptions, error codes, `std::optional`/`std::expected`/`std::variant` — follow the module's existing convention; flag if this feature needs to deviate and why.
- Concurrency/thread-safety: is the new code called from multiple threads? What needs synchronization, and what primitive (mutex, atomic, lock-free) fits the existing style?
- Template vs. non-template: prefer a concrete implementation unless genericity is an explicit requirement — templates increase compile time and error-message complexity.

### Build System Changes (if needed)
- New/modified CMake targets, `target_sources`/`target_link_libraries` changes.
- New third-party dependencies — how they're fetched (FetchContent/Conan/vcpkg/system package) and why this one was chosen over alternatives already vendored in the project.
- Any change to the public include path or exported target properties (`INTERFACE`/`PUBLIC`/`PRIVATE` scoping) — these affect every downstream consumer.

### Tests (always, for any module with logic changes)
Design test scenarios for the feature. Reference the spec's "Test Scenarios" section and translate them into concrete unit/integration tests:
- List each test case with: test name (following the project's GoogleTest/Catch2 naming convention), fixture (if any), inputs, and expected assertions.
- Distinguish **unit tests** (single class/function, dependencies mocked/faked) from **integration tests** (multiple components wired together, real I/O or filesystem where relevant) — both run locally.
- Identify new test fixtures, test doubles/mocks, or test data files needed.
- Cover boundary conditions explicitly: empty/zero-sized input, max-size input, move-from state, self-assignment (if relevant to a new type), exception-safety (does a throwing constructor/operation leave objects in a valid state?).

### Contracts (when the feature crosses a module boundary)
Define the interface contract between affected modules:

```
Header:    <module>/include/<module>/<name>.hpp
Signature: ReturnType FunctionOrMethod(ParamType param, ...);
Preconditions:  what the caller must guarantee
Postconditions: what holds after a successful call
Error behavior: what happens on invalid input (throws? returns error/optional? UB if precondition violated?)
Thread-safety:  safe to call concurrently? from which threads?
```

## Phase 4: Execution Plan
Create a numbered, ordered plan grouped by module/target. For each step specify:
- **What**: file(s) to create or modify (headers and corresponding `.cpp` files together).
- **Why**: what this step achieves.
- **Details**: key implementation notes (not full code, just enough to be unambiguous) — signature, ownership, error behavior.

Group steps so each module's implementation and its tests are adjacent (matching how `/implement` executes), and order modules by dependency — a module must be designed/implemented before modules that depend on it:

1. **Foundational/leaf modules first** (no dependency on other changed modules)
   - Headers/interfaces → unit tests → implementation → CMake target wiring
2. **Dependent modules** (consume the modules above)
   - Headers/interfaces → unit tests → implementation → CMake target wiring
3. **Integration tests** (exercise multiple modules together)
   - Test fixtures/data → integration test cases covering cross-module interaction
4. **CLI/app-level wiring** (if the feature is user-facing through a binary)
   - Argument parsing, wiring new functionality into `main`/command dispatch

## Phase 5: Self-Review
Before presenting the plan, validate design-level concerns:
- [ ] Every requirement from the spec is addressed.
- [ ] No unnecessary changes — each step is justified by a requirement.
- [ ] No over-engineering — simplest approach that meets requirements (no premature template genericity, no unnecessary abstraction layers).
- [ ] API contracts (preconditions/postconditions/error behavior) are stated for every new/changed public function.
- [ ] Ownership and lifetime are unambiguous for every new type — no implicit assumptions about who calls `delete` or how long a reference is valid.
- [ ] No unnecessary new third-party dependencies; if one is introduced, it's justified.
- [ ] ABI-breaking changes (changing a class layout, adding a virtual function, changing a function signature in a public header) are explicitly flagged.
- [ ] The plan follows existing conventions from `CLAUDE.md` files — no pattern reinvention (e.g. don't introduce exceptions into a module that consistently uses error codes).
- [ ] Test scenarios cover the spec's Test Scenarios section, including boundary and error cases.
- [ ] Test design follows `tests/CLAUDE.md` conventions (framework, fixture style, naming).

If any check fails, revise the plan before presenting it.

## Output
1. Present the plan to the user for review.
2. **Wait for explicit approval** before saving. Ask: "Does this plan look good? Any changes before I save it?"
3. After approval, save to `docs/plans/{feature-name}.md` so `/implement` can reference it by file path.
4. For large features (multiple modules), remind the user they can implement one module at a time: `/implement docs/plans/feature.md --module libfoo`.