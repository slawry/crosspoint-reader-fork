#pragma once

// TEMPORARY DEBUG-ONLY CODE.
//
// Task/list creation only happens from the iOS companion app via BLE sync
// (docs/design-spec.md Section 8), which doesn't exist yet. Until it does,
// this seeds TodoDataStore with hardcoded sample data at boot so the Todo
// list feature (navigation, completion, favouriting, list-cycling) can be
// exercised on a physical device.
//
// To disable without deleting anything: flip TODO_SEED_DEBUG_DATA to 0.
// To remove entirely once BLE sync lands: delete this file and its one call
// site in main.cpp (seedTodoDebugData()). Nothing else in the Todo app
// includes or calls into this file.
//
// Extending to Checklists/Habits later: add sibling functions following the
// same name+file pattern (e.g. seedChecklistDebugData() in
// TodoSeedData.cpp, or a new ChecklistSeedData.h/.cpp), each called from the
// same guarded block in main.cpp. Keep each category's seeding independent
// so any one of them can be deleted without touching the others.
#define TODO_SEED_DEBUG_DATA 1

#if TODO_SEED_DEBUG_DATA
// Populates TodoDataStore with hardcoded sample Todo lists/tasks for manual
// testing. Safe to call multiple times (clears existing data first).
void seedTodoDebugData();
#endif
