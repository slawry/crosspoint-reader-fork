#include "TodoMenuBar.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

TodoMenuBar::Action TodoMenuBar::handleSlot(const TodoButtonSlot slot) {
  switch (slot) {
    case TodoButtonSlot::LL:  // up: Back -> Sync -> Settings
      if (selectorIndex > ENTRY_SETTINGS) selectorIndex--;
      return Action::Consumed;
    case TodoButtonSlot::LR:  // down: Settings -> Sync -> Back, then out to the content below
      if (selectorIndex < ENTRY_BACK) {
        selectorIndex++;
      } else {
        focused = false;
      }
      return Action::Consumed;
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

TodoMenuBar::Action TodoMenuBar::processInput(const MappedInputManager& mappedInput) {
  const TodoButtonSlot slot = todoConsumeSlot(mappedInput, TodoButtonSlot::RL);
  if (slot == TodoButtonSlot::None) return Action::None;
  return handleSlot(slot);
}

TodoButtonHints TodoMenuBar::hintLabels() const {
  // LL is a no-op at the topmost entry (Settings); LR always does something --
  // moves down, or (at the bottom-most entry) closes the bar -- so it's never hidden.
  return todoButtonHints(selectorIndex > ENTRY_SETTINGS ? tr(STR_DIR_UP) : "", tr(STR_DIR_DOWN), tr(STR_SELECT), "");
}

int TodoMenuBar::renderAndGetContentTop(const GfxRenderer& renderer, const int x, const int y, const int width,
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

  const int dividerY = y + rowCount() * rowHeight + metrics.verticalSpacing / 2;
  renderer.drawLine(x + metrics.contentSidePadding, dividerY, x + width - metrics.contentSidePadding, dividerY);
  return dividerY + metrics.verticalSpacing;
}
