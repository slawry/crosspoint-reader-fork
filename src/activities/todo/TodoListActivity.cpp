#include "TodoListActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "TodoButtonMapping.h"
#include "TodoClock.h"
#include "TodoData.h"
#include "components/UITheme.h"
#include "fontIds.h"

TodoListActivity::CycleView TodoListActivity::buildCycleView(const TodoLocalTime* now) const {
  CycleView view;
  view.hasFavourites = (listType == TodoListType::Todo) && TODO_DATA.hasFavourites();
  view.lists = TODO_DATA.getVisibleLists(listType, now);
  return view;
}

int TodoListActivity::cycleEntryCount(const CycleView& view) const {
  return static_cast<int>(view.lists.size()) + (view.hasFavourites ? 1 : 0);
}

bool TodoListActivity::isFavouritesEntry(const int index, const CycleView& view) const {
  return view.hasFavourites && index == 0;
}

const TodoList* TodoListActivity::listForCycleEntry(const int index, const CycleView& view) const {
  if (isFavouritesEntry(index, view)) return nullptr;
  const int offset = view.hasFavourites ? 1 : 0;
  const int idx = index - offset;
  if (idx < 0 || idx >= static_cast<int>(view.lists.size())) return nullptr;
  return view.lists[idx];
}

std::string TodoListActivity::cycleEntryName(const int index, const CycleView& view) const {
  if (isFavouritesEntry(index, view)) return tr(STR_TODO_FAVOURITES);
  const TodoList* list = listForCycleEntry(index, view);
  return list ? list->name : "";
}

std::vector<int> TodoListActivity::currentTaskIds(const CycleView& view) const {
  if (isFavouritesEntry(cycleIndex, view)) return TODO_DATA.getFavouriteTaskIds();
  const TodoList* list = listForCycleEntry(cycleIndex, view);
  return list ? TODO_DATA.getTaskIdsInList(list->id) : std::vector<int>{};
}

int TodoListActivity::currentTaskCount(const CycleView& view) const {
  if (isFavouritesEntry(cycleIndex, view)) return TODO_DATA.getFavouriteTaskCount();
  const TodoList* list = listForCycleEntry(cycleIndex, view);
  return list ? TODO_DATA.getTaskCountInList(list->id) : 0;
}

void TodoListActivity::clampIndices(const CycleView& view) {
  const int count = cycleEntryCount(view);
  if (count <= 0) {
    cycleIndex = 0;
    taskCursor = 0;
    return;
  }
  cycleIndex = std::clamp(cycleIndex, 0, count - 1);

  const int taskCount = currentTaskCount(view);
  taskCursor = taskCount > 0 ? std::clamp(taskCursor, 0, taskCount - 1) : 0;
}

void TodoListActivity::switchList(const int direction, const CycleView& view) {
  const int count = cycleEntryCount(view);
  if (count <= 1) return;
  cycleIndex = (cycleIndex + direction + count) % count;
  taskCursor = 0;
  requestUpdate();
}

void TodoListActivity::toggleCompleteAll(const CycleView& view) {
  const auto ids = currentTaskIds(view);
  if (ids.empty()) return;

  if (completeAllSnapshotCycleIndex == cycleIndex) {
    // Undo: restore the exact snapshot taken when complete-all was triggered,
    // regardless of any individual completions toggled since (Section 11).
    TODO_DATA.restoreCompleted(ids, completeAllSnapshot);
    completeAllSnapshotCycleIndex = -1;
  } else {
    completeAllSnapshot = TODO_DATA.snapshotCompleted(ids);
    completeAllSnapshotCycleIndex = cycleIndex;
    TODO_DATA.setAllCompleted(ids, true);
  }
}

void TodoListActivity::onEnter() {
  Activity::onEnter();
  TodoLocalTime now;
  if (getTodoLocalTime(now)) TODO_DATA.applyChecklistResets(now.dayKey);
  requestUpdateAndWait();  // Draw immediately instead of waiting for the next input event.
}

