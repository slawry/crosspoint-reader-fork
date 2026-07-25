#include "HabitInfoActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>
#include <string>

#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoData.h"
#include "components/UITheme.h"
#include "fontIds.h"

void HabitInfoActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void HabitInfoActivity::loop() {
  // RR both opened this modal (HabitListActivity) and closes it, so it's read on release
  // here too -- this pops the activity, the same kind of transition todoConsumeSlot()'s
  // transitioningSlot exists for (see TodoButtonMapping.h).
  if (todoConsumeSlot(mappedInput, TodoButtonSlot::RR) == TodoButtonSlot::RR) {
    finish();
  }
}

void HabitInfoActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto* habit = TODO_DATA.getHabit(habitId);
  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  constexpr int titleFontId = UI_12_FONT_ID;
  constexpr int bodyFontId = NOTOSANS_16_FONT_ID;
  const int titleLineHeight = renderer.getLineHeight(titleFontId);
  const int bodyLineHeight = renderer.getLineHeight(bodyFontId);

  int y = metrics.topPadding;

  if (!habit) {
    renderer.drawCenteredText(titleFontId, y, tr(STR_TODO_LIST_EMPTY), true);
    const auto hints = todoButtonHints("", "", "", tr(STR_BACK));
    GUI.drawButtonHints(renderer, hints.raw[0], hints.raw[1], hints.raw[2], hints.raw[3]);
    renderer.displayBuffer();
    return;
  }

  renderer.drawCenteredText(titleFontId, y, habit->name.c_str(), true);
  y += titleLineHeight + metrics.verticalSpacing;

  const int32_t tally = TODO_DATA.getHabitWeeklyTally(*habit);
  const bool onTrack = habit->direction == HabitDirection::Build ? tally >= habit->target : tally <= habit->target;

  const char* directionLabel = habit->direction == HabitDirection::Build ? tr(STR_HABIT_BUILD) : tr(STR_HABIT_BREAK);
  renderer.drawText(bodyFontId, metrics.contentSidePadding, y, directionLabel, true);
  y += bodyLineHeight + metrics.verticalSpacing;

  char tallyBuf[64];
  snprintf(tallyBuf, sizeof(tallyBuf), "%s: %ld / %ld", tr(STR_HABIT_THIS_WEEK), static_cast<long>(tally),
           static_cast<long>(habit->target));
  renderer.drawText(bodyFontId, metrics.contentSidePadding, y, tallyBuf, true);
  y += bodyLineHeight + metrics.verticalSpacing;

  const char* statusLabel = onTrack                                     ? tr(STR_HABIT_ON_TRACK)
                            : habit->direction == HabitDirection::Build ? tr(STR_HABIT_BEHIND_TARGET)
                                                                        : tr(STR_HABIT_OVER_TARGET);
  renderer.drawText(bodyFontId, metrics.contentSidePadding, y, statusLabel, true);

  const auto hints = todoButtonHints("", "", "", tr(STR_BACK));
  GUI.drawButtonHints(renderer, hints.raw[0], hints.raw[1], hints.raw[2], hints.raw[3]);

  renderer.displayBuffer();
}
