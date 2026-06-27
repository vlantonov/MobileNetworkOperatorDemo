# Mobile Network Operator Billing — Implementation Plan

## Directory Layout

```
MobileNetworkOperatorDemo/
├── CMakeLists.txt
├── lib/
│   └── billing/
│       ├── CMakeLists.txt
│       ├── include/billing/
│       │   ├── DateTime.hpp
│       │   ├── BillingConfig.hpp
│       │   ├── SubscriberAccount.hpp
│       │   ├── CallResult.hpp
│       │   └── BillingEngine.hpp
│       └── src/
│           └── BillingEngine.cpp
├── app/
│   └── demo/
│       ├── CMakeLists.txt
│       └── main.cpp
└── tests/
    ├── CMakeLists.txt
    └── test_billing.cpp
```

## Dependency Graph

```
billing (static lib, no deps)
    ↑               ↑
tests/test_billing  app/demo
```

---

## Step 1 — `lib/billing/include/billing/DateTime.hpp`

**What:** Plain `DateTime` aggregate + public date helper declarations.  
**Why:** All other types and the engine depend on this; it has no dependencies itself.  
**Details:**
- `struct DateTime { int year, month, day, hour, minute, second; }` — all 1-based for date fields.
- Free functions declared in `namespace billing`:
  - `int weekdayOf(const DateTime& dt)` — 0=Sun … 6=Sat, using Tomohiko Sakamoto's algorithm.
  - `int daysBetween(const DateTime& a, const DateTime& b)` — returns `b − a` in calendar days (positive if b is after a), using proleptic Gregorian day serial (no `<ctime>`).
- Header-only declarations; implementations live in `BillingEngine.cpp`.

---

## Step 2 — `lib/billing/include/billing/BillingConfig.hpp`

**What:** All pricing constants in one configurable struct.  
**Why:** Satisfies "easy to configure/modify" — one place to change any rate or threshold.  
**Details:**
```cpp
struct BillingConfig {
    double connection_fee        = 0.33;
    double home_rate_per_min     = 0.50;
    double external_rate_per_min = 0.95;
    int    free_minutes_cap      = 30;
    int    pool_validity_days    = 30;
    int    weekend_free_minutes  = 5;
};
```

---

## Step 3 — `lib/billing/include/billing/SubscriberAccount.hpp`

**What:** Mutable subscriber state passed by non-const reference to `calculateCost`.  
**Why:** Pool balance must be updated in-place after each call.  
**Details:**
```cpp
struct SubscriberAccount {
    int      free_minutes_remaining;  // 0 .. BillingConfig::free_minutes_cap
    DateTime last_credit_date;
};
```

---

## Step 4 — `lib/billing/include/billing/CallResult.hpp`

**What:** Output struct returned by `calculateCost`.  
**Why:** Carries all computed values the demo and tests need to inspect.  
**Details:**
```cpp
struct CallResult {
    double total_cost;
    int    billed_minutes;
    int    pool_minutes_used;
    int    weekend_free_minutes;
};
```

---

## Step 5 — `lib/billing/include/billing/BillingEngine.hpp`

**What:** Main class declaration.  
**Why:** Public API surface consumed by demo and tests.  
**Details:**
```cpp
class BillingEngine {
public:
    explicit BillingEngine(
        std::vector<std::string> home_prefixes = {"050","066","095","099"},
        BillingConfig config = {});

    // Preconditions:  end >= start; called_number non-empty.
    // Postconditions: account.free_minutes_remaining decremented by result.pool_minutes_used.
    // Error:          throws std::invalid_argument if duration is negative.
    // Thread-safety:  BillingEngine immutable post-construction; callers own account sync.
    CallResult calculateCost(const DateTime&    start,
                             const DateTime&    end,
                             const std::string& called_number,
                             SubscriberAccount& account) const;

    // Preconditions:  none (empty string → false).
    // No-throw.
    bool isHomeNetwork(const std::string& number) const;

private:
    std::vector<std::string> home_prefixes_;
    BillingConfig            config_;
};
```

---

## Step 6 — `lib/billing/src/BillingEngine.cpp`

**What:** Full implementation of `BillingEngine` + private date/duration helpers.  
**Why:** All logic in one translation unit; helpers in anonymous namespace to avoid ODR issues.  
**Details:**

Private helpers (anonymous namespace):
```
durationSeconds(start, end)  → long long   (end - start in seconds)
billedMinutes(totalSeconds)  → int          (ceiling: (s + 59) / 60, or 0 if s==0)
isWeekend(dt)                → bool         (weekdayOf == 0 or 6)
isPoolValid(account, callStart, validity_days) → bool  (daysBetween <= validity_days)
weekdayOf(y, m, d)           → int          (Tomohiko Sakamoto)
daysBetween(a, b)            → int          (proleptic Gregorian serial difference)
```

