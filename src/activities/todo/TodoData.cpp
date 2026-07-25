#include "TodoData.h"

#include <algorithm>

int TodoDataStore::addList(const std::string& name) {
  const int id = nextListId++;
  TodoList list;
  list.id = id;
  list.name = name;
  lists.push_back(std::move(list));
  return id;
}

int TodoDataStore::addChecklist(const std::string& name, const ChecklistFrequency frequency,
                                const ChecklistDisplayMode displayMode, const uint8_t windowStartHour,
                                const uint8_t windowStartMinute, const uint8_t windowEndHour,
                                const uint8_t windowEndMinute, const uint8_t customDays) {
  const int id = nextListId++;
  TodoList list;
  list.id = id;
  list.name = name;
  list.type = TodoListType::Checklist;
  list.frequency = frequency;
  list.customDays = customDays;
  list.displayMode = displayMode;
  list.windowStartHour = windowStartHour;
  list.windowStartMinute = windowStartMinute;
  list.windowEndHour = windowEndHour;
  list.windowEndMinute = windowEndMinute;
  lists.push_back(std::move(list));
  return id;
}

bool TodoDataStore::isListCurrentlyVisible(const TodoList& list, const TodoLocalTime& now) const {
  if (list.type != TodoListType::Checklist) return true;  // Todo lists have no schedule.

  bool dayMatches = false;
  switch (list.frequency) {
    case ChecklistFrequency::Daily:
      dayMatches = true;
      break;
    case ChecklistFrequency::Weekdays:
      dayMatches = now.weekday >= 1 && now.weekday <= 5;
      break;
    case ChecklistFrequency::Weekends:
      dayMatches = now.weekday == 0 || now.weekday == 6;
      break;
    case ChecklistFrequency::Custom:
      dayMatches = (list.customDays & (1 << now.weekday)) != 0;
      break;
  }
  if (!dayMatches) return false;

  if (list.displayMode == ChecklistDisplayMode::UntilAllComplete) return true;

  const int nowMinutes = now.hour * 60 + now.minute;
  const int startMinutes = list.windowStartHour * 60 + list.windowStartMinute;
  const int endMinutes = list.windowEndHour * 60 + list.windowEndMinute;
  return nowMinutes >= startMinutes && nowMinutes < endMinutes;
}

std::vector<const TodoList*> TodoDataStore::getVisibleLists(const TodoListType type, const TodoLocalTime* now) const {
  std::vector<const TodoList*> result;
  result.reserve(lists.size());
  for (const auto& list : lists) {
    if (list.type != type) continue;
    if (list.type == TodoListType::Checklist && now != nullptr && !isListCurrentlyVisible(list, *now)) continue;
    result.push_back(&list);
  }
  return result;
}

void TodoDataStore::applyChecklistResets(const int32_t currentLocalDayKey) {
  for (auto& list : lists) {
    if (list.type != TodoListType::Checklist) continue;
    if (list.lastResetDayKey == currentLocalDayKey) continue;
    for (auto& task : tasks) {
      if (task.listId == list.id) task.completed = false;
    }
    list.lastResetDayKey = currentLocalDayKey;
  }
}

int TodoDataStore::addTask(const int listId, const std::string& title, const bool completed, const bool favourited) {
  const int id = nextTaskId++;
  tasks.push_back(TodoTask{id, title, completed, favourited, listId});
  return id;
}

void TodoDataStore::clear() {
  lists.clear();
  tasks.clear();
  nextListId = 1;
  nextTaskId = 1;
}

std::vector<int> TodoDataStore::getTaskIdsInList(const int listId) const {
  std::vector<int> ids;
  ids.reserve(tasks.size());
  for (const auto& task : tasks) {
    if (task.listId == listId) ids.push_back(task.id);
  }
  return ids;
}

int TodoDataStore::getTaskCountInList(const int listId) const {
  return static_cast<int>(std::count_if(tasks.begin(), tasks.end(),
                                        [listId](const TodoTask& task) { return task.listId == listId; }));
}

std::vector<int> TodoDataStore::getFavouriteTaskIds() const {
  std::vector<int> ids;
  ids.reserve(tasks.size());
  for (const auto& task : tasks) {
    if (task.favourited) ids.push_back(task.id);
  }
  return ids;
}

int TodoDataStore::getFavouriteTaskCount() const {
  return static_cast<int>(std::count_if(tasks.begin(), tasks.end(), [](const TodoTask& task) { return task.favourited; }));
}

bool TodoDataStore::hasFavourites() const {
  return std::any_of(tasks.begin(), tasks.end(), [](const TodoTask& task) { return task.favourited; });
}

TodoTask* TodoDataStore::getTask(const int taskId) {
  return const_cast<TodoTask*>(static_cast<const TodoDataStore*>(this)->getTask(taskId));
}

const TodoTask* TodoDataStore::getTask(const int taskId) const {
  const auto it = std::find_if(tasks.begin(), tasks.end(), [taskId](const TodoTask& task) { return task.id == taskId; });
  return it != tasks.end() ? &*it : nullptr;
}

void TodoDataStore::toggleCompleted(const int taskId) {
  if (auto* task = getTask(taskId)) task->completed = !task->completed;
}

void TodoDataStore::toggleFavourited(const int taskId) {
  if (auto* task = getTask(taskId)) task->favourited = !task->favourited;
}

std::vector<bool> TodoDataStore::snapshotCompleted(const std::vector<int>& taskIds) const {
  std::vector<bool> snapshot;
  snapshot.reserve(taskIds.size());
  for (const int taskId : taskIds) {
    const auto* task = getTask(taskId);
    snapshot.push_back(task != nullptr && task->completed);
  }
  return snapshot;
}

void TodoDataStore::setAllCompleted(const std::vector<int>& taskIds, const bool completed) {
  for (const int taskId : taskIds) {
    if (auto* task = getTask(taskId)) task->completed = completed;
  }
}

void TodoDataStore::restoreCompleted(const std::vector<int>& taskIds, const std::vector<bool>& snapshot) {
  for (size_t i = 0; i < taskIds.size() && i < snapshot.size(); i++) {
    if (auto* task = getTask(taskIds[i])) task->completed = snapshot[i];
  }
}
