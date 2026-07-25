#include "TodoState.h"

#include <algorithm>
#include <cstring>

void TodoState::toJson(JsonDocument& doc) const {
  JsonArray posArr = doc["listPosition"].to<JsonArray>();
  for (int i = 0; i < TODO_CATEGORY_COUNT; i++) posArr.add(listPosition[i]);
}

bool TodoState::fromJson(JsonVariantConst doc) {
  memset(listPosition, 0, sizeof(listPosition));
  JsonArrayConst posArr = doc["listPosition"];
  const int actualCount =
      posArr.isNull() ? 0 : std::min(static_cast<int>(posArr.size()), static_cast<int>(TODO_CATEGORY_COUNT));
  for (int i = 0; i < actualCount; i++) listPosition[i] = posArr[i] | static_cast<uint8_t>(0);
  return true;
}
