#include "TodoCategoryActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

void TodoCategoryActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void TodoCategoryActivity::loop() {
  // Placeholder only: real per-category button behavior (docs/design-spec.md
  // Section 5, "Inside a Todo or Checklist list" / "Inside the Habit list") isn't
  // built yet. Back returns to the Home screen, per Section 5's hierarchical Back.
  int x = 0;
  int y = 0;
  if (mappedInput.wasPressed(MappedInputManager::Button::Back) || mappedInput.wasScreenTapped(x, y)) {
    finish();
  }
}

void TodoCategoryActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto height = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (renderer.getScreenHeight() - height) / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, top, todoCategoryTitle(category), true);

  renderer.displayBuffer();
}
