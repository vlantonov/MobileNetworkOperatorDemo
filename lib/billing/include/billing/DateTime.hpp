#pragma once

namespace billing {

struct DateTime {
    int year;    // e.g. 2024
    int month;   // 1-12
    int day;     // 1-31
    int hour;    // 0-23
    int minute;  // 0-59
    int second;  // 0-59
};

/// Returns 0=Sunday, 1=Monday, …, 6=Saturday.
/// Uses Tomohiko Sakamoto's algorithm — no platform APIs.
int weekdayOf(const DateTime& dt);

/// Returns the number of calendar days from a to b (positive if b is after a).
/// Uses a proleptic Gregorian day serial — no <ctime> epoch dependency.
int daysBetween(const DateTime& a, const DateTime& b);

} // namespace billing
