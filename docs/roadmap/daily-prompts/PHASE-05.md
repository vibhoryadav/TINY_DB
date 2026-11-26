# Phase 05 — B+ Tree Indexing (`DAY-112` – `DAY-152`)

**Phase Budget:** 32.0 hours (36 × 45 min + 5 × 60 min = 41 sessions)
**Goal:** Implement a persistent, Buffer-Pool-backed B+ Tree index mapping `int64_t` keys to `RowID`s (supporting duplicate keys cleanly via composite `(key, RowID)` ordering, splits, point/range scans, deletion with redistribution/merging/root-shrinking, reload after restart, and an exhaustive structural validator).

---

## DAY-112 — Finalize 4KB B+ Tree Node Capacity Math and Composite Key Policy

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-112
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Finalize 4KB B+ Tree Node Capacity Math and Composite Key Policy
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 04 complete (`DAY-111`), `DAY-016` fanout notes.

### Objective
Define `IndexKey` (`(int64_t key, RowID rid)` total composite ordering + point-search lower bound `(key, min_rid)`), exact 4KB leaf/internal node byte capacities (`kLeafMaxSize`, `kInternalMaxSize`, `min_size` formulas), and configurable test-order constructor overrides (`max_leaf_size`, `max_internal_size`) in `include/tinydb/index/bplus_tree_types.hpp`.

### Concepts & Daily Learning
- **Concept:** Composite `(key, RowID)` uniqueness for non-unique SQL indexes, and configurable small-order B+ Trees (`order = 4, 5`) for rapid split/merge unit testing!
- **Why Database Engines Need It:** Real tables allow duplicate values (`age = 25` on 500 rows). Without `(key, RowID)` composite ordering, duplicate keys either fail or need brittle overflow-page chains. And testing a 4KB node requires ~250+ keys per split, whereas allowing a configurable `max_size` (defaulting to full 4KB capacity in production!) triggers 4-level trees in just 30 keys!
- **How It Works:** Compare `(k1, r1)` vs `(k2, r2)` lexicographically on `k` then `r.page_id` then `r.slot_id`. Define `BPlusTreeHeaderSize = 16` (`32B` common `PageHeader` + `16B` B+ Tree header = `48B`, leaving `4048B` payload!). Leaf entry = `8B key + 6B RowID = 14B` (`+ 4B next_page_id` -> `288` entries!). Internal entry = `14B composite separator + 4B child PageId = 18B` (or `8B` key + `6B` rid + `4B` child!).
- **Design Trade-offs:** Storing `(key, RowID)` in internal separators as well (18B per slot -> `~224` fanout!) makes duplicate keys across 10+ leaf pages split and merge identically to unique keys without ANY special duplicate-key edge cases!
- **Common Mistakes:** Allowing `max_size < 3` in small-order tests (where `ceil(max/2)` breaks split/merge invariants!). Enforce `max_size >= 4` (or `>= 3` with explicit math)!
- **Understanding Checkpoint:** Why does storing `(int64_t key, RowID rid)` as the internal separator key allow a single B+ Tree algorithm to handle 1,000 identical keys (`age = 30`) cleanly across multiple leaves?

### Component Context
- **Why It Exists:** Shared types, comparators, and capacity invariants for all Phase 05 files.
- **Module Relationships:** Used by `BPlusTreePage`, `LeafPage`, `InternalPage`, `BPlusTree`, and `BPlusTreeValidator`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_types.hpp` (new)
- `docs/btree-design.md` (update exact entry sizes, fanout math, and composite `(key, RowID)` policy)
- `tests/unit/index/bplus_tree_types_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `IndexEntryKey { std::int64_t key; RowID rid; }` (`<=>`, `SerializedSize = 14`), `kBTreeLeafMaxSize`, `kBTreeInternalMaxSize`, and `BPlusTreeConfig` (validating `>= 4` and `<= max_4kb_capacity`).
2. Add tests verifying composite ordering, `INT64_MIN`/`INT64_MAX`, and 4KB byte-fit assertions.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Capacity formulas satisfy `48 + 4 + max_leaf * 14 <= 4096` and `48 + max_internal * 18 <= 4096`.

### Definition of Done
Types and math finalized and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-112: define B+ Tree composite IndexEntryKey and 4KB capacity constants`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-113` (`BPlusTreePage` common node header).
````

---

