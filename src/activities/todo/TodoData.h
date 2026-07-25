#pragma once
#include <string>
#include <vector>

// Real, permanent Todo-app data model (docs/design-spec.md Section 6).
// Currently in-memory only: the BLE sync payload/persistence format is still
// undecided (Section 8), so this deliberately does not commit to a JSON
// schema yet. Populated by TodoSeedData.h for now; will be populated by the
// BLE sync receive path once that exists.

struct TodoTask {
  int id = 0;
  std::string title;
  bool completed = false;
  bool favourited = false;
  int listId = -1;  // Which TodoList this task belongs to.
};

struct TodoList {
  int id = 0;
  std::string name;
  // Only Todo-type lists exist so far. Checklist-specific fields (frequency,
  // display mode) will be added when Checklist logic is built.
};

class TodoDataStore {
 public:
  static TodoDataStore& getInstance() {
    static TodoDataStore instance;
    return instance;
  }

  // -- Population (seed data now; BLE sync receive path later) --
  int addList(const std::string& name);
  int addTask(int listId, const std::string& title, bool completed = false, bool favourited = false);
  void clear();

  // -- Queries --
  const std::vector<TodoList>& getLists() const { return lists; }
  // Task IDs belonging to a real list, in insertion order.
  std::vector<int> getTaskIdsInList(int listId) const;
  // Task IDs across all lists with favourited == true: a live filter, not a
  // stored list (Section 6's Favourites).
  std::vector<int> getFavouriteTaskIds() const;
  bool hasFavourites() const;
  TodoTask* getTask(int taskId);

  // -- Mutations --
  void toggleCompleted(int taskId);
  void toggleFavourited(int taskId);

  // List-name row's complete-all/undo (Section 5/11): snapshot each task's
  // completed state before the bulk action, so a later restoreCompleted()
  // call can undo it exactly, regardless of edits made in between.
  std::vector<bool> snapshotCompleted(const std::vector<int>& taskIds) const;
  void setAllCompleted(const std::vector<int>& taskIds, bool completed);
  void restoreCompleted(const std::vector<int>& taskIds, const std::vector<bool>& snapshot);

 private:
  TodoDataStore() = default;

  std::vector<TodoList> lists;
  std::vector<TodoTask> tasks;
  int nextListId = 1;
  int nextTaskId = 1;
};

#define TODO_DATA TodoDataStore::getInstance()
