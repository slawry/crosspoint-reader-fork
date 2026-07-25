#pragma once
#include "activities/Activity.h"

// Minimal placeholder screen: renders only the "Todo App" string.
// No list logic, data model, or persistence yet.
class TodoActivity final : public Activity {
 public:
  explicit TodoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Todo", renderer, mappedInput) {}
  void loop() override;
  void render(RenderLock&&) override;
};
