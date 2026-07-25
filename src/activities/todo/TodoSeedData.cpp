#include "TodoSeedData.h"

#if TODO_SEED_DEBUG_DATA

#include "TodoData.h"

void seedTodoDebugData() {
  TODO_DATA.clear();

  const int groceries = TODO_DATA.addList("Groceries");
  TODO_DATA.addTask(groceries, "Milk");
  TODO_DATA.addTask(groceries, "Eggs", false, true /* favourited */);
  TODO_DATA.addTask(groceries, "Bread", true /* completed */);

  const int work = TODO_DATA.addList("Work");
  TODO_DATA.addTask(work, "Finish report", false, true /* favourited */);
  TODO_DATA.addTask(work, "Email Bob");
  TODO_DATA.addTask(work, "Review PR", true /* completed */);
}

#endif
