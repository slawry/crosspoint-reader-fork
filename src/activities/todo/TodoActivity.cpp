#include "TodoActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "fontIds.h"

void TodoActivity::loop() {
  if (mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    onGoHome();
  }
}

void TodoActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto height = renderer.getLineHeight(UI_10_FONT_ID);
  const auto top = (renderer.getScreenHeight() - height) / 2;
  renderer.drawCenteredText(UI_10_FONT_ID, top, tr(STR_TODO_APP), true);

  renderer.displayBuffer();
}
