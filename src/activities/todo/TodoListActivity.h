#pragma once
#include <vector>

#include "TodoMenuBar.h"
#include "activities/Activity.h"

// Real Todo list screen: category list-cycle (Side L/R), list-name row, and
// task rows (docs/design-spec.md Section 4/5/6). Currently only the "To Dos"
// category uses this; Checklists/Habits still use the placeholder
// TodoCategoryActivity until their own logic is built.
class TodoListActivity final : public Activity {
  enum class Focus { ListNameRow, Tasks };

  TodoMenuBar menuBar;
  Focus focus = Focus::ListNameRow;

  // Index into the list-cycle: 0 is the Favourites pseudo-list when it's
  // present (TodoDataStore::hasFavourites()), otherwise the real lists
  // start at 0. Recomputed/clamped against live data every loop()/render().
  int cycleIndex = 0;
  int taskCursor = 0;

  // list-name row's complete-all/undo (Section 5/11): a snapshot of the
  // current cycle entry's task-completed states, captured the moment
  // complete-all fires, so undo restores it exactly regardless of any
  // individual edits made in between. Cleared when the cycle entry changes.
  bool hasCompleteAllSnapshot = false;
  int completeAllSnapshotCycleIndex = -1;
  std::vector<bool> completeAllSnapshot;

 public:
  explicit TodoListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("TodoList", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  int cycleEntryCount() const;
  bool isFavouritesEntry(int index) const;
  int listIdForCycleEntry(int index) const;
  std::string cycleEntryName(int index) const;
  std::vector<int> currentTaskIds() const;
  void clampIndices();
  void switchList(int direction);
  void toggleCompleteAll();
};
