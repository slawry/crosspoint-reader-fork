#include "TodoHomeActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <memory>
#include <string>

#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoCategoryActivity.h"
#include "TodoListActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kCategoryCount = TodoState::TODO_CATEGORY_COUNT;

const char* categoryTitle(int index) {
  switch (index) {
    case TodoState::TODO_CATEGORY_TODOS:
      return tr(STR_TODO_CATEGORY_TODOS);
    case TodoState::TODO_CATEGORY_CHECKLISTS:
      return tr(STR_TODO_CATEGORY_CHECKLISTS);
    case TodoState::TODO_CATEGORY_HABITS:
    default:
      return tr(STR_TODO_CATEGORY_HABITS);
  }
}
}  // namespace

void TodoHomeActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void TodoHomeActivity::loop() {
  // LL/LR (cursor movement) fire on press for responsiveness. RL (activate) fires
  // on release instead: activating can push/pop an Activity, and if that happened
  // on press, the same physical button's still-pending release would land on
  // whatever screen becomes current next -- which is exactly what caused Back to
  // bounce straight back into this screen (the release bled into HomeActivity's
  // own button handling right after the pop). Triggering on release means the
  // press+release cycle is fully spent here first, so nothing is left over.
  const int pressedButton = mappedInput.getPressedFrontButton();
  const int releasedButton = mappedInput.getReleasedFrontButton();

  if (menuBar.focused) {
    if (pressedButton >= 0) {
      const auto slot = todoRawButtonToSlot(pressedButton);
      if (slot == TodoButtonSlot::LL || slot == TodoButtonSlot::LR) {
        menuBar.handleSlot(slot);
        requestUpdate();
        return;
      }
    }
    if (releasedButton >= 0 && todoRawButtonToSlot(releasedButton) == TodoButtonSlot::RL) {
      switch (menuBar.handleSlot(TodoButtonSlot::RL)) {
        case TodoMenuBar::Action::ActivateBack:
          finish();  // Returns to whatever screen pushed this one (Section 5's hierarchical Back).
          return;
        default:
          return;  // Sync/Settings not implemented yet (design-spec.md Sections 8/9).
      }
    }
    return;
  }

  if (pressedButton >= 0) {
    switch (todoRawButtonToSlot(pressedButton)) {
      case TodoButtonSlot::LL:  // up, or into the menu bar from the first item (Section 11)
        if (selectorIndex > 0) {
          selectorIndex--;
        } else {
          menuBar.enter();
        }
        requestUpdate();
        return;
      case TodoButtonSlot::LR:  // down
        if (selectorIndex < kCategoryCount - 1) {
          selectorIndex++;
          requestUpdate();
        }
        return;
      default:
        break;  // RL handled below via release; RR/None are no-ops (Section 5).
    }
  }

  if (releasedButton >= 0 && todoRawButtonToSlot(releasedButton) == TodoButtonSlot::RL) {
    // Enter the highlighted category. Only "To Dos" has real logic so far;
    // Checklists/Habits still use the empty placeholder until their own
    // logic is built (docs/design-spec.md Section 10's integration plan).
    const auto category = static_cast<TodoState::TodoCategory>(selectorIndex);
    if (category == TodoState::TODO_CATEGORY_TODOS) {
      startActivityForResult(std::make_unique<TodoListActivity>(renderer, mappedInput), [](const ActivityResult&) {});
    } else {
      startActivityForResult(std::make_unique<TodoCategoryActivity>(renderer, mappedInput, category),
                             [](const ActivityResult&) {});
    }
  }
}

void TodoHomeActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  // Menu bar rows are condensed (text height + a little padding) rather than the
  // theme's touch-target-sized menuRowHeight, which is meant for HomeActivity's
  // own tap-friendly button menu. Font matches TodoMenuBar::render() (UI_12_FONT_ID).
  const int menuRowHeight = renderer.getLineHeight(UI_12_FONT_ID) + 16;
  const int menuBarHeight = menuBar.rowCount() * menuRowHeight;
  menuBar.render(renderer, 0, metrics.topPadding, pageWidth, menuRowHeight);

  const int dividerY = metrics.topPadding + menuBarHeight + metrics.verticalSpacing / 2;
  renderer.drawLine(metrics.contentSidePadding, dividerY, pageWidth - metrics.contentSidePadding, dividerY);

  // Category rows get extra breathing room below the divider (listRowHeight plus
  // a full verticalSpacing gap, instead of sitting back-to-back).
  const int listTop = dividerY + metrics.verticalSpacing;
  const int rowHeight = metrics.listRowHeight + metrics.verticalSpacing;
  // UI_12_FONT_ID is the largest built-in "UI" family size; NOTOSANS_16_FONT_ID
  // (already loaded globally, no new flash/heap cost) is the closest available
  // size to a 30% increase (12 -> 16pt, ~33%), at the cost of switching typeface
  // from the UI family to the reader's Noto Sans body font for this list only.
  constexpr int categoryFontId = NOTOSANS_16_FONT_ID;
  const int lineHeight = renderer.getLineHeight(categoryFontId);
  for (int i = 0; i < kCategoryCount; i++) {
    const int rowY = listTop + i * rowHeight;
    const bool selected = !menuBar.focused && i == selectorIndex;
    if (selected) {
      renderer.fillRect(0, rowY, pageWidth, rowHeight);
    }
    renderer.drawText(categoryFontId, metrics.contentSidePadding, rowY + (rowHeight - lineHeight) / 2,
                      categoryTitle(i), !selected);
  }

  renderer.displayBuffer();
}
