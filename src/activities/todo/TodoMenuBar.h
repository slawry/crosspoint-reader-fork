#pragma once
#include "TodoButtonMapping.h"

class GfxRenderer;
class MappedInputManager;

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
  enum class Action { None, Consumed, ActivateBack, ActivateSync, ActivateSettings };
  // Vertical order when expanded, top to bottom: Settings, Sync, Back.
  enum Entry { ENTRY_SETTINGS = 0, ENTRY_SYNC = 1, ENTRY_BACK = 2, ENTRY_COUNT = 3 };

  bool focused = false;
  int selectorIndex = ENTRY_BACK;

  // Shifts focus into the bar, landing on Back (closest to the content it came from).
  void enter() {
    focused = true;
    selectorIndex = ENTRY_BACK;
  }

  // Call once per loop() while focused == true. Handles LL/LR (press-driven
  // movement) and RL (release-driven activation -- see TodoButtonMapping.h's
  // todoConsumeSlot() for why) internally, including exiting focus when LR is
  // pressed at the bottom-most entry. Returns the entry to activate, if any.
  Action processInput(const MappedInputManager& mappedInput);

  // Rows currently rendered: 1 when collapsed, ENTRY_COUNT when focused.
  int rowCount() const { return focused ? ENTRY_COUNT : 1; }

  // Draws the bar and the divider line below it, using rowHeight-tall rows.
  // Returns the y coordinate the host's own content should start at.
  int renderAndGetContentTop(const GfxRenderer& renderer, int x, int y, int width, int rowHeight) const;

  // Button-hint labels (LL/LR/RL/RR, Default order) for the bar's current state.
  // Host screens use this instead of their own hints whenever `focused` is true --
  // see TodoButtonMapping.h's todoButtonHints() for turning this into raw physical
  // order for GUI.drawButtonHints().
  TodoButtonHints hintLabels() const;

 private:
  // Handles a single button slot while the bar has focus.
  Action handleSlot(TodoButtonSlot slot);
};
