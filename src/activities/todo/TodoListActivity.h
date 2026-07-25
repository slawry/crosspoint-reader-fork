#pragma once
#include <string>
#include <vector>

#include "TodoButtonMapping.h"
#include "TodoClock.h"
#include "TodoData.h"
#include "TodoMenuBar.h"
#include "activities/Activity.h"

// Real list screen shared by the To Dos and Checklists categories: category
// list-cycle (Side L/R), list-name row, and task rows (docs/design-spec.md
// Section 4/5/6) -- both categories use the identical interaction model per
// Section 5's "Inside a Todo or Checklist list" row. Habits still use the
// placeholder TodoCategoryActivity until their own logic is built.
class TodoListActivity final : public Activity {
  enum class Focus { ListNameRow, Tasks };

  // The lists currently in this screen's list-cycle, recomputed once per
  // loop()/render() pass (TODO_DATA.getVisibleLists() re-derives checklist
  // scheduling against the live clock, so it isn't cheap to call repeatedly
  // within a single pass). Favourites is only ever present for listType ==
  // Todo (Section 6: "the To Dos category's list-cycle").
  struct CycleView {
    bool hasFavourites = false;
    std::vector<const TodoList*> lists;
  };

  TodoListType listType;
  TodoMenuBar menuBar;
  Focus focus = Focus::ListNameRow;

  // Index into the list-cycle: 0 is the Favourites pseudo-list when it's
  // present, otherwise the real lists start at 0. Recomputed/clamped against
  // a fresh CycleView every loop()/render() pass.
  int cycleIndex = 0;
  int taskCursor = 0;

  // list-name row's complete-all/undo (Section 5/11): a snapshot of the
  // current cycle entry's task-completed states, captured the moment
  // complete-all fires, so undo restores it exactly regardless of any
  // individual edits made in between. -1 means no snapshot is held;
  // switching to a different cycle entry orphans it (guarded by the
  // cycleIndex match in toggleCompleteAll()).
  int completeAllSnapshotCycleIndex = -1;
  std::vector<bool> completeAllSnapshot;

 public:
  explicit TodoListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, TodoListType listType)
      : Activity("TodoList", renderer, mappedInput), listType(listType) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  CycleView buildCycleView(const TodoLocalTime* now) const;
  int cycleEntryCount(const CycleView& view) const;
  bool isFavouritesEntry(int index, const CycleView& view) const;
  const TodoList* listForCycleEntry(int index, const CycleView& view) const;
  std::string cycleEntryName(int index, const CycleView& view) const;
  std::vector<int> currentTaskIds(const CycleView& view) const;
  int currentTaskCount(const CycleView& view) const;
  void clampIndices(const CycleView& view);
  void switchList(int direction, const CycleView& view);
  void toggleCompleteAll(const CycleView& view);

  // Button-hint labels (Section 5's per-context table) for the current focus and
  // selection, in Default (LL/LR/RL/RR) order. `ids` is the current cycle entry's
  // task IDs, as already computed by the caller (currentTaskIds(view)).
  TodoButtonHints buttonHints(const CycleView& view, const std::vector<int>& ids) const;
};
