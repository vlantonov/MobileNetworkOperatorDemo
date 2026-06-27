#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <billing/BillingEngine.hpp>
#include <billing/DateTime.hpp>

using namespace billing;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Build a DateTime quickly: date only (time = 00:00:00)
static DateTime date(int y, int m, int d) {
    return {y, m, d, 0, 0, 0};
}

// Build a DateTime with full time
static DateTime dt(int y, int m, int d, int h, int min, int sec) {
    return {y, m, d, h, min, sec};
}

// Saturday 2024-06-01, Sunday 2024-06-02, Monday 2024-06-03 (verified)
// Friday  2024-05-31

// ---------------------------------------------------------------------------
// TEST SUITE: date helpers
// ---------------------------------------------------------------------------

TEST_SUITE("date_helpers") {

    TEST_CASE("weekdayOf/known_dates") {
        // 2024-06-01 is Saturday (6)
        CHECK(weekdayOf(date(2024, 6, 1)) == 6);
        // 2024-06-02 is Sunday (0)
        CHECK(weekdayOf(date(2024, 6, 2)) == 0);
        // 2024-06-03 is Monday (1)
        CHECK(weekdayOf(date(2024, 6, 3)) == 1);
        // 2024-05-31 is Friday (5)
        CHECK(weekdayOf(date(2024, 5, 31)) == 5);
    }

    TEST_CASE("daysBetween/same_date") {
        CHECK(daysBetween(date(2024, 6, 1), date(2024, 6, 1)) == 0);
    }

    TEST_CASE("daysBetween/one_day") {
        CHECK(daysBetween(date(2024, 6, 1), date(2024, 6, 2)) == 1);
    }

    TEST_CASE("daysBetween/across_month") {
        CHECK(daysBetween(date(2024, 5, 31), date(2024, 6, 3)) == 3);
    }

    TEST_CASE("daysBetween/30_days") {
        CHECK(daysBetween(date(2024, 5, 1), date(2024, 5, 31)) == 30);
    }

    TEST_CASE("daysBetween/31_days") {
        CHECK(daysBetween(date(2024, 5, 1), date(2024, 6, 1)) == 31);
    }
}

// ---------------------------------------------------------------------------
// TEST SUITE: home network detection
// ---------------------------------------------------------------------------

TEST_SUITE("home_network") {

    TEST_CASE("match_050_prefix") {   // T15
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("0501234567") == true);
    }

    TEST_CASE("match_066_prefix") {
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("0661234567") == true);
    }

    TEST_CASE("match_095_prefix") {
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("0951234567") == true);
    }

    TEST_CASE("match_099_prefix") {
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("0991234567") == true);
    }

    TEST_CASE("no_match_067") {   // T16
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("0671234567") == false);
    }

    TEST_CASE("empty_string") {
        BillingEngine engine;
        CHECK(engine.isHomeNetwork("") == false);
    }

    TEST_CASE("custom_prefixes") {
        BillingEngine engine({"073", "093"});
        CHECK(engine.isHomeNetwork("0731234567") == true);
        CHECK(engine.isHomeNetwork("0501234567") == false);
    }
}

// ---------------------------------------------------------------------------
// TEST SUITE: duration / billed minutes
// ---------------------------------------------------------------------------

TEST_SUITE("duration") {

    TEST_CASE("zero_seconds") {   // T14
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        // Monday 2024-06-03
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 0, 0),
            "0501234567", acc);
        CHECK(r.total_cost == doctest::Approx(0.0));
        CHECK(r.billed_minutes == 0);
        CHECK(r.pool_minutes_used == 0);
        CHECK(r.weekend_free_minutes == 0);
    }

    TEST_CASE("ceiling_1m3s") {   // T12
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 1, 3),
            "0501234567", acc);
        CHECK(r.billed_minutes == 2);
    }

    TEST_CASE("exact_60s") {   // T13
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 1, 0),
            "0501234567", acc);
        CHECK(r.billed_minutes == 1);
    }

    TEST_CASE("negative_duration_throws") {
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        CHECK_THROWS_AS(
            engine.calculateCost(
                dt(2024, 6, 3, 10, 1, 0),
                dt(2024, 6, 3, 10, 0, 0),
                "0501234567", acc),
            std::invalid_argument);
    }
}

// ---------------------------------------------------------------------------
// TEST SUITE: weekday calls
// ---------------------------------------------------------------------------

