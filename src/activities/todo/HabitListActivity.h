#pragma once
#include <string>
#include <vector>

#include "TodoButtonMapping.h"
#include "TodoClock.h"
#include "TodoData.h"
#include "TodoMenuBar.h"
#include "activities/Activity.h"

// Real list screen for the Habits category: category list-cycle (Side L/R), list-name
// row, and habit rows (docs/design-spec.md Section 4/5/6). Kept separate from
// TodoListActivity rather than folded into it: habits have no Favourites filter and
// their item-row interaction (RL toggles/logs, RR opens a weekly info modal) is
// unrelated to Todo/Checklist's complete/favourite pair, so sharing that class would
// mean branching its Task-shaped internals around a fundamentally different item type.
class HabitListActivity final : public Activity {
  enum class Focus { ListNameRow, Habits };

  // The Habit-type lists currently in this screen's list-cycle, recomputed once per
  // loop()/render() pass. Habit lists have no schedule (unlike checklists), so this is
  // just insertion order.
  struct CycleView {
    std::vector<const TodoList*> lists;
  };

  TodoMenuBar menuBar;
  Focus focus = Focus::ListNameRow;

  int cycleIndex = 0;
  int habitCursor = 0;

 public:
  explicit HabitListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("HabitList", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  CycleView buildCycleView() const;
  int cycleEntryCount(const CycleView& view) const;
  const TodoList* listForCycleEntry(int index, const CycleView& view) const;
  std::string cycleEntryName(int index, const CycleView& view) const;
  std::vector<int> currentHabitIds(const CycleView& view) const;
  int currentHabitCount(const CycleView& view) const;
  void clampIndices(const CycleView& view);
  void switchList(int direction, const CycleView& view);

  // Button-hint labels (Section 5's "Inside the Habit list" table) for the current
  // focus and selection, in Default (LL/LR/RL/RR) order. `ids` is the current cycle
  // entry's habit IDs, as already computed by the caller (currentHabitIds(view)).
  TodoButtonHints buttonHints(const CycleView& view, const std::vector<int>& ids) const;
};
