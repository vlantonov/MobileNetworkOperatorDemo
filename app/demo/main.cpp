#include <billing/BillingEngine.hpp>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace billing;

struct Scenario {
    std::string       label;
    DateTime          start;
    DateTime          end;
    std::string       called_number;
    SubscriberAccount account;
};

static DateTime date(int y, int m, int d) { return {y, m, d, 0, 0, 0}; }
static DateTime dt(int y, int m, int d, int h, int min, int sec) {
    return {y, m, d, h, min, sec};
}

int main() {
    BillingEngine engine; // default home prefixes: 050, 066, 095, 099

    // clang-format off
    std::vector<Scenario> scenarios = {
        // 1. Weekday, home network, full pool (30 min) — only connection fee
        {"Weekday home, full pool (3 min)",
         dt(2024,6,3,10,0,0), dt(2024,6,3,10,3,0), "0501234567", {30, date(2024,6,3)}},

        // 2. Weekday, home network, partial pool (2 min) — pool + 3 paid min
        {"Weekday home, partial pool (5 min)",
         dt(2024,6,3,10,0,0), dt(2024,6,3,10,5,0), "0501234567", {2, date(2024,6,3)}},

        // 3. Weekday, home network, pool exhausted — all minutes charged
        {"Weekday home, empty pool (4 min)",
         dt(2024,6,3,10,0,0), dt(2024,6,3,10,4,0), "0501234567", {0, date(2024,6,3)}},

        // 4. Weekday, external network — charged at 0.95/min, pool untouched
        {"Weekday external (3 min)",
         dt(2024,6,3,10,0,0), dt(2024,6,3,10,3,0), "0671234567", {30, date(2024,6,3)}},

        // 5. Weekend (Saturday), home, short call ≤5 min — entirely free
        {"Weekend home, short call ≤5 min (3 min)",
         dt(2024,6,1,10,0,0), dt(2024,6,1,10,3,0), "0501234567", {10, date(2024,6,1)}},

        // 6. Weekend (Saturday), home, long call — 5 free + pool covers rest
        {"Weekend home, long call, pool covers (8 min)",
         dt(2024,6,1,10,0,0), dt(2024,6,1,10,8,0), "0501234567", {10, date(2024,6,1)}},

        // 7. Weekend (Saturday), external, long call — 5 free + 3 charged at 0.95
        {"Weekend external, long call (8 min)",
         dt(2024,6,1,10,0,0), dt(2024,6,1,10,8,0), "0671234567", {30, date(2024,6,1)}},

        // 8. Weekday, home, pool expired (day 31) — all minutes charged
        {"Weekday home, pool expired (3 min)",
         dt(2024,6,3,10,0,0), dt(2024,6,3,10,3,0), "0501234567", {30, date(2024,5,3)}},
    };
    // clang-format on

    // Table header
    const int W = 40;
    std::cout << std::left << std::setw(W) << "Scenario"
              << std::right
              << std::setw(8)  << "Billed"
              << std::setw(10) << "Pool"
              << std::setw(10) << "WkndFree"
              << std::setw(12) << "Cost"
              << "\n";
    std::cout << std::string(W + 8 + 10 + 10 + 12, '-') << "\n";

    for (auto& s : scenarios) {
        CallResult r = engine.calculateCost(s.start, s.end, s.called_number, s.account);
        std::cout << std::left  << std::setw(W) << s.label
                  << std::right
                  << std::setw(8)  << r.billed_minutes
                  << std::setw(10) << r.pool_minutes_used
                  << std::setw(10) << r.weekend_free_minutes
                  << std::setw(11) << std::fixed << std::setprecision(2) << r.total_cost
                  << "\n";
    }

    return 0;
}