TEST_SUITE("weekday_calls") {

    // T01: home network, pool = 30, call 3 min → cost = 0.33, pool_used = 3
    TEST_CASE("home_full_pool") {
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 3, 0),
            "0501234567", acc);
        CHECK(r.total_cost        == doctest::Approx(0.33));
        CHECK(r.billed_minutes    == 3);
        CHECK(r.pool_minutes_used == 3);
        CHECK(r.weekend_free_minutes == 0);
        CHECK(acc.free_minutes_remaining == 27);
    }

    // T02: home network, pool = 2, call 5 min → cost = 0.33 + 3*0.50 = 1.83
    TEST_CASE("home_partial_pool") {
        BillingEngine engine;
        SubscriberAccount acc{2, date(2024, 6, 3)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 5, 0),
            "0501234567", acc);
        CHECK(r.total_cost        == doctest::Approx(0.33 + 3 * 0.50));
        CHECK(r.billed_minutes    == 5);
        CHECK(r.pool_minutes_used == 2);
        CHECK(acc.free_minutes_remaining == 0);
    }

    // T03: home network, pool = 0, call 4 min → cost = 0.33 + 4*0.50 = 2.33
    TEST_CASE("home_empty_pool") {
        BillingEngine engine;
        SubscriberAccount acc{0, date(2024, 6, 3)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 4, 0),
            "0501234567", acc);
        CHECK(r.total_cost        == doctest::Approx(0.33 + 4 * 0.50));
        CHECK(r.pool_minutes_used == 0);
    }

    // T04: external network, call 3 min → cost = 0.33 + 3*0.95 = 3.18
    TEST_CASE("external") {
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 3)};
        int pool_before = acc.free_minutes_remaining;
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 3, 0),
            "0671234567", acc);
        CHECK(r.total_cost        == doctest::Approx(0.33 + 3 * 0.95));
        CHECK(r.pool_minutes_used == 0);
        CHECK(acc.free_minutes_remaining == pool_before);  // pool untouched
    }

    // T11: pool expired (call is on day 31 after last credit)
    TEST_CASE("home_pool_expired") {
        BillingEngine engine;
        // last_credit_date = 2024-05-01; call on 2024-06-01 → 31 days later
        SubscriberAccount acc{30, date(2024, 5, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),   // Saturday — use Sunday instead to avoid weekend
            dt(2024, 6, 1, 10, 3, 0),
            "0501234567", acc);
        // 2024-06-01 is Saturday → weekend rule applies; use a weekday instead
        // Let's use 2024-06-03 (Monday), last_credit = 2024-05-03 → 31 days later
        acc = {30, date(2024, 5, 3)};
        r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 3, 0),
            "0501234567", acc);
        CHECK(r.pool_minutes_used == 0);
        CHECK(r.total_cost == doctest::Approx(0.33 + 3 * 0.50));
    }

    // Boundary: pool still valid on exactly day 30
    TEST_CASE("home_pool_valid_on_day_30") {
        BillingEngine engine;
        // last_credit = 2024-05-04; call on 2024-06-03 (Monday) → 30 days later
        SubscriberAccount acc{30, date(2024, 5, 4)};
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 3, 0),
            "0501234567", acc);
        CHECK(r.pool_minutes_used == 3);
        CHECK(r.total_cost == doctest::Approx(0.33));
    }

    // Pool depletes correctly across two sequential calls
    TEST_CASE("pool_depletion_across_calls") {
        BillingEngine engine;
        SubscriberAccount acc{5, date(2024, 6, 3)};

        // First call: 3 min → pool_used=3, remaining=2
        engine.calculateCost(
            dt(2024, 6, 3, 10, 0, 0),
            dt(2024, 6, 3, 10, 3, 0),
            "0501234567", acc);
        CHECK(acc.free_minutes_remaining == 2);

        // Second call: 4 min → pool_used=2, paid=2
        auto r = engine.calculateCost(
            dt(2024, 6, 3, 11, 0, 0),
            dt(2024, 6, 3, 11, 4, 0),
            "0501234567", acc);
        CHECK(r.pool_minutes_used == 2);
        CHECK(r.total_cost == doctest::Approx(0.33 + 2 * 0.50));
        CHECK(acc.free_minutes_remaining == 0);
    }
}

// ---------------------------------------------------------------------------
// TEST SUITE: weekend calls
// ---------------------------------------------------------------------------

