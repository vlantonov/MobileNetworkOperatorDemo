# tests/

Unit tests for the `billing` library.

## Framework
**doctest v2.4.11** — fetched via CMake `FetchContent` at configure time (test-only dependency).
`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` is defined at the top of `test_billing.cpp`; no separate
test runner or `main()` is needed.

## Structure
Single file: `test_billing.cpp`. Tests are grouped into `TEST_SUITE` blocks by topic:

| Suite           | What it covers                                        |
|-----------------|-------------------------------------------------------|
| `date_helpers`  | `weekdayOf`, `daysBetween` directly                   |
| `home_network`  | `isHomeNetwork` — prefix matching, empty string       |
| `duration`      | Billed-minutes ceiling, zero duration, negative throw |
| `weekday_calls` | All weekday pricing rules, pool expiry, depletion     |
| `weekend_calls` | Weekend discount — short/long calls, home/external    |

## Conventions
- **Float comparisons:** always use `doctest::Approx` — never compare `double` with `==`.
- **DateTime helpers:** two file-local static helpers at the top of `test_billing.cpp`:
  - `date(y, m, d)` — builds a `DateTime` with time = 00:00:00
  - `dt(y, m, d, h, min, sec)` — builds a full `DateTime`
- **Reference dates** used throughout (verified against `weekdayOf`):
  - Saturday = 2024-06-01, Sunday = 2024-06-02, Monday = 2024-06-03, Friday = 2024-05-31
- Each `TEST_CASE` is self-contained — constructs its own `BillingEngine` and
  `SubscriberAccount`; no shared fixtures or global state.
- No integration tests (the library has no I/O or external dependencies).

## Running

```bash
# via CTest (preferred — matches CI)
ctest --test-dir build --output-on-failure

# directly (shows per-case detail)
./build/tests/test_billing

# filter to a specific test case or pattern
./build/tests/test_billing -tc="home_network*"
./build/tests/test_billing --test-suite="weekday_calls"
```
