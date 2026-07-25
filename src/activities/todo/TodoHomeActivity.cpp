#include "TodoHomeActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <memory>
#include <string>

#include "HabitListActivity.h"
#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoCategoryActivity.h"
#include "TodoListActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int kCategoryCount = TodoState::TODO_CATEGORY_COUNT;

// To Dos and Checklists share TodoListActivity (Section 5's "Inside a Todo or
// Checklist list" is one interaction model), parameterized by which list type
// to show. Habits have their own screen (HabitListActivity) since their
// item-row interaction is unrelated to complete/favourite. New categories fail
// to compile here until handled, instead of silently falling through an
// if/else chain that TodoHomeActivity would otherwise need editing every time
// one more category graduates from placeholder to real logic.
std::unique_ptr<Activity> createTodoCategoryScreen(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                    const TodoState::TodoCategory category) {
  switch (category) {
    case TodoState::TODO_CATEGORY_TODOS:
      return std::make_unique<TodoListActivity>(renderer, mappedInput, TodoListType::Todo);
    case TodoState::TODO_CATEGORY_CHECKLISTS:
      return std::make_unique<TodoListActivity>(renderer, mappedInput, TodoListType::Checklist);
    case TodoState::TODO_CATEGORY_HABITS:
      return std::make_unique<HabitListActivity>(renderer, mappedInput);
    default:
      return std::make_unique<TodoCategoryActivity>(renderer, mappedInput, category);
  }
}
}  // namespace

void TodoHomeActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void TodoHomeActivity::loop() {
  if (menuBar.focused) {
    switch (menuBar.processInput(mappedInput)) {
      case TodoMenuBar::Action::ActivateBack:
        finish();  // Returns to whatever screen pushed this one (Section 5's hierarchical Back).
        return;
      case TodoMenuBar::Action::ActivateSync:
      case TodoMenuBar::Action::ActivateSettings:
        return;  // Not implemented yet (design-spec.md Sections 8/9).
      case TodoMenuBar::Action::Consumed:
        requestUpdate();
        return;
      case TodoMenuBar::Action::None:
      default:
        return;
    }
  }

  // "Enter category" pushes an Activity, so it's read on release, not press
  // (see TodoButtonMapping.h's todoConsumeSlot() for why). LL/LR (cursor
  // movement) don't transition, so they still resolve via press.
  switch (todoConsumeSlot(mappedInput, TodoButtonSlot::RL)) {
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
    case TodoButtonSlot::RL: {  // enter highlighted category
      const auto category = static_cast<TodoState::TodoCategory>(selectorIndex);
      startActivityForResult(createTodoCategoryScreen(renderer, mappedInput, category), [](const ActivityResult&) {});
      return;
    }
    case TodoButtonSlot::RR:  // no-op (Section 5)
    case TodoButtonSlot::None:
    default:
      return;
  }
}

void TodoHomeActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();

  // Menu bar rows are condensed (text height + a little padding) rather than the
  // theme's touch-target-sized menuRowHeight, which is meant for HomeActivity's
  // own tap-friendly button menu. Font matches TodoMenuBar's own row font.
  const int menuRowHeight = renderer.getLineHeight(UI_12_FONT_ID) + 16;
  const int listTop = menuBar.renderAndGetContentTop(renderer, 0, metrics.topPadding, pageWidth, menuRowHeight);

  // Category rows get extra breathing room below the divider (listRowHeight plus
  // a full verticalSpacing gap, instead of sitting back-to-back).
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
                      todoCategoryTitle(static_cast<TodoState::TodoCategory>(i)), !selected);
  }

  // Section 5: LL/LR are cursor up/down (LL doubles as "into the menu bar" at the
  // first item), RL enters the highlighted category, RR is a no-op on this screen.
  const char* ll = selectorIndex > 0 ? tr(STR_DIR_UP) : tr(STR_TODO_HINT_MENU);
  const char* lr = selectorIndex < kCategoryCount - 1 ? tr(STR_DIR_DOWN) : "";
  const auto hints = menuBar.focused ? menuBar.hintLabels() : todoButtonHints(ll, lr, tr(STR_SELECT), "");
  GUI.drawButtonHints(renderer, hints.raw[0], hints.raw[1], hints.raw[2], hints.raw[3]);

  renderer.displayBuffer();
}
