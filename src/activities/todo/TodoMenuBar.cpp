#include "TodoMenuBar.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "components/UITheme.h"
#include "fontIds.h"

TodoMenuBar::Action TodoMenuBar::handleSlot(const TodoButtonSlot slot) {
  switch (slot) {
    case TodoButtonSlot::LL:  // up: Back -> Sync -> Settings
      if (selectorIndex > ENTRY_SETTINGS) selectorIndex--;
      return Action::None;
    case TodoButtonSlot::LR:  // down: Settings -> Sync -> Back, then out to the content below
      if (selectorIndex < ENTRY_BACK) {
        selectorIndex++;
        return Action::None;
      }
      focused = false;
      return Action::ExitToContent;
    case TodoButtonSlot::RL:  // activate highlighted entry
      switch (selectorIndex) {
        case ENTRY_BACK:
          return Action::ActivateBack;
        case ENTRY_SYNC:
          return Action::ActivateSync;
        case ENTRY_SETTINGS:
        default:
          return Action::ActivateSettings;
      }
    case TodoButtonSlot::RR:  // tooltip -- not implemented yet
    case TodoButtonSlot::None:
    default:
      return Action::None;
  }
}

void TodoMenuBar::render(const GfxRenderer& renderer, const int x, const int y, const int width,
                         const int rowHeight) const {
  const auto& metrics = UITheme::getInstance().getMetrics();
  // When collapsed, only Back is drawn, at the row position it occupies when
  // expanded (immediately above the divider/content).
  const int firstEntry = focused ? ENTRY_SETTINGS : ENTRY_BACK;

  for (int entry = firstEntry; entry < ENTRY_COUNT; entry++) {
    const int row = entry - firstEntry;
    const int rowY = y + row * rowHeight;
    const bool selected = focused && entry == selectorIndex;
    if (selected) {
      renderer.fillRect(x, rowY, width, rowHeight);
    }
    const char* label = (entry == ENTRY_SETTINGS) ? tr(STR_SETTINGS_TITLE)
                        : (entry == ENTRY_SYNC)    ? tr(STR_TODO_MENU_SYNC)
                                                    : tr(STR_TODO_MENU_BACK);
    renderer.drawText(UI_12_FONT_ID, x + metrics.contentSidePadding,
                      rowY + (rowHeight - renderer.getLineHeight(UI_12_FONT_ID)) / 2, label, !selected);
  }
}
