#pragma once
#include "TodoState.h"
#include "activities/Activity.h"

// Placeholder screen for a Todo-app category (To Dos / Checklists / Habits).
// No list-cycle, list-name row, or task/habit logic yet -- just confirms the
// Home -> category -> Back navigation works. See docs/design-spec.md Section 4.
class TodoCategoryActivity final : public Activity {
  TodoState::TodoCategory category;

 public:
  explicit TodoCategoryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                 TodoState::TodoCategory category)
      : Activity("TodoCategory", renderer, mappedInput), category(category) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  const char* title() const;
};
