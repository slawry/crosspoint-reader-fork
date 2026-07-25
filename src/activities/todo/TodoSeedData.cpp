#include "TodoSeedData.h"

#if TODO_SEED_DEBUG_DATA

#include "TodoData.h"

void seedTodoDebugData() {
  TODO_DATA.clear();

  const int groceries = TODO_DATA.addList("Groceries");
  TODO_DATA.addTask(groceries, "Milk");
  TODO_DATA.addTask(groceries, "Eggs", false, true /* favourited */);
  TODO_DATA.addTask(groceries, "Bread", true /* completed */);

  const int work = TODO_DATA.addList("Work");
  TODO_DATA.addTask(work, "Finish report", false, true /* favourited */);
  TODO_DATA.addTask(work, "Email Bob");
  TODO_DATA.addTask(work, "Review PR", true /* completed */);

  // Checklists: one sample per frequency and per display mode (design-spec.md
  // Section 6), so the visibility/reset logic in TodoDataStore can actually be
  // exercised. Two TimeWindow checklists at different times of day, so at
  // least one is likely to be observably open/closed in a normal test
  // session; use Settings > Clock Offset to shift local time and watch either
  // one cross its window boundary without waiting for the real time to pass.
  const int morning = TODO_DATA.addChecklist("Morning Routine", ChecklistFrequency::Daily,
                                             ChecklistDisplayMode::TimeWindow, 6, 0, 9, 0);
  TODO_DATA.addTask(morning, "Make bed");
  TODO_DATA.addTask(morning, "Stretch");
  TODO_DATA.addTask(morning, "Drink water");

  const int evening = TODO_DATA.addChecklist("Evening Wind-down", ChecklistFrequency::Daily,
                                             ChecklistDisplayMode::TimeWindow, 20, 0, 22, 0);
  TODO_DATA.addTask(evening, "Journal");
  TODO_DATA.addTask(evening, "Read 10 pages");
  TODO_DATA.addTask(evening, "Set out clothes");

  const int weekdayFocus =
      TODO_DATA.addChecklist("Weekday Focus", ChecklistFrequency::Weekdays, ChecklistDisplayMode::UntilAllComplete);
  TODO_DATA.addTask(weekdayFocus, "Check calendar");
  TODO_DATA.addTask(weekdayFocus, "Review priorities");
  TODO_DATA.addTask(weekdayFocus, "Inbox zero");

  const int weekendChores =
      TODO_DATA.addChecklist("Weekend Chores", ChecklistFrequency::Weekends, ChecklistDisplayMode::UntilAllComplete);
  TODO_DATA.addTask(weekendChores, "Laundry");
  TODO_DATA.addTask(weekendChores, "Groceries");
  TODO_DATA.addTask(weekendChores, "Meal prep");

  // Custom frequency: Wednesday + Saturday (bit N = weekday N, 0=Sunday..6=Saturday).
  constexpr uint8_t kWednesday = 1 << 3;
  constexpr uint8_t kSaturday = 1 << 6;
  const int deepClean = TODO_DATA.addChecklist("Deep Clean", ChecklistFrequency::Custom,
                                               ChecklistDisplayMode::UntilAllComplete, 0, 0, 0, 0,
                                               kWednesday | kSaturday);
  TODO_DATA.addTask(deepClean, "Vacuum");
  TODO_DATA.addTask(deepClean, "Bathroom");
  TODO_DATA.addTask(deepClean, "Kitchen");

  // Habits: one sample per mode x direction combination (design-spec.md Section 6),
  // so both the weekly rollup and the weekly info modal's on-track/off-track status
  // can be exercised for every combination. Targets are set a little below a
  // typical week's activity so the seeded data reads as "in progress" rather than
  // already complete either way.
  const int wellness = TODO_DATA.addHabitList("Wellness");
  TODO_DATA.addHabit(wellness, "Meditate", HabitMode::Boolean, HabitDirection::Build, 5 /* days out of 7 */);
  TODO_DATA.addHabit(wellness, "No Late Snacking", HabitMode::Boolean, HabitDirection::Break,
                     2 /* max slip-days out of 7 */);
  TODO_DATA.addHabit(wellness, "Water (glasses)", HabitMode::Quantity, HabitDirection::Build,
                     50 /* weekly total target */);
  TODO_DATA.addHabit(wellness, "Over Screen Time Limit", HabitMode::Quantity, HabitDirection::Break,
                     3 /* max weekly occurrences */);
}

#endif
