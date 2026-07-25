#pragma once
#include "CrossPointSettings.h"
#include "MappedInputManager.h"

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

// Returns the slot that fired this frame, or None.
//
// `transitioningSlot` (if any) is read from the button *release* edge; every
// other slot is read from the *press* edge. This is the rule every Todo-app
// screen must follow for any action that can push/pop an Activity: firing on
// press lets the transition happen while the button is still physically
// held, leaving its release still pending -- and that stray release then
// lands on whatever screen becomes current next (see
// MappedInputManager::getReleasedFrontButton()'s doc comment; this is
// exactly what caused Back to bounce straight back into the screen it had
// just exited). Actions that don't transition (cursor movement,
// complete/favourite a task) stay press-triggered for responsiveness by
// simply not naming themselves as `transitioningSlot`.
inline TodoButtonSlot todoConsumeSlot(const MappedInputManager& mappedInput,
                                       const TodoButtonSlot transitioningSlot = TodoButtonSlot::None) {
  const int pressed = mappedInput.getPressedFrontButton();
  if (pressed >= 0) {
    const auto slot = todoRawButtonToSlot(pressed);
    if (slot != transitioningSlot) return slot;
  }
  const int released = mappedInput.getReleasedFrontButton();
  if (released >= 0) {
    const auto slot = todoRawButtonToSlot(released);
    if (slot == transitioningSlot) return slot;
  }
  return TodoButtonSlot::None;
}

// Four button-hint labels, one per raw physical button, in the left-to-right order
// GUI.drawButtonHints() renders them (see BaseTheme::drawButtonHints() and
// MappedInputManager::getPressedFrontButton()'s raw hardware ordering).
struct TodoButtonHints {
  const char* raw[4];
};

// Reorders four labels given in logical-slot (Default) order -- LL, LR, RL, RR -- into
// raw physical order, honoring the Default/System layout the same way
// todoRawButtonToSlot() does. Pass "" for a slot that does nothing in the caller's
// current state; GUI.drawButtonHints() hides empty labels. Every Todo-app screen
// should recompute this every render() pass, since which label applies to which slot
// changes as focus moves (list-name row vs. tasks, menu bar vs. not, etc.).
inline TodoButtonHints todoButtonHints(const char* ll, const char* lr, const char* rl, const char* rr) {
  const char* bySlot[4] = {ll, lr, rl, rr};
  TodoButtonHints hints;
  for (int raw = 0; raw < 4; raw++) {
    hints.raw[raw] = bySlot[static_cast<uint8_t>(todoRawButtonToSlot(raw))];
  }
  return hints;
}
