#pragma once

namespace billing {

/// All pricing constants in one place — change any value here to reconfigure billing rules.
struct BillingConfig {
    double connection_fee        = 0.33;
    double home_rate_per_min     = 0.50;
    double external_rate_per_min = 0.95;
    int    free_minutes_cap      = 30;   ///< Initial pool size when credits are added
    int    pool_validity_days    = 30;   ///< Pool is valid for this many days after last credit
    int    weekend_free_minutes  = 5;    ///< First N minutes of every weekend call are free
};

} // namespace billing
