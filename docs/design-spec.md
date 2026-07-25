# Xteink X3 Todo / Checklist / Habit App — Build Specification

Last updated: 23 July 2026

## 1. Project summary

A custom mode built into the Xteink X3's firmware, turning its existing six physical buttons into a todo list, recurring checklist, and habit tracker, syncing over Bluetooth with a companion iOS app. No new hardware, no case modifications, no soldering. The X3's stock buttons are sufficient.

## 2. Hardware baseline

- **Xteink X3**: ESP32-C3, 3.7" E Ink display (monochrome/limited grayscale, no colour), 650mAh battery (~10–14 day stock reading life), microSD, 2.4GHz WiFi + BLE.
- **Charging/data port**: 4-pin magnetic pogo connector. Carries UART for flashing/debug only, not a general-purpose GPIO breakout. Not usable for wired external buttons.
- **Units bought direct from xteink.com are not USB-locked.** No unlock tool needed.
- **Firmware base**: [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader), open-source, confirmed running on X3 and X4. Activity-based architecture (`ActivityManager` + per-screen `Activity` subclasses, organized into folders: `boot_sleep`, `browser`, `home`, `network`, `reader`, `settings`, `util`). Settings persisted via `CrossPointSettings.h`/`.cpp`, enum-driven. Existing hardware button abstraction (`FRONT_HW_BACK/CONFIRM/LEFT/RIGHT`, `SIDE_BUTTON_LAYOUT`) confirms raw button identity is already decoupled from meaning, good foundation for a fully custom remap.
- **RTC (DS3231)**: present on the board, detected at boot by every existing fork, unused by all of them. This is what checklist time-windows and any future habit reminders will run on.

## 3. Button naming convention

Two front rockers, each with two actuation points, plus two side buttons.

| Name | Physical position |
|---|---|
| LL | Left front rocker, left side |
| LR | Left front rocker, right side |
| RL | Right front rocker, left side |
| RR | Right front rocker, right side |
| Side L / Side R | Separate side buttons |

