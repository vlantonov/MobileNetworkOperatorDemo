#pragma once

#include "BillingConfig.hpp"
#include "CallResult.hpp"
#include "DateTime.hpp"
#include "SubscriberAccount.hpp"

#include <string>
#include <vector>

namespace billing {

/// Stateless (after construction) billing calculation engine.
///
/// Thread-safety: BillingEngine itself is immutable after construction and safe to share across
/// threads. The SubscriberAccount argument to calculateCost is mutated in-place — callers are
/// responsible for synchronising access to the same account object from multiple threads.
class BillingEngine {
public:
    /// @param home_prefixes  Phone number prefixes that belong to this operator's home network.
    /// @param config         Pricing constants. Uses defaults if not provided.
    explicit BillingEngine(
        std::vector<std::string> home_prefixes = {"050", "066", "095", "099"},
        BillingConfig config = {});

    /// Calculate the cost of a single call and update the subscriber's free-minutes pool.
    ///
    /// Preconditions:
    ///   - end >= start (in calendar time)
    ///   - called_number must not be empty
    /// Postconditions:
    ///   - account.free_minutes_remaining is decremented by result.pool_minutes_used
    ///   - returned CallResult is consistent with the billing rules in BillingConfig
    /// Error behavior:
    ///   - Throws std::invalid_argument if the call duration is negative (end < start).
    ///   - Normal pricing path is no-throw.
    CallResult calculateCost(const DateTime&    start,
                             const DateTime&    end,
                             const std::string& called_number,
                             SubscriberAccount& account) const;

    /// Returns true if 'number' starts with one of the configured home-network prefixes.
    /// Returns false for an empty string. No-throw.
    bool isHomeNetwork(const std::string& number) const;

private:
    std::vector<std::string> home_prefixes_;
    BillingConfig            config_;
};

} // namespace billing
