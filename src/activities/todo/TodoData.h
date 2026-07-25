#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "TodoClock.h"

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

enum class TodoListType : uint8_t { Todo = 0, Checklist = 1 };

// Which days a checklist is scheduled on (Section 6).
enum class ChecklistFrequency : uint8_t { Daily = 0, Weekdays = 1, Weekends = 2, Custom = 3 };

// Whether a checklist is only actionable within a daily time window, or
// available any time on its scheduled days until every task is done (Section 6).
enum class ChecklistDisplayMode : uint8_t { TimeWindow = 0, UntilAllComplete = 1 };

struct TodoList {
  int id = 0;
  std::string name;
  TodoListType type = TodoListType::Todo;

  // Checklist-only fields below (Section 6); unused when type == Todo.

  ChecklistFrequency frequency = ChecklistFrequency::Daily;
  // Bitmask of weekdays, bit N = TodoLocalTime::weekday N (0=Sunday..6=Saturday).
  // Only read when frequency == Custom.
  uint8_t customDays = 0;
  ChecklistDisplayMode displayMode = ChecklistDisplayMode::UntilAllComplete;
  // Only read when displayMode == TimeWindow. A window that starts before it
  // ends, within a single day (does not wrap past midnight).
  uint8_t windowStartHour = 0;
  uint8_t windowStartMinute = 0;
  uint8_t windowEndHour = 0;
  uint8_t windowEndMinute = 0;

  // Reset-and-drop tracking (Section 6): the local day-key (TodoClock.h) this
  // checklist's tasks were last reset for. INT32_MIN means never reset.
  // Compared lazily against "today" whenever the checklist is queried
  // (TodoDataStore::applyChecklistResets) -- no background timer needed.
  int32_t lastResetDayKey = INT32_MIN;
};

class TodoDataStore {
 public:
  static TodoDataStore& getInstance() {
    static TodoDataStore instance;
    return instance;
  }

  // -- Population (seed data now; BLE sync receive path later) --
  int addList(const std::string& name);
  int addChecklist(const std::string& name, ChecklistFrequency frequency, ChecklistDisplayMode displayMode,
                   uint8_t windowStartHour = 0, uint8_t windowStartMinute = 0, uint8_t windowEndHour = 0,
                   uint8_t windowEndMinute = 0, uint8_t customDays = 0);
  int addTask(int listId, const std::string& title, bool completed = false, bool favourited = false);
  void clear();

  // -- Queries --
  const std::vector<TodoList>& getLists() const { return lists; }
  // Lists of the given type, in insertion order. Checklist-type lists are
  // filtered to those currently scheduled/in-window (see
  // isListCurrentlyVisible); pass now == nullptr (RTC unavailable) to skip
  // that filtering and show them all. Todo-type lists are always returned
  // in full regardless of `now`.
  std::vector<const TodoList*> getVisibleLists(TodoListType type, const TodoLocalTime* now) const;
  // True if a Checklist-type list should currently be shown: today matches
  // its frequency, and -- for TimeWindow mode -- `now` falls within its
  // window. Always true for Todo-type lists (no schedule).
  bool isListCurrentlyVisible(const TodoList& list, const TodoLocalTime& now) const;
  // Applies the reset-and-drop rule (Section 6) to every Checklist-type list
  // whose local day has changed since it was last reset: clears `completed`
  // on all of its tasks and stamps lastResetDayKey. No-op for lists already
  // reset for today, and for Todo-type lists (no schedule, never reset).
  void applyChecklistResets(int32_t currentLocalDayKey);

  // Task IDs belonging to a real list, in insertion order.
  std::vector<int> getTaskIdsInList(int listId) const;
  int getTaskCountInList(int listId) const;
  // Task IDs across all lists with favourited == true: a live filter, not a
  // stored list (Section 6's Favourites).
  std::vector<int> getFavouriteTaskIds() const;
  int getFavouriteTaskCount() const;
  bool hasFavourites() const;
  TodoTask* getTask(int taskId);
  const TodoTask* getTask(int taskId) const;

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