(For reference only, no longer used in design: LL/LR/RL/RR correspond to the OEM manual's Back/OK/Next Page/Previous Page labels, which are not used as-is anywhere in this app.)

## 4. Screen hierarchy

```
E-reader book view (stock CrossPoint)
  └── Home screen: To Dos / Checklists / Habits
        └── Category list-cycle (Side L/R switch between named lists in that category)
              └── List-name row
                    └── Tasks / Habits
        └── Global menu bar: Back, Sync, Settings
```

The menu bar sits above both the Home screen and the list-name row, reachable from either via LL.

## 5. Button behavior, by context

**Home screen** (To Dos / Checklists / Habits)
| LL | LR | RL | RR | Side L/R |
|---|---|---|---|---|
| up | down | enter highlighted category | — | no-op |

**Inside a Todo or Checklist list**
| LL | LR | RL | RR | Side L/R |
|---|---|---|---|---|
| cursor up | cursor down | complete/uncomplete task | favourite/unfavourite task | previous/next list |

**Inside the Habit list**
| LL | LR | RL | RR | Side L/R |
|---|---|---|---|---|
| cursor up | cursor down | boolean: toggle today. quantity: +1 today | open weekly info modal | previous/next list |

**List-name row**
| LL | LR | RL | RR |
|---|---|---|---|
| up to menu bar | down to first item | complete all / undo (press again to restore prior per-item state) | show list info (task count, completion stats) |

**Menu bar** (entries: Back, Sync, Settings)
| LL | LR | RL | RR |
|---|---|---|---|
| cycle right through entries (Back → Sync → Settings) | down, out of menu bar | activate highlighted entry | tooltip for highlighted entry |

Side buttons do nothing while the menu bar is focused, they are reserved exclusively for switching lists within a category.

**Back, hierarchically:**
- From inside a category (Todos/Checklists/Habits) → Home screen
- From the Home screen → exits to the e-reader's book view

## 6. Data model

**Task**
- title
- completed (bool)
- favourited (bool)
- home list id

**List** (Todo or Checklist type)
- name
- type: todo | checklist
- Checklist only: frequency (daily / weekdays / weekends / custom), display mode (time window vs until-all-complete)
- Checklist reset behavior: unfinished items vanish at reset, no carry-over, no streak tracking

**Favourites**
- Not a stored list, a live filter (`favourited == true`) across all Todo lists
- Appears as the first entry in the To Dos category's list-cycle, but only when at least one task is currently favourited; otherwise it's skipped
- On-device only. Does not sync to the phone and has no representation in the iOS app

**Habit**
- name
- mode: boolean | quantity (chosen per habit at creation)
- direction: build | break
- target: boolean → day-count out of 7. quantity → weekly total (summed, not averaged)
- week start day: configurable in Settings (default Monday)
- quantity logging: each RL press adds +1 to today's count, no cap, resets at midnight

## 7. Time-awareness, current scope

- **Checklists use the RTC now.** The "time window" display mode is a v1 feature and needs real time-of-day logic from the start.
- **Habits explicitly do not, for now.** Per your call, habits ship with no due-time/reminder logic in v1, but the data model shouldn't preclude adding it later.

## 8. Sync architecture

- **Transport: Bluetooth (BLE)**, not WiFi. Chosen because the reader-side trigger (pressing Sync in the menu bar) already solves the "how does it know to listen" problem regardless of transport, so the deciding factor came down to phone-side experience. WiFi AP-mode syncing would require the phone to leave its current network and join the reader's hotspot, a visible, multi-second disruption. BLE doesn't touch the phone's WiFi at all.
- **Trade-off accepted**: no existing fork implements BLE, so this is a GATT service built from scratch on the firmware side, and Core Bluetooth integration from scratch on the iOS side. There was a cheaper path (extending CrossPoint's existing WiFi web server/AP-mode infrastructure), consciously not taken, for the UX reason above.
- **Trigger flow**: user presses Sync in the reader's menu bar → reader becomes BLE-connectable → iOS app connects and exchanges data.
- **Directionality, confirmed**:
  - **Phone → Reader**: newly added lists, tasks, checklists, or habits
  - **Reader → Phone**: completed tasks, completed habits, list completions, checklist completions
  - Habit completion is reader-only, not settable from the phone. Combined with tasks/lists/checklists already being device-authoritative, every completion type now flows one direction only, so there is no remaining case where the same field could be set differently on both sides. No conflict-resolution rule is needed for completions.
  - Favourites are an on-device filter only. They don't sync to the phone at all, and don't appear in the phone app.
- **Still undecided, needs resolving before the sync layer is built**: the exact GATT characteristic layout for each payload type (tasks, checklists, habits).

## 9. iOS companion app

- Platform: iOS only, SwiftUI + Core Bluetooth
- Lists, checklists, and habits are created/configured in the app; the device is where you interact with them day to day (check off tasks, favourite tasks on-device only, log habits) and syncs completions back, except favourites, which never leave the device

## 10. Firmware integration plan

1. Fork `crosspoint-reader/crosspoint-reader` (already cloned locally for reference)
2. Add a new `activities/todo/` folder: Home screen, Todo/Checklist list view, Habit list view, list-info modal, habit-info modal, menu bar, following the existing Activity pattern
3. Add new entries to `CrossPointSettings.h` for persisting this app's own state (current list positions, habit data, etc.) and the new button-layout preference (see Section 12), following the existing enum-driven pattern
4. Implement the BLE GATT service (ESP32-C3 supports BLE natively via ESP-IDF/Arduino libraries)
5. Take a full flash backup of the stock firmware before your first custom flash, so a bad build is a five-minute restore, not a lost device

## 11. Confirmed design decisions (previously open assumptions)

- Habit week runs Monday–Sunday by default, adjustable in Settings
- List-name row's complete-all undo restores an exact snapshot taken at the moment of the bulk action, regardless of any individual edits made in between
- Home screen has no header row of its own; LL-up from its first item goes straight to the menu bar
- Side buttons are a no-op on the Home screen itself

## 12. Button layout setting (Default vs System), confirmed

Settings offers two button-layout presets, swapping which physical rocker handles navigation versus action. Side buttons are unaffected either way, they always mean previous/next list.

- **Default** (everything specified in Sections 3–5 of this document): left rocker (LL/LR) navigates up/down, right rocker (RL/RR) selects/completes and favourites/info.
- **System** (mirrors the original manufacturer layout's rocker assignment, where the left rocker was the action rocker and the right rocker paged through content): the two rockers swap jobs wholesale.
  - LL → select/complete (Default's RL job)
  - LR → favourite/unfavourite or info (Default's RR job)
  - RL → cursor up (Default's LL job)
  - RR → cursor down (Default's LR job)