## DAY-113 — Design `BPlusTreePage` Common Node Header (`node_type`, `size`, `max_size`, `parent_page_id`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-113
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Design `BPlusTreePage` Common Node Header (`node_type`, `size`, `max_size`, `parent_page_id`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-112 complete.

### Objective
Define `BPlusTreeNodeHeader` (`include/tinydb/index/bplus_tree_page.hpp`) placed at byte offset `[32..47]` right after the 32-byte common `PageHeader`: `node_type` (`1B`), `reserved` (`3B`), `size` (`4B`), `max_size` (`4B`), `parent_page_id` (`4B`), plus helper methods (`IsRoot()`, `GetMinSize()`, `IsLeaf()`).

### Concepts & Daily Learning
- **Concept:** Uniform node header for polymorphic internal/leaf pages and exact `GetMinSize()` underflow rules.
- **Why Database Engines Need It:** `BPlusTree` traversal reads `PageHeader::page_type` (or `BPlusTreeNodeHeader::node_type`) first to know whether a page is `kBTreeInternal` or `kBTreeLeaf`, and needs `parent_page_id` / `size` / `max_size` for splits and merges!
- **How It Works:** Offset `[32..47]` (`16` bytes) holds `node_type`, `size`, `max_size`, `parent_page_id`. `IsRoot()` checks `parent_page_id == kInvalidPageId`. `GetMinSize()` equals `max_size / 2` for leaves and `(max_size + 1) / 2` (or standard `ceil(max_size / 2)` for children!) for internal nodes (root min size is `1` for leaf, `2` children for internal!).
- **Design Trade-offs:** Storing `parent_page_id` on disk simplifies merge/split propagation at the cost of updating children's `parent_page_id` whenever an internal node splits or merges (`Validator` checks this!).
- **Common Mistakes:** Forgetting to update children's `parent_page_id` after moving half of an internal node's children to a new sibling!
- **Understanding Checkpoint:** Why is the minimum size of a non-root leaf `max_size / 2` whereas a non-root internal node with `max_size` children has minimum child count `(max_size + 1) / 2` (and root internal has min `2`)?

### Component Context
- **Why It Exists:** Shared metadata header for all B+ Tree disk pages.
- **Module Relationships:** Embedded in `BPlusTreeLeafPage` and `BPlusTreeInternalPage`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_page.hpp` (new)
- `tests/unit/index/bplus_tree_node_header_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `BPlusTreeNodeHeader` constants (`kOffset = 32`, `kSerializedSize = 16`, `kBodyOffset = 48`) and min-size / root-status helpers.
2. Add layout/min-size unit tests across odd and even `max_size` values (`4, 5, 6, 7`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All min-size, offset, and invariant helpers compile cleanly and pass assertions.

### Definition of Done
Node header designed and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-113: design BPlusTreeNodeHeader layout and min-size invariants`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-114` (`BPlusTreeNodeHeader` serialization & deserialization).
````

---

## DAY-114 — Implement Explicit `BPlusTreeNodeHeader` Serialization and Deserialization

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-114
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Explicit `BPlusTreeNodeHeader` Serialization and Deserialization
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-113 complete.

### Objective
Implement `BPlusTreeNodeHeader::Serialize(Page&)` and `BPlusTreeNodeHeader::Deserialize(const Page&)` (`src/index/bplus_tree_page.cpp`) enforcing agreement between `PageHeader::page_type` and `node_type`, valid `max_size`, and `size <= max_size + 1` (Wait: allow `size == max_size` normally, or `<= max_size` on disk so overflowed splits never persist invalid oversized nodes—let's keep `size <= max_size` strictly on disk by splitting cleanly via local/temp or `max_size` check!).

### Concepts & Daily Learning
- **Concept:** Explicit node-header binary serialization and cross-header consistency (`PageHeader::page_type` vs `BPlusTreeNodeHeader::node_type`).
- **Why Database Engines Need It:** Prevents misinterpreting a `TABLE_HEAP` page or corrupted node header with `size = 50000` causing out-of-bounds reads in `[48..4095]`.
- **How It Works:** Read/write at offset `32..47` using `BinaryWriter`/`BinaryReader`. On `Deserialize`, verify `PageHeader::page_type` is `kBTreeLeaf` or `kBTreeInternal`, `max_size >= 4 && max_size <= type_max`, and `size <= max_size`.
- **Design Trade-offs:** Enforcing `size <= max_size` on serialized pages means insert/split uses an in-memory scratch entry (or inserts into a temporary vector of `max_size + 1` entries before splitting into two valid nodes of `<= max_size`) so a 4096-byte `Page` buffer NEVER overflows!
- **Common Mistakes:** Allowing `Insert` to write `max_size + 1` entries onto the 4KB `Page` buffer before splitting (overflowing byte `4096` when `max_size == k4KBMax`!).
- **Understanding Checkpoint:** Why is handling a full-node split (`max_size + 1` entries) via an explicit stack/split helper safer than temporarily writing entry `max_size` past byte `4095` of a 4KB `Page`?

### Component Context
- **Why It Exists:** Safe encoder/decoder for the node header of every B+ Tree page.
- **Module Relationships:** Called by `BPlusTreeLeafPage` and `BPlusTreeInternalPage`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_page.hpp`
- `src/index/bplus_tree_page.cpp` (new)
- `tests/unit/index/bplus_tree_node_header_test.cpp`
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTreeNodeHeader::Serialize` and `Deserialize`.
2. Add roundtrip and corruption tests (`size > max_size`, `max_size < 4`, `max_size > 4KB limit`, mismatched `PageType`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Roundtrip preserves all fields; invalid bounds fail with `StatusCode::kCorruption`.

### Definition of Done
Node header serialization tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-114: implement explicit BPlusTreeNodeHeader serialization and validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-115` (`BPlusTreeLeafPage` layout & binary search).
````

---

## DAY-115 — Design `BPlusTreeLeafPage` Layout and Binary Search (`KeyIndex` / `Lookup`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-115
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Design `BPlusTreeLeafPage` Layout and Binary Search (`KeyIndex` / `Lookup`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-009 (`algorithms.hpp`), DAY-114 complete.

### Objective
Define `BPlusTreeLeafPage` (`include/tinydb/index/bplus_tree_leaf_page.hpp`) storing `BPlusTreeNodeHeader`, `next_page_id` (`PageId`, stored in `PageHeader::next_page_id` or offset `[48..51]`), and up to `max_size` sorted `(IndexEntryKey)` entries (`(key, rid)`), plus `O(log N)` binary search `LowerBound(int64_t key)` and `FindExact(IndexEntryKey)`.

### Concepts & Daily Learning
- **Concept:** Contiguous sorted leaf array + binary search (`LowerBound`), and exact vs range start lookup.
- **Why Database Engines Need It:** Searching 280 leaf entries linearly takes ~140 comparisons on average; binary search takes at most `9` comparisons (`log2(280)`).
- **How It Works:** `LeafPage` loads from `Page` (or decodes entries into a typed view/vector cleanly, or operates on explicit accessors—actually decoding/manipulating `BPlusTreeLeafNode` in-memory struct `[header + next_page_id + vector/array<IndexEntryKey>]` and explicitly serializing back on write OR reading/writing slot-by-slot via `endian.hpp`! Notice: reading/modifying an explicit stack-bounded `BPlusTreeLeafData` or slot accessors keeps BOTH explicit serialization AND zero raw pointer casting!). Let's design explicit `ReadEntry(i)` / `WriteEntry(i)` or explicit `Decode`/`Encode` cleanly with zero struct casts!
- **Design Trade-offs:** Reading/writing individual 14-byte entries via `endian.hpp` keeps `LeafPage` operating directly over the 4KB `Page` buffer with zero heap allocation (`std::vector`), while `Decode` into a small stack/in-memory node or direct `ReadEntry` during binary search keeps zero UB!
- **Common Mistakes:** Searching with `key` alone instead of `(key, min_RowID)` when duplicates exist (landing in the middle of identical keys instead of the FIRST occurrence!).
- **Understanding Checkpoint:** When a leaf holds `[(10, rid1), (10, rid2), (10, rid3), (20, rid4)]`, what composite sentinel key must binary search pass to find the **first** `key == 10` entry?

### Component Context
- **Why It Exists:** Sorted leaf storage node of the B+ Tree.
- **Module Relationships:** Driven by `BPlusTree` (`DAY-123`+) and `BPlusTreeIterator`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_leaf_page.hpp` (new)
- `src/index/bplus_tree_leaf_page.cpp` (new)
- `tests/unit/index/bplus_tree_leaf_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTreeLeafPage` wrapper: `Init(Page&, PageId, PageId parent, uint32_t max_size)`, `KeyAt(idx)`, `EntryAt(idx)`, `NextPageId()`, `SetNextPageId()`, `LowerBound(IndexEntryKey)`, and `LookupKeyStart(int64_t key)`.
2. Test `Init` and binary search with duplicates (`10, 10, 10, 20`)!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `LookupKeyStart(10)` returns index `0` even with multiple `key == 10` entries; `LowerBound` matches `std::lower_bound`.

### Definition of Done
Leaf layout and binary search tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-115: design BPlusTreeLeafPage layout and composite binary search`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-116` (`LeafPage` validation & explicit serialization/deserialization).
````

---

## DAY-116 — Implement `BPlusTreeLeafPage` Explicit Entry Serialization, Validation, and Mutation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-116
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTreeLeafPage` Explicit Entry Serialization, Validation, and Mutation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-115 complete.

### Objective
Implement explicit `BPlusTreeLeafPage::InsertSorted(IndexEntryKey)` (rejecting exact duplicate `(key, rid)` pairs cleanly while allowing equal `key` with different `rid`!), `Remove(IndexEntryKey)`, and `ValidateLeaf(const Page&)` verifying strictly ascending `(key, rid)` order and valid `RowID`s.

### Concepts & Daily Learning
- **Concept:** Maintaining sorted array invariants on insert/delete and rejecting malformed/unsorted disk leaves.
- **Why Database Engines Need It:** Inserting the exact same `(key, rid)` twice indicates a caller/index-sync bug, whereas same `key` + different `rid` is a normal non-unique SQL value; catching unsorted leaf data on load prevents binary search from returning wrong answers!
- **How It Works:** `InsertSorted`: binary-search `LowerBound(entry)`. If `idx < size && EntryAt(idx) == entry`, return `kAlreadyExists`. If `size == max_size`, return `kResourceExhausted` (caller splits!). Else shift entries `[idx..size-1]` right by 1, write `entry` at `idx`, increment `size`.
- **Design Trade-offs:** Returning `kResourceExhausted` cleanly when `size == max_size` keeps `InsertSorted` pure and triggers explicit split logic (`DAY-117`).
- **Common Mistakes:** Shifting raw byte ranges with overlapping `memcpy` instead of `std::memmove` or backward loop; forgetting to decrement `size` on `Remove`.
- **Understanding Checkpoint:** Why does `InsertSorted` allow `(25, RowID(1, 2))` and `(25, RowID(1, 3))` in the same leaf while rejecting `(25, RowID(1, 2))` twice?

### Component Context
- **Why It Exists:** Core leaf-page CRUD + invariant check.
- **Module Relationships:** Used by `BPlusTree::Insert` and `BPlusTree::Remove`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_leaf_page.hpp`
- `src/index/bplus_tree_leaf_page.cpp`
- `tests/unit/index/bplus_tree_leaf_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `InsertSorted`, `Remove`, `GetValuesForKey(int64_t key)`, and `ValidateLeaf`.
2. Write unit tests verifying out-of-order inserts maintain sorted order, duplicate `key` with distinct `RowID`s works, exact `(key, rid)` duplicate rejects `kAlreadyExists`, `Remove` shifts cleanly, and `ValidateLeaf` catches unsorted/invalid entries.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Sorted invariant and valid `RowID` checks hold across all inserts/removes and catch injected corruption.

### Definition of Done
Leaf CRUD and validation passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-116: implement BPlusTreeLeafPage sorted insert, delete, and validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-117` (`LeafPage` split & merge helpers).
````

---

## DAY-117 — Implement `BPlusTreeLeafPage` Split (`InsertAndSplitTo`) and Merge/Borrow Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-117
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTreeLeafPage` Split (`InsertAndSplitTo`) and Merge/Borrow Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-116 complete.

### Objective
Implement leaf node balancing helpers on `BPlusTreeLeafPage`: `InsertAndSplitTo(entry, Page& recipient_leaf)` (inserting into `max_size + 1` logically without overflowing 4096 bytes, moving upper half to `recipient`, linking `recipient.next = old_next`, `this.next = recipient_id`), `MoveAllTo(recipient)`, `MoveFirstToEndOf(recipient)`, and `MoveLastToFrontOf(recipient)`.

### Concepts & Daily Learning
- **Concept:** Overflow-free leaf splitting (`max_size + 1` -> `ceil/floor` split), copy-up separator key (`recipient.EntryAt(0)`!), and leaf sibling pointer maintenance.
- **Why Database Engines Need It:** In a B+ Tree, leaf splits *copy* (not move!) the right leaf's smallest key up to the parent internal node so ALL keys stay present in leaves for range scans!
- **How It Works:** `InsertAndSplitTo`: read `max_size` entries into local scratch array (`max_size + 1`), insert `new_entry` in sorted position, check not exact duplicate, split at `mid = (max_size + 1) / 2` (ensuring both halves satisfy `>= min_size`!), write `[0..mid)` back to left leaf, `[mid..size)` to right leaf, link `right.next = left.next`, `left.next = right_page_id`. Return `right.EntryAt(0)` as the copy-up separator!
- **Design Trade-offs:** Using a local stack/small vector of `(max_size + 1)` items prevents writing past byte `4095` of a 4KB `Page`!
- **Common Mistakes:** Moving vs copying the middle key out of a leaf (losing 1 record from leaf scans!); updating `left.next` BEFORE copying `left.next` into `right.next`!
- **Understanding Checkpoint:** After splitting a leaf of `max_size = 4` upon inserting a 5th entry `[10, 20, 30, 40, 50]`, what entries reside in the left leaf, the right leaf, and what separator key is copied up to the parent?

### Component Context
- **Why It Exists:** Reusable primitives for `BPlusTree` insertion splits and deletion merges/redistribution.
- **Module Relationships:** Called by `BPlusTree` (`DAY-126` & `DAY-134`–`DAY-135`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree_leaf_page.hpp`
- `src/index/bplus_tree_leaf_page.cpp`
- `tests/unit/index/bplus_tree_leaf_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `InsertAndSplitTo`, `MoveAllTo`, `MoveFirstToEndOf`, `MoveLastToFrontOf`.
2. Write unit tests verifying splits at `max_size = 4, 5` (for inserts at start, middle, and end!), exact sibling `next_page_id` links, `MoveFirstToEndOf`, `MoveLastToFrontOf`, and `MoveAllTo`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both post-split halves have `>= min_size` entries, valid sorted order, and linked `next_page_id`.

### Definition of Done
Leaf split/merge/borrow primitives tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-117: implement BPlusTreeLeafPage split, borrow, and merge helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-118` (`BPlusTreeInternalPage` layout & lookup).
````

---

## DAY-118 — Design `BPlusTreeInternalPage` Layout and Child Lookup (`Lookup`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-118
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Design `BPlusTreeInternalPage` Layout and Child Lookup (`Lookup`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-117 complete.

### Objective
Define `BPlusTreeInternalPage` (`include/tinydb/index/bplus_tree_internal_page.hpp`, `src/index/bplus_tree_internal_page.cpp`) storing `size` child `PageId`s (`[0..size-1]`) and `size - 1` separator `IndexEntryKey`s (`[1..size-1]`, with slot `0`'s key unused/zeroed sentinel!), plus `O(log N)` binary search `Lookup(IndexEntryKey)` returning the child `PageId` to descend into.

### Concepts & Daily Learning
- **Concept:** Internal B+ Tree routing invariants (`keys[i] <= subtree(children[i]) < keys[i+1]`, with `keys[0]` invalid/dummy) and `O(log N)` routing lookup.
- **Why Database Engines Need It:** An internal node with `size` child pointers only needs `size - 1` separator keys separating them; keeping `(key_i, child_i)` pairs indexed `0..size-1` (ignoring `key_0`) makes array indexing uniform!
- **How It Works:** Invariant: all entries in `children[0]` are `< keys[1]`; all entries in `children[i]` (`i >= 1`) satisfy `keys[i] <= entry < keys[i+1]`. Therefore `Lookup(target)` finds the **last** `i in [1..size-1]` such that `keys[i] <= target` (via `std::upper_bound` on `[1..size)` minus 1!) and returns `children[i]`.
- **Design Trade-offs:** Ignoring `keys[0]` wastes `14` bytes per 4KB internal node while avoiding off-by-one `keys[i-1]` vs `children[i]` index bugs everywhere!
- **Common Mistakes:** Binary searching from index `0` instead of `1` (comparing against dummy `keys[0]`!); using `<` instead of `<=` when `target == keys[i]` (sending search down `children[i-1]` when `keys[i]` actually lives in `children[i]`!).
- **Understanding Checkpoint:** With `children = [P1, P2, P3]` and `keys = [dummy, 20, 50]`, which child page does `Lookup(20)` return, and why?

### Component Context
- **Why It Exists:** Multi-way routing node enabling shallow `O(log_M N)` B+ Trees.
- **Module Relationships:** Guided by `BPlusTree::FindLeafPage` (`DAY-122`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree_internal_page.hpp` (new)
- `src/index/bplus_tree_internal_page.cpp` (new)
- `tests/unit/index/bplus_tree_internal_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Init`, `Size`, `MaxSize`, `KeyAt(i)`, `SetKeyAt(i, k)`, `ChildAt(i)`, `SetChildAt(i, pid)`, `ChildIndex(pid)`, and `Lookup(IndexEntryKey)`.
2. Write unit tests testing `Lookup` below `keys[1]`, exactly ON `keys[i]`, between separators, above `keys[size-1]`, and with duplicate `key` + differing `rid`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `Lookup` routes `target < keys[1]` to `children[0]` and `target == keys[i]` to `children[i]` consistently.

### Definition of Done
Internal node layout and `Lookup` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-118: implement BPlusTreeInternalPage layout and routing Lookup`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-119` (`PopulateNewRoot`, serialization & validation).
````

---

## DAY-119 — Implement `BPlusTreeInternalPage` `PopulateNewRoot` and Structural Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-119
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTreeInternalPage` `PopulateNewRoot` and Structural Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-118 complete.

### Objective
Implement `PopulateNewRoot(old_root_pid, sep_key, new_child_pid)` and `ValidateInternal(const Page&)` checking `size >= 2` (or `>= min_size` for non-root), strictly ascending separator keys `[1..size-1]`, valid `DataPageId`s for all `children[0..size-1]`, and no duplicate child `PageId`s inside a node.

### Concepts & Daily Learning
- **Concept:** Root creation upon leaf/root split and internal node integrity invariants.
- **Why Database Engines Need It:** A corrupt internal node with duplicate child `PageId`s or unsorted separators silently breaks tree routing and creates cycles during traversal!
- **How It Works:** `PopulateNewRoot(left, sep, right)` sets `size = 2`, `children[0] = left`, `keys[1] = sep`, `children[1] = right`. `ValidateInternal` verifies `BPlusTreeNodeHeader::Deserialize`, `size >= 1`, valid `children`, and `keys[i] < keys[i+1]` for `1 <= i < size - 1`.
- **Design Trade-offs:** Checking `ValidateInternal` at load/test time immediately exposes broken split/merge logic.
- **Common Mistakes:** Setting `size = 1` instead of `2` in `PopulateNewRoot` (forgetting `size` counts children, not keys!).
- **Understanding Checkpoint:** Why does an internal node created by `PopulateNewRoot` have `size == 2` even though it only holds `1` separator key?

### Component Context
- **Why It Exists:** Initializes new root levels and enforces internal-node invariants.
- **Module Relationships:** Used when root splits (`DAY-126`–`DAY-127`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree_internal_page.hpp`
- `src/index/bplus_tree_internal_page.cpp`
- `tests/unit/index/bplus_tree_internal_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `PopulateNewRoot` and `ValidateInternal`.
2. Add tests verifying `PopulateNewRoot` routes `< sep` left and `>= sep` right, and `ValidateInternal` catches unsorted keys, invalid child `PageId`, and duplicate child `PageId`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `PopulateNewRoot` and `ValidateInternal` pass all positive and negative unit tests.

### Definition of Done
Implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-119: implement PopulateNewRoot and internal page validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-120` (`InternalPage` insert, split push-up, and merge/borrow helpers).
````

---

## DAY-120 — Implement `BPlusTreeInternalPage` Insert, Push-Up Split, and Merge/Borrow Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-120
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTreeInternalPage` Insert, Push-Up Split, and Merge/Borrow Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-119 complete.

### Objective
Implement `InsertNodeAfter(old_child, new_key, new_child)`, `InsertAndSplitTo(old_child, new_key, new_child, Page& recipient)` (**pushing up** the middle separator key—NOT copying it!), `RemoveAt(index)`, `MoveAllTo(recipient, middle_key)`, `MoveFirstToEndOf(recipient, middle_key)`, and `MoveLastToFrontOf(recipient, middle_key)`.

### Concepts & Daily Learning
- **Concept:** Internal node **push-up** splits (moving middle key into parent) and **pull-down** merges/redistributions via parent's `middle_key`.
- **Why Database Engines Need It:** Unlike leaves, internal nodes don't store user data—keeping the middle separator key inside the split child would leave an extra unused `keys[0]` slot and violate strict routing invariants! Similarly, when internal siblings borrow or merge, the parent's `middle_key` pulls down cleanly to become the valid separator at `recipient.keys[0]` (or `right.keys[0]`)!
- **How It Works:** On `InsertAndSplitTo` with `max_size + 1` children: place `(new_key, new_child)` after `old_child` in local buffer, split at `mid = (max_size + 1) / 2`. Left keeps children `[0..mid)`, keys `[1..mid)`. **`pushed_up_key = entries[mid].key`** moves up to parent! Right receives children `[mid..max_size]` and keys `[mid+1..max_size]` (at `right.keys[1..]`!).
- **Design Trade-offs:** Returning `pushed_up_key` alongside updating children avoids losing keys or overflowing 4KB buffers.
- **Common Mistakes:** Copying `entries[mid].key` into `right.keys[1]` instead of pushing it up (creating a shifted mismatch between `right.keys` and `right.children`!); forgetting `middle_key` when calling `MoveLastToFrontOf` on internal nodes!
- **Understanding Checkpoint:** Contrast a **leaf split** (copy-up) vs an **internal node split** (push-up): why does the middle key disappear from the right child in an internal split while staying in the right child in a leaf split?

### Component Context
- **Why It Exists:** Complete internal-node manipulation toolkit for `BPlusTree`.
- **Module Relationships:** Driven by `BPlusTree` (`DAY-127`, `DAY-136`, `DAY-137`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree_internal_page.hpp`
- `src/index/bplus_tree_internal_page.cpp`
- `tests/unit/index/bplus_tree_internal_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `InsertNodeAfter`, `InsertAndSplitTo`, `RemoveAt`, `MoveAllTo`, `MoveFirstToEndOf`, `MoveLastToFrontOf`.
2. Write unit tests verifying internal push-up split (`size = 4` -> split produces 2 children of size 2 and 3 + pushed-up middle key!), pull-down merge (`MoveAllTo`), and pull-down borrow (`MoveFirst`/`MoveLast`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All internal split/borrow/merge tests satisfy `ValidateInternal` and exact routing expectations.

### Definition of Done
Internal node helpers implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-120: implement BPlusTreeInternalPage insert, push-up split, and merge helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-121` (Index root metadata persistence).
````

---

## DAY-121 — Implement B+ Tree Root Metadata Persistence Integration (`root_page_id` Updates)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-121
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement B+ Tree Root Metadata Persistence Integration (`root_page_id` Updates)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 03 (`CatalogManager::UpdateIndexRootPage`), DAY-120 complete.

### Objective
Define a clean `IndexRootSink` / `RootPageCallback` (or lightweight dedicated index header page / `CatalogManager` hook—let's evaluate both!) so every time a `BPlusTree`'s `root_page_id_` changes (`StartNewTree`, root split, root shrink, tree empty), the new `root_page_id_` persists atomically on disk!

### Concepts & Daily Learning
- **Concept:** Root page indirection (`IndexHeaderPage` or `CatalogManager::UpdateIndexRootPage`) vs hardcoding root page ID.
- **Why Database Engines Need It:** A B+ Tree root can start as a single leaf (Page 4), split into an internal root (Page 7), and later shrink back to Page 4 or become empty (`kInvalidPageId`); storing `root_page_id` in `IndexMetadata` (or a fixed `IndexHeaderPage` whose own `PageId` never changes!) lets restarts find the exact current root!
- **How It Works:** Think! If we store `header_page_id` in `CatalogManager` OR invoke a callback `std::function<Status(PageId)> on_root_changed` (backed by `CatalogManager::UpdateIndexRootPage` in production AND a simple variable/disk stub in standalone B+ Tree unit tests!), `BPlusTree` stays testable standalone AND integrates effortlessly with `CatalogManager` without circular dependencies! Even better: `BPlusTree` never pulls in `CatalogManager` directly!
- **Design Trade-offs:** Using a clean `RootChangeHandler` callback avoids circular dependency (`catalog` <-> `index`) while keeping `root_page_id` 100% persistent across restarts!
- **Common Mistakes:** Updating `this->root_page_id_` in memory on a root split without persisting it—so reopening the DB points at the old child leaf instead of the new internal root!
- **Understanding Checkpoint:** Why is using an `on_root_changed` callback (or header page) superior to making `BPlusTree` directly depend on `CatalogManager` headers?

### Component Context
- **Why It Exists:** Decoupled persistent root tracking across splits and shrinks.
- **Module Relationships:** Connects `BPlusTree` (`src/index`) to `CatalogManager` (`src/catalog`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree.hpp` (new — `BPlusTree` declaration & root callback hook)
- `src/index/bplus_tree.cpp` (new — constructor, `IsEmpty()`, `RootPageId()`, root-update helper)
- `tests/unit/index/bplus_tree_init_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Declare `BPlusTree` full v1 API (`GetValue`, `Insert`, `Remove`, `Begin`, `Begin(key)`, `RangeScan`, `IsEmpty`, `RootPageId`, `Destroy`) taking `BufferPoolManager&`, `BPlusTreeConfig`, initial `root_page_id`, and `RootUpdateCallback`.
2. Add constructor/init tests verifying empty tree invariants and callback invocation on root transitions.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `BPlusTree` compiles cleanly with `-Werror` and passes init/empty tests.

### Definition of Done
`BPlusTree` skeleton and root callback wired, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-121: create BPlusTree skeleton and persistent root_page_id hook`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-122` (`FindLeafPage` traversal).
````

---

## DAY-122 — Implement `BPlusTree::FindLeafPage` Traversal Using `PageGuard`s

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-122
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTree::FindLeafPage` Traversal Using `PageGuard`s
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-121 complete.

### Objective
Implement `FindLeafPageRead(IndexEntryKey)` and `FindLeafPageWrite(IndexEntryKey)` (plus `FindLeftmostLeafRead()`) descending from `root_page_id_` via `ReadPageGuard` / `WritePageGuard` hand-over-hand move-assignment so at most 1–2 frames stay pinned simultaneously (`O(1)` pin footprint regardless of tree height!), plus depth-cycle protection (`max_depth = 32`).

### Concepts & Daily Learning
- **Concept:** Hand-over-hand `PageGuard` release during root-to-leaf descent and cycle defense.
- **Why Database Engines Need It:** If `FindLeafPage` kept every ancestor pinned all the way down a 4-level tree, a 3-frame buffer pool would exhaust (`kResourceExhausted`); swapping `guard = std::move(child_guard)` releases the parent frame immediately!
- **How It Works:** If `IsEmpty()` return `std::nullopt`/not-found. Start with `guard = FetchPageRead(root_page_id_)`. Loop (bounded by `kMaxTreeDepth = 32` to guard against corrupt disk cycles): check `PageHeader::page_type`. If `kBTreeLeaf`, return `guard`! If `kBTreeInternal`, compute `child_id = internal.Lookup(target)`, fetch `child_guard`, assign `guard = std::move(child_guard)` (automatically unpinning parent!).
- **Design Trade-offs:** For `FindLeafPageWrite` during inserts/deletes that might split/merge, we can either walk down returning the leaf `WritePageGuard` and re-fetch parent via `parent_page_id` upward (safe in single-writer v1!) OR record a lightweight path vector of `PageId`s (`<= 8` bytes * depth) without pinning every ancestor! Recording `path` of `PageId`s (`O(height)` IDs, ZERO pinned frames!) is clean AND handles tiny buffer pools seamlessly!
- **Common Mistakes:** Holding a `PageGuard` on EVERY ancestor along the path when `pool_size = 4` and height = 4; failing to bound loop depth on corrupted circular pages.
- **Understanding Checkpoint:** Why is recording the ancestor `PageId` stack (`std::vector<PageId>`) while holding only 1 `PageGuard` during descent safe in v1 and friendly to tiny buffer pools?

### Component Context
- **Why It Exists:** Core search path shared by `GetValue`, `Insert`, `Remove`, `Begin(key)`, and `RangeScan`.
- **Module Relationships:** Uses `BufferPoolManager::FetchPageRead`/`FetchPageWrite`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree.hpp`
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_traversal_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FindLeafPageRead` and `FindLeftmostLeafRead` (and path-tracking `FindLeafPageForMutation`).
2. Write unit tests manually assembling a 2-level tree (1 internal root + 3 leaves) via `BufferPoolManager` and verifying traversal routes keys cleanly to the right leaf with `ActivePinCount() == 0` afterward!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Traversal finds the exact target leaf across 2 levels on a 3-frame buffer pool and leaves `ActivePinCount() == 0`.

### Definition of Done
Traversal tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-122: implement PageGuard-safe FindLeafPage traversal`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-123` (Point search `GetValue`).
````

---

## DAY-123 — Implement `BPlusTree::GetValue` Point Search (Single and Duplicate Matches)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-123
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTree::GetValue` Point Search (Single and Duplicate Matches)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-122 complete.

### Objective
Implement `Result<std::vector<RowID>> BPlusTree::GetValue(std::int64_t key)` finding the leaf containing `(key, min_RowID)` via `FindLeafPageRead` and scanning matching `key` entries forward—including crossing `leaf.NextPageId()` boundaries if duplicate keys span two or more leaves!

### Concepts & Daily Learning
- **Concept:** Point lookup across potentially multi-leaf duplicate key runs.
- **Why Database Engines Need It:** If 400 rows all share `dept_id = 5`, those `(5, rid)` entries span 2 adjacent leaves! Stopping at the end of the first leaf would silently drop half the matching rows!
- **How It Works:** Search `FindLeafPageRead({key, RowID{0, 0}})` (using minimum possible `RowID{0, 0}` so binary search lands on the very first `(key, ...)` entry!). Collect all entries with `entry.key == key`; if the scan reaches `idx == leaf.Size()` while `entry.key == key` AND `next_page_id != kInvalidPageId`, follow `next_page_id` until `entry.key > key` or end of leaves!
- **Design Trade-offs:** Checking whether `idx == leaf.Size()` after landing on `(key, {0, 0})` handles the exact separator edge case cleanly without maintaining extra backward sibling pointers on leaf nodes! Specifically, if leaf L1 ends at `(4, {9,9})` and leaf L2 starts at `(5, {2, 1})`, L2's separator key in parent is `(5, {2, 1})`. Calling `FindLeafPageRead({5, {0, 0}})` compares `{5, {0, 0}} < {5, {2, 1}}`, landing at the end of L1 (`idx == L1.Size()`), so advancing onto `L1.NextPageId()` (`L2`) finds `(5, {2, 1})` 100% accurately!
- **Common Mistakes:** Stopping `GetValue` at the end of the first leaf instead of continuing across `next_page_id`, OR failing to step into `NextPageId()` when `idx == leaf.Size()`!
- **Understanding Checkpoint:** Trace why searching for `(5, RowID{0, 0})` when L2's first key is `(5, RowID{2, 1})` can land at `idx == L1.Size()`, and how stepping onto `L1.NextPageId()` resolves it cleanly!

### Component Context
- **Why It Exists:** Point lookup primitive for `WHERE indexed_col = K`.
- **Module Relationships:** Used by `IndexScanExecutor` (Phase 07).

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_search_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTree::GetValue(int64_t key)` handling empty trees, single matches, multi-leaf duplicate runs, AND the `idx == leaf.Size()` sibling boundary edge case!
2. Write unit tests verifying that exact `(5, {2, 1})` cross-leaf boundary case explicitly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Point lookup finds 100% of matches even across leaf splits and separator boundaries; missing keys return empty vector cleanly.

### Definition of Done
`GetValue` implemented and boundary-tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-123: implement BPlusTree::GetValue handling multi-leaf duplicates`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-124` (Leaf insertion without split).
````

---

## DAY-124 — Implement `BPlusTree::Insert` Leaf-Only Insertion (No Split)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-124
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTree::Insert` Leaf-Only Insertion (No Split)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-123 complete.

### Objective
Implement `Status BPlusTree::Insert(std::int64_t key, RowID rid)` handling `StartNewTree` (when empty, allocating the first root leaf page and invoking `on_root_changed_`) and inserting directly into an existing non-full leaf via `BPlusTreeLeafPage::InsertSorted`.

### Concepts & Daily Learning
- **Concept:** Single-leaf root creation (`StartNewTree`), validating `rid.IsValid()`, and non-splitting fast path.
- **Why Database Engines Need It:** A brand-new index starts empty (`root_page_id_ == kInvalidPageId`) without wasting a disk page until the first row is inserted; most inserts (~99% in large fanout trees) hit a non-full leaf without splitting!
- **How It Works:** Reject `!rid.IsValid()`. If `IsEmpty()`: call `bpm_.NewPageGuarded(PageType::kBTreeLeaf)`, initialize leaf with `parent = kInvalidPageId`, insert `(key, rid)`, set `root_page_id_ = new_root_id`, invoke `on_root_changed_(new_root_id)`. Otherwise traverse to leaf via `FindLeafPageForMutation`, try `InsertSorted`—if it succeeds, return `Status::Ok()`! (If `kResourceExhausted`, leave split hook ready for `DAY-125`–`DAY-126`.)
- **Design Trade-offs:** Allocating the root page lazily on first insert saves pages on unused empty indexes.
- **Common Mistakes:** Accepting invalid `RowID`s into the index; forgetting to persist `root_page_id_` via `on_root_changed_` in `StartNewTree`.
- **Understanding Checkpoint:** Why must `BPlusTree::Insert` reject `!rid.IsValid()` up front?

### Component Context
- **Why It Exists:** Base insertion path for empty and non-full B+ Trees.
- **Module Relationships:** Paired with `DAY-125` (root leaf split) and `DAY-126` (internal splits).

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_insert_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `StartNewTree` and non-full leaf insertion inside `BPlusTree::Insert`.
2. Add tests inserting up to `max_leaf_size` entries (in scrambled order!), verifying `GetValue` retrieves all of them, duplicate `(key, rid)` rejects `kAlreadyExists`, and same `key` with new `rid` succeeds!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Scrambled inserts into a single-leaf tree stay sorted, persist `root_page_id`, and reject invalid `RowID` / exact duplicate.

### Definition of Done
Single-leaf insert implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-124: implement BPlusTree::StartNewTree and non-full leaf insertion`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-125` (First leaf split & new root internal page).
````

---

## DAY-125 — Implement First Leaf Split and New Internal Root Creation (`InsertIntoParent`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-125
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement First Leaf Split and New Internal Root Creation (`InsertIntoParent`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-124 complete.

### Objective
Implement leaf-split handling in `BPlusTree::Insert` and the root-creation case of `InsertIntoParent(old_node, separator_key, new_node)`: when a full root leaf splits, allocate a new `BPlusTreeLeafPage` (right sibling), split entries, allocate a new `BPlusTreeInternalPage` root, call `PopulateNewRoot`, update both leaves' `parent_page_id`, and update `root_page_id_` persistently!

### Concepts & Daily Learning
- **Concept:** Growing B+ Tree height upward (`height 1 -> height 2`) from the root while maintaining parent pointers.
- **Why Database Engines Need It:** Unlike unbalanced binary trees that grow downward at leaves (becoming skewed!), B+ Trees grow **upward at the root**, guaranteeing ALL leaves remain at identical depth!
- **How It Works:** When `leaf.InsertSorted` returns `kResourceExhausted`: allocate `right_leaf` via `NewPageGuarded`, call `left_leaf.InsertAndSplitTo(entry, right_leaf)` obtaining `sep_key`. Call `InsertIntoParent(left_page_id, sep_key, right_page_id)`. If `left_leaf` had `parent == kInvalidPageId` (was root!), allocate `new_root` (`kBTreeInternal`), `PopulateNewRoot(left_id, sep_key, right_id)`, update `left.parent = new_root_id` and `right.parent = new_root_id`, set `root_page_id_ = new_root_id`, notify `on_root_changed_`.
- **Design Trade-offs:** Checking exact duplicate BEFORE allocating `right_leaf` avoids allocating and immediately freeing a page on duplicate `(key, rid)` insert!
- **Common Mistakes:** Updating `left.parent` and forgetting `right.parent` (or forgetting `on_root_changed_`!); allocating `right_leaf` before checking duplicate!
- **Understanding Checkpoint:** Why do B+ Trees always remain 100% height-balanced when growing from 1 leaf to 2 leaves?

### Component Context
- **Why It Exists:** Transitions a tree from 1 page to multi-page height 2.
- **Module Relationships:** Extended to arbitrary height in `DAY-126`.

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_insert_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement root leaf split + `InsertIntoParent` root creation + inserting into a non-full internal parent (`height = 2` with multiple leaf splits!).
2. Test with `max_leaf_size = 4, max_internal_size = 4`: insert 12 keys -> triggers 3 leaf splits -> verifies root `PageId` updated, all 12 keys found via `GetValue`, and `ActivePinCount() == 0`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-leaf height-2 tree (`<= max_internal_size` leaves) inserts and queries all keys accurately with `ActivePinCount() == 0`.

### Definition of Done
Height-2 leaf splitting tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-125: implement leaf splitting and internal root creation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-126` (Recursive internal node splits).
````

---

## DAY-126 — Implement Recursive Internal Node Splitting (`SplitInternal`) and Arbitrary-Height Growth

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-126
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Recursive Internal Node Splitting (`SplitInternal`) and Arbitrary-Height Growth
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-125 complete.

### Objective
Complete `InsertIntoParent` for the full-parent case: when an internal node is full (`size == max_internal_size`), allocate a new sibling internal node, call `InsertAndSplitTo` (push-up middle separator!), update `parent_page_id` on all children moved to the new right internal node, and recurse/loop upward up to the root (`height 2 -> 3 -> 4...`)!

### Concepts & Daily Learning
- **Concept:** Cascading bottom-up internal splits and updating `parent_page_id` on migrated children.
- **Why Database Engines Need It:** Inserting millions of rows requires trees of height 3, 4, or more; failing to update `parent_page_id` on children moved into the new right internal node breaks subsequent deletions/merges!
- **How It Works:** When `parent` internal page is full, allocate `right_internal`, split via `InsertAndSplitTo` obtaining `pushed_up_key`, iterate every `child_id` in `right_internal[0..size-1]` (fetching each briefly via `WritePageGuard` and setting `parent_page_id = right_internal_id`!), then propagate `(parent_id, pushed_up_key, right_internal_id)` up to `parent`'s parent (or create a new internal root if `parent` was the root!).
- **Design Trade-offs:** Updating `child.parent_page_id` immediately upon moving to `right_internal` keeps parent pointers 100% accurate at the cost of `right_internal.size` short page touches during rare internal splits.
- **Common Mistakes:** Holding `left_internal` and `right_internal` AND `new_root` AND `child` guards all pinned simultaneously on a 3-frame buffer pool! (Drop `left`/`right` guards before iterating children to update `parent_page_id`, keeping simultaneous pins `<= 3`!).
- **Understanding Checkpoint:** During an internal node split, which children need their `parent_page_id` updated, and how do you avoid holding too many `WritePageGuard`s simultaneously?

### Component Context
- **Why It Exists:** Enables B+ Trees of arbitrary size and depth (`100k+` keys).
- **Module Relationships:** Completes full B+ Tree insertion mechanics!

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_insert_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement cascading `InsertIntoParent` loop/recursion and child `parent_page_id` updates with tight `PageGuard` scopes (`<= 3` frames pinned at once).
2. Test with `max_leaf_size = 4, max_internal_size = 4`: insert 200 ascending keys, 200 descending keys, and 200 shuffled keys (reaching height >= 4!), verifying ALL 200 keys retrieve accurately via `GetValue`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Ascending, descending, and shuffled multi-level inserts reach height `>= 4`, verify every key, and leave `ActivePinCount() == 0`.

### Definition of Done
Arbitrary-height insertion and internal splits passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-126: implement cascading internal node splits and child parent updates`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-127` (Duplicate-key multi-level split verification).
````

---

## DAY-127 — Verify Duplicate-Key Splitting Across Multi-Level B+ Trees

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-127
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Verify Duplicate-Key Splitting Across Multi-Level B+ Trees
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-126 complete.

### Objective
Write targeted duplicate-key stress tests (`tests/unit/index/bplus_tree_duplicate_test.cpp`) inserting 500 identical keys (`key = 42` with distinct `RowID`s in scrambled order!) flanked by neighbor keys (`41`, `43`) on small-order (`4`) trees, verifying `GetValue(42)` returns all 500 `RowID`s across 100+ leaves without dropping or mis-routing a single entry!

### Concepts & Daily Learning
- **Concept:** Multi-leaf spans of identical logical keys separated by `(key, RowID)` in internal routing nodes.
- **Why Database Engines Need It:** Low-cardinality indexed columns (e.g., `status_code = 1` on 10,000 rows) cause identical keys to span dozens of leaves and appear as internal separators `(1, rid_a)` and `(1, rid_b)`.
- **How It Works:** Verify that when `GetValue(42)` seeks `(42, {0,0})`, even if an internal separator is `(42, {10, 0})` and `FindLeafPageRead` lands at the end of a leaf containing `(41, ...)` or `(42, {1, 0})`, advancing across `next_page_id` picks up every single `42` until `entry.key > 42`!
- **Design Trade-offs:** Testing scrambled `RowID` insertion order (so new `(42, rid)` inserts land BEFORE, BETWEEN, and AFTER existing `42` leaves) catches separator-comparison bugs immediately.
- **Common Mistakes:** Stopping `GetValue` at the end of the first leaf instead of continuing across `next_page_id`; missing the case where `(42, min_rid)` lands on the leaf immediately left of the first `42`!
- **Understanding Checkpoint:** Suppose leaves L1, L2, L3 ALL contain entries with `key = 42`, and L0 contains `key = 41` while L1 starts at `(42, RowID{1, 5})`. Trace `GetValue(42)` step-by-step from root to L3!

### Component Context
- **Why It Exists:** Proves non-unique SQL index semantics hold under heavy duplicates.
- **Module Relationships:** Validates `BPlusTree::Insert` + `GetValue` + `LeafPage`/`InternalPage` comparators.

### Files Expected to Change
- `tests/unit/index/bplus_tree_duplicate_test.cpp` (new)
- `src/index/bplus_tree.cpp` (fix any cross-leaf edge case revealed)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `bplus_tree_duplicate_test.cpp` testing: 500 identical keys inserted in ascending, descending, and shuffled `RowID` order; mixed duplicate clusters (`10, 42x500, 99`); and exact `(42, {0, 0})` vs `(42, {1, 5})` boundary.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `GetValue` returns exact `RowID` sets for all clusters with zero missing or extra entries.

### Definition of Done
Duplicate-key tests passing under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-127: verify multi-level duplicate-key B+ Tree splits and lookups`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-128` (`BPlusTreeIterator` & `RangeScan`).
````

---

## DAY-128 — Implement `BPlusTreeIterator` and Range Search (`Begin`, `Begin(key)`, `RangeScan`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-128
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTreeIterator` and Range Search (`Begin`, `Begin(key)`, `RangeScan`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-127 complete.

### Objective
Implement `BPlusTree::Iterator` (`include/tinydb/index/bplus_tree_iterator.hpp`, `src/index/bplus_tree_iterator.cpp`) supporting `Begin()`, `BeginAtKey(int64_t key)`, `HasNext()`, `Next()`, `Close()`, and `RangeScan(optional<int64_t> min_key, bool min_inclusive, optional<int64_t> max_key, bool max_inclusive)` for `>`, `>=`, `<`, `<=`, and full ordered index scans!

### Concepts & Daily Learning
- **Concept:** Pull-based leaf-chain cursor over `ReadPageGuard`s supporting open/closed range bounds (`>`, `>=`, `<`, `<=`).
- **Why Database Engines Need It:** `WHERE age >= 21 AND age < 30` only traverses the tree ONCE to find `21`, then walks leaf sibling pointers horizontally in sequential cache-friendly order until `age >= 30`!
- **How It Works:** Iterator holds a single `ReadPageGuard` for the current leaf and `size_t index`. On `Next()`, return `EntryAt(index++)`; when `index == leaf.Size()`, follow `next_page_id`, swap `guard_ = FetchPageRead(next_id)` (dropping old guard immediately!), and reset `index = 0` until `kInvalidPageId`. `Close()`/`~Iterator()` drops `guard_` immediately!
- **Design Trade-offs:** Holding only 1 leaf `ReadPageGuard` during iteration allows scanning millions of keys on a 2-frame buffer pool, while documenting that caller must not mutate the tree while an iterator holds its `ReadPageGuard`!
- **Common Mistakes:** Leaving `index == leaf.Size()` at the start of `BeginAtKey(key)` (e.g. `(5, {0,0})` landing at end of left leaf) without immediately advancing to `next_page_id`! Keeping the leaf pinned past `HasNext() == false` instead of dropping `guard_` at end!
- **Understanding Checkpoint:** Why MUST `BPlusTree::Iterator` drop its `ReadPageGuard` immediately as soon as iteration reaches the end (`!HasNext()`)?

### Component Context
- **Why It Exists:** Range-scan cursor powering `IndexScanExecutor` (Phase 07).
- **Module Relationships:** Consumed by `IndexScanExecutor` (`WHERE col > X AND col <= Y`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree_iterator.hpp` (new)
- `src/index/bplus_tree_iterator.cpp` (new)
- `include/tinydb/index/bplus_tree.hpp`
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_range_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTree::Iterator` and `BPlusTree::RangeScan`.
2. Write unit tests verifying: full `Begin()` sorted scan, `>`, `>=`, `<`, `<=`, empty ranges (`lo > hi`), duplicate keys at range boundaries, and `ActivePinCount() == 0` when iterator exhausts or goes out of scope early!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All range scans (`<, <=, >, >=, [lo, hi]`) match `std::multimap` oracle 100% and leave `ActivePinCount() == 0`.

### Definition of Done
Iterator and range scans tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-128: implement BPlusTreeIterator and bounded RangeScan`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-129` (B+ Tree Validator Part 1).
````

---

## DAY-129 — Implement B+ Tree Structural Validator Part 1 (Node Bounds, Sorted Keys, Child Pointers)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-129
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement B+ Tree Structural Validator Part 1 (Node Bounds, Sorted Keys, Child Pointers)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-128 complete.

### Objective
Create `BPlusTreeValidator` (`include/tinydb/index/bplus_tree_validator.hpp`, `src/index/bplus_tree_validator.cpp`) Part 1 validating: empty tree (`root == kInvalidPageId`) cleanly, root `parent_page_id == kInvalidPageId`, non-root size bounds (`min_size <= size <= max_size`), root min size (`>= 1` leaf, `>= 2` internal), strictly sorted keys within every node, valid `RowID`s in leaves, valid child `PageId`s, cycle-free tree topology, and `child.parent_page_id == current_page_id`.

### Concepts & Daily Learning
- **Concept:** Automated structural invariant verification for disk-backed trees.
- **Why Database Engines Need It:** A B+ Tree split/merge bug can leave an underfull node (`size < min_size`), a stale `parent_page_id`, or an unsorted separator key while simple `GetValue` tests still appear to pass! A structural validator catches EVERY invariant violation immediately!
- **How It Works:** Perform DFS/BFS from `root_page_id_` maintaining a `visited` set (rejecting cycles/DAG sharing!), checking each node's header, `size` vs `min_size`/`max_size`, `parent_page_id == expected_parent`, internal key order, and leaf `RowID::IsValid()` + key order.
- **Design Trade-offs:** Returning a descriptive `Status::Corruption` explaining *which* page and *which* invariant failed makes debugging split/merge tests 10x faster.
- **Common Mistakes:** Pinning all nodes along the DFS stack simultaneously instead of releasing parent before recursing (extracting lightweight metadata or scoping `ReadPageGuard`s cleanly so validator works even on small buffer pools!).
- **Understanding Checkpoint:** Why must `BPlusTreeValidator` reject a tree where two internal nodes point to the same child `PageId` (DAG instead of tree)?

### Component Context
- **Why It Exists:** First half of the mandatory Phase 05 invariant checker.
- **Module Relationships:** Completed in `DAY-130`; run across all insert/delete/fuzz tests.

### Files Expected to Change
- `include/tinydb/index/bplus_tree_validator.hpp` (new)
- `src/index/bplus_tree_validator.cpp` (new)
- `tests/unit/index/bplus_tree_validator_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTreeValidator::Validate(const BPlusTree&)` (Part 1 checks, structured cleanly for Part 2 additions tomorrow).
2. Add positive tests on valid multi-level trees AND negative fault-injection tests (breaking `size < min_size`, wrong `parent_page_id`, unsorted leaf key, cycle in children).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid trees pass; all 4 injected structural faults fail with descriptive `kCorruption`.

### Definition of Done
Validator Part 1 implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-129: implement BPlusTreeValidator node bounds, order, and parent checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-130` (B+ Tree Validator Part 2).
````

---

## DAY-130 — Implement B+ Tree Structural Validator Part 2 (Separators, Leaf Siblings, Equal Depth, Indexed RowIDs)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-130
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement B+ Tree Structural Validator Part 2 (Separators, Leaf Siblings, Equal Depth, Indexed RowIDs)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-129 complete.

### Objective
Complete `BPlusTreeValidator` checking ALL remaining invariants from Section 6:
1. Separator-key correctness (`min_bound <= subtree_entries < max_bound` recursively across all internal levels!).
2. Equal leaf depth (every root-to-leaf path has identical depth!).
3. Leaf sibling consistency (`next_page_id` chain from leftmost leaf visits the exact ordered sequence of all DFS leaves with `last.next_page_id == kInvalidPageId` and zero cycles!).
4. Optional `TableHeap*` cross-check verifying every indexed `(key, RowID)` actually exists in the table and matches the indexed column value!

### Concepts & Daily Learning
- **Concept:** Global subtree key-range bounds (`[lo, hi)`) and topological equivalence between tree DFS leaf sequence and `next_page_id` sibling list.
- **Why Database Engines Need It:** Checking `keys[i] < keys[i+1]` locally inside one internal node doesn't catch a bug where a child subtree contains a key smaller than its ancestor separator! Passing recursive `[lower_bound, upper_bound)` bounds proves **global** B+ Tree search-tree ordering!
- **How It Works:** Recurse `(page_id, expected_parent, depth, optional<IndexEntryKey> lo, optional<IndexEntryKey> hi)` collecting `dfs_leaves` and verifying `depth == first_leaf_depth` for every leaf. Then walk from `dfs_leaves[0]` via `next_page_id` and assert exact sequence equality `sibling_walk == dfs_leaves`! Optionally scan `TableHeap` or `GetTuple(rid)` verifying indexed `RowID`s exist and match `key`!
- **Design Trade-offs:** Making `const TableHeap* expected_table = nullptr` + `size_t indexed_col = 0` an optional parameter lets standalone B+ Tree tests validate structural invariants AND lets Phase 07/09 integration tests validate table-index consistency!
- **Common Mistakes:** Checking `lo <= entry` on `lo` without checking `entry < hi` on `hi`; forgetting to check `dfs_leaves.back().next_page_id == kInvalidPageId`.
- **Understanding Checkpoint:** Suppose a split bug leaves a key `50` in `children[0]` even though `parent.keys[1] == 40`. Which exact check in `BPlusTreeValidator` catches it?

### Component Context
- **Why It Exists:** Complete gold-standard B+ Tree correctness oracle.
- **Module Relationships:** Guards every insert/delete/stress/recovery test in Phases 05, 07, 08, and 09!

### Files Expected to Change
- `include/tinydb/index/bplus_tree_validator.hpp`
- `src/index/bplus_tree_validator.cpp`
- `tests/unit/index/bplus_tree_validator_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Complete `BPlusTreeValidator::Validate(const BPlusTree&, const TableHeap* = nullptr, std::size_t col_idx = 0)`.
2. Write negative tests injecting: wrong separator bound, unequal leaf depth, broken `next_page_id` sibling chain (skipping a leaf or trailing loop), and invalid/stale `RowID` against a `TableHeap`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every structural invariant passes on valid trees and fails on every injected fault.

### Definition of Done
Complete validator implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-130: complete BPlusTreeValidator separator, sibling, depth, and RowID checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-131` (Running validator after every insert).
````

---

## DAY-131 — Run Step-by-Step Invariant Validation Across Multi-Level Insertions

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-131
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Run Step-by-Step Invariant Validation Across Multi-Level Insertions
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-130 complete.

### Objective
Upgrade insertion tests (`tests/unit/index/bplus_tree_insert_test.cpp`) to invoke `BPlusTreeValidator::Validate` after EVERY single `Insert` across small-order (`4`, `5`, `7`), ascending, descending, duplicate-heavy, and random sequences—fixing any subtle split/parent/separator bug uncovered!

### Concepts & Daily Learning
- **Concept:** Stepwise invariant checking ("fail at the exact insert step that broke the tree").
- **Why Database Engines Need It:** If a subtle separator bug occurs at insert #47 and isn't noticed until insert #150, debugging without stepwise validation is miserable; checking after every mutation isolates bugs immediately!
- **How It Works:** Loop `for (auto& [k, rid] : inputs) { ASSERT_TRUE(tree.Insert(k, rid).ok()); ASSERT_TRUE(BPlusTreeValidator::Validate(tree).ok()); }` across order `4` (even), order `5` (odd), and order `7`.
- **Design Trade-offs:** Stepwise validation is `O(N^2)` in test suites, so keep per-test `N ~ 150–300` keys for instant runs while validating thousands of steps across seeds!
- **Common Mistakes:** Only validating at the end of 500 inserts instead of after every single split.
- **Understanding Checkpoint:** Why is testing BOTH even (`max_size = 4`) and odd (`max_size = 5`) node capacities essential for split math (`(max_size + 1) / 2`)?

### Component Context
- **Why It Exists:** Proves all Phase 05 insertion paths produce structurally pristine B+ Trees.
- **Module Relationships:** Exercises `BPlusTree::Insert` + `BPlusTreeValidator`.

### Files Expected to Change
- `tests/unit/index/bplus_tree_insert_test.cpp`
- `src/index/bplus_tree.cpp` (any fix needed if validator catches an edge case!)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add stepwise `BPlusTreeValidator::Validate` assertions across orders `4, 5, 7` and ascending/descending/shuffled/duplicate sequences.
2. Confirm `ActivePinCount() == 0` after every step.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 100% of insertion steps pass `BPlusTreeValidator::Validate` across even and odd orders with zero pin leaks.

### Definition of Done
Stepwise insert validation passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-131: enforce step-by-step BPlusTreeValidator checks across insertions`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-132` (`BPlusTree::Remove` without underflow).
````

---

## DAY-132 — Implement `BPlusTree::Remove` Leaf Deletion Without Underflow

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-132
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement `BPlusTree::Remove` Leaf Deletion Without Underflow
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-131 complete.

### Objective
Implement `Status BPlusTree::Remove(std::int64_t key, RowID rid)` finding the target leaf (handling exact `(key, rid)` removal cleanly, returning `StatusCode::kNotFound` if absent), deleting from root leaf directly (setting `root_page_id_ = kInvalidPageId` and deallocating page via `bpm_.DeletePage` when root leaf becomes empty!), and committing non-underflowing leaf deletions (`size >= min_size`).

### Concepts & Daily Learning
- **Concept:** Root leaf emptying (`tree becomes empty`) and non-underflowing leaf deletion.
- **Why Database Engines Need It:** Deleting the very last key in a B+ Tree must reclaim the root page cleanly (`DeletePage`) and mark the tree empty (`root_page_id = kInvalidPageId`) so `CatalogManager` persists the empty state and doesn't leak orphan pages!
- **How It Works:** If `IsEmpty()` return `kNotFound`. Find leaf containing `(key, rid)`. Call `leaf.Remove({key, rid})`; if missing return `kNotFound`. If `leaf.IsRoot()`: if `leaf.Size() == 0`, drop guard, call `bpm_.DeletePage(root_id)`, set `root_page_id_ = kInvalidPageId`, notify `on_root_changed_(kInvalidPageId)`. If non-root and `leaf.Size() >= leaf.GetMinSize()`, done! (Underflow triggers `CoalesceOrRedistribute` implemented in `DAY-133`–`DAY-137`!)
- **Design Trade-offs:** Removing exact `(key, rid)` (rather than removing all entries matching `key`) ensures `DELETE FROM t WHERE id = 5 AND name = 'bob'` or `UPDATE` only removes the specific `RowID` deleted from `TableHeap`!
- **Common Mistakes:** Leaving an empty root leaf page (`size == 0`) alive in the tree; trying to call `DeletePage(root_id)` while `WritePageGuard` on `root_id` is STILL pinned! (Must `guard.Drop()` BEFORE `bpm_.DeletePage`!).
- **Understanding Checkpoint:** Why MUST you call `leaf_guard.Drop()` BEFORE calling `bpm_.DeletePage(leaf_page_id)` when the last entry in a root leaf is deleted?

### Component Context
- **Why It Exists:** Foundation of B+ Tree deletion (`Remove`).
- **Module Relationships:** Prepares for leaf/internal rebalancing (`DAY-133`–`DAY-137`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree.hpp`
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTree::Remove(key, rid)` for empty tree, missing key, root leaf deletion-to-empty, and non-underflowing non-root deletion.
2. Add tests verifying single-leaf insert + delete-all leaves `IsEmpty() == true`, `ActivePinCount() == 0`, and `FreePageCount()` increases cleanly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Emptying root leaf frees disk page (`FreePageCount()` increments) and passes `BPlusTreeValidator`.

### Definition of Done
Base `Remove` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-132: implement BPlusTree::Remove base deletion and empty-root cleanup`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-133` (Deletion underflow: leaf redistribution/borrowing).
````

---

## DAY-133 — Implement Leaf Deletion Underflow: Sibling Redistribution (Borrow Left / Right)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-133
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Leaf Deletion Underflow: Sibling Redistribution (Borrow Left / Right)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-132 complete.

### Objective
Implement `CoalesceOrRedistribute` sibling lookup (finding left sibling `idx - 1` or right sibling `idx + 1` via `parent_page_id`) and **leaf redistribution** (`RedistributeLeaf`): borrowing the last entry from a rich left sibling (`left.Size() > left.GetMinSize()`) or first entry from a rich right sibling (`right.Size() > right.GetMinSize()`) and updating the parent's separator key!

### Concepts & Daily Learning
- **Concept:** Borrow-before-merge balancing in B+ Trees and parent separator updates on leaf redistribution.
- **Why Database Engines Need It:** Borrowing one entry from an adjacent sibling is an `O(1)` local 3-page operation that avoids freeing a page and cascading merges up the tree!
- **How It Works:** Look up `node_idx = parent.ChildIndex(node_id)`. Try sibling (`left` if `node_idx > 0`, else `right` if `node_idx + 1 < parent.Size()`—actually check BOTH siblings if `0 < node_idx < parent.Size() - 1` or check `left` first then `right` for borrowing!).
  - Borrow from left (`index > 0`): `left.MoveLastToFrontOf(node)` -> update `parent.SetKeyAt(node_idx, node.EntryAt(0))`!
  - Borrow from right (`index == 0` or left couldn't spare): `right.MoveFirstToEndOf(node)` -> update `parent.SetKeyAt(right_idx, right.EntryAt(0))`!
- **Design Trade-offs:** Checking *either* sibling that has `> min_size` before merging maximizes page utilization (fewer merges), though checking immediate neighbor is also standard—either way is valid; let's check left (if `idx > 0 && left.Size() > left.GetMinSize()`), else right (if `idx + 1 < parent.Size() && right.Size() > right.GetMinSize()`), else merge!
- **Common Mistakes:** Updating `parent.SetKeyAt(node_idx, ...)` with the wrong index when borrowing from the *right* sibling (`right_idx = node_idx + 1`, NOT `node_idx`!)!
- **Understanding Checkpoint:** When leaf `node` (at `parent` child index `0`) borrows the first entry from right sibling `right` (at index `1`), which `parent.keys[k]` index updates, and to what value?

### Component Context
- **Why It Exists:** Restores leaf `min_size` without shrinking tree height when a sibling has surplus entries.
- **Module Relationships:** Invoked inside `BPlusTree::Remove` (`DAY-132`) and paired with leaf merge (`DAY-134`).

### Files Expected to Change
- `include/tinydb/index/bplus_tree.hpp`
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `RedistributeLeaf` (left and right cases) inside `CoalesceOrRedistribute`.
2. Write targeted unit tests forcing both borrow-from-left AND borrow-from-right on a 2-level tree and running `BPlusTreeValidator::Validate` after every delete!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both left-borrow and right-borrow pass `BPlusTreeValidator::Validate` with exact separator keys and zero pin leaks.

### Definition of Done
Leaf redistribution implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-133: implement leaf sibling redistribution on B+ Tree underflow`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-134` (Leaf merging & sibling pointer updates).
````

---

## DAY-134 — Implement Leaf Deletion Underflow: Leaf Merging (`CoalesceLeaf`) and Sibling Pointer Updates

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-134
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Leaf Deletion Underflow: Leaf Merging (`CoalesceLeaf`) and Sibling Pointer Updates
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-133 complete.

### Objective
Implement `CoalesceLeaf`: when neither left nor right sibling can spare an entry, merge the right-hand leaf into the left-hand leaf (`left.MoveAllTo(right)`? Wait: always normalize `(left, right)` so if `node` was `child[0]`, merge `right` into `node` (`node = left, neighbor = right`)! Else merge `node` into `left` (`left = neighbor, right = node`)!), update `left.SetNextPageId(right.NextPageId())`, remove `right`'s separator/pointer from `parent`, drop `right`'s guard, call `bpm_.DeletePage(right_id)`, and check if `parent` now underflows!

### Concepts & Daily Learning
- **Concept:** Canonical `(left, right)` normalization before merging, `next_page_id` splicing, and freeing the merged right page.
- **Why Database Engines Need It:** Normalizing `(left, right)` cuts merge code in half (one single merge direction: `right` ALWAYS folds into `left`, and `right` is ALWAYS deleted!); forgetting to splice `left.next = right.next` leaves a dangling freed `PageId` in the leaf chain!
- **How It Works:** If `node_idx == 0`, swap roles so `left = node, right = sibling_1, remove_idx = 1`. Otherwise `left = sibling_{node_idx-1}, right = node, remove_idx = node_idx`. Call `right.MoveAllTo(left)`, `left.SetNextPageId(right.NextPageId())`, release `right` guard and `bpm_.DeletePage(right_id)`, call `parent.RemoveAt(remove_idx)`.
- **Design Trade-offs:** Always merging right-into-left ensures `left`'s leftmost key never changes, so `left`'s own separator key in its grandparent remains valid!
- **Common Mistakes:** Merging `left` into `right` when `node_idx == 0` WITHOUT updating `left_neighbor->next_page_id` or deleting `right` while a `WritePageGuard` still pins it!
- **Understanding Checkpoint:** Why does merging `right` into `left` (`left.next = right.next`, deleting `right`) guarantee we never need to find `left`'s *left* sibling in another subtree just to fix `next_page_id`?

### Component Context
- **Why It Exists:** Reclaims underutilized leaf pages and preserves the contiguous leaf sibling chain.
- **Module Relationships:** Feeds parent underflow upward (`DAY-135`–`DAY-137`).

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CoalesceLeaf` inside `CoalesceOrRedistribute` (with root shrink check when `parent` is root and `parent.Size() == 1`!).
2. Test merging leaves on `node_idx == 0` AND `node_idx > 0`, verifying `BPlusTreeValidator` AND `FreePageCount()` reclaiming merged pages!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Leaf merges splice `next_page_id`, reclaim merged pages cleanly, and pass `BPlusTreeValidator`.

### Definition of Done
Leaf merge implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-134: implement B+ Tree leaf Coalesce merge and sibling splicing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-135` (Internal node redistribution).
````

---

## DAY-135 — Implement Internal Node Redistribution (Pull-Down / Push-Up Separator Rotation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-135
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Internal Node Redistribution (Pull-Down / Push-Up Separator Rotation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-134 complete.

### Objective
Implement internal node borrowing (`RedistributeInternal`): when an internal node underflows (`size < min_size`) and a left or right sibling internal node has `> min_size` children, rotate one child + parent separator key through `parent`, and update the moved child's `parent_page_id`!

### Concepts & Daily Learning
- **Concept:** Internal node separator rotation (`parent.key` pulls down into borrower; borrowed edge separator pushes up to `parent`) and `child.parent_page_id` repair.
- **Why Database Engines Need It:** You CANNOT just move `right.keys[1]` into `node` directly—the separator between `node`'s last child and `right`'s first child lives in **`parent.keys[right_idx]`**! Rotating through `parent` preserves strict search-tree partitioning!
- **How It Works:**
  - Borrow from left internal (`idx > 0`): `middle_key = parent.KeyAt(idx)`. Call `left.MoveLastToFrontOf(node, middle_key)` -> moves `left`'s last child to `node.children[0]`, puts `middle_key` at `node.keys[1]`, and returns `new_parent_key` (which was `left`'s last key!). Set `parent.SetKeyAt(idx, new_parent_key)`. Fetch moved `child` and update `child.parent_page_id = node_id`!
  - Borrow from right internal (`right_idx = idx + 1`): `middle_key = parent.KeyAt(right_idx)`. Call `right.MoveFirstToEndOf(node, middle_key)` -> moves `right.children[0]` to end of `node` with `middle_key`, returns `right`'s old `keys[1]` to install in `parent.SetKeyAt(right_idx, new_parent_key)`. Update moved child's `parent_page_id = node_id`!
- **Design Trade-offs:** Updating only the single moved child's `parent_page_id` takes `O(1)` page touches.
- **Common Mistakes:** Forgetting to update the moved child's `parent_page_id` (breaking future deletions inside that subtree!); pulling wrong key into `parent`!
- **Understanding Checkpoint:** Trace an internal borrow-from-right step by step: which key ends up at `node.keys[end]`, which key moves up to `parent`, and which child's `parent_page_id` changes?

### Component Context
- **Why It Exists:** Rebalances internal B+ Tree levels (`height >= 3`) without cascading merges.
- **Module Relationships:** Pairs with internal merging (`DAY-136`).

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `RedistributeInternal` (left & right) cleanly inside `CoalesceOrRedistribute`.
2. Write unit tests building a height-3 (`max_size = 4`) tree and triggering internal borrow-from-left and borrow-from-right, verified by `BPlusTreeValidator::Validate`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Height-3 internal redistributions pass `BPlusTreeValidator::Validate` (including `parent_page_id` checks on moved subtrees!).

### Definition of Done
Internal redistribution implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-135: implement internal node redistribution with separator rotation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-136` (Internal node merging & cascading underflow).
````

---

## DAY-136 — Implement Internal Node Merging and Cascading Underflow to Root

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-136
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Internal Node Merging and Cascading Underflow to Root
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-135 complete.

### Objective
Implement `CoalesceInternal` (normalizing `(left, right)`, pulling `parent.KeyAt(right_idx)` down between `left` and `right` via `right.MoveAllTo(left, middle_key)`, updating `parent_page_id = left_id` on all children moved from `right`, freeing `right_id`, removing `right_idx` from `parent`, and cascading `CoalesceOrRedistribute(parent)` upward iteratively/recursively!).

### Concepts & Daily Learning
- **Concept:** Cascading bottom-up B+ Tree merges (`height >= 3`) and pull-down of parent separator during internal coalescing.
- **Why Database Engines Need It:** Deleting half a database table causes internal nodes to shrink below `min_size`; merging two internal nodes pulls one separator down from their parent—which can itself cause the parent to underflow, cascading upward to the root!
- **How It Works:** Normalize `(left, right, remove_idx)`. Pull `middle_key = parent.KeyAt(remove_idx)`. Move all entries from `right` into `left` with `middle_key` (`left` now has `left.Size() + right.Size()` children). Update `parent_page_id = left_id` on every child that came from `right`. Release `right` guard and `bpm_.DeletePage(right_id)`. Call `parent.RemoveAt(remove_idx)`. If `parent` is root or underflows (`size < parent.GetMinSize()`), continue rebalancing `parent`!
- **Design Trade-offs:** Using an iterative `while (underflow)` loop (or clean bounded recursion <= height `32`) keeps stack depth tiny and Buffer Pool pin count bounded (`<= 3` simultaneous pins by releasing child guards before ascending!).
- **Common Mistakes:** Holding `node` + `sibling` + `parent` + `child` guards all pinned while recursing up a height-4 tree (pin-exhausting small pools!); forgetting to update `parent_page_id` on children moved from `right` to `left`!
- **Understanding Checkpoint:** Why must `CoalesceInternal` drop its `PageGuard`s on `left` and `right` BEFORE ascending to rebalance `parent`?

### Component Context
- **Why It Exists:** Completes multi-level internal node shrinking.
- **Module Relationships:** Paired with `AdjustRoot` (`DAY-137`).

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CoalesceInternal` and iterative/bounded upward propagation inside `CoalesceOrRedistribute`.
2. Test cascading merges across a height-3/4 small-order tree (`pool_size = 5`), verifying `BPlusTreeValidator` passes at every step!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Cascading multi-level deletes pass `BPlusTreeValidator` with zero pin exhaustion on `pool_size = 5`.

### Definition of Done
Internal merges and cascading propagation tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-136: implement internal node merging and cascading rebalance`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-137` (Root shrinking `AdjustRoot`).
````

---

## DAY-137 — Implement Root Shrinking (`AdjustRoot`: Promoting Sole Child and Reclaiming Old Root)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-137
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement Root Shrinking (`AdjustRoot`: Promoting Sole Child and Reclaiming Old Root)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-136 complete.

### Objective
Implement `AdjustRoot(old_root_id)` handling when an internal root shrinks to `size == 1` (only 1 child left after its 2 children merged!): promote `sole_child_id` as the new `root_page_id_`, set `sole_child.parent_page_id = kInvalidPageId`, deallocate `old_root_id` via `bpm_.DeletePage`, notify `on_root_changed_(sole_child_id)`, and reduce tree height by `1` cleanly!

### Concepts & Daily Learning
- **Concept:** Height contraction (`height 3 -> 2 -> 1 -> 0`) when the root internal node has only 1 child remaining.
- **Why Database Engines Need It:** If an internal root stays alive with 1 child (`size == 1`), `GetMinSize()` for internal root (`>= 2`) is violated and empty wrapper levels waste I/O; promoting the sole child keeps height `O(log_M N)` even after 99% of rows are deleted!
- **How It Works:** When `node.IsRoot()`:
  - If `node` is internal and `node.Size() == 1`: extract `new_root_id = node.ChildAt(0)`, drop `node_guard`, fetch `new_root_id` and set `parent_page_id = kInvalidPageId`, call `bpm_.DeletePage(old_root_id)`, update `root_page_id_ = new_root_id`, invoke `on_root_changed_(new_root_id)`.
  - If `node` is leaf and `node.Size() == 0`: handled cleanly (`root_page_id_ = kInvalidPageId`, `DeletePage`).
- **Design Trade-offs:** Reclaiming old root pages immediately keeps `FreePageCount()` accurate.
- **Common Mistakes:** Leaving `new_root.parent_page_id` pointing to the deleted `old_root_id` instead of `kInvalidPageId`! Calling `DeletePage(old_root_id)` while holding `old_root` pinned!
- **Understanding Checkpoint:** Trace deleting keys from a height-3 tree all the way down to 0 keys: how many times does `AdjustRoot` fire, and what must `root_page_id_` and `FreePageCount()` be at the end?

### Component Context
- **Why It Exists:** Completes full symmetric height growth and height shrinkage of the B+ Tree.
- **Module Relationships:** Completes `BPlusTree::Remove`!

### Files Expected to Change
- `src/index/bplus_tree.cpp`
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `AdjustRoot` cleanly inside `BPlusTree`.
2. Add tests shrinking a height-4 tree all the way to height 3 -> 2 -> 1 -> empty (`IsEmpty() == true`), verifying `BPlusTreeValidator` after EVERY delete and confirming all pages are returned to `DiskManager` freelist!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Height shrinks smoothly from `4 -> 3 -> 2 -> 1 -> 0`, every intermediate step passes `BPlusTreeValidator`, and `ActivePinCount() == 0`.

### Definition of Done
`AdjustRoot` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-137: implement B+ Tree AdjustRoot height contraction`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-138` (Sequential, reverse & random delete validation).
````

---

## DAY-138 — Test Sequential, Reverse, and Random Deletions with Stepwise Validator Checks

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-138
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Test Sequential, Reverse, and Random Deletions with Stepwise Validator Checks
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-137 complete.

### Objective
Expand `tests/unit/index/bplus_tree_delete_test.cpp` to delete 250+ keys in ascending order (always stressing left-edge `node_idx == 0` merges!), descending order (stressing right-edge merges!), and multiple random permutation orders across even and odd node sizes (`4`, `5`, `6`), validating `BPlusTreeValidator` and remaining keys after EVERY single `Remove`!

### Concepts & Daily Learning
- **Concept:** Adversarial deletion orders (left-to-right, right-to-left, and random) exposing edge asymmetry bugs.
- **Why Database Engines Need It:** Ascending deletes repeatedly empty `children[0]` (always exercising `node_idx == 0` merges & root shifts), whereas descending deletes always shrink the rightmost branch; testing both catches asymmetric index bugs!
- **How It Works:** Build 250-key trees (both unique and duplicate keys), remove keys one-by-one in each order, verify `GetValue(deleted_key)` misses, verify `BPlusTreeValidator`, and verify remaining keys match golden set via `Begin()` iterator!
- **Design Trade-offs:** Checking `Begin()` + `BPlusTreeValidator` at every step guarantees both physical structure and logical scan order stay intact.
- **Common Mistakes:** Testing only random deletions and missing edge bugs that only appear in pure ascending/descending tear-down.
- **Understanding Checkpoint:** Why do pure ascending deletions expose different merge code paths than pure descending deletions?

### Component Context
- **Why It Exists:** Comprehensive correctness proof for `BPlusTree::Remove`.
- **Module Relationships:** Validates `BPlusTree` deletion under all orders.

### Files Expected to Change
- `tests/unit/index/bplus_tree_delete_test.cpp`
- `src/index/bplus_tree.cpp` (fix any edge bug surfaced)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement ascending, descending, and shuffled full-teardown test cases for orders `4, 5, 6` (with and without duplicates).
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 100% of deletion sequences empty cleanly to `IsEmpty() == true` and pass `BPlusTreeValidator` at every single step!

### Definition of Done
Deletion verification suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-138: test ascending, descending, and random B+ Tree deletions with validator`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-139` (Mixed insert/delete/split/merge vs oracle).
````

---

## DAY-139 — Test Interleaved Mixed Insert/Delete/Split/Merge Workloads Against `std::multimap` Oracle

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-139
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Test Interleaved Mixed Insert/Delete/Split/Merge Workloads Against `std::multimap` Oracle
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-138 complete.

### Objective
Write interleaved workload tests (`tests/unit/index/bplus_tree_mixed_test.cpp`) mixing repeated growth/shrink waves (`insert 100 -> delete 70 -> insert 150 -> delete 180 -> refill`) and random mixed ops verified against a `std::set<IndexEntryKey>` / `std::multimap` oracle, `RangeScan`, and `BPlusTreeValidator`.

### Concepts & Daily Learning
- **Concept:** Cyclic growth-shrink oscillation and multi-wave split/merge equivalence against a mathematical set/multimap oracle.
- **Why Database Engines Need It:** Real databases don't just insert everything then delete everything; pages constantly split, partially empty, reuse freed `PageId`s, and split again!
- **How It Works:** Alternate waves of inserts and deletes over 1,500+ total ops on small-order trees (`order = 4, 5`), cross-checking full `Begin()` iterator output and `RangeScan` queries against the oracle at periodic checkpoints and `BPlusTreeValidator` after mutations.
- **Design Trade-offs:** Checking full `Begin()` equality every 25 ops keeps tests super fast (`< 1s`) while validating exact ordering.
- **Common Mistakes:** Leaving stale data inside a recycled freed page that re-enters the B+ Tree when `NewPage` reuses a `PageId`!
- **Understanding Checkpoint:** Why do multi-wave insert/delete workloads expose stale-freed-page bugs that pure insert-all-then-delete-all tests miss?

### Component Context
- **Why It Exists:** Validates tree-page recycling and mixed churn.
- **Module Relationships:** Exercises `BPlusTree` + `BufferPoolManager` + `DiskManager` freelist reuse!

### Files Expected to Change
- `tests/unit/index/bplus_tree_mixed_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement multi-wave and interleaved random insert/delete/range-scan tests in `bplus_tree_mixed_test.cpp`.
2. Verify `BPlusTreeValidator::Validate` and `ActivePinCount() == 0` throughout.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Mixed churn matches oracle 100%, passes `BPlusTreeValidator`, and returns zero pin leaks.

### Definition of Done
Mixed workload tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-139: verify interleaved B+ Tree insert/delete waves against oracle`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-140` (Buffer pool pinning audit).
````

---

## DAY-140 — Audit Buffer Pool Pinning Across Every `BPlusTree` Path (Zero Pin Leaks)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-140
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Audit Buffer Pool Pinning Across Every `BPlusTree` Path (Zero Pin Leaks)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-139 complete.

### Objective
Audit every single `FetchPageRead`, `FetchPageWrite`, `NewPageGuarded`, and early `return` path in `src/index/*.cpp` to verify `ActivePinCount() == 0` after EVERY operation (success, duplicate-key error, missing-key delete error, `kResourceExhausted`, corruption error, and dropped/exhausted iterators!), and add dedicated pin-audit tests (`tests/unit/index/bplus_tree_pin_audit_test.cpp`).

### Concepts & Daily Learning
- **Concept:** Error-path pin leak auditing and RAII guard discipline.
- **Why Database Engines Need It:** A pin leak on an error path (`Remove` missing key or `Insert` duplicate) silently consumes 1 buffer frame per failed query until the database deadlocks!
- **How It Works:** Inspect every function in `bplus_tree.cpp` and `bplus_tree_iterator.cpp`; write tests explicitly asserting `bpm.ActivePinCount() == 0` after: empty-tree `GetValue`/`Remove`/`Begin`, missing-key `Remove`, duplicate `(key, rid)` `Insert`, multi-leaf `RangeScan` aborted after 1st result (iterator goes out of scope early!), and split/merge cycles!
- **Design Trade-offs:** Using `ReadPageGuard` / `WritePageGuard` exclusively (zero raw `FetchPage`/`UnpinPage`) makes leak prevention structural.
- **Common Mistakes:** Holding an iterator open in a test variable while calling `Insert`/`Remove` on the same tiny pool!
- **Understanding Checkpoint:** Why must aborting a `RangeScan` iterator after reading just the first 2 matching keys still immediately restore `bpm.ActivePinCount()` to `0`?

### Component Context
- **Why It Exists:** Guarantees memory safety of `BPlusTree` + `BufferPoolManager` under errors and partial scans.
- **Module Relationships:** Audits `BPlusTree` + `BPlusTreeIterator` + `BufferPoolManager`.

### Files Expected to Change
- `src/index/bplus_tree.cpp`, `src/index/bplus_tree_iterator.cpp` (tighten any guard scopes if needed)
- `tests/unit/index/bplus_tree_pin_audit_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit all `src/index/*.cpp` guard lifetimes.
2. Add `bplus_tree_pin_audit_test.cpp` checking `ActivePinCount() == 0` after every success and error path.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every single error and early-exit path leaves `ActivePinCount() == 0`.

### Definition of Done
Pin audit complete and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-140: audit B+ Tree Buffer Pool pinning on all success and error paths`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-141` (Small buffer pool eviction stress on height >= 3 tree).
````

---

## DAY-141 — Stress-Test Height >= 3 B+ Tree Under Tiny 5-Frame Buffer Pool

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-141
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Stress-Test Height >= 3 B+ Tree Under Tiny 5-Frame Buffer Pool
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-140 complete.

### Objective
Write a harsh eviction-pressure integration test (`tests/integration/phase05_tiny_pool_btree_test.cpp`) running a 4-level B+ Tree (`> 40` pages, `max_size = 4`, `pool_size = 5`) through full insert, search, range scan, and delete-to-empty, verifying forced evictions (`> 500`), zero pin exhaustion, and full validator pass!

### Concepts & Daily Learning
- **Concept:** Bounding simultaneous pinned frames (`<= 3–4`) across deep splits, merges, and redistributions.
- **Why Database Engines Need It:** If a tree has 1,000+ pages on a real system with constrained buffer memory, splits/merges must NEVER deadlock waiting for frames held unnecessarily by the same thread!
- **How It Works:** Configure `pool_size = 5` (or even `4`—test exact minimum pin footprint needed during `CoalesceInternal`!). Track `bpm.GetStats().evictions` and `ActivePinCount()`. Verify `Insert` + `Remove` + `RangeScan` never hit `kResourceExhausted`.
- **Design Trade-offs:** Tightly scoping guards in `CoalesceOrRedistribute` (dropping `left` and `right` before touching `children` or ascending to `grandparent`) guarantees `<= 4` simultaneous pins even on 4-level trees!
- **Common Mistakes:** Holding `parent_guard` AND `node_guard` AND `sibling_guard` AND `grandparent_guard` all open at the same time (4–5 frames!).
- **Understanding Checkpoint:** Count the maximum simultaneous `PageGuard`s held at any instant in `InsertIntoParent` and `CoalesceInternal`—how can you keep that count `<= 3` or `<= 4`?

### Component Context
- **Why It Exists:** Verifies B+ Tree memory footprint scales independently of tree height/page count.
- **Module Relationships:** Stress-tests `BPlusTree` + `BufferPoolManager`.

### Files Expected to Change
- `tests/integration/phase05_tiny_pool_btree_test.cpp` (new)
- `src/index/bplus_tree.cpp` (tighten guard scope if `pool_size = 5` ever hits exhaustion!)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Build `phase05_tiny_pool_btree_test.cpp` with `pool_size = 5` and a height-4 B+ Tree (`~300` keys).
2. Assert `evictions > 100`, `BPlusTreeValidator` passes, and `ActivePinCount() == 0`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Height-4 B+ Tree executes full lifecycle on a 5-frame buffer pool without pin exhaustion.

### Definition of Done
Tiny-pool B+ Tree stress test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-141: stress-test height-4 B+ Tree under 5-frame buffer eviction`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-142` (Index persistence & reload after restart).
````

---

## DAY-142 — Verify B+ Tree Persistence and Reload Across Full Database Restarts

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-142
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Verify B+ Tree Persistence and Reload Across Full Database Restarts
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-141 complete.

### Objective
Write cold-restart persistence tests (`tests/integration/phase05_btree_persistence_test.cpp`) confirming a B+ Tree survives `Shutdown() -> Close() -> Reopen()` across multiple stages: empty creation, multi-level insert (root page ID changed!), partial deletes causing height shrink (`root_page_id` changed again!), reload via persisted `root_page_id`, further inserts/searches, and full `BPlusTreeValidator` verification!

### Concepts & Daily Learning
- **Concept:** Cold reload of multi-level disk trees solely from persisted `.tinydb` pages + `root_page_id`.
- **Why Database Engines Need It:** If any node field (`next_page_id`, `parent_page_id`, `max_size`, `root_page_id`) wasn't flushed or deserialized properly, the tree looks fine in RAM and breaks silently after restart!
- **How It Works:** Run Stage 1 (build height-3 tree, flush/shutdown, destroy all managers), Stage 2 (reopen cold with persisted `root_page_id`, verify `BPlusTreeValidator` + `GetValue` + `RangeScan`, delete 60% causing root shrink!), Stage 3 (reopen cold again with shrunk `root_page_id`, verify `BPlusTreeValidator` + insert new keys!).
- **Design Trade-offs:** Notice `max_leaf_size` / `max_internal_size` stored inside each node's `BPlusTreeNodeHeader` ensures a reloaded node remembers its exact capacity!
- **Common Mistakes:** Updating a node buffer via `Page*` without using `WritePageGuard` (so `is_dirty` stays `false` and changes never flush on `Shutdown()`!).
- **Understanding Checkpoint:** Why is storing `max_size` inside `BPlusTreeNodeHeader` on disk helpful when reloading an index page?

### Component Context
- **Why It Exists:** Proves B+ Tree durability across restarts before integrating with `CatalogManager`.
- **Module Relationships:** Exercises `BPlusTree` + `BufferPoolManager::Shutdown` + `DiskManager::Open`.

### Files Expected to Change
- `tests/integration/phase05_btree_persistence_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement multi-stage restart persistence test in `phase05_btree_persistence_test.cpp`.
2. Verify all stages pass `BPlusTreeValidator::Validate` under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-restart grow/shrink/reload cycles preserve 100% of indexed keys and structural invariants.

### Definition of Done
Persistence integration test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-142: verify B+ Tree persistence and reload across database restarts`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-143` (Corrupt B+ Tree node rejection).
````

---

## DAY-143 — Test Corrupt B+ Tree Node Rejection (Invalid Counts, Cycles, Wrong Page Types)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-143
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Test Corrupt B+ Tree Node Rejection (Invalid Counts, Cycles, Wrong Page Types)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-142 complete.

### Objective
Write adversarial tests (`tests/unit/index/bplus_tree_corruption_test.cpp`) verifying `BPlusTree` (`GetValue`, `Insert`, `Remove`, `Begin`, `RangeScan`) and `BPlusTreeValidator` reject corrupt node states cleanly without infinite loops or crashes: cyclic internal pointers, cyclic leaf `next_page_id`, `kTableHeap` page referenced as tree node, out-of-range `size`, and invalid `RowID`s!

### Concepts & Daily Learning
- **Concept:** Cycle defense (`kMaxTreeDepth`, `visited` sets / page-count cap) and runtime `PageType` checks during traversal.
- **Why Database Engines Need It:** A cyclic `next_page_id` in a corrupted leaf chain turns `RangeScan` into an infinite loop hanging the database server unless bounded!
- **How It Works:** In `BPlusTree::FindLeafPage` and `BPlusTreeIterator`: track steps/visited count against `max_depth = 32` (for vertical descent) and `max_pages = DiskManager::PageCount()` (or reasonable cycle guard in `BPlusTreeIterator` / `GetValue`), returning `StatusCode::kCorruption` if exceeded or if `PageType` isn't `kBTreeLeaf`/`kBTreeInternal`!
- **Design Trade-offs:** Checking `steps > max_tree_pages` in horizontal leaf iteration prevents infinite loops with tiny `O(1)` integer counter overhead!
- **Common Mistakes:** Assuming `BPlusTreeIterator` can loop `while (next_page_id != kInvalidPageId)` forever without bounding against corrupt self-loops (`leaf.next_page_id == leaf.page_id`)!
- **Understanding Checkpoint:** How does checking `next_page_id != current_page_id` plus a step bound `<= page_count` protect `GetValue` and `BPlusTreeIterator` against corrupt leaf cycles in `O(1)` memory?

### Component Context
- **Why It Exists:** Hardens B+ Tree runtime paths against malformed or tampered pages.
- **Module Relationships:** Hardens `BPlusTree` + `BPlusTreeIterator` + `BPlusTreeValidator`.

### Files Expected to Change
- `src/index/bplus_tree.cpp`, `src/index/bplus_tree_iterator.cpp` (add leaf-cycle & `PageType` checks)
- `tests/unit/index/bplus_tree_corruption_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Harden `FindLeafPage`, `GetValue`, and `BPlusTreeIterator` against cycles and wrong `PageType`s.
2. Add tests injecting leaf self-cycle, 2-node leaf cycle, vertical internal cycle, `kTableHeap` page in tree, and oversized `size`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every injected corruption terminates immediately with `StatusCode::kCorruption` (zero hangs, zero crashes, zero pin leaks).

### Definition of Done
Corruption tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-143: reject corrupt B+ Tree cycles, bad counts, and wrong page types`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-144` (Heavy duplicate-key stress tests).
````

---

## DAY-144 — Run Heavy Duplicate-Key Stress Tests (1,000 Identical Keys Across Splits and Deletes)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-144
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Run Heavy Duplicate-Key Stress Tests (1,000 Identical Keys Across Splits and Deletes)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-143 complete.

### Objective
Extend `tests/unit/index/bplus_tree_duplicate_test.cpp` to insert 1,000+ identical keys (`key = 777` with shuffled `RowID`s), delete arbitrary subsets (leaving 250, deleting remaining 250, mixing duplicate clusters across restarts), verify `GetValue(777)`, `RangeScan`, and `BPlusTreeValidator` at every milestone!

### Concepts & Daily Learning
- **Concept:** Stress-verifying composite `(key, RowID)` splitting AND merging/redistribution under pure-duplicate datasets.
- **Why Database Engines Need It:** Inserting AND deleting hundreds of duplicate keys exercises internal separator rotation when `key` is equal across siblings and ONLY `RowID` breaks ties!
- **How It Works:** Test 1,000 identical keys on `max_size = 8` (and full 4KB capacity!), delete every even `RowID`, verify `500` remain in exact order, delete remaining `500`, verify empty tree & `FreePageCount()`.
- **Design Trade-offs:** Using `order = 8` creates ~125+ leaves filled solely with `key = 777` while running in `< 100ms`.
- **Common Mistakes:** Comparing only `.key` instead of `(key, rid)` anywhere inside `BPlusTreeInternalPage::Lookup` or `BPlusTreeValidator`!
- **Understanding Checkpoint:** If two leaves both start with `key = 777` (differing only in `rid`), how does `BPlusTreeValidator` ensure the separator key between them preserves strict ordering?

### Component Context
- **Why It Exists:** Guarantees duplicate keys behave identically to unique keys under heavy deletion and splits.
- **Module Relationships:** Verifies `BPlusTree` + `BPlusTreeValidator`.

### Files Expected to Change
- `tests/unit/index/bplus_tree_duplicate_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add 1,000-duplicate insert + partial delete + full delete stress test on `order = 8` and 4KB default order.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 1,000 identical keys split, merge, scan, and delete cleanly with `BPlusTreeValidator` passing throughout.

### Definition of Done
Duplicate stress tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-144: add 1000-entry duplicate-key split and deletion stress tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-145` (`BPlusTree` <-> `CatalogManager` integration).
````

---

## DAY-145 — Integrate `BPlusTree` Lifecycle with `CatalogManager` and `TableHeap`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-145
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Integrate `BPlusTree` Lifecycle with `CatalogManager` and `TableHeap`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-144 complete.

### Objective
Create `IndexManager` / catalog integration methods (`CatalogManager::GetOrOpenIndex`, `Destroy` cleanup on `DropIndex` / `DropTable` reclaiming all B+ Tree pages!) so creating, updating (`root_page_id` persisted atomically in `.cat`), reloading across restarts, and dropping B+ Tree indexes integrates seamlessly with `CatalogManager` and `BufferPoolManager`.

### Concepts & Daily Learning
- **Concept:** Coordinating persistent B+ Tree page trees with `CatalogManager` metadata and `DropTable` page reclamation.
- **Why Database Engines Need It:** In Phase 03, `DropTable`/`DropIndex` removed catalog entries; once B+ Trees actually allocate dozens of pages, `DropTable`/`DropIndex` MUST also invoke `BPlusTree::Destroy()` to return all tree pages to the freelist!
- **How It Works:** Implement `BPlusTree::Destroy()` (traversing all tree pages from `root_page_id_` and calling `bpm_.DeletePage(pid)` on every node safely!). Wire `CatalogManager` (holding `BufferPoolManager*` optionally or `IPageProvider`) to instantiate `std::shared_ptr<BPlusTree>` per index with an `on_root_changed` callback invoking `UpdateIndexRootPage`, and call `tree->Destroy()` inside `DropIndex` / `DropTable`!
- **Design Trade-offs:** Keeping `BPlusTree::Destroy()` separate from `~BPlusTree()` ensures closing the database does NOT destroy the index—only explicit `DROP INDEX` / `DROP TABLE` does!
- **Common Mistakes:** Putting `Destroy()` inside `~BPlusTree()` (wiping persisted indexes on shutdown!!); forgetting to reclaim index pages when `DROP TABLE` drops a table that has indexes!
- **Understanding Checkpoint:** Why MUST `BPlusTree::Destroy()` be an explicit DDL action (`DROP INDEX`/`DROP TABLE`) and NEVER called from `~BPlusTree()`?

### Component Context
- **Why It Exists:** Connects Phase 05 `BPlusTree` cleanly to Phase 03 `CatalogManager` for Phase 07 `ExecutionEngine`.
- **Module Relationships:** Bridges `CatalogManager`, `BufferPoolManager`, `TableHeap`, and `BPlusTree`.

### Files Expected to Change
- `include/tinydb/index/bplus_tree.hpp`, `src/index/bplus_tree.cpp` (`Destroy()`)
- `include/tinydb/catalog/catalog_manager.hpp`, `src/catalog/catalog_manager.cpp`
- `tests/integration/phase05_catalog_btree_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BPlusTree::Destroy()` reclaiming every leaf and internal page in post-order/BFS without leaking pins.
2. Wire `CatalogManager` index lifecycle (`CreateIndex` + root persistence + reload + `DropIndex`/`DropTable` reclamation).
3. Test: create table -> insert 200 rows -> create index & backfill -> restart DB -> lookup via index + verify `TableHeap` `RowID`s with `BPlusTreeValidator` -> `DropTable` -> verify `FreePageCount()` reclaimed BOTH heap AND index pages!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Index `root_page_id` persists across root splits/shrinks and DB restarts; `DropTable` reclaims 100% of table and index pages.

### Definition of Done
Catalog + B+ Tree integration tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-145: integrate BPlusTree persistence and Destroy with CatalogManager`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-146` (4KB B+ Tree scaling & I/O sanity benchmark).
````

---

## DAY-146 — Verify Full 4KB Page Fanout Scaling and I/O Reduction vs Sequential Scan

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-146
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Verify Full 4KB Page Fanout Scaling and I/O Reduction vs Sequential Scan
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-145 complete.

### Objective
Test a full production-sized 4096-byte-capacity B+ Tree (`kBTreeLeafMaxSize`, `kBTreeInternalMaxSize`) with 20,000+ keys, measuring actual tree height (`<= 2–3`), `DiskStats::page_reads` per `GetValue` lookup vs `TableHeap` sequential scan, and verifying `BPlusTreeValidator::Validate` on a full 4KB multi-level tree!

### Concepts & Daily Learning
- **Concept:** Measuring real `O(log_M N)` page-read scaling vs `O(N)` heap scans using `DiskStats`.
- **Why Database Engines Need It:** Quantitative proof that full 4KB fanout (`~220–280` entries per node) achieves 20,000 keys in height `2` (1 internal root + ~75 leaves), finding any key in `<= 2` page reads even with a cold cache!
- **How It Works:** Insert 20,000 `(key, rid)` entries into a fresh 4KB-configured `BPlusTree`, flush/reopen cold (or reset `DiskStats`), run `GetValue(12345)` on a fresh cold `BufferPoolManager`, and verify `DiskStats::page_reads == tree_height` (`2` or `3` reads vs `hundreds` of heap pages!).
- **Design Trade-offs:** Using real `DiskStats` counters rather than guessing timings produces exact hardware-independent metrics.
- **Common Mistakes:** Only ever testing small-order (`4`) trees and missing a 4KB serialization boundary bug at entry `#288`!
- **Understanding Checkpoint:** With `kBTreeLeafMaxSize ~ 288` and `kBTreeInternalMaxSize ~ 224`, why does a 20,000-key index fit in height `2` (`1 root + ~70 leaves`) instead of height `15`?

### Component Context
- **Why It Exists:** Validates full-scale 4096-byte production node capacities.
- **Module Relationships:** Benchmarks `BPlusTree` + `BufferPoolManager` + `DiskManager`.

### Files Expected to Change
- `tests/integration/phase05_4kb_scale_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `phase05_4kb_scale_test.cpp` inserting 20,000 keys into a production-capacity 4KB `BPlusTree`, validating structure, verifying height `<= 3`, and asserting cold-cache point lookup reads `<= 3` pages!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 20,000-key 4KB tree passes `BPlusTreeValidator` and cold `GetValue` completes in `<= 3` disk page reads.

### Definition of Done
4KB scale and I/O test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-146: verify full 4KB B+ Tree scale, height, and cold I/O bounds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-147` (10,000-op randomized fuzz test).
````

---

## DAY-147 — Implement 10,000-Op Randomized B+ Tree Fuzz Test (`Insert`, `Remove`, `GetValue`, `RangeScan`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-147
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement 10,000-Op Randomized B+ Tree Fuzz Test (`Insert`, `Remove`, `GetValue`, `RangeScan`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-146 complete.

### Objective
Build a seeded randomized fuzz test (`tests/stress/bplus_tree_fuzz_test.cpp`) running 10,000+ mixed operations (`Insert`, `Remove`, `GetValue`, `RangeScan`, plus periodic restarts) across multiple seeds and small/medium node sizes against an oracle `std::multimap` + periodic `BPlusTreeValidator::Validate`.

### Concepts & Daily Learning
- **Concept:** Long-horizon differential fuzzing for complex balanced tree algorithms.
- **Why Database Engines Need It:** Rare 3-level rebalance sequences (e.g., delete triggers right-borrow at leaf, or internal merge at depth 2 right after split) only reliably appear over thousands of random operations!
- **How It Works:** Maintain `std::set<IndexEntryKey>` oracle. Randomly pick 45% insert (including deliberate duplicate keys from narrow key range `[0..300]`!), 35% delete (existing or random missing key!), 10% point lookup, 10% bounded range scan. Validate structure every 50 mutating ops and at end!
- **Design Trade-offs:** Narrow key domain (`[0..300]`) + `order = 4` / `5` maximizes split/merge/duplicate density per 10,000 ops while completing in ~1–2 seconds!
- **Common Mistakes:** Testing only huge key domains (`[0..10^9]`) where collisions/duplicates and deletes never hit!
- **Understanding Checkpoint:** Why does choosing a narrow random key range (`[0..300]`) stress B+ Tree edge cases harder than `[0..1,000,000,000]`?

### Component Context
- **Why It Exists:** High-confidence stress gate for Phase 05.
- **Module Relationships:** Exercises entire `src/index/` subsystem.

### Files Expected to Change
- `tests/stress/bplus_tree_fuzz_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `bplus_tree_fuzz_test.cpp` (3 deterministic seeds, 10,000+ ops total).
2. Run under Debug + ASan/UBSan and Release.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 10,000+ operations match oracle 100% and pass `BPlusTreeValidator`.

### Definition of Done
Randomized fuzz suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-147: add 10k-op randomized B+ Tree fuzz test against oracle`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-148` (ASan/UBSan hardening).
````

---

## DAY-148 — Run ASan/UBSan Audit and Boundary Hardening Across `src/index/`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-148
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Run ASan/UBSan Audit and Boundary Hardening Across `src/index/`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-147 complete.

### Objective
Audit `include/tinydb/index/*` and `src/index/*` for signed/unsigned conversion warnings, null/empty-iterator edge cases, and extreme key boundaries (`INT64_MIN`, `INT64_MAX`, `0`, `-1`) with explicit regression tests in `tests/unit/index/bplus_tree_boundary_test.cpp`.

### Concepts & Daily Learning
- **Concept:** Integer extreme boundaries (`INT64_MIN`/`INT64_MAX`) in key comparisons and sentinel traps!
- **Why Database Engines Need It:** If any internal sentinel or comparator accidentally subtracted `a.key - b.key` (signed overflow UB on `INT64_MIN - 1`!) or used `INT64_MIN` as a dummy key for `keys[0]` while also colliding with real `INT64_MIN` keys in range checks, extreme values fail!
- **How It Works:** Verify `<=>` / `<` comparisons NEVER subtract `int64_t` keys (`a < b` only!), verify `keys[0]` is NEVER compared during `Lookup` or `Validate`, and add tests inserting/searching/range-scanning/deleting `INT64_MIN`, `INT64_MIN + 1`, `-1`, `0`, `1`, `INT64_MAX - 1`, `INT64_MAX`.
- **Design Trade-offs:** Strict `<` / `==` relational comparisons are completely immune to signed overflow UB.
- **Common Mistakes:** Writing `int64_t cmp = a.key - b.key;` in a comparator (classic signed overflow UB!).
- **Understanding Checkpoint:** Why is `return a.key < b.key ? -1 : (a.key > b.key ? 1 : 0)` safe for `INT64_MIN` and `INT64_MAX` whereas `return a.key - b.key` invokes Undefined Behavior?

### Component Context
- **Why It Exists:** Eliminates signed-extreme UB in key indexing.
- **Module Relationships:** Audits all `src/index/` comparators and boundary paths.

### Files Expected to Change
- `include/tinydb/index/*.hpp`, `src/index/*.cpp` (audit fixes if any)
- `tests/unit/index/bplus_tree_boundary_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit all comparators, casts, and iterator boundaries in `src/index/`.
2. Add `bplus_tree_boundary_test.cpp` exercising `INT64_MIN`, `INT64_MAX`, `max_size = 4` vs 4KB boundary, and empty-tree operations.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan (`-Werror`).

### Acceptance Criteria
- Zero UBSan signed-overflow or narrowing warnings; `INT64_MIN`/`INT64_MAX` work identically to `0`.

### Definition of Done
Boundary audit complete and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-148: harden B+ Tree INT64_MIN/MAX boundaries under ASan and UBSan`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-149` (Code cleanup & helper extraction).
````

---

## DAY-149 — Refactor and Clean Up `src/index/` Helper Methods and Docstrings

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-149
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Refactor and Clean Up `src/index/` Helper Methods and Docstrings
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-148 complete.

### Objective
Review `src/index/bplus_tree.cpp`, `bplus_tree_leaf_page.cpp`, `bplus_tree_internal_page.cpp`, and `bplus_tree_validator.cpp` for function length, shallow nesting, duplicated rebalance logic, clear docstrings, and strict `clang-format` compliance without altering public behavior.

### Concepts & Daily Learning
- **Concept:** Disciplined refactoring under full automated test coverage (improving readability without breaking invariants).
- **Why Database Engines Need It:** `BPlusTree` is one of the longest subsystems in TinyDB; extracting clean helpers (`RebalanceLeaf`, `RebalanceInternal`, `UpdateChildParent`) makes Phase 08 WAL/rollback changes and technical interview explanations crystal clear!
- **How It Works:** Split oversized functions (`> 60` lines) into single-purpose private helpers with early returns, add concise docstrings explaining *why* on subtle steps (`AdjustRoot`, `Coalesce` normalization, composite separator bounds), and verify formatting + full CTest suite.
- **Design Trade-offs:** Pure readability/modularity refactoring guarded by the 10k-op fuzz suite ensures zero regressions.
- **Common Mistakes:** Mixing behavioral feature changes with cleanup refactoring in the same commit.
- **Understanding Checkpoint:** Walk through how `CoalesceOrRedistribute` splits cleanly into `TryRedistribute` + `MergeSiblings` + `AdjustRoot`.

### Component Context
- **Why It Exists:** Keeps the most complex algorithm in TinyDB maintainable and interview-readable.
- **Module Relationships:** Refactors `src/index/`.

### Files Expected to Change
- `include/tinydb/index/*.hpp`
- `src/index/*.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Extract cohesive private helpers, add concise docstrings/why-comments, and run `clang-format` across `src/index/` and `include/tinydb/index/`.
2. Re-run entire test suite (Debug + ASan/UBSan + Release).

### Testing Instructions
- Build and run `ctest --output-on-failure` across Debug, ASan+UBSan, and Release + `clang-format --dry-run --Werror`.

### Acceptance Criteria
- All Phase 00–05 tests pass 100% after refactoring with cleaner, shorter functions.

### Definition of Done
Code refactored, formatted, and fully re-tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-149: refactor B+ Tree helpers and polish docstrings`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-150` (`docs/btree-design.md` documentation).
````

---

## DAY-150 — Finalize `docs/btree-design.md` Architecture and On-Disk Format Documentation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-150
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Finalize `docs/btree-design.md` Architecture and On-Disk Format Documentation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-149 complete.

### Objective
Complete `docs/btree-design.md` with exact byte offsets, leaf/internal layouts, composite `(key, RowID)` diagram, copy-up vs push-up split diagrams, borrow/merge/adjust-root state transitions, `BufferPoolManager` pin-budget guarantees, and validator invariants matching the implemented code 100%.

### Concepts & Daily Learning
- **Concept:** Accurate systems documentation and Mermaid diagramming of B+ Tree mechanics.
- **Why Database Engines Need It:** Clear diagrams of internal vs leaf pages and copy-up vs push-up splits make TinyDB's B+ Tree effortless to defend in technical interviews.
- **How It Works:** Cross-check every byte offset and constant in `bplus_tree_types.hpp`, `bplus_tree_page.hpp`, `bplus_tree_leaf_page.hpp`, and `bplus_tree_internal_page.hpp` against `docs/btree-design.md`, and update `docs/storage-format.md` cross-references if needed.
- **Design Trade-offs:** Documenting exact trade-offs (`(key, RowID)` vs overflow pages, single-thread/v1 latching) shows architectural maturity.
- **Common Mistakes:** Writing idealized diagrams that contradict the actual offsets (`32B` common header + `16B` B+ Tree header!).
- **Understanding Checkpoint:** Verify from `docs/btree-design.md` alone the exact byte offset of entry `0` in a leaf page vs an internal page.

### Component Context
- **Why It Exists:** Authoritative technical reference for Phase 05.
- **Module Relationships:** Documents `src/index/`.

### Files Expected to Change
- `docs/btree-design.md`
- `docs/storage-format.md` (verify B+ Tree cross-references)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Finalize `docs/btree-design.md` with exact tables and Mermaid diagrams verified against the header constants.
2. Verify docs and run `ctest --output-on-failure` cleanly.

### Testing Instructions
- Verify build and tests remain green (`ctest --test-dir build --output-on-failure`).

### Acceptance Criteria
- `docs/btree-design.md` accurately describes 100% of implemented constants, layouts, and algorithms.

### Definition of Done
Documentation complete, build/tests clean, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-150: finalize B+ Tree technical design documentation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-151` (End-to-end B+ Tree + TableHeap backfill & sync test).
````

---

## DAY-151 — Implement End-to-End Table + B+ Tree Backfill and Sync Integration Test

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-151
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Implement End-to-End Table + B+ Tree Backfill and Sync Integration Test
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-150 complete.

### Objective
Write `tests/integration/phase05_table_index_sync_test.cpp` simulating realistic table + secondary index operations (backfilling existing `TableHeap` rows into a new `BPlusTree`, inserting new rows, cross-page `TableHeap::UpdateTuple` migrating `old_rid -> new_rid` in the index, deleting rows from both heap & index, cold restart, and `BPlusTreeValidator::Validate(&tree, &table_heap, indexed_col)`!).

### Concepts & Daily Learning
- **Concept:** Dual-structure synchronization (`TableHeap` + secondary `BPlusTree`s) and `BPlusTreeValidator` table cross-validation.
- **Why Database Engines Need It:** Before Phase 07's `InsertExecutor`/`UpdateExecutor`/`DeleteExecutor` automate index maintenance, testing manual heap+index synchronization (including cross-page `RowID` migration!) ensures Phase 05 + Phase 02 + Phase 03 + Phase 04 compose seamlessly!
- **How It Works:** Populate 300 rows (`INT` id, `BIGINT` score, `VARCHAR(200)` bio), create 2 B+ Trees (one on `id`, one on `score` with duplicates!), backfill via `TableHeap::ScanIterator`, update `bio` to trigger cross-page `RowID` migration while updating both indexes, delete rows, restart DB cold, and run `BPlusTreeValidator::Validate(tree, &table, col)` confirming EVERY indexed `RowID` fetches the matching live tuple!
- **Design Trade-offs:** Validating both `INT` (`int32_t` widened to `int64_t`) and `BIGINT` indexes verifies both integer column types supported in v1.
- **Common Mistakes:** Updating the B+ Tree when a `TableHeap::UpdateTuple` relocates a row (`new_rid != old_rid`) WITHOUT removing `(key, old_rid)` first!
- **Understanding Checkpoint:** Trace the exact sequence of index updates required when `UPDATE users SET score = 50, bio = '<long>'` moves a row from `old_rid` to `new_rid`.

### Component Context
- **Why It Exists:** Verifies full storage + buffer + catalog + B+ Tree synergy prior to SQL frontend.
- **Module Relationships:** Previews Phase 07 index-maintained DML workflows.

### Files Expected to Change
- `tests/integration/phase05_table_index_sync_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `phase05_table_index_sync_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both `INT` and `BIGINT` secondary indexes match `TableHeap` 100% across cross-page updates, deletes, and cold restarts via `BPlusTreeValidator`.

### Definition of Done
Integration test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-151: add TableHeap and BPlusTree backfill/update/delete sync integration test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-152` (Phase 05 sign-off).
````

---

## DAY-152 — Complete Phase 05 Acceptance Verification and Sign Off

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-152
- **Phase:** Phase 05 — B+ Tree Indexing
- **Session Title:** Complete Phase 05 Acceptance Verification and Sign Off
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-112 through DAY-151 complete.

### Objective
Execute the full Phase 00–05 verification suite across Debug (`-Werror`), ASan+UBSan (`-Werror`), and Release (`-Werror`), verify `clang-format`, review `PROGRESS.md` metrics (`118.0 / 220.0` hours completed — over halfway through the entire engine!), and sign off Phase 05.

### Concepts & Daily Learning
- **Concept:** Storage-engine completion gate prior to building the SQL frontend & query processor.
- **Why Database Engines Need It:** With Phase 05 complete, TinyDB now possesses a complete persistent relational storage & indexing engine (`DiskManager`, `TableHeap`, `CatalogManager`, `BufferPoolManager`, `BPlusTree`); verifying zero leaks/warnings across all builds ensures the SQL layer builds on solid bedrock!
- **How It Works:** Run all unit, integration, and stress tests under Debug, ASan+UBSan, and Release, audit `git status` for zero scratch files, update `PROGRESS.md` and `NEXT_SESSION.md` (`DAY-153`).
- **Design Trade-offs:** Clean sign-off prevents storage bugs from masquerading as SQL bugs in Phases 06–07.
- **Common Mistakes:** Skipping Release build check (where `-O3` and `-DNDEBUG` can expose uninitialized variables or `assert()` side-effect bugs!).
- **Understanding Checkpoint:** Why must Release (`-O3 -DNDEBUG`) AND Debug+ASan+UBSan both be verified at every phase gate?

### Component Context
- **Why It Exists:** Completes Phase 05 (`32.0` hours).
- **Module Relationships:** Hands off the complete storage + index engine to Phase 06 (`SQL Lexer & Parser`).

### Files Expected to Change
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Run Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format --dry-run --Werror`.
2. Update `PROGRESS.md` (`118.0 / 220.0` hours, Phase 05 complete, Phase 06 active) and `NEXT_SESSION.md` -> `DAY-153`.

### Testing Instructions
- Full Debug, ASan+UBSan, Release `ctest --output-on-failure` + formatting check.

### Acceptance Criteria
- 100% of Phase 00–05 tests pass across all 3 build configurations with zero warnings or sanitizer errors.

### Definition of Done
Phase 05 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-152: verify full build matrix and sign off Phase 05 B+ Tree`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 06 (`DAY-153` — SQL formal grammar & source locations).
````
