Collect context and produce a clear, complete `.spec.md` file that `/design` can consume directly.

## Input
- If $ARGUMENTS points to a file, read it as initial context (partial spec, notes, investigation, etc.).
- If $ARGUMENTS is free text, treat it as an initial feature description.
- If $ARGUMENTS is empty, ask the user what feature they want to specify.
- The user may combine approaches: provide a file AND free-text clarifications.

## Phase 1: Gather Context
1. Read the initial input carefully.
2. Read the relevant `CLAUDE.md` files to understand the project:
   - `CLAUDE.md` (root) — domain model, target platforms, build system, dependency management.
   - `<module>/CLAUDE.md` for each library/module likely affected — public API conventions, internal architecture, ownership/lifetime conventions.
   - `tests/CLAUDE.md` (or per-module `tests/` dirs) — existing test conventions, test framework in use (GoogleTest/Catch2), fixtures.
3. Check `docs/plans/` for existing specs (`*.spec.md`) and plans that relate to the feature — avoid duplicating or contradicting prior decisions.
4. If the input references other files (investigations, existing code, headers, ADRs), read them.
5. Identify the relevant C++ standard (e.g. C++17/20/23) and any platform constraints (Linux/Windows/embedded, compiler set) from `CMakeLists.txt` / `CLAUDE.md` — these shape what's feasible to propose.

## Phase 2: Analyze & Identify Gaps
From the initial context, extract:
- **Goal**: what the feature should achieve and for which consumer (library user, internal module, CLI/binary, etc.).
- **Scope**: which modules/libraries/targets are likely affected (e.g. `libfoo`, `app/cli`, `tests/`).
- **Constraints**: explicit constraints or non-goals — performance budgets, memory/allocation constraints, ABI/API stability requirements, thread-safety requirements, supported compilers/standards.
- **Unknowns**: requirements that are ambiguous, missing, or could be interpreted multiple ways.

For each unknown, determine whether you can resolve it by:
1. **Reading the codebase** — if the answer is in existing headers/implementation/CMake config, look it up instead of asking.
2. **Making a reasonable assumption** — based on project conventions and domain knowledge.
3. **Asking the user** — only for genuine design decisions that cannot be inferred (e.g. API stability guarantees, performance targets, whether ABI compatibility must be preserved).

## Phase 3: Interactive Clarification
Present your understanding to the user in this structure:

### What I understand
Summarize the feature goal, scope, and any constraints — in your own words, not just repeating the input.

### Assumptions I'm making
List assumptions you've made based on project conventions or domain reasoning. For each, explain **why** you assumed it. Ask the user to confirm, correct, or expand. Flag any assumption about performance, memory ownership, or thread-safety explicitly — these are expensive to get wrong in C++.

### Questions
Ask only questions that you genuinely cannot answer from the codebase or reasonable inference. Group them by topic. Limit to the most important questions — do not overwhelm the user. Prioritize questions whose answers would change the spec significantly (e.g. "does this need to be header-only?", "is this on a hot path with a latency budget?", "must this preserve ABI compatibility for existing consumers?").

**Wait for the user to respond before proceeding.** Iterate this phase if the answers reveal new unknowns. Keep rounds to a minimum — aim for 1-2 rounds max.

## Phase 4: Draft the Spec
Once you have enough clarity, produce the `.spec.md` file. Follow this structure:

```markdown
# {Feature Name}

## Goal
One-paragraph description of what this feature achieves and why it matters.

## Scope
Which modules/libraries/targets are affected (e.g. `libfoo`, `app/cli`, `bindings/`). Briefly state what each needs to do.

## Requirements
Numbered list of concrete, testable requirements. Each requirement should be specific enough that `/design` can produce an implementation plan without further questions.

## Non-Functional Requirements
- Performance/latency/throughput budgets, if any.
- Memory/allocation constraints (e.g. no heap allocation on the hot path, must work in a no-exceptions build).
- Thread-safety requirements (is this type/function safe to call concurrently? from which threads?).
- Supported C++ standard, compilers, and platforms.

## Test Scenarios
List concrete scenarios that exercise the feature, to be implemented as unit and/or integration tests:
- Format: "Given [state], when [action], then [expected outcome]".
- Cover the happy path, boundary/edge cases (empty input, max size, null/optional handling), error conditions (exceptions, error codes, invalid input), and concurrency cases if thread-safety is in scope.
- Reference existing test patterns from `tests/CLAUDE.md` where applicable.

## API Sketch (if introducing or changing a public interface)
Rough signatures of new/changed public functions, classes, or templates — enough to anchor the design phase, not a full declaration.

## Constraints
- Explicit constraints, non-goals, or things intentionally left out.
- Technical constraints (e.g., must not break existing ABI, must not introduce new third-party dependencies, must support both GCC and Clang).

## Assumptions
- Key assumptions made during clarification, with brief rationale.
- These help `/design` understand the reasoning behind requirements.

## Open Questions (if any)
- Questions that were deliberately deferred or need input from others.
- Mark these clearly so `/design` knows to flag them.
```

Keep the spec concise and actionable. Every requirement should inform a design decision. Avoid vague language like "should be fast" or "should be safe" — translate intent into concrete, testable behavior (e.g. "must complete in O(log n)", "must not throw; returns `std::expected<T, Error>` on failure"). Ensure the Test Scenarios section contains enough detail for `/design` to produce concrete unit/integration test cases.

## Phase 5: Review & Save
1. Present the draft spec to the user.
2. Ask: "Does this spec capture everything? Any changes before I save it?"
3. After explicit approval, save to `docs/plans/{feature-name}.spec.md`.
   - If the user specified a different output path, use that instead.
   - Use kebab-case for the filename, derived from the feature name.
4. Tell the user they can now run `/design docs/plans/{feature-name}.spec.md` to produce the implementation plan.