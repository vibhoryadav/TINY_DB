# Phase 04 — Buffer Pool (`DAY-086` – `DAY-111`)

**Phase Budget:** 20.0 hours (24 × 45 min + 2 × 60 min = 26 sessions)
**Goal:** Implement TinyDB's in-memory `BufferPoolManager` with `PageFrame`s, O(1) `LRUReplacer`, pin-count protection, sticky dirty tracking, RAII `PageGuard`s, shutdown flushing, and seamless `TableHeap` integration.

---

## DAY-086 — Design `BufferPoolManager` Architecture and `PageFrame` Structure

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-086
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Design `BufferPoolManager` Architecture and `PageFrame` Structure
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 03 complete (`DAY-085`).

### Objective
Define `tinydb::PageFrame` (`include/tinydb/buffer/page_frame.hpp`) holding a 4096-byte `Page`, `page_id`, `pin_count`, `is_dirty`, and `no_steal_pinned` (or Phase 08 reservation flag), plus `Reset()` invariants, and refine `docs/buffer-pool.md` with exact frame state transitions.

### Concepts & Daily Learning
- **Concept:** Buffer pool frames vs disk pages (`FrameId` indexes the fixed-size RAM array `[0..pool_size-1]`, `PageId` indexes the disk file `[1..page_count-1]`).
- **Why Database Engines Need It:** Keeping a fixed array of `PageFrame`s in RAM avoids constant 4KB `malloc`/`free` churn, lets hot index roots stay resident in memory, and isolates disk I/O from query execution.
- **How It Works:** `PageFrame` wraps `Page page_`, `PageId page_id_ = kInvalidPageId`, `std::uint32_t pin_count_ = 0`, `bool is_dirty_ = false`. `Reset()` zeroes metadata and resets `page_` cleanly when reassigned to a new `PageId`.
- **Design Trade-offs:** Pre-allocating `pool_size` frames at startup gives predictable RAM usage (`pool_size * ~4.1 KiB`), avoiding OOM surprises mid-query.
- **Common Mistakes:** Confusing `FrameId` (`0..pool_size-1`) with `PageId` (`1..N`)! Storing `PageFrame` copies when frames are moved (must keep stable frame addresses or index by `FrameId`).
- **Understanding Checkpoint:** What is the difference between `FrameId` and `PageId`, and why must `PageFrame::Reset()` never run while `pin_count_ > 0`?

### Component Context
- **Why It Exists:** Memory slot container managed by `BufferPoolManager`.
- **Module Relationships:** Owned by `BufferPoolManager`; wrapped by `PageGuard`s (`DAY-100`).

### Files Expected to Change
- `include/tinydb/buffer/page_frame.hpp` (new)
- `src/buffer/page_frame.cpp` (new)
- `docs/buffer-pool.md` (add state-transition table)
- `tests/unit/buffer/page_frame_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `PageFrame` class with accessors, `Reset()`, `IncrementPin()`, `DecrementPin()` (returning error/bool on underflow!), `MarkDirty()`, `ClearDirty()`.
2. Write unit tests verifying initial state, pin increment/decrement underflow rejection, and `Reset()`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Decrementing `pin_count` at `0` fails cleanly without unsigned wraparound (`UINT32_MAX`!).

### Definition of Done
`PageFrame` implemented and tested, docs updated, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-086: design BufferPool PageFrame structure and pin invariants`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-087` (`Replacer` interface & `LRUReplacer` design).
````

---

## DAY-087 — Define `Replacer` Interface and `LRUReplacer` Data Structure Design

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-087
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Define `Replacer` Interface and `LRUReplacer` Data Structure Design
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-086 complete.

### Objective
Define the abstract `Replacer` policy interface (`include/tinydb/buffer/replacer.hpp`: `Victim`, `Pin`, `Unpin`, `Size`) and `LRUReplacer` class layout (`include/tinydb/buffer/lru_replacer.hpp`) tracking **only unpinned evictable frames** in O(1) time.

### Concepts & Daily Learning
- **Concept:** Eviction policy separation (Strategy pattern) and LRU tracking solely over *unpinned* frames (`pin_count == 0`).
- **Why Database Engines Need It:** Pinned frames are actively referenced by running operators/B+ Tree traversals and MUST NEVER be considered by `Victim()`. Separating `Replacer` keeps eviction policy clean and modular (allowing Clock or LRU-K later!).
- **How It Works:** `Unpin(frame_id)` inserts an unpinned frame as most-recently-used (if not already present); `Pin(frame_id)` removes an actively-pinned frame from evictable candidates; `Victim(&frame_id)` pops the least-recently-used candidate.
- **Design Trade-offs:** Pure LRU is simple and O(1) with `std::list` + `std::unordered_map`, though vulnerable to sequential scan pollution (mitigated/upgradable to Clock/LRU-K in future versions via the `Replacer` interface!).
- **Common Mistakes:** Updating LRU position on `Pin()` by moving to front instead of removing it—allowing `Victim()` to evict a pinned frame!
- **Understanding Checkpoint:** Why does `LRUReplacer::Pin(frame_id)` *remove* the frame from the LRU list rather than keeping it inside the list?

### Component Context
- **Why It Exists:** Decides which unpinned frame `BufferPoolManager` evicts when all frames are occupied.
- **Module Relationships:** Owned by `BufferPoolManager`.

### Files Expected to Change
- `include/tinydb/buffer/replacer.hpp` (new)
- `include/tinydb/buffer/lru_replacer.hpp` (new)
- `docs/buffer-pool.md` (document `Replacer` Pin/Unpin/Victim semantics)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define virtual `Replacer` base class (`virtual ~Replacer() = default`, `Victim(FrameId*)`, `Pin(FrameId)`, `Unpin(FrameId)`, `Size() const`).
2. Declare `LRUReplacer` with `capacity_`, `std::list<FrameId> lru_list_`, `std::unordered_map<FrameId, std::list<FrameId>::iterator> lru_map_`, and `std::mutex latch_`.

### Testing Instructions
- Verify clean compilation (`cmake --build build`).

### Acceptance Criteria
- Interface headers compile cleanly with `-Werror`.

### Definition of Done
Interfaces designed and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-087: define Replacer interface and LRUReplacer structure`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-088` (`LRUReplacer` implementation).
````

---

## DAY-088 — Implement `LRUReplacer` in O(1) Time (`std::list` + `std::unordered_map`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-088
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `LRUReplacer` in O(1) Time (`std::list` + `std::unordered_map`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-087 complete.

### Objective
Implement `LRUReplacer` (`src/buffer/lru_replacer.cpp`) with strict O(1) `Victim`, `Pin`, `Unpin`, and `Size`, enforcing `frame_id < capacity_` bounds, capacity enforcement, and idempotency when calling `Pin` on an un-tracked frame or `Unpin` twice.

### Concepts & Daily Learning
- **Concept:** O(1) doubly-linked list + hash table LRU implementation, iterator invalidation safety, and idempotent Pin/Unpin handling.
- **Why Database Engines Need It:** Every `FetchPage` and `UnpinPage` touches the replacer; `O(N)` scanning of frames would degrade buffer-pool throughput on large pools.
- **How It Works:**
  - `Unpin(fid)`: if `fid >= capacity_` ignore/return safely; if `lru_map_.contains(fid)` already, do nothing (or document: already evictable!); else `push_front(fid)` and record `lru_list_.begin()` in `lru_map_`.
  - `Pin(fid)`: if `lru_map_.find(fid)` exists, erase iterator from `lru_list_` and erase from `lru_map_`.
  - `Victim(&fid)`: if `lru_list_.empty()` return `false`; pop `lru_list_.back()`, erase from `lru_map_`, return `true`.
- **Design Trade-offs:** Using `push_front` / `pop_back` keeps LRU eviction deterministic and O(1).
- **Common Mistakes:** Calling `lru_list_.erase(it)` *after* erasing `it` from `lru_map_` (use-after-free on `it` reference if taken by ref!); exceeding `capacity_` when invalid `FrameId`s are passed.
- **Understanding Checkpoint:** What happens if `Unpin(fid)` is called twice consecutively without an intervening `Pin(fid)`, and why must `LRUReplacer` guard against duplicate insertions?

### Component Context
- **Why It Exists:** Concrete LRU eviction engine for `BufferPoolManager`.
- **Module Relationships:** Owned directly by `BufferPoolManager`.

### Files Expected to Change
- `src/buffer/lru_replacer.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all `LRUReplacer` methods thread-safely under `latch_`.
2. Ensure zero invalid `FrameId` entries (`fid >= capacity_`) or duplicate entries can ever enter `lru_list_`.

### Testing Instructions
- Build cleanly with `-Werror` and ASan/UBSan.

### Acceptance Criteria
- Compiles cleanly with zero warnings; logic matches O(1) specification.

### Definition of Done
`LRUReplacer` implemented, build verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-088: implement O(1) LRUReplacer with capacity and idempotency guards`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-089` (`LRUReplacer` unit tests).
````

---

## DAY-089 — Write Exhaustive `LRUReplacer` Unit Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-089
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Write Exhaustive `LRUReplacer` Unit Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-088 complete.

