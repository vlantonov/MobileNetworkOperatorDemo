### Assumptions I'm making

1. **Build system: CMake** — the workflow in README.md explicitly references CMake. I'll target CMake 3.16+ with a `CMakeLists.txt`.
2. **C++ standard: C++17** — broadest modern support, no platform-specific features, satisfies the constraint.
3. **Call day determined by start time** — if a call starts Friday and ends Saturday, it is treated as a Friday call (no weekend discount). Standard billing practice.
4. **Free minutes expire on day 31** — "valid for 30 days since last credit" means they are zeroed out after 30 days, not rolled over.
5. **Cost precision: `double`, rounded to 2 decimal places** in the demo output.
6. **Home network prefixes are runtime-configurable** — a small change to "easy to configure" means the prefix list is passed in at construction, not hardcoded.

---

### Questions

**Q1 — Weekend discount + free minutes pool interaction (most important)**
On a weekend call to a home-network number, the first 5 minutes are free (weekend rule). Do those 5 free weekend minutes **consume from** the subscriber's 30-minute pool, or are they **independent** (pool only starts counting after minute 5)?

> Example: subscriber has 10 free minutes left, calls home network on Saturday for 8 minutes.
> - **Option A**: Weekend covers min 1–5 free; pool covers min 6–8 (3 min used from pool). Cost = 0.33.
> - **Option B**: All 8 minutes are free — weekend covers 1–5, pool covers 6–8. Cost = 0.33.
> - **Option C**: Weekend free minutes do count against the pool (first 5 consumed from pool, pool now has 5 left). Cost = 0.33.

**Q2 — Connection fee on a fully-free call**
If a call on a weekend lasts ≤ 5 minutes, is **only the connection fee charged** (0.33), or is the entire call free including the connection fee?

**Q3 — Test framework preference**
Any preference between **GoogleTest**, **Catch2**, or **doctest**? (All are cross-platform and header-friendly.) I'll default to **GoogleTest** if no preference.

---

Please answer these three questions and I'll draft the spec immediately.