TEST_SUITE("weekend_calls") {

    // T05: Saturday, home, pool=10, call 3 min (≤5) → cost = 0.00, pool_used=0
    TEST_CASE("home_short_le5_saturday") {
        BillingEngine engine;
        SubscriberAccount acc{10, date(2024, 6, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),
            dt(2024, 6, 1, 10, 3, 0),
            "0501234567", acc);
        CHECK(r.total_cost           == doctest::Approx(0.0));
        CHECK(r.pool_minutes_used    == 0);
        CHECK(r.weekend_free_minutes == 3);
        CHECK(acc.free_minutes_remaining == 10);  // pool untouched
    }

    // T06: Weekend, home, pool=10, call exactly 5 min → cost = 0.00
    TEST_CASE("home_exact_5_sunday") {
        BillingEngine engine;
        SubscriberAccount acc{10, date(2024, 6, 2)};
        auto r = engine.calculateCost(
            dt(2024, 6, 2, 10, 0, 0),
            dt(2024, 6, 2, 10, 5, 0),
            "0501234567", acc);
        CHECK(r.total_cost           == doctest::Approx(0.0));
        CHECK(r.pool_minutes_used    == 0);
        CHECK(r.weekend_free_minutes == 5);
    }

    // T07: Weekend, home, pool=10, call 8 min → weekend_free=5, pool_used=3, cost=0.33
    TEST_CASE("home_long_pool_covers") {
        BillingEngine engine;
        SubscriberAccount acc{10, date(2024, 6, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),
            dt(2024, 6, 1, 10, 8, 0),
            "0501234567", acc);
        CHECK(r.total_cost           == doctest::Approx(0.33));
        CHECK(r.billed_minutes       == 8);
        CHECK(r.weekend_free_minutes == 5);
        CHECK(r.pool_minutes_used    == 3);
        CHECK(acc.free_minutes_remaining == 7);
    }

    // T08: Weekend, home, pool=2, call 8 min → 5 free + pool=2 + paid=1 → cost=0.33+0.50=0.83
    TEST_CASE("home_long_partial_pool") {
        BillingEngine engine;
        SubscriberAccount acc{2, date(2024, 6, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),
            dt(2024, 6, 1, 10, 8, 0),
            "0501234567", acc);
        CHECK(r.total_cost           == doctest::Approx(0.33 + 1 * 0.50));
        CHECK(r.weekend_free_minutes == 5);
        CHECK(r.pool_minutes_used    == 2);
        CHECK(acc.free_minutes_remaining == 0);
    }

    // T09: Weekend, external, pool=30, call 3 min (≤5) → cost=0.00, pool unchanged
    TEST_CASE("external_short_le5") {
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),
            dt(2024, 6, 1, 10, 3, 0),
            "0671234567", acc);
        CHECK(r.total_cost        == doctest::Approx(0.0));
        CHECK(r.pool_minutes_used == 0);
        CHECK(acc.free_minutes_remaining == 30);
    }

    // T10: Weekend, external, call 8 min → 5 free + 3 paid at 0.95 → cost=0.33+3*0.95=3.18
    TEST_CASE("external_long") {
        BillingEngine engine;
        SubscriberAccount acc{30, date(2024, 6, 1)};
        auto r = engine.calculateCost(
            dt(2024, 6, 1, 10, 0, 0),
            dt(2024, 6, 1, 10, 8, 0),
            "0671234567", acc);
        CHECK(r.total_cost           == doctest::Approx(0.33 + 3 * 0.95));
        CHECK(r.weekend_free_minutes == 5);
        CHECK(r.pool_minutes_used    == 0);
        CHECK(acc.free_minutes_remaining == 30);
    }

    // T17: Friday call starting 23:59, lasting ~3 min (crosses into Saturday) → no discount
    TEST_CASE("midnight_crossover_friday_to_saturday") {
        BillingEngine engine;
        // 2024-05-31 is Friday; call starts 23:59 and ends 00:02 next day
        SubscriberAccount acc{0, date(2024, 5, 31)};
        auto r = engine.calculateCost(
            dt(2024, 5, 31, 23, 59, 0),
            dt(2024, 6,  1,  0,  2, 0),  // 3 minutes later (Saturday, but start was Friday)
            "0671234567", acc);
        // Start is Friday → no weekend discount
        CHECK(r.weekend_free_minutes == 0);
        CHECK(r.total_cost == doctest::Approx(0.33 + 3 * 0.95));
    }
}
