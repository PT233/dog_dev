#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace project_shared {

inline constexpr std::array<const char*, 4> kServoNames = {
    "front_left",
    "front_right",
    "rear_left",
    "rear_right",
};

inline constexpr size_t kServoCount = kServoNames.size();

inline const char* servo_id_to_name(uint8_t servo_id) {
  return servo_id < kServoNames.size() ? kServoNames[servo_id] : nullptr;
}

inline int servo_name_to_id(std::string_view name) {
  for (size_t i = 0; i < kServoNames.size(); ++i) {
    if (name == kServoNames[i]) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace project_shared
