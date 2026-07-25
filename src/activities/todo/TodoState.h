#pragma once
#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <cstdint>

// Runtime state for the Todo app, persisted separately from the core
// reader's CrossPointState so this app's data stays self-contained.
// No habit data yet — placeholder deferred until its shape is designed.
class TodoState : public PersistableStore<TodoState> {
  TodoState() = default;

  friend class PersistableStore<TodoState>;

 public:
  // Order matches the Home screen's list (docs/design-spec.md Section 4): To Dos, Checklists, Habits.
  enum TodoCategory { TODO_CATEGORY_TODOS = 0, TODO_CATEGORY_CHECKLISTS = 1, TODO_CATEGORY_HABITS = 2, TODO_CATEGORY_COUNT };

  // List scroll/selection position per category, indexed by TodoCategory.
  uint8_t listPosition[TODO_CATEGORY_COUNT] = {};

  static const char* getFilePath() { return "/.crosspoint/todo_state.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
};

// Helper macro to access Todo app state
#define TODO_STATE TodoState::getInstance()

// Display name for a category, shared by every screen that lists categories
// (TodoHomeActivity's list, TodoCategoryActivity's placeholder title).
const char* todoCategoryTitle(TodoState::TodoCategory category);
