#pragma once

namespace billing {

/// Output of BillingEngine::calculateCost.
struct CallResult {
    double total_cost;           ///< Total charge in currency units (not rounded)
    int    billed_minutes;       ///< Ceiling of call duration in minutes
    int    pool_minutes_used;    ///< Free-pool minutes consumed by this call
    int    weekend_free_minutes; ///< Weekend-discount minutes applied to this call
};

} // namespace billing
