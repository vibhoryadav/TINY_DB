#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace tinydb::core {

/// Returns true iff [offset, offset + length) lies within [0, total_size] without overflow.
[[nodiscard]] bool IsValidSliceRange(std::size_t total_size, std::size_t offset,
                                     std::size_t length) noexcept;

/// Returns a mutable sub-span [offset, offset + length) if in bounds, or std::nullopt.
[[nodiscard]] std::optional<std::span<std::byte>> CheckedSubspan(std::span<std::byte> bytes,
                                                                 std::size_t offset,
                                                                 std::size_t length) noexcept;

/// Returns a const sub-span [offset, offset + length) if in bounds, or std::nullopt.
[[nodiscard]] std::optional<std::span<const std::byte>> CheckedSubspan(
    std::span<const std::byte> bytes, std::size_t offset, std::size_t length) noexcept;

/// Returns a mutable sub-span [offset, bytes.size()) if offset <= bytes.size(), or std::nullopt.
[[nodiscard]] std::optional<std::span<std::byte>> CheckedSubspan(std::span<std::byte> bytes,
                                                                 std::size_t offset) noexcept;

/// Returns a const sub-span [offset, bytes.size()) if offset <= bytes.size(), or std::nullopt.
[[nodiscard]] std::optional<std::span<const std::byte>> CheckedSubspan(
    std::span<const std::byte> bytes, std::size_t offset) noexcept;

/// Copies all bytes from src into the prefix of dst; returns false without copying if src > dst.
[[nodiscard]] bool CopyInto(std::span<std::byte> dst, std::span<const std::byte> src) noexcept;

/// Zeroes all bytes in dst safely (handles empty spans as a no-op).
void FillZero(std::span<std::byte> dst) noexcept;

/// Compares two byte spans for exact length and byte-by-byte equality (NUL-safe).
[[nodiscard]] bool EqualBytes(std::span<const std::byte> lhs,
                              std::span<const std::byte> rhs) noexcept;

}  // namespace tinydb::core
