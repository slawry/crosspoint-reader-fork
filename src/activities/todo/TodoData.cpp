#include "TodoData.h"

#include <algorithm>

#include "CrossPointSettings.h"

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

int TodoDataStore::addHabitList(const std::string& name) {
  const int id = nextListId++;
  TodoList list;
  list.id = id;
  list.name = name;
  list.type = TodoListType::Habit;
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

int TodoDataStore::addHabit(const int listId, const std::string& name, const HabitMode mode,
                            const HabitDirection direction, const int32_t target) {
  const int id = nextHabitId++;
  Habit habit;
  habit.id = id;
  habit.name = name;
  habit.listId = listId;
  habit.mode = mode;
  habit.direction = direction;
  habit.target = target;
  habits.push_back(std::move(habit));
  return id;
}

void TodoDataStore::clear() {
  lists.clear();
  tasks.clear();
  habits.clear();
  nextListId = 1;
  nextTaskId = 1;
  nextHabitId = 1;
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

std::vector<int> TodoDataStore::getHabitIdsInList(const int listId) const {
  std::vector<int> ids;
  ids.reserve(habits.size());
  for (const auto& habit : habits) {
    if (habit.listId == listId) ids.push_back(habit.id);
  }
  return ids;
}

int TodoDataStore::getHabitCountInList(const int listId) const {
  return static_cast<int>(std::count_if(habits.begin(), habits.end(),
                                        [listId](const Habit& habit) { return habit.listId == listId; }));
}

Habit* TodoDataStore::getHabit(const int habitId) {
  return const_cast<Habit*>(static_cast<const TodoDataStore*>(this)->getHabit(habitId));
}

const Habit* TodoDataStore::getHabit(const int habitId) const {
  const auto it = std::find_if(habits.begin(), habits.end(), [habitId](const Habit& habit) { return habit.id == habitId; });
  return it != habits.end() ? &*it : nullptr;
}

void TodoDataStore::applyHabitRollovers(const TodoLocalTime& now) {
  // Week-start weekday per TodoLocalTime::weekday's convention (0=Sunday..6=Saturday).
  const uint8_t weekStartWeekday = (SETTINGS.todoHabitWeekStart == CrossPointSettings::HABIT_WEEK_START_SUNDAY) ? 0 : 1;
  const int32_t offsetFromWeekStart = (static_cast<int32_t>(now.weekday) - weekStartWeekday + 7) % 7;
  const int32_t currentWeekStartDayKey = now.dayKey - offsetFromWeekStart;

  for (auto& habit : habits) {
    if (habit.weekStartDayKey == currentWeekStartDayKey) continue;
    for (auto& day : habit.days) day = HabitDayEntry{};
    habit.weekStartDayKey = currentWeekStartDayKey;
  }
}

void TodoDataStore::logHabitToday(const int habitId, const TodoLocalTime& now) {
  auto* habit = getHabit(habitId);
  if (!habit) return;
  const int32_t offset = now.dayKey - habit->weekStartDayKey;
  if (offset < 0 || offset > 6) return;  // applyHabitRollovers(now) wasn't called first.

  auto& day = habit->days[offset];
  if (habit->mode == HabitMode::Boolean) {
    day.completed = !day.completed;
  } else {
    day.count++;
  }
}

const HabitDayEntry* TodoDataStore::getHabitTodayEntry(const Habit& habit, const TodoLocalTime& now) const {
  const int32_t offset = now.dayKey - habit.weekStartDayKey;
  if (offset < 0 || offset > 6) return nullptr;
  return &habit.days[offset];
}

int32_t TodoDataStore::getHabitWeeklyTally(const Habit& habit) const {
  int32_t tally = 0;
  for (const auto& day : habit.days) {
    tally += habit.mode == HabitMode::Boolean ? (day.completed ? 1 : 0) : day.count;
  }
  return tally;
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
