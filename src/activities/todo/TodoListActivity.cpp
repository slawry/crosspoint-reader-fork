#include "TodoListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoData.h"
#include "components/UITheme.h"
#include "fontIds.h"

void TodoListActivity::onEnter() {
  Activity::onEnter();
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

int TodoListActivity::cycleEntryCount() const {
  return static_cast<int>(TODO_DATA.getLists().size()) + (TODO_DATA.hasFavourites() ? 1 : 0);
}

bool TodoListActivity::isFavouritesEntry(const int index) const { return TODO_DATA.hasFavourites() && index == 0; }

int TodoListActivity::listIdForCycleEntry(const int index) const {
  if (isFavouritesEntry(index)) return -1;
  const int offset = TODO_DATA.hasFavourites() ? 1 : 0;
  const auto& lists = TODO_DATA.getLists();
  const int idx = index - offset;
  if (idx < 0 || idx >= static_cast<int>(lists.size())) return -1;
  return lists[idx].id;
}

std::string TodoListActivity::cycleEntryName(const int index) const {
  if (isFavouritesEntry(index)) return tr(STR_TODO_FAVOURITES);
  const int listId = listIdForCycleEntry(index);
  for (const auto& list : TODO_DATA.getLists()) {
    if (list.id == listId) return list.name;
  }
  return "";
}

std::vector<int> TodoListActivity::currentTaskIds() const {
  if (isFavouritesEntry(cycleIndex)) return TODO_DATA.getFavouriteTaskIds();
  return TODO_DATA.getTaskIdsInList(listIdForCycleEntry(cycleIndex));
}

void TodoListActivity::clampIndices() {
  const int count = cycleEntryCount();
  if (count <= 0) {
    cycleIndex = 0;
    taskCursor = 0;
    return;
  }
  if (cycleIndex >= count) cycleIndex = count - 1;
  if (cycleIndex < 0) cycleIndex = 0;

  const int taskCount = static_cast<int>(currentTaskIds().size());
  if (taskCursor >= taskCount) taskCursor = std::max(0, taskCount - 1);
  if (taskCursor < 0) taskCursor = 0;
}

void TodoListActivity::switchList(const int direction) {
  const int count = cycleEntryCount();
  if (count <= 1) return;
  cycleIndex = (cycleIndex + direction + count) % count;
  taskCursor = 0;
  requestUpdate();
}

void TodoListActivity::toggleCompleteAll() {
  const auto ids = currentTaskIds();
  if (ids.empty()) return;

  if (hasCompleteAllSnapshot && completeAllSnapshotCycleIndex == cycleIndex) {
    // Undo: restore the exact snapshot taken when complete-all was triggered,
    // regardless of any individual completions toggled since (Section 11).
    TODO_DATA.restoreCompleted(ids, completeAllSnapshot);
    hasCompleteAllSnapshot = false;
  } else {
    completeAllSnapshot = TODO_DATA.snapshotCompleted(ids);
    completeAllSnapshotCycleIndex = cycleIndex;
    hasCompleteAllSnapshot = true;
    TODO_DATA.setAllCompleted(ids, true);
  }
}

void TodoListActivity::loop() {
  clampIndices();

  const int pressedButton = mappedInput.getPressedFrontButton();
  // RL only for actions that push/pop an Activity (see TodoHomeActivity for
  // why: firing on release avoids a stray release bleeding into whatever
  // screen becomes current next). Complete/favourite/complete-all never
  // transition activities, so they stay press-triggered for responsiveness.
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
          finish();  // Returns to the Home screen (Section 5's hierarchical Back).
          return;
        default:
          return;  // Sync/Settings not implemented yet.
      }
    }
    return;
  }

  // Side L/R: switch lists within this category. No-op while the menu bar is
  // focused (handled above), reserved exclusively for this otherwise (Section 5).
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    switchList(-1);
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    switchList(1);
    return;
  }

  if (focus == Focus::ListNameRow) {
    if (pressedButton < 0) return;
    switch (todoRawButtonToSlot(pressedButton)) {
      case TodoButtonSlot::LL:  // up to the menu bar
        menuBar.enter();
        requestUpdate();
        return;
      case TodoButtonSlot::LR:  // down to the first task
        if (!currentTaskIds().empty()) {
          focus = Focus::Tasks;
          taskCursor = 0;
          requestUpdate();
        }
        return;
      case TodoButtonSlot::RL:  // complete all / undo
        toggleCompleteAll();
        requestUpdate();
        return;
      case TodoButtonSlot::RR:  // show list info -- not implemented yet
      case TodoButtonSlot::None:
      default:
        return;
    }
  }

  // Focus::Tasks
  if (pressedButton < 0) return;
  const auto ids = currentTaskIds();
  switch (todoRawButtonToSlot(pressedButton)) {
    case TodoButtonSlot::LL:  // cursor up, or back up to the list-name row from the first task
      if (taskCursor > 0) {
        taskCursor--;
      } else {
        focus = Focus::ListNameRow;
      }
      requestUpdate();
      return;
    case TodoButtonSlot::LR:  // cursor down
      if (taskCursor < static_cast<int>(ids.size()) - 1) {
        taskCursor++;
        requestUpdate();
      }
      return;
    case TodoButtonSlot::RL:  // complete/uncomplete highlighted task
      if (taskCursor >= 0 && taskCursor < static_cast<int>(ids.size())) {
        TODO_DATA.toggleCompleted(ids[taskCursor]);
        requestUpdate();
      }
      return;
    case TodoButtonSlot::RR:  // favourite/unfavourite highlighted task
      if (taskCursor >= 0 && taskCursor < static_cast<int>(ids.size())) {
        TODO_DATA.toggleFavourited(ids[taskCursor]);
        requestUpdate();
      }
      return;
    case TodoButtonSlot::None:
    default:
      return;
  }
}

void TodoListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  constexpr int contentFontId = NOTOSANS_16_FONT_ID;
  const int contentLineHeight = renderer.getLineHeight(contentFontId);

  const int menuRowHeight = renderer.getLineHeight(UI_12_FONT_ID) + 16;
  const int menuBarHeight = menuBar.rowCount() * menuRowHeight;
  menuBar.render(renderer, 0, metrics.topPadding, pageWidth, menuRowHeight);

  const int dividerY = metrics.topPadding + menuBarHeight + metrics.verticalSpacing / 2;
  renderer.drawLine(metrics.contentSidePadding, dividerY, pageWidth - metrics.contentSidePadding, dividerY);

  const int rowHeight = metrics.listRowHeight + metrics.verticalSpacing;
  const int textWidth = pageWidth - metrics.contentSidePadding * 2;

  // List-name row.
  const int nameRowY = dividerY + metrics.verticalSpacing;
  const bool nameRowSelected = !menuBar.focused && focus == Focus::ListNameRow;
  if (nameRowSelected) {
    renderer.fillRect(0, nameRowY, pageWidth, rowHeight);
  }
  const std::string name = cycleEntryCount() > 0 ? cycleEntryName(cycleIndex) : "";
  const auto truncatedName = renderer.truncatedText(contentFontId, name.c_str(), textWidth);
  renderer.drawText(contentFontId, metrics.contentSidePadding, nameRowY + (rowHeight - contentLineHeight) / 2,
                    truncatedName.c_str(), !nameRowSelected);

  // Task rows.
  const auto ids = currentTaskIds();
  const int tasksTop = nameRowY + rowHeight + metrics.verticalSpacing;
  for (int i = 0; i < static_cast<int>(ids.size()); i++) {
    const int rowY = tasksTop + i * rowHeight;
    if (rowY + rowHeight > pageHeight) break;  // No paging yet -- seed data is small enough to fit.

    const bool selected = !menuBar.focused && focus == Focus::Tasks && i == taskCursor;
    if (selected) {
      renderer.fillRect(0, rowY, pageWidth, rowHeight);
    }

    auto* task = TODO_DATA.getTask(ids[i]);
    if (!task) continue;

    std::string label = (task->completed ? "[x] " : "[ ] ") + task->title;
    if (task->favourited) label += " *";
    const auto truncated = renderer.truncatedText(contentFontId, label.c_str(), textWidth);
    renderer.drawText(contentFontId, metrics.contentSidePadding, rowY + (rowHeight - contentLineHeight) / 2,
                      truncated.c_str(), !selected);
  }

  renderer.displayBuffer();
}