### Objective
Write comprehensive `LRUReplacer` unit tests (`tests/unit/buffer/lru_replacer_test.cpp`) verifying strict LRU victim ordering, `Pin` removing candidates from middle/head/tail, `Unpin` re-inserting as MRU, empty pool handling, duplicate `Unpin`, out-of-range `FrameId`s, and zero-capacity behavior.

### Concepts & Daily Learning
- **Concept:** Verifying data structure state transitions across edge cases before integrating into a larger manager.
- **Why Database Engines Need It:** An off-by-one or duplicate-insert bug in `LRUReplacer` manifests later as mysterious corrupted pages or phantom deadlocks inside `BufferPoolManager`.
- **How It Works:** Exercise canonical sequences (Unpin 1..6 -> Victim 1 -> Pin 3, 4 -> Unpin 4 -> Victim 2, 5, 6, 4) and invalid inputs (`Pin(999)`, `Victim(nullptr)`, `Unpin(1)` twice).
- **Design Trade-offs:** Thorough unit testing takes 45 minutes now and saves hours of debugging later in Phase 05 B+ Tree tests.
- **Common Mistakes:** Only testing sequential `Unpin` -> `Victim` without interleaved `Pin` + re-`Unpin` reordering.
- **Understanding Checkpoint:** Trace `Unpin(1), Unpin(2), Unpin(3), Pin(2), Unpin(2)`—which frame must `Victim()` return first and why?

### Component Context
- **Why It Exists:** Proves `LRUReplacer` obeys its contract 100%.
- **Module Relationships:** Verifies `LRUReplacer` before building `BufferPoolManager`.

