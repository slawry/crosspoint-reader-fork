#include "TodoClock.h"

#include <HalClock.h>
#include <Rtc.h>

#include "CrossPointSettings.h"

namespace {

// Days since 1970-01-01 for a proleptic-Gregorian civil date. Howard
// Hinnant's well-known constexpr algorithm (public domain):
// https://howardhinnant.github.io/date_algorithms.html#days_from_civil
int32_t daysFromCivil(int32_t y, const uint32_t m, const uint32_t d) {
  y -= m <= 2;
  const int32_t era = (y >= 0 ? y : y - 399) / 400;
  const uint32_t yoe = static_cast<uint32_t>(y - era * 400);
  const uint32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const uint32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int32_t>(doe) - 719468;
}

// Floor division (rounds toward negative infinity), needed so a negative UTC
// offset near local midnight rolls the day key backward correctly instead of
// truncating toward zero.
int64_t floorDiv(const int64_t a, const int64_t b) {
  int64_t q = a / b;
  const int64_t r = a % b;
  if (r != 0 && ((r < 0) != (b < 0))) q--;
  return q;
}

}  // namespace

bool getTodoLocalTime(TodoLocalTime& out) {
  Rtc::DateTime utc;
  if (!halClock.getDateTime(utc)) return false;

  const int32_t utcDays = daysFromCivil(utc.year, utc.month, utc.day);
  const int32_t offsetMinutes = (static_cast<int32_t>(SETTINGS.clockUtcOffsetQ) - 48) * 15;
  const int64_t totalMinutes = static_cast<int64_t>(utcDays) * 1440 + utc.hour * 60 + utc.minute + offsetMinutes;

  const int64_t localDays = floorDiv(totalMinutes, 1440);
  const int32_t minuteOfDay = static_cast<int32_t>(totalMinutes - localDays * 1440);

  out.dayKey = static_cast<int32_t>(localDays);
  out.hour = static_cast<uint8_t>(minuteOfDay / 60);
  out.minute = static_cast<uint8_t>(minuteOfDay % 60);
  out.weekday = static_cast<uint8_t>((((localDays % 7) + 7) % 7 + 4) % 7);
  return true;
}
