#include "tinydb/build_info.hpp"

namespace tinydb {

std::string_view GetVersionBanner() {
  return "TinyDB v0.1.0-dev (C++20, 4096-byte page architecture)";
}

}  // namespace tinydb