### Files Expected to Change
- `tests/unit/buffer/lru_replacer_test.cpp` (new)
- `src/buffer/lru_replacer.cpp` (fix any edge cases found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all `LRUReplacer` unit tests in `lru_replacer_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every edge case (`nullptr`, out-of-range `FrameId`, duplicate `Unpin`, empty `Victim`, interleaved `Pin`/`Unpin`) passes cleanly.

### Definition of Done
Tests passing under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-089: add comprehensive LRUReplacer unit tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-090` (`BufferPoolManager` skeleton).
````

---

## DAY-090 — Implement `BufferPoolManager` Skeleton (Frames, `page_table_`, `free_list_`, Metrics)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-090
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager` Skeleton (Frames, `page_table_`, `free_list_`, Metrics)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-089 complete, Phase 01 `DiskManager`.

### Objective
Create `BufferPoolManager` (`include/tinydb/buffer/buffer_pool_manager.hpp`, `src/buffer/buffer_pool_manager.cpp`) initializing `pool_size` `PageFrame`s, `free_list_` containing `[0..pool_size-1]`, `page_table_` (`PageId -> FrameId`), `LRUReplacer`, `DiskManager&`, and frame-acquisition helper `FindReplacementFrameLocked(FrameId* out_frame)`.

### Concepts & Daily Learning
- **Concept:** Free-frame allocation vs victim eviction, `page_table_` mapping invariants, and pool metrics (`hits`, `misses`, `evictions`).
- **Why Database Engines Need It:** `free_list_` gives `O(1)` instant frames on startup without consulting `LRUReplacer`; once `free_list_` empties, `LRUReplacer::Victim()` finds the oldest unpinned frame.
- **How It Works:** Constructor validates `pool_size >= 1` (using `std::vector<PageFrame>` or `std::unique_ptr<PageFrame[]>`), populates `free_list_` with `0..pool_size-1`, creates `LRUReplacer(pool_size)`. `FindReplacementFrameLocked` checks `!free_list_.empty()` first, otherwise calls `replacer_->Victim(&fid)` (returning `kResourceExhausted` if all frames are pinned!).
- **Design Trade-offs:** Storing `PageFrame` inside a fixed-size `std::vector<PageFrame>(pool_size)` (never resized after construction!) guarantees stable `&frames_[fid]` pointers across the pool's entire lifetime!
- **Common Mistakes:** Resizing `frames_` after construction (invalidating pointers handed out by `FetchPage`!); returning a victim frame WITHOUT checking/flushing if it was dirty and removing its old `page_id` from `page_table_`!
- **Understanding Checkpoint:** Why must `BufferPoolManager::frames_` never reallocate after construction, and why are all frames initially in `free_list_` rather than `replacer_`?

### Component Context
- **Why It Exists:** Central memory-management hub between `TableHeap`/`BPlusTree` and `DiskManager`.
- **Module Relationships:** Owns `PageFrame[]` and `LRUReplacer`; talks to `DiskManager`.

### Files Expected to Change
- `include/tinydb/buffer/buffer_pool_manager.hpp` (new)
- `src/buffer/buffer_pool_manager.cpp` (new)
- `tests/unit/buffer/buffer_pool_init_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Declare the complete `BufferPoolManager` API (`FetchPage`, `UnpinPage`, `NewPage`, `DeletePage`, `FlushPage`, `FlushAllPages`, `FetchPageGuard`, `NewPageGuard`, `PoolSize`, `GetStats`, `Shutdown`).
2. Implement constructor, `FindReplacementFrameLocked` (skeleton), and initial pool state tests (`PoolSize()`, `FreeFrameCount() == pool_size`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Compiles cleanly with `-Werror`; pool initialization invariant tests pass.

### Definition of Done
Skeleton implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-090: create BufferPoolManager skeleton, page table, and free list`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-091` (`FetchPage` cache-hit path).
````

---

## DAY-091 — Implement `BufferPoolManager::FetchPage` Cache-Hit Path

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-091
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::FetchPage` Cache-Hit Path
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-090 complete.

### Objective
Implement the `page_table_.contains(page_id)` cache-hit branch of `BufferPoolManager::FetchPage(PageId)` (incrementing `pin_count`, calling `replacer_->Pin(frame_id)`, incrementing `hits`, and rejecting invalid `page_id`s like `0` or `kInvalidPageId` up front).

### Concepts & Daily Learning
- **Concept:** Buffer pool cache hits, pin-count transitions (`0 -> 1` and `1 -> 2`), and keeping disk I/O counters at `0` on hits.
- **Why Database Engines Need It:** Hot pages like B+ Tree roots or active table tail pages are fetched thousands of times; serving hits in RAM without calling `DiskManager::ReadPage` is the primary speedup of a buffer pool!
- **How It Works:** Under `latch_`, reject `!IsDataPageId(page_id)`. If `page_table_.find(page_id)` hits: get `fid`, increment `frames_[fid].pin_count_`, call `replacer_->Pin(fid)` (ensuring even a previously unpinned cached frame is immediately protected from eviction!), increment `stats_.hits`, return `&frames_[fid].page_`.
- **Design Trade-offs:** Returning `Result<Page*>` (with clear `Status` on miss/corruption/exhaustion) instead of silent `nullptr` lets callers distinguish "all frames pinned" from "page corrupted on disk"!
- **Common Mistakes:** Forgetting to call `replacer_->Pin(fid)` on a cache hit when `pin_count` transitions `0 -> 1` (leaving a pinned frame inside `LRUReplacer`!).
- **Understanding Checkpoint:** Suppose Page 5 is in RAM with `pin_count == 0` (sitting in `LRUReplacer`). What catastrophe happens if `FetchPage(5)` increments `pin_count` to `1` without calling `replacer_->Pin(fid)`?

### Component Context
- **Why It Exists:** Fast RAM read path for resident pages.
- **Module Relationships:** Paired with cache-miss load (`DAY-092`) and `UnpinPage` (`DAY-094`).

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_fetch_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FetchPage` validation + cache-hit path (leaving cache-miss delegating cleanly to `DAY-092`'s load helper).
2. Test invalid `PageId` rejections (`0`, `kInvalidPageId`) cleanly now.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Invalid `PageId`s rejected before touching `page_table_` or `DiskManager`.

### Definition of Done
Hit path and validation tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-091: implement BufferPoolManager::FetchPage validation and cache-hit path`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-092` (`FetchPage` cache-miss path).
````

---

## DAY-092 — Implement `BufferPoolManager::FetchPage` Cache-Miss Path

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-092
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::FetchPage` Cache-Miss Path
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-091 complete.

### Objective
Implement the cache-miss path of `BufferPoolManager::FetchPage(PageId)`: acquiring a free frame (or clean victim frame), loading from `DiskManager::ReadPage(page_id, frame.page_)`, rolling back the frame safely to `free_list_` if `DiskManager::ReadPage` fails (e.g. out-of-bounds or corruption!), installing in `page_table_`, and pinning (`pin_count = 1`).

### Concepts & Daily Learning
- **Concept:** Atomic frame installation on cache miss and error rollback when `DiskManager::ReadPage` fails.
- **Why Database Engines Need It:** If `FetchPage(999)` grabs a free frame, fails `ReadPage(999)`, and forgets to put that frame back onto `free_list_`, every failed `FetchPage` permanently leaks 1 buffer frame until the pool runs out of RAM!
- **How It Works:** Acquire `fid` from `free_list_` (or evict clean victim via `DAY-093`), call `disk_manager_.ReadPage(page_id, temp_or_frame)`. If `!read_status.ok()`, reset frame and return `fid` to `free_list_`! Only after `ReadPage` succeeds: set `frame.page_id_ = page_id`, `pin_count = 1`, `is_dirty = false`, `page_table_[page_id] = fid`, `replacer_->Pin(fid)`.
- **Design Trade-offs:** Restoring `fid` to `free_list_` on `ReadPage` failure keeps pool capacity lossless across invalid/corrupt page accesses!
- **Common Mistakes:** Inserting `page_table_[page_id] = fid` *before* `ReadPage` succeeds (leaving garbage mapped in `page_table_` if disk read fails!); forgetting to verify zero disk reads on subsequent `FetchPage(same_page_id)` hits!
- **Understanding Checkpoint:** Trace what happens to `free_list_` and `page_table_` if `FetchPage(nonexistent_id)` fails inside `DiskManager::ReadPage`.

### Component Context
- **Why It Exists:** Loads disk pages into memory frames on demand.
- **Module Relationships:** Calls `DiskManager::ReadPage`; pairs with dirty eviction (`DAY-093`).

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_fetch_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement cache-miss loading and failure rollback in `BufferPoolManager::FetchPage`.
2. Add tests using `DiskManager` + `DiskStats`: first `FetchPage(1)` increments `page_reads` by 1 (`misses == 1`), second `FetchPage(1)` leaves `page_reads` unchanged (`hits == 1`, `pin_count == 2`), and `FetchPage(999)` returns error without leaking a free frame!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Cache hit performs 0 disk reads; failed `FetchPage(999)` leaks 0 frames; `pin_count` tracks multi-fetch accurately.

### Definition of Done
Cache-miss path and hit/miss `DiskStats` tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-092: implement BufferPoolManager::FetchPage miss load and failure rollback`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-093` (Dirty-page writeback during victim eviction).
````

---

## DAY-093 — Implement Dirty-Page Writeback During Victim Eviction

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-093
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement Dirty-Page Writeback During Victim Eviction
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-092 complete.

### Objective
Implement victim eviction inside `AcquireFrameLocked(FrameId* out_frame)`: popping LRU victim from `replacer_`, writing back dirty victim via `DiskManager::WritePage` BEFORE erasing its `page_table_` mapping or overwriting the frame, and aborting eviction cleanly if `WritePage` fails!

### Concepts & Daily Learning
- **Concept:** Write-back caching (dirty eviction), preserving old frame integrity if flush fails, and `page_table_` removal ordering.
- **Why Database Engines Need It:** If a modified dirty page is evicted without writing it back to disk, committed/saved table updates vanish silently! Worse, if `WritePage` fails (disk error) AFTER erasing `page_table_[old_id]`, the dirty page is lost in limbo!
- **How It Works:** When `free_list_` is empty, call `replacer_->Victim(&fid)`. If `!Victim`, return `StatusCode::kResourceExhausted`. Inspect `frames_[fid]`: if `is_dirty_`, call `disk_manager_.WritePage(old_page_id, frames_[fid].page_)`. If write fails, re-`Unpin` `fid` back into `replacer_` and return error! On success, erase `old_page_id` from `page_table_`, call `frames_[fid].Reset()`, and return `fid`.
- **Design Trade-offs:** Rolling back failed victim writeback back into `replacer_` preserves pool consistency on I/O errors.
- **Common Mistakes:** Resetting `frames_[fid]` or erasing `page_table_[old_page_id]` *before* checking `is_dirty_`!
- **Understanding Checkpoint:** Why MUST `WritePage(old_page_id, frame.page_)` run *before* `frame.Reset()` and `page_table_.erase(old_page_id)` during victim eviction?

### Component Context
- **Why It Exists:** Enables a bounded buffer pool (`pool_size << database_pages`) to manage arbitrarily large databases without losing dirty changes.
- **Module Relationships:** Completes full replacement inside `FetchPage` (`DAY-092`) and `NewPage` (`DAY-095`).

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_eviction_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Complete `AcquireFrameLocked` with dirty writeback and error rollback.
2. Add eviction tests using a tiny pool (`pool_size = 2`) accessing 4 pages (tested cleanly once `UnpinPage` or minimal helper sets `Unpin` on `DAY-094`—actually let's check: `DAY-094` is `UnpinPage`, so either implement basic `UnpinPage` test hook or verify frame limit exhaustion now and dirty-eviction test on `DAY-094` / implement clean test!). Let's verify all-pinned exhaustion (`pool_size = 2`, fetch 2, fetch 3rd fails `kResourceExhausted`) on `DAY-093` and dirty writeback test on `DAY-094` once `UnpinPage` lands!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Fetching a 3rd page when 2/2 frames are pinned returns `kResourceExhausted` without evicting pinned frames!

### Definition of Done
Victim writeback logic implemented, pinned-exhaustion tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-093: implement dirty-page writeback on victim eviction`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-094` (`UnpinPage` & sticky dirty bit).
````

---

## DAY-094 — Implement `BufferPoolManager::UnpinPage` with Double-Unpin Guard and Sticky Dirty Tracking

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-094
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::UnpinPage` with Double-Unpin Guard and Sticky Dirty Tracking
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-093 complete.

### Objective
Implement `Status BufferPoolManager::UnpinPage(PageId page_id, bool is_dirty)` enforcing: `page_id` present in `page_table_`, `pin_count > 0` (rejecting double-unpins!), sticky dirty invariant (`frame.is_dirty_ |= is_dirty`), and calling `replacer_->Unpin(fid)` ONLY when `pin_count` reaches `0`!

### Concepts & Daily Learning
- **Concept:** Reference-counted page pinning, sticky dirty semantics, and double-unpin underflow prevention.
- **Why Database Engines Need It:** If Thread/Operator A modifies a page (`Unpin(p, true)`) and Operator B only read it (`Unpin(p, false)`), setting `frame.is_dirty_ = is_dirty` instead of `|=` would WIPE Operator A's dirty mark and lose data on eviction! And double-unpinning while `pin_count == 0` breaks `pin_count` invariants!
- **How It Works:** Under `latch_`, find `page_id` in `page_table_` (`kNotFound` if absent). If `frame.pin_count_ == 0`, return `Status::InvalidArgument("Double unpin on page")`. Decrement `pin_count_`, apply `frame.is_dirty_ = frame.is_dirty_ || is_dirty`, and if `pin_count_ == 0`, call `replacer_->Unpin(fid)`.
- **Design Trade-offs:** Returning explicit `Status` from `UnpinPage` surfaces double-unpin bugs immediately in unit tests instead of silently ignoring them!
- **Common Mistakes:** Writing `frame.is_dirty_ = is_dirty;` (sticky-dirty bug!); calling `replacer_->Unpin(fid)` while `pin_count_ > 0`!
- **Understanding Checkpoint:** Trace two concurrent readers/writers: Reader 1 fetches Page 3, Writer 2 fetches Page 3, Writer 2 modifies & calls `UnpinPage(3, true)`, Reader 1 calls `UnpinPage(3, false)`. What MUST `frame.is_dirty_` and `pin_count_` equal after each step?

### Component Context
- **Why It Exists:** Completes the core `FetchPage` <-> `UnpinPage` lifecycle and unlocks full dirty-eviction testing!
- **Module Relationships:** Called by `PageGuard` (`DAY-100`), `TableHeap`, and `BPlusTree`.

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_eviction_test.cpp` (full LRU + sticky-dirty eviction tests!)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BufferPoolManager::UnpinPage`.
2. Add comprehensive tests: double-unpin rejected, non-resident page rejected, sticky dirty (`Unpin(true)` then `Unpin(false)` still writes back on eviction!), and LRU eviction verifying dirty page changes persist to disk when evicted from `pool_size = 2`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Sticky dirty eviction writes modified bytes to disk (`DiskStats::page_writes` increments on eviction); double-unpin fails cleanly.

### Definition of Done
`UnpinPage` and full eviction tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-094: implement UnpinPage with double-unpin guard and sticky dirty tracking`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-095` (`NewPage`).
````

---

## DAY-095 — Implement `BufferPoolManager::NewPage` (Allocating and Pinning Fresh Disk Pages)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-095
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::NewPage` (Allocating and Pinning Fresh Disk Pages)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-094 complete.

### Objective
Implement `Result<std::pair<PageId, Page*>> BufferPoolManager::NewPage(PageType initial_type)` acquiring a buffer frame FIRST (so we never allocate an un-bufferable orphan disk page when all frames are pinned!), allocating a page via `DiskManager::AllocatePage(initial_type)`, initializing the frame (`pin_count = 1`, `is_dirty = false`), and installing in `page_table_`.

### Concepts & Daily Learning
- **Concept:** Frame-first allocation ordering (`AcquireFrame` BEFORE `DiskManager::AllocatePage`) to prevent disk-page leaks on memory exhaustion!
- **Why Database Engines Need It:** If `NewPage` allocated Page 10 on disk first and THEN found all buffer frames were pinned (`kResourceExhausted`), Page 10 would leak permanently on disk with no caller ever receiving its `PageId`!
- **How It Works:** Under `latch_`, acquire `fid` via `AcquireFrameLocked`. If that fails (`kResourceExhausted`), return immediately—zero disk allocation happened! Otherwise call `disk_manager_.AllocatePage(initial_type)`; if that fails, return `fid` to `free_list_`. Initialize `frames_[fid].page_.Initialize(new_id, initial_type)`, set `page_id_ = new_id`, `pin_count_ = 1`, `page_table_[new_id] = fid`, `replacer_->Pin(fid)`.
- **Design Trade-offs:** Returning `(PageId, Page*)` cleanly avoids uninitialized output pointer parameters.
- **Common Mistakes:** Calling `DiskManager::AllocatePage` BEFORE checking if a buffer frame is available! Leaving stale data in the reused buffer frame instead of initializing it with `new_id` and `initial_type`.
- **Understanding Checkpoint:** Why must `NewPage` obtain an available buffer frame *before* asking `DiskManager` to allocate a new page on disk?

### Component Context
- **Why It Exists:** Creates new `TableHeap` and `BPlusTree` pages directly inside the Buffer Pool.
- **Module Relationships:** Calls `DiskManager::AllocatePage`; used by `TableHeap` and `BPlusTree`.

### Files Expected to Change
- `include/tinydb/buffer/buffer_pool_manager.hpp`
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_new_delete_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BufferPoolManager::NewPage(PageType)`.
2. Test: creating new pages when frames are free, creating new page forcing dirty eviction of an unpinned page, and failing `NewPage` when all frames are pinned WITHOUT increasing `DiskManager::PageCount()`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Failed `NewPage` when pool is full allocates `0` pages in `DiskManager`.
- Returned `Page*` has valid initialized `PageHeader` matching `PageId` and `initial_type`.

### Definition of Done
`NewPage` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-095: implement BufferPoolManager::NewPage with leak-free ordering`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-096` (`DeletePage`).
````

---

## DAY-096 — Implement `BufferPoolManager::DeletePage` (Pin Protection, Eviction, and Disk Reclamation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-096
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::DeletePage` (Pin Protection, Eviction, and Disk Reclamation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-095 complete.

### Objective
Implement `Status BufferPoolManager::DeletePage(PageId page_id)` checking if `page_id` is in `page_table_`: refusing deletion if `pin_count > 0`, otherwise removing it from `replacer_` and `page_table_`, resetting the frame back onto `free_list_` (without wasting disk writes on a deleted dirty page!), and calling `DiskManager::DeallocatePage(page_id)`.

### Concepts & Daily Learning
- **Concept:** Safe page deallocation synchronizing in-memory cache eviction with on-disk freelist reclamation.
- **Why Database Engines Need It:** If `DiskManager::DeallocatePage(5)` ran while Page 5 remained cached in `BufferPoolManager` (or worse, pinned!), a later `FlushAllPages` would overwrite the newly-freed `kFree` page on disk, or a `FetchPage(5)` would return stale cached memory!
- **How It Works:** Under `latch_`, reject `!IsDataPageId(page_id)`. If `page_id` is in `page_table_`: if `frames_[fid].pin_count_ > 0` return `Status::InvalidArgument("Cannot delete pinned page")`; else remove `fid` from `replacer_`, call `disk_manager_.DeallocatePage(page_id)` (on error, restore `replacer_->Unpin(fid)` and return error!), then erase `page_id` from `page_table_`, reset `frames_[fid]`, push `fid` back onto `free_list_`. If not in `page_table_`, still forward to `disk_manager_.DeallocatePage(page_id)`.
- **Design Trade-offs:** Deallocating via `DiskManager` BEFORE erasing `page_table_` or resetting the frame makes failed deallocation (e.g. invalid/already-free page) leave cache state coherent.
- **Common Mistakes:** Flushing a dirty page being deleted instead of discarding its dirty flag; forgetting `replacer_->Pin(fid)` when returning `fid` to `free_list_` (leaving a free frame still listed in `LRUReplacer`!).
- **Understanding Checkpoint:** Why MUST `DeletePage` remove the frame from `LRUReplacer` before pushing `fid` back onto `free_list_`?

### Component Context
- **Why It Exists:** Safe cache-coherent page reclamation for `DROP TABLE` and B+ Tree node merges.
- **Module Relationships:** Connects `BufferPoolManager` with `DiskManager::DeallocatePage`.

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_new_delete_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BufferPoolManager::DeletePage`.
2. Add tests: deleting pinned page fails; deleting unpinned dirty cached page discards dirty buffer, returns frame to `free_list_`, reclaims disk page (`FreePageCount() == 1`), and subsequent `FetchPage` on that ID fails! Deleting an uncached disk page also succeeds.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero stale cache entries or dangling `LRUReplacer` entries after `DeletePage`; pinned pages protected 100%.

### Definition of Done
`DeletePage` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-096: implement BufferPoolManager::DeletePage with pin and cache safety`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-097` (`FlushPage` & `FlushAllPages`).
````

---

## DAY-097 — Implement `BufferPoolManager::FlushPage` and `BufferPoolManager::FlushAllPages`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-097
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::FlushPage` and `BufferPoolManager::FlushAllPages`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-096 complete.

### Objective
Implement `FlushPage(PageId)` (writing & syncing target page cleanly if resident, regardless of pin count unless blocked by future no-steal hold!) and `FlushAllPages()` (writing all dirty cached pages and calling `DiskManager::FlushAll()`).

### Concepts & Daily Learning
- **Concept:** Explicit dirty-page flushing (`FlushPage` / `FlushAllPages`), clearing `is_dirty_` only after disk write succeeds, and preserving `pin_count_` & `LRUReplacer` state!
- **Why Database Engines Need It:** Before checkpoints or clean shutdown, dirty frames in RAM must be persisted to `.tinydb` even if they aren't evicted yet.
- **How It Works:**
  - `FlushPage(page_id)`: validate `IsDataPageId`, look up in `page_table_` (`kNotFound` if not cached—or no-op? Returning `kNotFound` or documented bool/Status; let's document clearly: `FlushPage` writes & clears `is_dirty_` if cached and dirty, or syncs!). Wait: should `FlushPage` fail `kNotFound` if `page_id` isn't in `page_table_`? Standard BusTub returns `false` if not in `page_table_`, and writes unconditionally (clearing `is_dirty_`) if found! Returning `kNotFound` (or writing only if dirty + syncing) is super clear—let's write if cached (and clear `is_dirty_`), returning `kNotFound` if absent from `page_table_`.
  - `FlushAllPages()`: iterate all entries in `page_table_`, write every frame with `is_dirty_ == true` (skipping clean frames to avoid redundant disk writes!), clear `is_dirty_ = false`, then call `disk_manager_.FlushAll()`.
- **Design Trade-offs:** Skipping clean frames in `FlushAllPages` avoids rewriting hundreds of untouched cached pages during shutdown!
- **Common Mistakes:** Modifying `pin_count_` or `replacer_` during `FlushPage` (flushing does NOT unpin or change LRU residency!).
- **Understanding Checkpoint:** Why should `FlushAllPages()` skip non-dirty cached pages, and why must it leave `pin_count_` untouched?

### Component Context
- **Why It Exists:** Explicit memory-to-disk synchronization boundary.
- **Module Relationships:** Called by `Shutdown()` (`DAY-102`) and post-commit checkpointing (`Phase 08`).

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_flush_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FlushPage` and `FlushAllPages`.
2. Write unit tests verifying: dirty cached page persists after `FlushPage`, `is_dirty_` resets to `false` (so subsequent eviction does NOT trigger an extra `page_writes`!), `FlushAllPages` writes ONLY dirty pages, and clean pages incur zero writes.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Only dirty frames incur disk writes on `FlushAllPages`; `is_dirty_` clears cleanly so eviction skips redundant writes.

### Definition of Done
`FlushPage` and `FlushAllPages` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-097: implement BufferPoolManager::FlushPage and FlushAllPages`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-098` (All-frames-pinned & eviction exhaustion).
````

---

## DAY-098 — Harden All-Frames-Pinned Exhaustion and Zero-Eviction Guarantees

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-098
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Harden All-Frames-Pinned Exhaustion and Zero-Eviction Guarantees
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-097 complete.

### Objective
Write targeted adversarial tests (`tests/unit/buffer/buffer_pool_pin_safety_test.cpp`) proving `BufferPoolManager` NEVER evicts a pinned frame under pool saturation (`pool_size = 1, 2, 4`), preserves existing cached dirty contents when `FetchPage`/`NewPage` fails with `kResourceExhausted`, and recovers smoothly as soon as a single frame is unpinned.

### Concepts & Daily Learning
- **Concept:** Pin-count exhaustion behavior, graceful degradation (`kResourceExhausted`), and recovery upon unpin.
- **Why Database Engines Need It:** If a caller forgets to unpin pages or holds too many pages simultaneously, evicting a pinned frame silently corrupts live pointers! Failing cleanly with `kResourceExhausted` protects memory safety and exposes pin leaks immediately.
- **How It Works:** Verify `pool_size = 1` and `pool_size = 3` under mixed `FetchPage` + `NewPage` saturation: new requests fail `kResourceExhausted`, existing pinned `Page*` pointers remain identical (`address + contents`), unpinning 1 frame allows the next `FetchPage`/`NewPage` to succeed by evicting ONLY that unpinned frame!
- **Design Trade-offs:** Strict non-eviction of pinned frames prevents use-after-eviction bugs; RAII `PageGuard`s (`DAY-100`) prevent callers from leaking pins in the first place.
- **Common Mistakes:** Returning a stale pointer or corrupting an existing pinned frame when `AcquireFrameLocked` fails.
- **Understanding Checkpoint:** Why is returning `StatusCode::kResourceExhausted` when all frames are pinned infinitely better than forcibly evicting the oldest pinned frame?

### Component Context
- **Why It Exists:** Validates TinyDB's #1 buffer pool rule ("Never evict pinned pages").
- **Module Relationships:** Exercises `BufferPoolManager` + `LRUReplacer`.

### Files Expected to Change
- `tests/unit/buffer/buffer_pool_pin_safety_test.cpp` (new)
- `src/buffer/buffer_pool_manager.cpp` (any invariant tightening found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement saturation, pointer-stability, and post-unpin recovery tests in `buffer_pool_pin_safety_test.cpp`.
2. Add a diagnostic `ActivePinCount()` / `PinnedFrameCount()` helper for leak assertions in tests!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Saturated pools reject new allocations/fetches cleanly, keep pinned addresses intact, and recover when 1 frame unpins.

### Definition of Done
Pin-safety tests passing under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-098: harden all-pinned exhaustion handling and add ActivePinCount helper`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-099` (No-steal pin/hold hook preparation + guard design).
````

---

## DAY-099 — Design RAII `PageGuard` (`ReadPageGuard` / `WritePageGuard`) Ownership Model

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-099
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Design RAII `PageGuard` (`ReadPageGuard` / `WritePageGuard`) Ownership Model
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-098 complete.

### Objective
Design `PageGuard`, `ReadPageGuard`, and `WritePageGuard` (`include/tinydb/buffer/page_guard.hpp`) eliminating manual `UnpinPage` calls: `ReadPageGuard` exposes `const Page&` and auto-unpins with `is_dirty = false`, while `WritePageGuard` exposes mutable `Page&`, tracks `is_dirty = true`, and auto-unpins cleanly on destruction or `Drop()`.

### Concepts & Daily Learning
- **Concept:** Move-only RAII guards (`PageGuard`), idempotent `Drop()`, and `const`-correct separation between read-only (`ReadPageGuard`) and mutating (`WritePageGuard`) page access.
- **Why Database Engines Need It:** Every `if (!ok) return status;` branch after `FetchPage` is a potential pin-count leak if callers call `UnpinPage` manually; RAII guards guarantee automatic `UnpinPage` on EVERY exit path—and `ReadPageGuard` prevents accidentally mutating a page without marking it dirty!
- **How It Works:** `BasicPageGuard` holds `(BufferPoolManager* bpm_, Page* page_, PageId page_id_, bool is_dirty_)`. Move constructor/assignment transfer ownership and set `other.bpm_ = nullptr`, `other.page_ = nullptr` (calling `Drop()` on existing held guard before move-assign overwrite!). `ReadPageGuard` wraps `BasicPageGuard` with `const Page&` access; `WritePageGuard` exposes `Page&` and sets `is_dirty_ = true`.
- **Design Trade-offs:** Checking out `WritePageGuard` explicitly marks the page dirty on access (or exposes `MarkDirty()`), preventing silent forgotten-dirty bugs!
- **Common Mistakes:** Move-assigning over an already-active `PageGuard` WITHOUT calling `Drop()` on `*this` first (leaking the old pin count!); not nulling `other.bpm_` after move (causing double-unpin!).
- **Understanding Checkpoint:** Why MUST `PageGuard::operator=(PageGuard&& other)` call `Drop()` on `*this` *before* moving `other`'s fields into `*this`?

### Component Context
- **Why It Exists:** Safe ergonomic RAII page interface used by `TableHeap` and `BPlusTree`.
- **Module Relationships:** Returned by `BufferPoolManager::FetchPageRead`, `FetchPageWrite`, `NewPageGuarded`.

### Files Expected to Change
- `include/tinydb/buffer/page_guard.hpp` (new)
- `docs/buffer-pool.md` (document `PageGuard` ownership & move rules)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `BasicPageGuard`, `ReadPageGuard`, and `WritePageGuard` declarations with deleted copy operations and `noexcept` move declarations.
2. Add `FetchPageRead`, `FetchPageWrite`, `NewPageWrite` declarations to `BufferPoolManager`.

### Testing Instructions
- Verify clean compilation (`cmake --build build`).

### Acceptance Criteria
- Header interfaces compile with `-Werror`; design covers move-assign overwrite and `Drop()` idempotency.

### Definition of Done
Guard classes designed and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-099: design move-only RAII ReadPageGuard and WritePageGuard`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-100` (Implementing `PageGuard`s).
````

---

## DAY-100 — Implement `PageGuard`, `ReadPageGuard`, and `WritePageGuard`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-100
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `PageGuard`, `ReadPageGuard`, and `WritePageGuard`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-099 complete.

### Objective
Implement `src/buffer/page_guard.cpp` and `BufferPoolManager::FetchPageRead`, `FetchPageWrite`, and `NewPageGuarded`, ensuring `Drop()` is idempotent, destructors never leak pins, and `WritePageGuard` marks dirty accurately.

### Concepts & Daily Learning
- **Concept:** Implementing Rule-of-Five move-only wrappers and error-safe `FetchPage` -> `Guard` factory methods.
- **Why Database Engines Need It:** Calling `WritePageGuard` inside `BPlusTree` splits and `TableHeap` scans ensures no frame ever stays pinned past a statement or error return.
- **How It Works:** `Drop()` checks `if (bpm_ != nullptr && page_ != nullptr)` -> calls `(void)bpm_->UnpinPage(page_id_, is_dirty_)`, then resets `bpm_ = nullptr`, `page_ = nullptr`. Move-assignment guards `if (this == &other) return *this; Drop(); ...`.
- **Design Trade-offs:** Returning `Result<ReadPageGuard>` / `Result<WritePageGuard>` prevents callers from constructing invalid empty guards accidentally unless default-constructed explicitly (`IsValid() == false`).
- **Common Mistakes:** Accessing `guard.Page()` after `guard.Drop()` or `std::move(guard)` (document + debug-assert `IsValid()`!).
- **Understanding Checkpoint:** Trace `ActivePinCount()` across: `auto g1 = FetchPageWrite(1); auto g2 = std::move(g1); g2.Drop(); g2.Drop();`—how many `UnpinPage` calls happen?

### Component Context
- **Why It Exists:** Safe RAII page lifecycle implementation.
- **Module Relationships:** Used everywhere above `BufferPoolManager` (`TableHeap`, `BPlusTree`).

### Files Expected to Change
- `src/buffer/page_guard.cpp` (new)
- `include/tinydb/buffer/buffer_pool_manager.hpp`
- `src/buffer/buffer_pool_manager.cpp`
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BasicPageGuard`, `ReadPageGuard`, `WritePageGuard` in `src/buffer/page_guard.cpp` and guard factories in `buffer_pool_manager.cpp`.
2. Ensure `CMakeLists.txt` compiles cleanly with `-Werror` and ASan/UBSan.

### Testing Instructions
- Build cleanly and run existing `ctest --output-on-failure`.

### Acceptance Criteria
- Compiles cleanly with zero warnings; `static_assert` verifies guards are move-only (`!is_copy_constructible`).

### Definition of Done
Guards implemented, build green, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-100: implement move-only PageGuard, ReadPageGuard, and WritePageGuard`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-101` (`PageGuard` unit tests).
````

---

## DAY-101 — Write `PageGuard` Move, Overwrite, `Drop()`, and Double-Unpin Prevention Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-101
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Write `PageGuard` Move, Overwrite, `Drop()`, and Double-Unpin Prevention Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-100 complete.

### Objective
Write thorough unit tests (`tests/unit/buffer/page_guard_test.cpp`) verifying `ActivePinCount() == 0` after scope exits, early returns, exceptions, move construction, move-assignment over an active guard (verifying old guard unpins!), double `Drop()`, and `WritePageGuard` dirty persistence on eviction.

### Concepts & Daily Learning
- **Concept:** Exhaustive lifetime and edge-case testing for RAII resource handles.
- **Why Database Engines Need It:** A subtle leak in move-assignment (`g1 = std::move(g2)` while `g1` holds a page!) manifests later inside `BPlusTree` traversal (`FindLeafPage` "crabbing" where parent guard is overwritten by child guard!).
- **How It Works:** Create `pool_size = 1`, hold `g1 = FetchPageRead(1)`, move-assign `g1 = FetchPageRead(1)`? Wait: with `pool_size = 1`, `g1 = std::move(g2)` with `pool_size = 2` tests: `g1 = Page 1`, `g2 = Page 2`, `g1 = std::move(g2)` MUST drop Page 1 immediately (`ActivePinCount() == 1`, NOT `2`!) so Page 1 can be evicted immediately!
- **Design Trade-offs:** Testing `g1 = std::move(g2)` on a `pool_size = 2` pool proves lock-crabbing/parent-drop patterns work without leaks!
- **Common Mistakes:** Not testing move-assignment onto an already-valid `PageGuard`.
- **Understanding Checkpoint:** Why is testing `g1 = std::move(g2)` (when BOTH `g1` and `g2` hold different pages) critical before implementing B+ Tree traversal?

### Component Context
- **Why It Exists:** Proves `PageGuard`s cannot leak or double-unpin under any move/drop pattern.
- **Module Relationships:** Exercises `PageGuard` and `BufferPoolManager`.

### Files Expected to Change
- `tests/unit/buffer/page_guard_test.cpp` (new)
- `src/buffer/page_guard.cpp` (any fixes if uncovered)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all `PageGuard` tests in `page_guard_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `ActivePinCount() == 0` after every move, overwrite, exception, and double-`Drop()`.

### Definition of Done
All `PageGuard` tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-101: add PageGuard move-overwrite, Drop, and pin-leak tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-102` (Shutdown handling).
````

---

## DAY-102 — Implement `BufferPoolManager::Shutdown` and Clean Destructor Handling

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-102
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement `BufferPoolManager::Shutdown` and Clean Destructor Handling
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-101 complete.

### Objective
Implement `Status BufferPoolManager::Shutdown()` (flushing all dirty pages, syncing `DiskManager`, validating `ActivePinCount() == 0` or reporting leaked pins cleanly, clearing `page_table_` and `replacer_`) and safe `~BufferPoolManager()` teardown.

### Concepts & Daily Learning
- **Concept:** Ordered engine shutdown (`TableHeap`/`BPlusTree` -> `BufferPoolManager::Shutdown()` -> `DiskManager::Close()`) and leak diagnostics.
- **Why Database Engines Need It:** If `DiskManager` closes or is destroyed while `BufferPoolManager` still holds dirty un-flushed pages in RAM, all un-evicted writes vanish on exit!
- **How It Works:** `Shutdown()` checks `is_shutdown_`, flushes all dirty pages (`FlushAllPagesLocked()`), checks `ActivePinCount() == 0` (returning warning/error `Status` if callers leaked pins while still flushing cleanly!), resets frames/maps, and marks `is_shutdown_ = true` (idempotent on destructor).
- **Design Trade-offs:** Returning an explicit error from `Shutdown()` if pins remain leaked helps catch dangling `PageGuard` bugs immediately in tests!
- **Common Mistakes:** Destroying `DiskManager` *before* `BufferPoolManager` (dangling `DiskManager&` reference in `~BufferPoolManager`!).
- **Understanding Checkpoint:** In what reverse order must `(DiskManager, BufferPoolManager, CatalogManager)` be constructed and destroyed so `~BufferPoolManager()` never touches a closed `DiskManager`?

### Component Context
- **Why It Exists:** Guarantees zero lost dirty pages on graceful shutdown.
- **Module Relationships:** Paired with `DiskManager::Close()`.

### Files Expected to Change
- `include/tinydb/buffer/buffer_pool_manager.hpp`
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_shutdown_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Shutdown()` and `~BufferPoolManager()`.
2. Write unit tests verifying: dirty pages written via `WritePageGuard` survive `Shutdown()` + `DiskManager::Close()` + reopen even with zero evictions (`pool_size = 16` and only 2 pages!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Un-evicted dirty pages persist across clean `Shutdown()` + reopen; double-`Shutdown()` is idempotent.

### Definition of Done
Shutdown handling implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-102: implement BufferPoolManager::Shutdown and safe teardown`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-103` (Integrating `TableHeap` with `BufferPoolManager`).
````

---

## DAY-103 — Integrate `TableHeap` with `BufferPoolManager` via `BufferPoolPageProvider`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-103
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Integrate `TableHeap` with `BufferPoolManager` via `BufferPoolPageProvider`
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-070 (`IPageProvider`), DAY-102 complete.

### Objective
Implement `BufferPoolPageProvider` (`include/tinydb/buffer/buffer_pool_page_provider.hpp`, `src/buffer/buffer_pool_page_provider.cpp`) bridging `IPageProvider` (or upgrading `TableHeap` to use `ReadPageGuard`/`WritePageGuard` directly!) onto `BufferPoolManager` so `TableHeap` and `CatalogManager` perform ZERO direct disk reads/writes when cached!

### Concepts & Daily Learning
- **Concept:** Adapter pattern upgrading storage layers from direct disk I/O to cached buffer-pool I/O without breaking earlier contracts.
- **Why Database Engines Need It:** Running `TableHeap` directly on `DiskManager` forces a `pread`/`pwrite` syscall on every single `GetTuple`/`InsertTuple`; routing through `BufferPoolManager` serves hot pages from RAM!
- **How It Works:** `BufferPoolPageProvider` implements `IPageProvider`: `ReadPage` fetches via `FetchPageRead` and copies into caller `Page` (or even better: introduce zero-copy `WithReadPage` / `WithWritePage` callback or `ReadPageGuard`/`WritePageGuard` methods on `IPageProvider` so `TableHeap` mutates the pinned frame directly in place without 4KB copies!). Let's give `IPageProvider` clean `FetchRead` / `FetchWrite` / `AllocatePage` / `DeallocatePage` (backed by `BufferPoolManager` guards OR lightweight `DiskManager` guard wrapper!) so BOTH zero-copy caching AND backwards-compatible `DiskManager` tests work seamlessly!
- **Design Trade-offs:** Using a guard-returning or callback `IPageProvider` avoids copying 4096 bytes per tuple read/insert while preserving `DiskPageProvider` for isolated storage tests!
- **Common Mistakes:** Mutating a `Page` inside `BufferPoolManager` and calling `UnpinPage(id, false)` (forgetting `is_dirty = true`)!
- **Understanding Checkpoint:** Why is mutating a pinned `WritePageGuard` frame directly inside `BufferPoolManager` far faster than copying 4096 bytes out, modifying it, and copying 4096 bytes back?

### Component Context
- **Why It Exists:** Connects `TableHeap` and `CatalogManager` to `BufferPoolManager`.
- **Module Relationships:** Bridges Phase 02/03 (`TableHeap`, `CatalogManager`) with Phase 04 (`BufferPoolManager`).

### Files Expected to Change
- `include/tinydb/storage/page_provider.hpp`
- `include/tinydb/buffer/buffer_pool_page_provider.hpp` (new)
- `src/buffer/buffer_pool_page_provider.cpp` (new)
- `src/storage/table_heap.cpp`
- `tests/integration/phase04_table_buffer_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BufferPoolPageProvider` and verify `TableHeap` + `CatalogManager` operations route through `BufferPoolManager`.
2. Write integration test proving `TableHeap` sequential scans and inserts generate `BufferPoolManager` cache hits with zero redundant `DiskStats::page_reads`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Repeated `TableHeap::GetTuple` on cached pages produces `0` additional `DiskManager::page_reads`; mutations survive `Shutdown()` + reopen!

### Definition of Done
`TableHeap` integrated cleanly with `BufferPoolManager`, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-103: integrate TableHeap and CatalogManager with BufferPoolManager`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-104` (Tiny-pool heavy eviction `TableHeap` tests).
````

---

## DAY-104 — Test Multi-Page `TableHeap` Under Tiny Buffer Pool (3 Frames) Forcing Constant Eviction

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-104
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Test Multi-Page `TableHeap` Under Tiny Buffer Pool (3 Frames) Forcing Constant Eviction
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-103 complete.

### Objective
Write stress/integration tests (`tests/integration/phase04_tiny_pool_test.cpp`) running `TableHeap` inserts, updates, cross-page migrations, deletes, compaction, and sequential scans over 15+ heap pages using a tiny 3-frame `BufferPoolManager`, proving zero pin exhaustion and zero lost dirty pages!

### Concepts & Daily Learning
- **Concept:** Pin budget discipline (how many frames an operation pins simultaneously) under severe memory constraint.
- **Why Database Engines Need It:** If `TableHeap::InsertTuple` or `ScanIterator` carelessly holds guards on 5 pages simultaneously while traversing a chain, a small buffer pool deadlocks/exhausts; scoping guards per single page keeps peak simultaneous pins `<= 2`!
- **How It Works:** Verify every `TableHeap` loop releases Page `i`'s guard *before* fetching Page `i+1` (or at most holds 2 guards during `NewPage` link patching!).
- **Design Trade-offs:** Tightly scoping `PageGuard`s inside loops keeps peak pin footprint minimal (`<= 2` frames!).
- **Common Mistakes:** Holding a `PageGuard` for Page 1 open in a local variable across a `while (page_id != kInvalidPageId)` loop traversing Pages 2..15!
- **Understanding Checkpoint:** How many buffer frames does `TableHeap::InsertTuple` ever need pinned at the exact same instant when appending and linking a new tail page?

### Component Context
- **Why It Exists:** Proves `TableHeap` obeys strict pin budgets even under extreme eviction pressure.
- **Module Relationships:** Stress-tests `TableHeap` + `BufferPoolManager`.

### Files Expected to Change
- `tests/integration/phase04_tiny_pool_test.cpp` (new)
- `src/storage/table_heap.cpp` (tighten any guard scope if >2 pins held simultaneously)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `phase04_tiny_pool_test.cpp` with `pool_size = 3` and 500+ rows spanning >15 pages, verifying heavy `evictions > 0`, `ActivePinCount() == 0` after every operation, and exact oracle match after `Shutdown()` + reopen!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 500+ row operations across >15 pages succeed on a 3-frame buffer pool with `ActivePinCount() == 0`.

### Definition of Done
Tiny-pool integration test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-104: verify multi-page TableHeap under 3-frame forced eviction`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-105` (Invalid `PageId` & corrupt-read error propagation).
````

---

## DAY-105 — Test Buffer Pool Invalid `PageId` and Corrupt-Read Error Propagation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-105
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Test Buffer Pool Invalid `PageId` and Corrupt-Read Error Propagation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-104 complete.

### Objective
Write negative tests (`tests/unit/buffer/buffer_pool_error_test.cpp`) verifying `BufferPoolManager` propagates `DiskManager` corruption (`kCorruption`), out-of-range `PageId` (`kOutOfRange`), deleted-page fetch (`kCorruption`), and post-shutdown call rejections without corrupting `page_table_` or `free_list_`.

### Concepts & Daily Learning
- **Concept:** Clean error propagation and container invariant preservation across I/O faults.
- **Why Database Engines Need It:** If reading a corrupt page from disk fails CRC32 inside `DiskManager::ReadPage`, `BufferPoolManager` must NOT leave that broken page in `page_table_` or drop the frame!
- **How It Works:** Corrupt Page 2 raw bytes on disk while `BufferPoolManager` is closed, reopen `DiskManager` + fresh `BufferPoolManager`, call `FetchPageRead(2)`, assert `kCorruption`, then verify `FetchPageRead(1)` still succeeds and `FreeFrameCount()` didn't shrink!
- **Design Trade-offs:** Returning `Status` / `Result` cleanly instead of `nullptr` preserves exact cause (`kCorruption` vs `kResourceExhausted`).
- **Common Mistakes:** Leaving a corrupt frame inside `page_table_` after `ReadPage` failure (so a second call hits cache instead of rejecting!).
- **Understanding Checkpoint:** If `FetchPage(2)` encounters CRC32 corruption on disk during eviction of dirty Page 1, should Page 1's eviction still have succeeded safely, and what state must Page 2's temporary frame return to?

### Component Context
- **Why It Exists:** Ensures `BufferPoolManager` stays consistent under disk errors.
- **Module Relationships:** Tests error seams between `DiskManager` and `BufferPoolManager`.

### Files Expected to Change
- `tests/unit/buffer/buffer_pool_error_test.cpp` (new)
- `src/buffer/buffer_pool_manager.cpp` (fix any error-path edge case found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement tests for corrupt disk page, deleted disk page, out-of-range `PageId`, Page 0 access, and post-shutdown calls.
2. Verify `FreeFrameCount() + page_table_.size() == PoolSize()` invariant holds after every error!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every error case returns clean `Status` and preserves `FreeFrameCount() + CachedPageCount() == PoolSize()`.

### Definition of Done
Error-propagation tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-105: verify BufferPoolManager error propagation and invariant preservation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-106` (Concurrency safety audit & multi-threaded stress test).
````

---

## DAY-106 — Audit Concurrency Safety and Run Multi-Threaded `BufferPoolManager` Stress Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-106
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Audit Concurrency Safety and Run Multi-Threaded `BufferPoolManager` Stress Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-105 complete.

### Objective
Audit `BufferPoolManager` locking (`latch_` vs `LRUReplacer::latch_` lock ordering—eliminating nested lock deadlocks!) and add a multi-threaded stress test (`tests/unit/buffer/buffer_pool_concurrency_test.cpp`) where 4 threads concurrently `FetchPage`, `UnpinPage`, and `FlushPage` across shared pages cleanly.

### Concepts & Daily Learning
- **Concept:** Lock ordering between `BufferPoolManager::latch_` and `LRUReplacer::latch_`, and page-content synchronization (`ReadPageGuard`/`WritePageGuard` vs pool-level `latch_`).
- **Why Database Engines Need It:** Holding `BufferPoolManager::latch_` across all calls ensures `page_table_`, `free_list_`, and `LRUReplacer` stay consistent; however, if `PageFrame` contents are also read/written concurrently without per-page latching, data races occur unless callers synchronize page access!
- **How It Works:** Audit lock order: `BufferPoolManager::latch_` -> `LRUReplacer::latch_` (never the reverse!). Add per-frame `ReaderWriterLatch` (or test read-only shared page fetches + disjoint-page concurrent mutations under ASan/UBSan + `-pthread`). Adding an optional per-frame `ReaderWriterLatch` inside `PageFrame` (taken automatically by `ReadPageGuard`/`WritePageGuard`—carefully AFTER releasing `bpm->latch_`!) makes even concurrent page payload access race-free! Wait: be careful about lock-order inversion if a caller tries to hold `WritePageGuard` on Page A while fetching Page B—test single-thread & multi-thread cleanly!
- **Design Trade-offs:** Documenting clearly whether page-content latching is per-frame or engine-serialized (remember `v1` has a single active transaction, while testing multi-threaded pool-metadata safety prevents `bpm->latch_` deadlocks).
- **Common Mistakes:** Holding `BufferPoolManager::latch_` while blocking on another frame's latch (instant deadlock!).
- **Understanding Checkpoint:** Why would acquiring a `PageFrame`'s `rw_latch_` *before* calling `UnpinPage` (which acquires `BufferPoolManager::latch_`), while `FetchPageRead` holds `BufferPoolManager::latch_` first, cause a deadlock?

### Component Context
- **Why It Exists:** Ensures `BufferPoolManager` metadata operations are deadlock-free and race-free.
- **Module Relationships:** Audits `BufferPoolManager` and `LRUReplacer`.

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp` (audit lock scopes)
- `tests/unit/buffer/buffer_pool_concurrency_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit lock hierarchy in `BufferPoolManager` + `LRUReplacer`.
2. Add multi-threaded test spawning 4 worker threads performing 500 concurrent read-only `FetchPageRead` + `FlushPage` + `NewPage` operations across a bounded pool (`pool_size = 8`, `pages = 20`) with zero deadlocks and zero ASan/UBSan issues.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-threaded test completes reliably with `ActivePinCount() == 0` and all page checksums intact.

### Definition of Done
Concurrency audit and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-106: audit BufferPoolManager lock ordering and add concurrency stress test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-107` (Buffer Pool metrics & diagnostics).
````

---

## DAY-107 — Implement Buffer Pool Hit/Miss/Eviction Metrics and Diagnostic Inspection

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-107
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement Buffer Pool Hit/Miss/Eviction Metrics and Diagnostic Inspection
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-106 complete.

### Objective
Complete `BufferPoolStats` (`hits`, `misses`, `evictions`, `dirty_evictions`, `flushes`, `HitRatio()`) and `ResetStats()` on `BufferPoolManager` with unit tests verifying exact cache hit ratios under working-set vs thrashing workloads.

### Concepts & Daily Learning
- **Concept:** Buffer pool hit ratio (`hits / (hits + misses)`), working-set size vs pool size, and thrashing metrics.
- **Why Database Engines Need It:** Measuring hit ratio and `dirty_evictions` lets us tune pool size and prove in benchmarks (Phase 10) that B+ Tree lookups hit upper internal pages in RAM (`>95%` hit rate on repeated queries!).
- **How It Works:** Expose `BufferPoolStats GetStats() const` (snapshot under `latch_`) and `HitRatio()` helper returning `0.0` when `hits + misses == 0`.
- **Design Trade-offs:** Keeping stats snapshots copyable by value avoids atomic contention on non-hot paths while returning consistent cross-field ratios.
- **Common Mistakes:** Counting `NewPage` as a cache miss or hit inconsistently without documenting it; dividing by zero when `hits + misses == 0`.
- **Understanding Checkpoint:** Why does a sequential scan over 100 pages on a 10-frame pure-LRU buffer pool yield a `0%` hit rate on the second full scan ("sequential flooding")?

### Component Context
- **Why It Exists:** Quantitative performance instrumentation for the memory layer.
- **Module Relationships:** Inspected in Phase 05 B+ Tree tests and Phase 10 benchmarks.

### Files Expected to Change
- `include/tinydb/buffer/buffer_pool_manager.hpp`
- `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_stats_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Finalize `BufferPoolStats` (`HitRatio()`) and `ResetStats()`.
2. Write tests verifying: hot working set (`4` pages in `4`-frame pool -> `100%` hit ratio after warmup) vs thrashing (`5` pages cycled in `4`-frame LRU pool -> `0%` hit ratio!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Exact `hits`, `misses`, `evictions`, `dirty_evictions`, and `HitRatio()` match theoretical expectations.

### Definition of Done
Stats metrics tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-107: add BufferPoolStats hit/miss/eviction metrics and tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-108` (Sticky-dirty & multi-unpin regression tests).
````

---

## DAY-108 — Add Regression Tests for Sticky Dirty Flags Across Multiple Pin/Unpin Cycles

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-108
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Add Regression Tests for Sticky Dirty Flags Across Multiple Pin/Unpin Cycles
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-107 complete.

### Objective
Write focused regression tests (`tests/unit/buffer/buffer_pool_regression_test.cpp`) verifying sticky dirty preservation across complex interleavings: `FetchWrite -> Write -> Unpin(true) -> FetchRead -> Unpin(false) -> Evict`, `FlushPage` clearing dirty while `pin_count > 0`, and `PageGuard` conversions/scopes.

### Concepts & Daily Learning
- **Concept:** Regression test suites capturing subtle caching bugs permanently.
- **Why Database Engines Need It:** Lost dirty writes are silent data-loss bugs—the hardest kind to diagnose in production; permanent regression tests ensure future refactors never break `is_dirty |= is_dirty`.
- **How It Works:** Test exact combinations: (1) dirty + clean unpin stays dirty; (2) clean + dirty unpin becomes dirty; (3) `FlushPage` clears dirty so eviction writes 0 times; (4) `WritePageGuard` dropped -> `ReadPageGuard` fetched & dropped -> dirty preserved on eviction.
- **Design Trade-offs:** Dedicated regression suites keep failure triage instant when modifying `BufferPoolManager`.
- **Common Mistakes:** Testing only `Unpin(true)` followed by immediate `FlushAllPages()`, missing the `Unpin(false)` overwrite case!
- **Understanding Checkpoint:** If Page 1 is pinned twice, unpinned once with `is_dirty = true`, flushed via `FlushPage(1)`, and THEN unpinned by the second holder with `is_dirty = false`—what should `is_dirty_` be? (And what if the second holder used `WritePageGuard` and mutated AFTER `FlushPage`?)

### Component Context
- **Why It Exists:** Permanent safety guard on dirty-page accounting.
- **Module Relationships:** Protects `BufferPoolManager`.

### Files Expected to Change
- `tests/unit/buffer/buffer_pool_regression_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all sticky-dirty and guard regression cases in `buffer_pool_regression_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All sticky-dirty regression tests pass cleanly with zero leaks.

### Definition of Done
Regression tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-108: add sticky-dirty and multi-pin BufferPool regression tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-109` (Clean vs dirty eviction & reopen tests).
````

---

## DAY-109 — Add Regression Tests for Clean vs Dirty Eviction and Restart Durability

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-109
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Add Regression Tests for Clean vs Dirty Eviction and Restart Durability
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-108 complete.

### Objective
Write tests in `tests/unit/buffer/buffer_pool_regression_test.cpp` verifying that clean cached frames NEVER incur disk writes on eviction (`dirty_evictions == 0`), dirty evicted frames DO write back cleanly and survive full process/manager reconstruction, and `DeletePage` removes cached pages cleanly across restarts.

### Concepts & Daily Learning
- **Concept:** Read-only workload zero-write guarantees and eviction-vs-shutdown durability equivalence.
- **Why Database Engines Need It:** Writing clean pages during read-heavy query eviction cuts SSD lifetime and halves read throughput; verifying `dirty_evictions == 0` under read workloads confirms zero wasted writes!
- **How It Works:** Populate 10 pages, `Shutdown()` + reopen, run 100 random `FetchPageRead` across `pool_size = 3` (forcing >50 evictions), assert `DiskStats::page_writes == 0` and `dirty_evictions == 0`! Then mutate alternating pages via `WritePageGuard`, reopen cold, and verify all mutations persisted.
- **Design Trade-offs:** Checking exact `DiskStats::page_writes` prevents silent regressions where `FetchPageRead` accidentally marks frames dirty.
- **Common Mistakes:** Initializing `WritePageGuard` where `ReadPageGuard` was intended inside internal helpers!
- **Understanding Checkpoint:** Why must `FetchPageRead` followed by `~ReadPageGuard()` on a clean frame result in ZERO `page_writes` even after 1,000 evictions?

### Component Context
- **Why It Exists:** Verifies I/O efficiency and durability invariants across restarts.
- **Module Relationships:** Validates `BufferPoolManager` + `DiskManager`.

### Files Expected to Change
- `tests/unit/buffer/buffer_pool_regression_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add read-only zero-write eviction and mixed dirty eviction + restart durability tests.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Read-only eviction causes `0` disk writes; dirty eviction + shutdown persists 100% of writes across cold restarts.

### Definition of Done
Durability and clean-eviction tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-109: verify zero-write clean evictions and dirty restart durability`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-110` (Randomized BufferPool fuzz test).
````

---

## DAY-110 — Implement Randomized Buffer Pool Read-Modify-Evict-Delete Fuzz Test vs Oracle

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-110
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Implement Randomized Buffer Pool Read-Modify-Evict-Delete Fuzz Test vs Oracle
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-109 complete.

### Objective
Write a seeded randomized oracle test (`tests/unit/buffer/buffer_pool_fuzz_test.cpp`) executing 3,000+ interleaved `NewPage`, `FetchPageRead`, `FetchPageWrite` (mutating payload patterns), `DeletePage`, `FlushPage`, `FlushAllPages`, and cold `Shutdown() -> Reopen` cycles against a golden in-memory `std::map<PageId, std::array<std::byte, 4064>>` oracle with `pool_size = 5`.

### Concepts & Daily Learning
- **Concept:** Randomized long-horizon model checking for caching + eviction + deletion + persistence.
- **Why Database Engines Need It:** Complex interactions—like allocating a freed `PageId` immediately after deleting it while the pool is near capacity—only surface under randomized multi-step churn.
- **How It Works:** Maintain golden `live_pages` map in memory plus vector of currently held `ReadPageGuard`/`WritePageGuard`s (`<= 3` simultaneous pins so pool doesn't stay saturated!). Randomly pick: hold/drop guard, allocate, mutate, verify, delete (only unpinned!), flush, or restart `BufferPoolManager` + `DiskManager`.
- **Design Trade-offs:** Holding multiple simultaneous random guards tests overlapping pin lifetimes (`pin_count > 1`) just like real B+ Tree crabbing!
- **Common Mistakes:** Attempting to `DeletePage` while the test harness itself still holds an active `PageGuard` on that `PageId` (and confusing expected `InvalidArgument` rejection with failure!).
- **Understanding Checkpoint:** Why is randomly holding up to 3 overlapping guards while running `NewPage`/`Fetch`/`Delete` on a 5-frame pool an effective test of `BufferPoolManager`?

### Component Context
- **Why It Exists:** High-confidence stress proof before building the B+ Tree index atop `BufferPoolManager`.
- **Module Relationships:** Exercises the entire Phase 04 API.

### Files Expected to Change
- `tests/unit/buffer/buffer_pool_fuzz_test.cpp` (new)
- `src/buffer/buffer_pool_manager.cpp` (fix any rare bug surfaced)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `buffer_pool_fuzz_test.cpp` (fixed seed, 3,000+ ops, periodic restarts).
2. Run under Debug + ASan/UBSan and verify `< 2s` runtime.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 3,000+ randomized overlapping-guard operations + restarts match golden oracle 100% with zero ASan/UBSan findings.

### Definition of Done
Fuzz suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-110: add oracle-verified randomized BufferPoolManager fuzz test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-111` (Phase 04 documentation & sign-off).
````

---

## DAY-111 — Finalize `docs/buffer-pool.md` and Sign Off Phase 04

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-111
- **Phase:** Phase 04 — Buffer Pool
- **Session Title:** Finalize `docs/buffer-pool.md` and Sign Off Phase 04
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-086 through DAY-110 complete.

### Objective
Audit and finalize `docs/buffer-pool.md` (frame states, `LRUReplacer` invariants, `PageGuard` RAII guarantees, hit/eviction metrics, `TableHeap` integration), run the entire Phase 00–04 suite across Debug/Release/ASan+UBSan, and formally sign off Phase 04 (`86.0 / 220.0` hours).

### Concepts & Daily Learning
- **Concept:** Phase completion audit and technical interview readiness check for buffer management.
- **Why Database Engines Need It:** Being able to walk smoothly through `FetchPage` hit vs miss vs dirty eviction vs `UnpinPage` sticky-dirty vs `PageGuard` is a core database systems skill.
- **How It Works:** Verify every API, invariant, and file path in `docs/buffer-pool.md` matches the actual codebase, check `clang-format`, run all tests, update `PROGRESS.md` (`86.0` completed hours), and point `NEXT_SESSION.md` to `DAY-112`.
- **Design Trade-offs:** Clean sign-off ensures zero lingering technical debt as TinyDB enters its largest phase (`Phase 05 — B+ Tree Indexing`, 32 hours).
- **Common Mistakes:** Leaving Phase 04 docs outdated relative to `ReadPageGuard`/`WritePageGuard` APIs.
- **Understanding Checkpoint:** Explain the full lifecycle of a 4KB page inside `BufferPoolManager` from `NewPageGuarded` -> mutation -> eviction -> `FetchPageRead` -> `DeletePage`.

### Component Context
- **Why It Exists:** Completes Phase 04 (`20.0` hours).
- **Module Relationships:** Hands off a rock-solid `BufferPoolManager` to Phase 05 (`BPlusTree`).

### Files Expected to Change
- `docs/buffer-pool.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Update `docs/buffer-pool.md` to match final code and symbols.
2. Run full Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format` check.
3. Update `PROGRESS.md` (`86.0 / 220.0` hours) and `NEXT_SESSION.md` -> `DAY-112`.

### Testing Instructions
- Full Debug, ASan+UBSan, Release `ctest --output-on-failure` + `clang-format` dry-run.

### Acceptance Criteria
- 100% of Phase 00–04 tests pass across all configs with zero warnings.

### Definition of Done
Phase 04 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-111: finalize Buffer Pool docs and sign off Phase 04`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 05 (`DAY-112` — B+ Tree capacity math & node layout).
````
