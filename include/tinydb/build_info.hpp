#pragma once

#include <cstdint>
#include <string_view>

namespace tinydb {

struct BuildInfo {
  static constexpr std::string_view kProjectName = "TinyDB";
  static constexpr std::string_view kVersion = "0.1.0-dev";
  static constexpr std::uint32_t kPageSizeBytes = 4096;
};

[[nodiscard]] std::string_view GetVersionBanner();

}  // namespace tinydb
