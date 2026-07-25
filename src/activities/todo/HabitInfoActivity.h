#pragma once
#include "activities/Activity.h"

// Weekly info modal for a single habit (docs/design-spec.md Section 5's "Inside the Habit
// list" RR action). Read-only: shows this week's tally against the habit's target and
// whether that's on track for its build/break direction. Dismissed with Back, like every
// other pushed screen in this app.
class HabitInfoActivity final : public Activity {
  int habitId;

 public:
  explicit HabitInfoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const int habitId)
      : Activity("HabitInfo", renderer, mappedInput), habitId(habitId) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
