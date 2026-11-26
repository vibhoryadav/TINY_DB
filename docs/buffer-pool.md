# Buffer Pool Design (Planned — Phase 04)

> **Status:** Specification Blueprint. Implemented incrementally in Phase 04 (`DAY-086`–`DAY-111`).

## Responsibilities
The Buffer Pool Manager (`BufferPoolManager`) caches fixed 4096-byte disk pages in a bounded pool of `PageFrame`s in RAM, decoupling upper engine layers from direct disk I/O.

## Key Invariants
1. **Never Evict Pinned Pages:** Only frames with `pin_count == 0` are eligible in `LRUReplacer`.
2. **Sticky Dirty Bit:** `UnpinPage(page_id, is_dirty)` performs `frame.is_dirty = frame.is_dirty || is_dirty`. Passing `is_dirty = false` after an earlier `true` must never discard dirty state.
3. **Prevent Double Unpinning:** Decrementing `pin_count` below `0` is rejected cleanly.
4. **No-Steal Coordination (Phase 08):** Uncommitted pages modified by an active transaction are held un-evictable / never written to disk until `COMMIT` finishes writing and flushing the WAL.
5. **RAII Ownership (`PageGuard`):** Callers prefer move-only `PageGuard` wrappers over raw `FetchPage`/`UnpinPage` pairs so early returns or exceptions never leak pin counts.
