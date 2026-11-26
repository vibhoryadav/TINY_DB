# Architectural Design Decisions (v1.0)

This log records every major design choice in TinyDB v1.0, the alternatives considered, and the interview-ready rationale behind each decision.

---

## DD-001: Fixed 4096-Byte Disk Pages
- **Decision:** Every database page is fixed at `4096` bytes (`4 KiB`).
- **Alternatives:** Variable-size pages or larger pages (`8 KiB` / `16 KiB`).
- **Rationale:** `4096` bytes matches standard OS virtual memory pages and common filesystem block sizes, simplifying aligned I/O, keeping buffer pool math predictable, and providing realistic B+ Tree fanout (~100–250 entries per page) while still exercising multi-level splits early in testing.

## DD-002: Explicit Field Serialization (Zero Raw Struct Disk Writes)
- **Decision:** Never write C++ structs (`reinterpret_cast<const char*>(&obj)`) directly to disk. Serialize and deserialize every header, slot, tuple, node, and WAL record field-by-field in documented little-endian byte order.
- **Alternatives:** `memcpy` of packed structs (`__attribute__((packed))`).
- **Rationale:** Raw struct memory layouts depend on compiler padding, alignment, ABI, and CPU endianness, and packed structs cause unaligned access UB. Explicit serialization guarantees portability, deterministic file formats, and strict bounds validation during deserialization.

## DD-003: Single-Column Integer B+ Tree with Composite `(key, RowID)` Ordering
- **Decision:** Initial B+ Tree indexes store `int64_t` keys mapped to `RowID`, ordering entries lexicographically by `(key, RowID)` in leaf and internal nodes.
- **Alternatives:** Overflow page chains for duplicate keys, or unique-only indexes.
- **Rationale:** Real tables frequently contain duplicate indexed values (e.g., `age = 25`). Ordering by `(key, RowID)` makes every entry uniquely identifiable without complex overflow-page management, supports clean `Delete(key, row_id)` during `UPDATE`/`DELETE`, and keeps structural invariants simple and verifiable.

## DD-004: Separate Versioned Catalog File with Atomic Rename
- **Decision:** Store table, column, and index metadata in a separate versioned catalog file updated via write-temp + `fsync` + atomic `rename`, keeping DDL outside SQL transactions in v1.
- **Alternatives:** Bootstrapping system catalog tables (`pg_class`, `pg_attribute`) inside the main heap/B+ Tree before SQL exists.
- **Rationale:** Storing catalog metadata separately breaks the chicken-and-egg bootstrap dependency in Phases 02–03, keeps metadata resilient via atomic POSIX rename, and clearly separates transactional DML from non-transactional v1 DDL.

## DD-005: Pull-Based Volcano Execution Architecture
- **Decision:** Implement query execution using `Init()` / `Next()` pull-based iterators (`AbstractExecutor`).
- **Alternatives:** Materialized operator pipelines or compiled JIT execution.
- **Rationale:** Volcano iterators stream tuples one at a time without materializing large intermediate tables in memory, map cleanly onto C++ class hierarchies, and are the canonical architecture taught in database internals.

## DD-006: Single Active Transaction + No-Steal, Redo-Only WAL Recovery
- **Decision:** Support one active transaction at a time using a no-steal buffer pool policy, in-memory before-images for runtime `ROLLBACK`, and page after-images in WAL for redo-only crash recovery.
- **Alternatives:** Full ARIES (steal/no-force with Analysis, Redo, Undo, CLRs, and physiological undo logging).
- **Rationale:** With one active transaction and no-steal buffering, uncommitted dirty pages never reach disk, eliminating the need for disk-based UNDO and CLRs in v1 while still teaching LSNs, WAL write-ahead ordering, durable `fsync` commit, idempotent `page_lsn` redo, and real crash testing. Full ARIES and MVCC are deferred to v2.0.

## DD-007: GoogleTest + Offline-Friendly CMake Discovery
- **Decision:** Use GoogleTest as the test framework, resolved via `find_package(GTest CONFIG QUIET)` first with `FetchContent` fallback.
- **Alternatives:** Vendoring a full framework into Git or relying strictly on online `FetchContent`.
- **Rationale:** Local `find_package` builds work offline in restricted sandboxes without bloating Git history, while `FetchContent` ensures zero-setup builds on CI runners and fresh developer machines.
