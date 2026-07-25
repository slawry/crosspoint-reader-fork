#pragma once
#include <cstdint>

// Local (UTC-offset-applied) wall-clock reading derived from the RTC, used
// for checklist frequency/time-window scheduling (docs/design-spec.md
// Section 7). The RTC itself stores UTC (see HalClock::syncFromNTP()); this
// applies SETTINGS.clockUtcOffsetQ with calendar-correct day-boundary
// handling, so a checklist window still lands on the correct local day even
// at large offsets near midnight UTC.
struct TodoLocalTime {
  uint8_t weekday = 0;  // 0=Sunday..6=Saturday
  uint8_t hour = 0;     // 0-23
  uint8_t minute = 0;   // 0-59
  int32_t dayKey = 0;   // Days since 1970-01-01 in local time; unique per local calendar day.
};

// Returns false if the RTC is unavailable.
bool getTodoLocalTime(TodoLocalTime& out);
