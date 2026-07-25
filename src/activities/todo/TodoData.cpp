#include "TodoData.h"

#include <algorithm>

int TodoDataStore::addList(const std::string& name) {
  const int id = nextListId++;
  lists.push_back(TodoList{id, name});
  return id;
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

std::vector<int> TodoDataStore::getFavouriteTaskIds() const {
  std::vector<int> ids;
  ids.reserve(tasks.size());
  for (const auto& task : tasks) {
    if (task.favourited) ids.push_back(task.id);
  }
  return ids;
}

bool TodoDataStore::hasFavourites() const {
  return std::any_of(tasks.begin(), tasks.end(), [](const TodoTask& task) { return task.favourited; });
}

TodoTask* TodoDataStore::getTask(const int taskId) {
  for (auto& task : tasks) {
    if (task.id == taskId) return &task;
  }
  return nullptr;
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
    const auto it = std::find_if(tasks.begin(), tasks.end(), [taskId](const TodoTask& t) { return t.id == taskId; });
    snapshot.push_back(it != tasks.end() && it->completed);
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
