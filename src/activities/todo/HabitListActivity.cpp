#include "HabitListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdio>

#include "HabitInfoActivity.h"
#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoClock.h"
#include "TodoData.h"
#include "components/UITheme.h"
#include "fontIds.h"

HabitListActivity::CycleView HabitListActivity::buildCycleView() const {
  CycleView view;
  view.lists = TODO_DATA.getVisibleLists(TodoListType::Habit, nullptr);  // Habit lists have no schedule.
  return view;
}

int HabitListActivity::cycleEntryCount(const CycleView& view) const { return static_cast<int>(view.lists.size()); }

const TodoList* HabitListActivity::listForCycleEntry(const int index, const CycleView& view) const {
  if (index < 0 || index >= static_cast<int>(view.lists.size())) return nullptr;
  return view.lists[index];
}

std::string HabitListActivity::cycleEntryName(const int index, const CycleView& view) const {
  const TodoList* list = listForCycleEntry(index, view);
  return list ? list->name : "";
}

std::vector<int> HabitListActivity::currentHabitIds(const CycleView& view) const {
  const TodoList* list = listForCycleEntry(cycleIndex, view);
  return list ? TODO_DATA.getHabitIdsInList(list->id) : std::vector<int>{};
}

int HabitListActivity::currentHabitCount(const CycleView& view) const {
  const TodoList* list = listForCycleEntry(cycleIndex, view);
  return list ? TODO_DATA.getHabitCountInList(list->id) : 0;
}

void HabitListActivity::clampIndices(const CycleView& view) {
  const int count = cycleEntryCount(view);
  if (count <= 0) {
    cycleIndex = 0;
    habitCursor = 0;
    return;
  }
  cycleIndex = std::clamp(cycleIndex, 0, count - 1);

  const int habitCount = currentHabitCount(view);
  habitCursor = habitCount > 0 ? std::clamp(habitCursor, 0, habitCount - 1) : 0;
}

void HabitListActivity::switchList(const int direction, const CycleView& view) {
  const int count = cycleEntryCount(view);
  if (count <= 1) return;
  cycleIndex = (cycleIndex + direction + count) % count;
  habitCursor = 0;
  requestUpdate();
}

void HabitListActivity::onEnter() {
  Activity::onEnter();
  TodoLocalTime now;
  if (getTodoLocalTime(now)) TODO_DATA.applyHabitRollovers(now);
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void HabitListActivity::loop() {
  // Weekly rollover (Section 6): a no-op for habits already tracking the current week.
  // Applied here (not in buildCycleView(), which only reads state) so it stays an
  // explicit, once-per-pass step -- mirrors TodoListActivity's applyChecklistResets().
  TodoLocalTime now;
  const bool hasClock = getTodoLocalTime(now);
  if (hasClock) TODO_DATA.applyHabitRollovers(now);
  const CycleView view = buildCycleView();
  clampIndices(view);

  if (menuBar.focused) {
    switch (menuBar.processInput(mappedInput)) {
      case TodoMenuBar::Action::ActivateBack:
        finish();  // Returns to the Home screen (Section 5's hierarchical Back).
        return;
      case TodoMenuBar::Action::ActivateSync:
      case TodoMenuBar::Action::ActivateSettings:
        return;  // Not implemented yet.
      case TodoMenuBar::Action::Consumed:
        requestUpdate();
        return;
      case TodoMenuBar::Action::None:
      default:
        return;
    }
  }

  // Side L/R: switch lists within this category. No-op while the menu bar is
  // focused (handled above), reserved exclusively for this otherwise (Section 5).
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    switchList(-1, view);
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    switchList(1, view);
    return;
  }

  // RR pushes the weekly info modal (a real Activity transition) only while focused on
  // a habit row, so it's the only slot read from release there (see TodoButtonMapping.h's
  // todoConsumeSlot() for why). Every other slot here is press-triggered.
  const TodoButtonSlot transitioningSlot = (focus == Focus::Habits) ? TodoButtonSlot::RR : TodoButtonSlot::None;
  const TodoButtonSlot slot = todoConsumeSlot(mappedInput, transitioningSlot);
  if (slot == TodoButtonSlot::None) return;

  if (focus == Focus::ListNameRow) {
    switch (slot) {
      case TodoButtonSlot::LL:  // up to the menu bar
        menuBar.enter();
        requestUpdate();
        return;
      case TodoButtonSlot::LR:  // down to the first habit
        if (currentHabitCount(view) > 0) {
          focus = Focus::Habits;
          habitCursor = 0;
          requestUpdate();
        }
        return;
      case TodoButtonSlot::RL:  // complete all / undo -- not defined for habits (Section 6 has
                                // no "complete" concept for quantity habits), left as a no-op
      case TodoButtonSlot::RR:  // show list info -- not implemented yet
      default:
        return;
    }
  }

  // Focus::Habits
  const auto ids = currentHabitIds(view);
  switch (slot) {
    case TodoButtonSlot::LL:  // cursor up, or back up to the list-name row from the first habit
      if (habitCursor > 0) {
        habitCursor--;
      } else {
        focus = Focus::ListNameRow;
      }
      requestUpdate();
      return;
    case TodoButtonSlot::LR:  // cursor down
      if (habitCursor < static_cast<int>(ids.size()) - 1) {
        habitCursor++;
        requestUpdate();
      }
      return;
    case TodoButtonSlot::RL:  // boolean: toggle today. quantity: +1 today (Section 5/6). Requires
                              // the RTC for "today"'s day-key/weekday -- not scheduling, just
                              // bookkeeping (Section 7 only rules out due-time/reminder logic).
                              // Rollover for `now` was already applied above this pass.
      if (hasClock && habitCursor >= 0 && habitCursor < static_cast<int>(ids.size())) {
        TODO_DATA.logHabitToday(ids[habitCursor], now);
        requestUpdate();
      }
      return;
    case TodoButtonSlot::RR:  // open the weekly info modal (Section 5: replaces favouriting, which
                              // doesn't apply to habits)
      if (habitCursor >= 0 && habitCursor < static_cast<int>(ids.size())) {
        startActivityForResult(std::make_unique<HabitInfoActivity>(renderer, mappedInput, ids[habitCursor]),
                               [](const ActivityResult&) {});
      }
      return;
    default:
      return;
  }
}