`calculateCost` algorithm:
```
1. totalSec = durationSeconds(start, end)
2. if totalSec < 0: throw std::invalid_argument("negative call duration")
3. if totalSec == 0: return {0.0, 0, 0, 0}
4. billed = billedMinutes(totalSec)
5. home    = isHomeNetwork(called_number)
6. weekend = isWeekend(start)
7. weekendFree = weekend ? min(billed, config_.weekend_free_minutes) : 0
8. if weekend && billed <= config_.weekend_free_minutes:
       return {0.0, billed, 0, weekendFree}   // fully free, no connection fee
9. chargeable = billed - weekendFree
10. poolUsed = 0
    if home && isPoolValid(account, start, config_.pool_validity_days):
        poolUsed = min(chargeable, account.free_minutes_remaining)
        account.free_minutes_remaining -= poolUsed
11. paidMinutes = chargeable - poolUsed
12. rate = home ? config_.home_rate_per_min : config_.external_rate_per_min
13. totalCost = config_.connection_fee + paidMinutes * rate
14. return {totalCost, billed, poolUsed, weekendFree}
```

Day serial formula for `daysBetween` (no `<ctime>`):
- Adjust Jan/Feb: if month <= 2, decrement year, add 12 to month.
- `serial = 365*y + y/4 - y/100 + y/400 + (153*(m-3)+2)/5 + d`
- Return `serialB - serialA`.

Tomohiko Sakamoto weekday (well-known public-domain algorithm):
```cpp
int weekdayOf(int y, int m, int d) {
    static const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (m < 3) --y;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}
```

---

## Step 7 — `lib/billing/CMakeLists.txt`

**What:** CMake target for the static library.  
**Details:**
```cmake
add_library(billing STATIC src/BillingEngine.cpp)
target_include_directories(billing PUBLIC include)
target_compile_features(billing PUBLIC cxx_std_17)
# Strict warnings
if(MSVC)
    target_compile_options(billing PRIVATE /W4 /WX)
else()
    target_compile_options(billing PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
```

---

## Step 8 — `tests/test_billing.cpp`

**What:** doctest unit tests for all spec scenarios plus boundary cases.  
**Why:** Verify every billing rule before the demo is wired up.  
**Details:** `#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` at top. One `TEST_SUITE` per topic.

| Test case | Spec | Assertion |
|---|---|---|
| `duration/zero` | T14 | cost = 0.0, all fields 0 |
| `duration/ceiling_1m3s` | T12 | billed_minutes = 2 |
| `duration/exact_60s` | T13 | billed_minutes = 1 |
| `home_network/match` | T15 | isHomeNetwork("0501234567") = true |
| `home_network/no_match` | T16 | isHomeNetwork("0671234567") = false |
| `home_network/empty` | — | isHomeNetwork("") = false |
| `weekday/home/full_pool` | T01 | cost ≈ 0.33, pool_used=3, account.remaining=27 |
| `weekday/home/partial_pool` | T02 | cost ≈ 1.83, pool_used=2 |
| `weekday/home/empty_pool` | T03 | cost ≈ 2.33, pool_used=0 |
| `weekday/external` | T04 | cost ≈ 3.18, pool_used=0, account unchanged |
| `weekend/home/short_le5` | T05 | cost = 0.0, pool_used=0, weekend_free=3 |
| `weekend/home/exact_5` | T06 | cost = 0.0, pool_used=0, weekend_free=5 |
| `weekend/home/long_pool_covers` | T07 | cost ≈ 0.33, pool_used=3, weekend_free=5 |
| `weekend/home/long_partial_pool` | T08 | cost ≈ 0.83, pool_used=2, weekend_free=5 |
| `weekend/external/short` | T09 | cost = 0.0, pool unchanged |
| `weekend/external/long` | T10 | cost ≈ 3.18, pool unchanged |
| `pool/expired_day31` | T11 | cost ≈ 1.83, pool_used=0 |
| `pool/valid_on_day30` | — | pool still consumed on day 30 |
| `weekend/midnight_crossover` | T17 | Friday 23:59 + 3min → no discount |
| `pool/depletion_across_calls` | — | two sequential calls deplete pool correctly |

Use `doctest::Approx` for all `double` comparisons (tolerance 0.001).

---

## Step 9 — `tests/CMakeLists.txt`

```cmake
add_executable(test_billing test_billing.cpp)
target_link_libraries(test_billing PRIVATE billing doctest::doctest)
add_test(NAME billing_tests COMMAND test_billing)
```

---

## Step 10 — `app/demo/main.cpp`

**What:** CLI demo printing a scenario table.  
**Why:** Satisfies "demo application that allows testing with example data".  
**Details:**
- Define a `Scenario` struct: `{ string label, DateTime start, end, string number, SubscriberAccount account }`.
- Hard-code 8 scenarios covering all rule combinations (see spec).
- Loop: call `engine.calculateCost`, print label + billed_min + pool_used + weekend_free + cost (2 d.p.) in a formatted table using `std::setw`/`std::fixed`/`std::setprecision(2)`.
- No argument parsing; no external input.

---

## Step 11 — `app/demo/CMakeLists.txt`

```cmake
add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE billing)
```

---

## Step 12 — Root `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.16)
project(MobileNetworkOperatorDemo CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lib/billing)
add_subdirectory(app/demo)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11)
    FetchContent_MakeAvailable(doctest)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Build & Run Instructions (to be written into README)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure   # run tests
./build/app/demo/demo                        # run demo
```

To disable tests: `cmake -B build -DBUILD_TESTS=OFF`
