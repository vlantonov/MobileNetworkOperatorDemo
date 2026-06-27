# Mobile Network Operator Billing

## Goal

Provide a portable C++17 library that calculates the cost of a phone call given the call's start/end datetime, the dialed number, and the subscriber's account state. Accompany the library with a demo application exercising representative scenarios and a full unit-test suite. Everything must build with CMake on any standard platform without platform-specific headers or compiler extensions.

---

## Scope

| Target | Responsibility |
|---|---|
| `lib/billing` | Core calculation library — all pricing logic, home-network detection, free-minutes pool logic |
| `app/demo` | CLI demo: runs a fixed set of hard-coded scenarios and prints itemised results |
| `tests/` | doctest unit tests covering every billing rule and edge case |
| `CMakeLists.txt` (root + sub) | Cross-platform build; fetches doctest via CMake FetchContent |

---

## Requirements

**Home network detection**
1. A called number belongs to the home network if it starts with one of the configured prefixes. The prefix list is provided at construction time and must be runtime-configurable (not hardcoded). Default prefixes: `{"050", "066", "095", "099"}`.

**Call duration (billed minutes)**
2. Billed minutes = ⌈seconds / 60⌉ (ceiling). A call of 0 seconds costs 0.00 (no connection fee, no minute fee).
3. The call day is determined by the start timestamp; a call that crosses midnight uses the weekday of the start.

**Weekend discount**
4. If the call starts on a Saturday or Sunday, the first 5 billed minutes are free (zero cost, no pool consumption).
5. If the entire call is covered by the weekend discount (billed minutes ≤ 5 on a Saturday or Sunday), the call costs **0.00** — the connection fee is also waived.

**Free-minutes pool**
6. Each subscriber account holds a free-minutes balance (0–30) and a `last_credit_date`. The pool is valid for exactly 30 days from `last_credit_date` (expires at the start of day 31; still valid on day 30).
7. The pool applies **only** to home-network calls and **only** to minutes that are not already covered by the weekend discount (pool consumption begins after the 5 weekend-free minutes).
8. Free-pool minutes do not incur the per-minute fee; they do not reduce the cost below the connection fee (except as per Req 5).
9. The `SubscriberAccount` struct/class must expose the current pool balance after a call is processed.

**Minute rates**
10. Home-network minutes beyond the free pool: **0.50 / min**.
11. External-network minutes: **0.95 / min** (weekend discount applies; pool does NOT apply to external numbers).

**Connection fee**
12. A connection fee of **0.33** is added to every call that has a non-zero total cost (i.e., at least one chargeable minute exists, or at least one pool minute is consumed but the connection fee is still charged). Exception: Req 5 (fully-free weekend call → no connection fee).
    - Clarification: the connection fee is charged whenever the call is not entirely free. A call is entirely free only under Req 5. Weekday calls where all minutes are covered by the free pool still incur the connection fee.

**Output**
13. The calculation function returns a `CallResult` containing: `total_cost` (`double`), `billed_minutes` (`int`), `pool_minutes_used` (`int`), and `weekend_free_minutes` (`int`).
14. `total_cost` is computed as a `double`; rounding to 2 decimal places is the demo's responsibility, not the library's.

---

## Non-Functional Requirements

- **C++ standard**: C++17. No platform-specific headers (`<windows.h>`, POSIX-only APIs). Use `<chrono>` and `<ctime>` for date/time only if portable; prefer a thin custom `DateTime` struct to keep the interface dependency-free.
- **Compilers**: GCC ≥ 9, Clang ≥ 10, MSVC 2019+.
- **No exceptions required**, but the library may use them for programmer-error cases (invalid date, negative duration). Normal pricing path must not throw.
- **No heap allocation** required on the billing hot path.
- **Thread safety**: the library is stateless except for `SubscriberAccount` mutation; callers are responsible for synchronisation of the account object.
- **No third-party runtime dependencies** beyond the C++ standard library. doctest is a test-only dependency fetched at configure time.

---

## Test Scenarios

All scenarios use a `DateTime` struct for start/end times.

