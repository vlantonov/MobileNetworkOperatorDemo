#include "billing/BillingEngine.hpp"

#include <algorithm>
#include <stdexcept>

namespace billing {

// ---------------------------------------------------------------------------
// Private date / duration helpers
// ---------------------------------------------------------------------------

namespace {

/// Tomohiko Sakamoto's algorithm — returns 0=Sun, 1=Mon, …, 6=Sat.
/// Correct for any Gregorian date. No platform APIs.
int weekdayImpl(int y, int m, int d) {
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        --y;
    }
    return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

/// Proleptic Gregorian day serial (no epoch / no <ctime>).
/// Adjusts Jan/Feb to be months 13/14 of the prior year so that
/// the leap-day always falls at the end of the "year" for this formula.
int daySerial(int y, int m, int d) {
    if (m <= 2) {
        --y;
        m += 12;
    }
    return 365 * y + y / 4 - y / 100 + y / 400 + (153 * (m - 3) + 2) / 5 + d;
}

/// Total seconds from start to end (may be negative — callers validate).
long long durationSeconds(const DateTime& s, const DateTime& e) {
    // Convert each DateTime to a total-seconds count from an arbitrary epoch.
    // We use the Gregorian day serial * 86400 + time-of-day.
    auto toSec = [](const DateTime& dt) -> long long {
        long long days = static_cast<long long>(daySerial(dt.year, dt.month, dt.day));
        return days * 86400LL
             + static_cast<long long>(dt.hour)   * 3600LL
             + static_cast<long long>(dt.minute) * 60LL
             + static_cast<long long>(dt.second);
    };
    return toSec(e) - toSec(s);
}

/// Ceiling of totalSeconds / 60; returns 0 when totalSeconds == 0.
int billedMinutes(long long totalSeconds) {
    if (totalSeconds <= 0) return 0;
    return static_cast<int>((totalSeconds + 59LL) / 60LL);
}

bool isWeekendDay(const DateTime& dt) {
    int w = weekdayImpl(dt.year, dt.month, dt.day);
    return w == 0 || w == 6; // Sunday or Saturday
}

bool isPoolValid(const SubscriberAccount& account, const DateTime& callStart,
                 int validity_days) {
    int diff = daySerial(callStart.year, callStart.month, callStart.day)
             - daySerial(account.last_credit_date.year,
                         account.last_credit_date.month,
                         account.last_credit_date.day);
    return diff <= validity_days;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public date helpers (declared in DateTime.hpp)
// ---------------------------------------------------------------------------

int weekdayOf(const DateTime& dt) {
    return weekdayImpl(dt.year, dt.month, dt.day);
}

int daysBetween(const DateTime& a, const DateTime& b) {
    return daySerial(b.year, b.month, b.day) - daySerial(a.year, a.month, a.day);
}

// ---------------------------------------------------------------------------
// BillingEngine
// ---------------------------------------------------------------------------

BillingEngine::BillingEngine(std::vector<std::string> home_prefixes, BillingConfig config)
    : home_prefixes_(std::move(home_prefixes)), config_(config) {}

bool BillingEngine::isHomeNetwork(const std::string& number) const {
    if (number.empty()) return false;
    for (const auto& prefix : home_prefixes_) {
        if (number.size() >= prefix.size() &&
            number.compare(0, prefix.size(), prefix) == 0) {
            return true;
        }
    }
    return false;
}

CallResult BillingEngine::calculateCost(const DateTime&    start,
                                         const DateTime&    end,
                                         const std::string& called_number,
                                         SubscriberAccount& account) const {
    const long long totalSec = durationSeconds(start, end);

    if (totalSec < 0) {
        throw std::invalid_argument("negative call duration");
    }

    if (totalSec == 0) {
        return {0.0, 0, 0, 0};
    }

    const int  billed  = billedMinutes(totalSec);
    const bool home    = isHomeNetwork(called_number);
    const bool weekend = isWeekendDay(start);

    // Weekend free minutes (independent of pool)
    const int weekendFree = weekend
        ? std::min(billed, config_.weekend_free_minutes)
        : 0;

    // Fully-free weekend call: no connection fee, no pool consumption
    if (weekend && billed <= config_.weekend_free_minutes) {
        return {0.0, billed, 0, weekendFree};
    }

    const int chargeable = billed - weekendFree;

    // Free-pool minutes (home network only; pool must still be valid)
    int poolUsed = 0;
    if (home && isPoolValid(account, start, config_.pool_validity_days)) {
        poolUsed = std::min(chargeable, account.free_minutes_remaining);
        account.free_minutes_remaining -= poolUsed;
    }

    const int    paidMinutes = chargeable - poolUsed;
    const double rate        = home ? config_.home_rate_per_min
                                    : config_.external_rate_per_min;
    const double totalCost   = config_.connection_fee
                             + static_cast<double>(paidMinutes) * rate;

    return {totalCost, billed, poolUsed, weekendFree};
}

} // namespace billing