void TodoListActivity::loop() {
  TodoLocalTime now;
  const bool hasClock = getTodoLocalTime(now);
  // Reset-and-drop (Section 6): a no-op for Todo-type lists and for
  // checklists already reset today. Applied here (not in buildCycleView(),
  // which only reads state) so it stays an explicit, once-per-pass step.
  if (hasClock) TODO_DATA.applyChecklistResets(now.dayKey);
  const CycleView view = buildCycleView(hasClock ? &now : nullptr);
  clampIndices(view);

  if (menuBar.focused) {
    switch (menuBar.processInput(mappedInput)) {
      case TodoMenuBar::Action::ActivateBack:
        finish();  // Returns to the Home screen (Section 5's hierarchical Back).
        return;
      case TodoMenuBar::Action::ActivateSync:
      case TodoMenuBar::Action::ActivateSettings:
        return;  // Not implemented yet.
      case TodoMenuBar::Action::Consumed:
        requestUpdate();
        return;
      case TodoMenuBar::Action::None:
      default:
        return;
    }
  }

  // Side L/R: switch lists within this category. No-op while the menu bar is
  // focused (handled above), reserved exclusively for this otherwise (Section 5).
  if (mappedInput.wasPressed(MappedInputManager::Button::Up)) {
    switchList(-1, view);
    return;
  }
  if (mappedInput.wasPressed(MappedInputManager::Button::Down)) {
    switchList(1, view);
    return;
  }

  // Nothing below transitions to a different Activity, so every slot here is
  // press-triggered (see TodoButtonMapping.h's todoConsumeSlot()).
  const TodoButtonSlot slot = todoConsumeSlot(mappedInput);
  if (slot == TodoButtonSlot::None) return;

  if (focus == Focus::ListNameRow) {
    switch (slot) {
      case TodoButtonSlot::LL:  // up to the menu bar
        menuBar.enter();
        requestUpdate();
        return;
      case TodoButtonSlot::LR:  // down to the first task
        if (currentTaskCount(view) > 0) {
          focus = Focus::Tasks;
          taskCursor = 0;
          requestUpdate();
        }
        return;
      case TodoButtonSlot::RL:  // complete all / undo
        toggleCompleteAll(view);
        requestUpdate();
        return;
      case TodoButtonSlot::RR:  // show list info -- not implemented yet
      default:
        return;
    }
  }

  // Focus::Tasks
  const auto ids = currentTaskIds(view);
  switch (slot) {
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
    case TodoButtonSlot::RR:  // favourite/unfavourite highlighted task -- Todo lists only (Section 6: the
                              // Favourites filter is scoped to "all Todo lists"; no-op inside a Checklist)
      if (listType == TodoListType::Todo && taskCursor >= 0 && taskCursor < static_cast<int>(ids.size())) {
        TODO_DATA.toggleFavourited(ids[taskCursor]);
        requestUpdate();
      }
      return;
    default:
      return;
  }
}

TodoButtonHints TodoListActivity::buttonHints(const CycleView& view, const std::vector<int>& ids) const {
  if (focus == Focus::ListNameRow) {
    const char* lr = currentTaskCount(view) > 0 ? tr(STR_DIR_DOWN) : "";
    const char* rl =
        ids.empty() ? "" : (completeAllSnapshotCycleIndex == cycleIndex ? tr(STR_TODO_HINT_UNDO)
                                                                        : tr(STR_TODO_HINT_COMPLETE));
    return todoButtonHints(tr(STR_TODO_HINT_MENU), lr, rl, "");
  }

  // Focus::Tasks
  const bool hasSelection = taskCursor >= 0 && taskCursor < static_cast<int>(ids.size());
  const auto* task = hasSelection ? TODO_DATA.getTask(ids[taskCursor]) : nullptr;
  const char* lr = taskCursor < static_cast<int>(ids.size()) - 1 ? tr(STR_DIR_DOWN) : "";
  const char* rl = task ? (task->completed ? tr(STR_TODO_HINT_UNCOMPLETE) : tr(STR_TODO_HINT_COMPLETE)) : "";
  // RR (favourite/unfavourite) is Todo-only, matching the toggleFavourited() gate in loop().
  const char* rr = (listType == TodoListType::Todo && task)
                       ? (task->favourited ? tr(STR_TODO_HINT_UNFAVOURITE) : tr(STR_TODO_HINT_FAVOURITE))
                       : "";
  return todoButtonHints(tr(STR_DIR_UP), lr, rl, rr);
}

void TodoListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  TodoLocalTime now;
  const bool hasClock = getTodoLocalTime(now);
  const CycleView view = buildCycleView(hasClock ? &now : nullptr);

  const auto& metrics = UITheme::getInstance().getMetrics();
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  constexpr int contentFontId = NOTOSANS_16_FONT_ID;
  const int contentLineHeight = renderer.getLineHeight(contentFontId);

  const int menuRowHeight = renderer.getLineHeight(UI_12_FONT_ID) + 16;
  const int nameRowY = menuBar.renderAndGetContentTop(renderer, 0, metrics.topPadding, pageWidth, menuRowHeight);

  const int rowHeight = metrics.listRowHeight + metrics.verticalSpacing;
  const int textWidth = pageWidth - metrics.contentSidePadding * 2;
  const int entryCount = cycleEntryCount(view);

  // List-name row.
  const bool nameRowSelected = !menuBar.focused && focus == Focus::ListNameRow;
  if (nameRowSelected) {
    renderer.fillRect(0, nameRowY, pageWidth, rowHeight);
  }
  const std::string name = entryCount > 0 ? cycleEntryName(cycleIndex, view) : tr(STR_TODO_LIST_EMPTY);
  const auto truncatedName = renderer.truncatedText(contentFontId, name.c_str(), textWidth);
  renderer.drawText(contentFontId, metrics.contentSidePadding, nameRowY + (rowHeight - contentLineHeight) / 2,
                    truncatedName.c_str(), !nameRowSelected);

  // Task rows.
  const auto ids = entryCount > 0 ? currentTaskIds(view) : std::vector<int>{};
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

  const auto hints = menuBar.focused ? menuBar.hintLabels() : buttonHints(view, ids);
  GUI.drawButtonHints(renderer, hints.raw[0], hints.raw[1], hints.raw[2], hints.raw[3]);

  renderer.displayBuffer();
}
