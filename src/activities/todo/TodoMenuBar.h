#pragma once
#include "TodoButtonMapping.h"

class GfxRenderer;

// Todo app menu bar: collapsed to a single "Back" row above a screen's own
// content; expands into a vertical Settings/Sync/Back stack (top to bottom)
// once focused, pushing the content below it down to make room.
//
// Not an Activity: the host screen embeds one and folds its render()/loop()
// calls into its own, so activating "Back" is just that screen's own
// finish() -- no separate push/pop level, no multi-level back-stack to
// unwind. See docs/design-spec.md Section 4/5/11.
class TodoMenuBar {
 public:
  enum class Action { None, ExitToContent, ActivateBack, ActivateSync, ActivateSettings };
  // Vertical order when expanded, top to bottom: Settings, Sync, Back.
  enum Entry { ENTRY_SETTINGS = 0, ENTRY_SYNC = 1, ENTRY_BACK = 2, ENTRY_COUNT = 3 };

  bool focused = false;
  int selectorIndex = ENTRY_BACK;

  // Shifts focus into the bar, landing on Back (closest to the content it came from).
  void enter() {
    focused = true;
    selectorIndex = ENTRY_BACK;
  }

  // Handles a button slot while the bar has focus. Returns ExitToContent when
  // LR at Back (the bottom-most entry) should hand focus back to the host's content.
  Action handleSlot(TodoButtonSlot slot);

  // Rows currently rendered: 1 when collapsed, ENTRY_COUNT when focused.
  int rowCount() const { return focused ? ENTRY_COUNT : 1; }

  // rowHeight: pixel height of a single row (e.g. ThemeMetrics::menuRowHeight).
  void render(const GfxRenderer& renderer, int x, int y, int width, int rowHeight) const;
};
