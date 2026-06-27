# lib/billing

Static library implementing mobile network operator billing rules.
All code lives in `namespace billing`. No runtime dependencies beyond the C++ standard library.

## Public Headers (`include/billing/`)

| Header                  | Contents                                        |
|-------------------------|-------------------------------------------------|
| `DateTime.hpp`          | `DateTime` struct + `weekdayOf`, `daysBetween`  |
| `BillingConfig.hpp`     | `BillingConfig` — all pricing constants         |
| `SubscriberAccount.hpp` | `SubscriberAccount` — mutable subscriber state  |
| `CallResult.hpp`        | `CallResult` — output of `calculateCost`        |
| `BillingEngine.hpp`     | `BillingEngine` — main class                    |

## Key Types

### `DateTime`
Plain aggregate `{ year, month, day, hour, minute, second }` — all `int`, 1-based for date
fields. No platform types; no `<ctime>` dependency.

### `BillingConfig`
Single source of truth for all pricing constants (rates, pool size, expiry, weekend threshold).
Pass a custom instance to `BillingEngine`'s constructor to change any value without recompiling
the library.

### `SubscriberAccount`
Mutable state passed **by non-const reference** to `calculateCost`. The field
`free_minutes_remaining` is decremented in-place after each call.

### `BillingEngine`
Immutable after construction. Safe to share across threads. Thread-safety of a shared
`SubscriberAccount` is the caller's responsibility.

## API Contracts

### `BillingEngine::calculateCost`
- **Preconditions:** `end >= start`; `called_number` non-empty (preconditions are not validated
  for the empty-number case — caller's responsibility to ensure non-empty).
- **Postconditions:** `account.free_minutes_remaining` is decremented by `result.pool_minutes_used`.
- **Throws:** `std::invalid_argument("negative call duration")` if `end < start`. This is the
  only throwing path; all other paths are no-throw.
- **Zero-duration special case:** returns `{0.0, 0, 0, 0}` without charging anything or
  modifying the account.

### `BillingEngine::isHomeNetwork`
- Returns `true` iff `number` starts with one of the configured home-network prefixes.
- Returns `false` for an empty string. No-throw.

### `weekdayOf(dt)`
- Returns 0=Sun … 6=Sat. Uses Tomohiko Sakamoto's algorithm (no `<ctime>`, no platform APIs).
- No-throw. Precondition: valid Gregorian date (not validated).

### `daysBetween(a, b)`
- Returns `b − a` in calendar days (positive if b is after a). Uses a proleptic Gregorian day
  serial (no `<ctime>`, no epoch dependency).
- No-throw. Precondition: valid Gregorian dates (not validated).

## Billing Rules (implemented in `BillingEngine::calculateCost`)
1. **Billed minutes:** ceiling of call duration in seconds ÷ 60.
2. **Call day:** determined by `start` — calls crossing midnight use the start day.
3. **Weekend discount** (Sat/Sun): first `BillingConfig::weekend_free_minutes` (default 5)
   minutes of every call are free and do **not** consume the free-minutes pool.
4. **Fully-free weekend call** (billed minutes ≤ weekend_free_minutes): `total_cost = 0.0`,
   connection fee also waived.
5. **Free-minutes pool:** home network calls only; pool starts counting after weekend-free
   minutes. Valid for `pool_validity_days` (default 30) days from `last_credit_date` inclusive;
   expires on day 31.
6. **Connection fee** (`connection_fee`, default 0.33): added to every call that is not
   entirely free (i.e., all calls except the fully-free weekend case).
7. **Home rate:** `home_rate_per_min` (default 0.50) per minute after pool exhausted.
8. **External rate:** `external_rate_per_min` (default 0.95) per minute; pool never applied.

## Internal Implementation Notes
- All private helpers (`durationSeconds`, `billedMinutes`, `isWeekendDay`, `isPoolValid`,
  `weekdayImpl`, `daySerial`) live in an **anonymous namespace** in `BillingEngine.cpp` —
  not visible outside the translation unit.
- `weekdayOf` / `daysBetween` are the only internal helpers promoted to the public header
  (needed so tests can exercise date arithmetic directly).
- No heap allocation on the billing hot path; all locals are stack-allocated aggregates.
