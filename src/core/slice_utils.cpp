#include "tinydb/core/slice_utils.hpp"

#include <algorithm>
#include <cstring>

namespace tinydb::core {

bool IsValidSliceRange(std::size_t total_size, std::size_t offset, std::size_t length) noexcept {
  // Subtraction avoids unsigned wraparound when offset + length would overflow std::size_t.
  return offset <= total_size && length <= (total_size - offset);
}

std::optional<std::span<std::byte>> CheckedSubspan(std::span<std::byte> bytes, std::size_t offset,
                                                   std::size_t length) noexcept {
  if (!IsValidSliceRange(bytes.size(), offset, length)) {
    return std::nullopt;
  }
  return bytes.subspan(offset, length);
}

std::optional<std::span<const std::byte>> CheckedSubspan(std::span<const std::byte> bytes,
                                                         std::size_t offset,
                                                         std::size_t length) noexcept {
  if (!IsValidSliceRange(bytes.size(), offset, length)) {
    return std::nullopt;
  }
  return bytes.subspan(offset, length);
}

std::optional<std::span<std::byte>> CheckedSubspan(std::span<std::byte> bytes,
                                                   std::size_t offset) noexcept {
  if (offset > bytes.size()) {
    return std::nullopt;
  }
  return bytes.subspan(offset);
}

std::optional<std::span<const std::byte>> CheckedSubspan(std::span<const std::byte> bytes,
                                                         std::size_t offset) noexcept {
  if (offset > bytes.size()) {
    return std::nullopt;
  }
  return bytes.subspan(offset);
}

bool CopyInto(std::span<std::byte> dst, std::span<const std::byte> src) noexcept {
  if (src.size() > dst.size()) {
    return false;
  }
  if (!src.empty()) {
    // memmove handles intra-buffer overlapping copies (e.g., slotted-page shifts) safely.
    std::memmove(dst.data(), src.data(), src.size());
  }
  return true;
}

void FillZero(std::span<std::byte> dst) noexcept {
  if (!dst.empty()) {
    std::memset(dst.data(), 0, dst.size());
  }
}

bool EqualBytes(std::span<const std::byte> lhs, std::span<const std::byte> rhs) noexcept {
  return std::ranges::equal(lhs, rhs);
}

}  // namespace tinydb::core