| # | Given | When | Expected |
|---|---|---|---|
| T01 | Weekday, home network, pool = 30 min, call 3 min | `calculateCost` | cost = 0.33, pool_used = 3, pool_remaining = 27 |
| T02 | Weekday, home network, pool = 2 min, call 5 min | `calculateCost` | cost = 0.33 + 3×0.50 = 1.83, pool_used = 2 |
| T03 | Weekday, home network, pool = 0 min, call 4 min | `calculateCost` | cost = 0.33 + 4×0.50 = 2.33 |
| T04 | Weekday, external network, call 3 min | `calculateCost` | cost = 0.33 + 3×0.95 = 3.18, pool_used = 0 |
| T05 | Weekend (Saturday), home network, pool = 10, call 3 min (≤5) | `calculateCost` | cost = 0.00, pool_used = 0 |
| T06 | Weekend, home network, pool = 10, call 5 min exactly (≤5) | `calculateCost` | cost = 0.00, pool_used = 0 |
| T07 | Weekend, home network, pool = 10, call 8 min | `calculateCost` | weekend_free = 5, chargeable = 3, pool_used = 3, cost = 0.33 |
| T08 | Weekend, home network, pool = 2, call 8 min | `calculateCost` | weekend_free = 5, chargeable = 3, pool_used = 2, paid = 1, cost = 0.33 + 1×0.50 = 0.83 |
| T09 | Weekend, external network, pool = 30, call 3 min (≤5) | `calculateCost` | cost = 0.00, pool unchanged |
| T10 | Weekend, external network, call 8 min | `calculateCost` | weekend_free = 5, paid = 3, cost = 0.33 + 3×0.95 = 3.18, pool unchanged |
| T11 | Weekday, home network, pool expired (day 31), call 3 min | `calculateCost` | pool treated as 0, cost = 0.33 + 3×0.50 = 1.83 |
| T12 | Duration 1 min 3 sec | billed minutes | 2 |
| T13 | Duration exactly 60 sec | billed minutes | 1 |
| T14 | Duration 0 sec | `calculateCost` | cost = 0.00 |
| T15 | Home network detection: "0501234567" with prefix "050" | `isHomeNetwork` | true |
| T16 | Home network detection: "0671234567" with prefixes {050,066,095,099} | `isHomeNetwork` | false |
| T17 | Friday call starting 23:59, lasting 3 min (crosses into Saturday) | `calculateCost` | treated as Friday → no weekend discount |

---

## API Sketch

```cpp
// DateTime: plain struct, no platform-specific types
struct DateTime {
    int year, month, day;      // 1-based
    int hour, minute, second;
};

// Mutable per-subscriber state passed in and updated by calculateCost
struct SubscriberAccount {
    int      free_minutes_remaining;   // 0..30
    DateTime last_credit_date;         // date pool was last (re)filled
};

struct CallResult {
    double total_cost;
    int    billed_minutes;
    int    pool_minutes_used;
    int    weekend_free_minutes;
};

class BillingEngine {
public:
    explicit BillingEngine(std::vector<std::string> home_prefixes = {"050","066","095","099"});

    // Calculates cost; updates account.free_minutes_remaining in-place.
    CallResult calculateCost(const DateTime&      start,
                             const DateTime&      end,
                             const std::string&   called_number,
                             SubscriberAccount&   account) const;

    bool isHomeNetwork(const std::string& number) const;
};
```

---

## Constraints

- No use of `std::chrono::system_clock` epoch arithmetic for weekday/date math (not reliably portable before C++20). Implement a minimal weekday function (e.g. Tomohiko Sakamoto's algorithm) from first principles.
- No Boost, Qt, or other heavyweight libraries.
- The library must compile cleanly with `-Wall -Wextra -Wpedantic` (GCC/Clang) and `/W4` (MSVC) with zero warnings.
- ABI stability is **not** a concern — this is a demo project with no versioning requirements.

---

## Assumptions

1. **CMake 3.16+** — widely available; needed for `FetchContent_MakeAvailable`.
2. **C++17** — avoids C++20 calendar types while still giving structured bindings, `std::string_view`, etc.
3. **Call day = start day** — the start timestamp alone determines weekday/weekend classification.
4. **Pool expiry = strict 30-day window** — day 0 is `last_credit_date`; pool expires at the start of day 31 (i.e., `current_date - last_credit_date > 30` means expired).
5. **Cost type = `double`** — sufficient precision for a billing demo; rounding at display layer.
6. **Prefixes are runtime-configurable** — constructor argument satisfies "easy to configure".

---

## Open Questions

None — all clarifications resolved before spec was written.
