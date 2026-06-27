#pragma once

#include "DateTime.hpp"

namespace billing {

/// Mutable per-subscriber state. Passed by non-const reference to BillingEngine::calculateCost,
/// which decrements free_minutes_remaining in-place.
struct SubscriberAccount {
    int      free_minutes_remaining;  ///< Current pool balance (0 .. BillingConfig::free_minutes_cap)
    DateTime last_credit_date;        ///< Date the pool was last filled / credits last added
};

} // namespace billing
