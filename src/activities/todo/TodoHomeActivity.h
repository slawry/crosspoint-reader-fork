#pragma once
#include "TodoMenuBar.h"
#include "TodoState.h"
#include "activities/Activity.h"

// Todo app Home screen: a 3-entry list (To Dos / Checklists / Habits) with a
// collapsible menu bar (Back / Sync / Settings) above it.
// See docs/design-spec.md Section 4 (screen hierarchy) and Section 5 (button behavior).
class TodoHomeActivity final : public Activity {
  int selectorIndex = TodoState::TODO_CATEGORY_TODOS;
  TodoMenuBar menuBar;

 public:
  explicit TodoHomeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("TodoHome", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
};
