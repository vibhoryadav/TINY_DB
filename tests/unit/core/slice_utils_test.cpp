#include "tinydb/core/slice_utils.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace {

using tinydb::core::CheckedSubspan;
using tinydb::core::CopyInto;
using tinydb::core::EqualBytes;
using tinydb::core::FillZero;
using tinydb::core::IsValidSliceRange;

TEST(SliceUtilsTest, RangeChecksPreventUnsignedOverflow) {
  constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();
  EXPECT_TRUE(IsValidSliceRange(10, 0, 10));
  EXPECT_TRUE(IsValidSliceRange(10, 10, 0));
  EXPECT_TRUE(IsValidSliceRange(0, 0, 0));

  EXPECT_FALSE(IsValidSliceRange(10, 11, 0));
  EXPECT_FALSE(IsValidSliceRange(10, 5, 6));
  EXPECT_FALSE(IsValidSliceRange(10, 1, kMax));
  EXPECT_FALSE(IsValidSliceRange(10, kMax, 1));
  EXPECT_FALSE(IsValidSliceRange(10, kMax, kMax));
}

TEST(SliceUtilsTest, CheckedSubspanConstAndMutableBoundarySlices) {
  std::array<std::byte, 6> buf = {std::byte{10}, std::byte{20}, std::byte{30},
                                  std::byte{40}, std::byte{50}, std::byte{60}};

  // Valid mutable subspan writes through to underlying stack buffer.
  auto mut_slice = CheckedSubspan(std::span<std::byte>(buf), 2, 3);
  ASSERT_TRUE(mut_slice.has_value());
  EXPECT_EQ(mut_slice->size(), 3U);
  (*mut_slice)[0] = std::byte{99};
  EXPECT_EQ(buf[2], std::byte{99});

  // Exact end boundary (empty slice at offset == size).
  std::span<const std::byte> const_view(buf);
  auto tail_empty = CheckedSubspan(const_view, 6, 0);
  ASSERT_TRUE(tail_empty.has_value());
  EXPECT_TRUE(tail_empty->empty());

  auto tail_rem = CheckedSubspan(const_view, 4);
  ASSERT_TRUE(tail_rem.has_value());
  EXPECT_EQ(tail_rem->size(), 2U);
  EXPECT_EQ((*tail_rem)[0], std::byte{50});

  // Out-of-bounds and SIZE_MAX overflow traps.
  constexpr std::size_t kMax = std::numeric_limits<std::size_t>::max();
  EXPECT_FALSE(CheckedSubspan(std::span<std::byte>(buf), 7).has_value());
  EXPECT_FALSE(CheckedSubspan(const_view, 7).has_value());
  EXPECT_FALSE(CheckedSubspan(std::span<std::byte>(buf), 4, 3).has_value());
  EXPECT_FALSE(CheckedSubspan(const_view, 2, kMax).has_value());
  EXPECT_FALSE(CheckedSubspan(const_view, kMax, 2).has_value());
}

TEST(SliceUtilsTest, EmptyContainerSpansHandleEdgeCases) {
  std::vector<std::byte> empty_vec;
  std::span<std::byte> empty_mut(empty_vec);
  std::span<const std::byte> empty_const(empty_vec);

  auto s1 = CheckedSubspan(empty_mut, 0, 0);
  auto s2 = CheckedSubspan(empty_const, 0);
  ASSERT_TRUE(s1.has_value());
  ASSERT_TRUE(s2.has_value());
  EXPECT_TRUE(s1->empty());
  EXPECT_TRUE(s2->empty());

  EXPECT_FALSE(CheckedSubspan(empty_mut, 0, 1).has_value());
  EXPECT_FALSE(CheckedSubspan(empty_const, 1).has_value());

  FillZero(empty_mut);
  EXPECT_TRUE(CopyInto(empty_mut, empty_const));
}

TEST(SliceUtilsTest, CopyIntoHeapBufferAndRejectsOversizedSource) {
  auto heap_dst = std::make_unique<std::array<std::byte, 4>>();
  heap_dst->fill(std::byte{0xAA});

  const std::array<std::byte, 3> src = {std::byte{1}, std::byte{2}, std::byte{3}};
  EXPECT_TRUE(CopyInto(std::span<std::byte>(*heap_dst), src));
  EXPECT_EQ((*heap_dst)[0], std::byte{1});
  EXPECT_EQ((*heap_dst)[1], std::byte{2});
  EXPECT_EQ((*heap_dst)[2], std::byte{3});
  EXPECT_EQ((*heap_dst)[3], std::byte{0xAA});

  const std::array<std::byte, 5> too_large = {std::byte{9}, std::byte{8}, std::byte{7},
                                              std::byte{6}, std::byte{5}};
  EXPECT_FALSE(CopyInto(std::span<std::byte>(*heap_dst), too_large));
  // Destination remains untouched when CopyInto fails.
  EXPECT_EQ((*heap_dst)[0], std::byte{1});

  // Overlapping intra-buffer copy works safely without UB.
  auto dst_overlap = CheckedSubspan(std::span<std::byte>(*heap_dst), 1, 3);
  auto src_overlap = CheckedSubspan(std::span<const std::byte>(*heap_dst), 0, 2);
  ASSERT_TRUE(dst_overlap.has_value());
  ASSERT_TRUE(src_overlap.has_value());
  EXPECT_TRUE(CopyInto(*dst_overlap, *src_overlap));
  EXPECT_EQ((*heap_dst)[1], std::byte{1});
  EXPECT_EQ((*heap_dst)[2], std::byte{2});
}

TEST(SliceUtilsTest, FillZeroAndEqualBytesWorkWithEmbeddedNuls) {
  std::array<std::byte, 4> a = {std::byte{'T'}, std::byte{0}, std::byte{'D'}, std::byte{1}};
  std::array<std::byte, 4> b = {std::byte{'T'}, std::byte{0}, std::byte{'D'}, std::byte{2}};
  std::array<std::byte, 4> a_copy = a;

  EXPECT_TRUE(EqualBytes(a, a_copy));
  EXPECT_FALSE(EqualBytes(a, b));

  FillZero(a);
  for (std::byte byte : a) {
    EXPECT_EQ(byte, std::byte{0});
  }
}

}  // namespace
