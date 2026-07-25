#pragma once
#include "CrossPointSettings.h"

// Maps a raw front-button press (HalGPIO::BTN_BACK/CONFIRM/LEFT/RIGHT, i.e. LL/LR/RL/RR
// in physical order as returned by MappedInputManager::getPressedFrontButton()) to one
// of four logical action slots, honoring the Default/System button-layout preference
// (docs/design-spec.md Section 12).
//
// Every Todo-app screen defines its own four actions in Default order (LL, LR, RL, RR)
// and switches on the slot this function returns for whichever raw button just fired.
// System swaps the two rockers position-for-position (L-side stays L-side, R-side stays
// R-side): raw LL<->raw RL, raw LR<->raw RR.
enum class TodoButtonSlot : uint8_t { LL = 0, LR = 1, RL = 2, RR = 3, None = 0xFF };

inline TodoButtonSlot todoRawButtonToSlot(int rawButton) {
  if (rawButton < 0 || rawButton > 3) return TodoButtonSlot::None;
  static constexpr uint8_t defaultOrder[4] = {0, 1, 2, 3};
  static constexpr uint8_t systemOrder[4] = {2, 3, 0, 1};
  const uint8_t* order =
      (SETTINGS.todoButtonLayout == CrossPointSettings::TODO_BTN_LAYOUT_SYSTEM) ? systemOrder : defaultOrder;
  return static_cast<TodoButtonSlot>(order[rawButton]);
}