TodoButtonHints HabitListActivity::buttonHints(const CycleView& view, const std::vector<int>& ids) const {
  if (focus == Focus::ListNameRow) {
    const char* lr = currentHabitCount(view) > 0 ? tr(STR_DIR_DOWN) : "";
    // RL (complete all) and RR (list info) are both not implemented for habit lists --
    // see the matching case in loop().
    return todoButtonHints(tr(STR_TODO_HINT_MENU), lr, "", "");
  }

  // Focus::Habits
  const bool hasSelection = habitCursor >= 0 && habitCursor < static_cast<int>(ids.size());
  const auto* habit = hasSelection ? TODO_DATA.getHabit(ids[habitCursor]) : nullptr;
  const char* lr = habitCursor < static_cast<int>(ids.size()) - 1 ? tr(STR_DIR_DOWN) : "";
  const char* rl = habit ? (habit->mode == HabitMode::Boolean ? tr(STR_TOGGLE) : "+1") : "";
  const char* rr = habit ? tr(STR_TODO_HINT_INFO) : "";
  return todoButtonHints(tr(STR_DIR_UP), lr, rl, rr);
}

void HabitListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const CycleView view = buildCycleView();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  constexpr int contentFontId = NOTOSANS_16_FONT_ID;
  const int contentLineHeight = renderer.getLineHeight(contentFontId);

  const int menuRowHeight = renderer.getLineHeight(UI_12_FONT_ID) + 16;
  const int nameRowY = menuBar.renderAndGetContentTop(renderer, 0, metrics.topPadding, pageWidth, menuRowHeight);

  const int rowHeight = metrics.listRowHeight + metrics.verticalSpacing;
  const int textWidth = pageWidth - metrics.contentSidePadding * 2;
  const int entryCount = cycleEntryCount(view);

  // List-name row.
  const bool nameRowSelected = !menuBar.focused && focus == Focus::ListNameRow;
  if (nameRowSelected) {
    renderer.fillRect(0, nameRowY, pageWidth, rowHeight);
  }
  const std::string name = entryCount > 0 ? cycleEntryName(cycleIndex, view) : tr(STR_TODO_LIST_EMPTY);
  const auto truncatedName = renderer.truncatedText(contentFontId, name.c_str(), textWidth);
  renderer.drawText(contentFontId, metrics.contentSidePadding, nameRowY + (rowHeight - contentLineHeight) / 2,
                    truncatedName.c_str(), !nameRowSelected);

  // Habit rows.
  TodoLocalTime now;
  const bool hasClock = getTodoLocalTime(now);
  const auto ids = entryCount > 0 ? currentHabitIds(view) : std::vector<int>{};
  const int habitsTop = nameRowY + rowHeight + metrics.verticalSpacing;
  for (int i = 0; i < static_cast<int>(ids.size()); i++) {
    const int rowY = habitsTop + i * rowHeight;
    if (rowY + rowHeight > pageHeight) break;  // No paging yet -- seed data is small enough to fit.

    const bool selected = !menuBar.focused && focus == Focus::Habits && i == habitCursor;
    if (selected) {
      renderer.fillRect(0, rowY, pageWidth, rowHeight);
    }

    const auto* habit = TODO_DATA.getHabit(ids[i]);
    if (!habit) continue;

    const int32_t tally = TODO_DATA.getHabitWeeklyTally(*habit);
    char statusBuf[24];
    snprintf(statusBuf, sizeof(statusBuf), "%ld/%ld", static_cast<long>(tally), static_cast<long>(habit->target));

    std::string label;
    if (habit->mode == HabitMode::Boolean) {
      const auto* today = hasClock ? TODO_DATA.getHabitTodayEntry(*habit, now) : nullptr;
      label = (today && today->completed ? "[x] " : "[ ] ") + habit->name;
    } else {
      label = habit->name;
    }
    label += "  ";
    label += statusBuf;

    const auto truncated = renderer.truncatedText(contentFontId, label.c_str(), textWidth);
    renderer.drawText(contentFontId, metrics.contentSidePadding, rowY + (rowHeight - contentLineHeight) / 2,
                      truncated.c_str(), !selected);
  }

  const auto hints = menuBar.focused ? menuBar.hintLabels() : buttonHints(view, ids);
  GUI.drawButtonHints(renderer, hints.raw[0], hints.raw[1], hints.raw[2], hints.raw[3]);

  renderer.displayBuffer();
}
