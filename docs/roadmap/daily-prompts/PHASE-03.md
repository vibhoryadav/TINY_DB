# Phase 03 — Catalog Management (`DAY-076` – `DAY-085`)

**Phase Budget:** 8.0 hours (8 × 45 min + 2 × 60 min = 10 sessions)
**Goal:** Implement TinyDB's `CatalogManager` storing table, column, and index metadata with unique monotonic IDs in a separate versioned metadata file updated via write-temp + `fsync` + atomic `rename`.

---

## DAY-076 — Design `TableMetadata`, `ColumnMetadata`, and `IndexMetadata` Structures

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-076
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Design `TableMetadata`, `ColumnMetadata`, and `IndexMetadata` Structures
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 02 complete (`DAY-075`).

### Objective
Define `TableMetadata` (`table_id`, `name`, `schema`, `first_page_id`), `IndexMetadata` (`index_id`, `index_name`, `table_id`, `column_name`, `column_index`, `root_page_id`), and `CatalogSnapshot` structures in `include/tinydb/catalog/catalog_types.hpp`.

### Concepts & Daily Learning
- **Concept:** System catalog / data dictionary metadata separating logical object names (`"users"`) from physical storage anchors (`first_page_id`, `root_page_id`).
- **Why Database Engines Need It:** Without a persistent catalog, reopening `.tinydb` tells `DiskManager` how many pages exist—not which tables exist, what columns they have, or which `PageId` starts each `TableHeap` or `BPlusTree`!
- **How It Works:** `TableMetadata` binds `TableId` + normalized `name` + `Schema` + `first_page_id`. `IndexMetadata` binds `IndexId` + `name` + `table_id` + `column_index` + `root_page_id`.
- **Design Trade-offs:** Using a dedicated `.cat` metadata file outside `.tinydb` transactions avoids chicken-and-egg self-referential catalog table bootstrapping in v1 (see `DD-004`).
- **Common Mistakes:** Reusing deleted `TableId`s or `IndexId`s across drops/creates instead of strictly monotonic IDs (`next_table_id_++`), risking stale ID confusion.
- **Understanding Checkpoint:** Why should `TableId` and `IndexId` be monotonically increasing (`1, 2, 3...`) and never recycled after `DROP TABLE`?

### Component Context
- **Why It Exists:** Data structures representing TinyDB's schema directory.
- **Module Relationships:** Owned by `CatalogManager`; queried by `Planner`, `ExecutionEngine`, and `.tables`/`.schema` CLI commands.

### Files Expected to Change
- `include/tinydb/catalog/catalog_types.hpp` (new)
- `tests/unit/catalog/catalog_types_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `TableMetadata` and `IndexMetadata` in `include/tinydb/catalog/catalog_types.hpp` with clean equality and validation helpers.
2. Add unit tests verifying struct invariants and `IndexMetadata` column binding checks.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Types compile cleanly under `-Werror` and validate cleanly.

### Definition of Done
Metadata structures defined and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-076: define TableMetadata and IndexMetadata structures`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-077` (Catalog binary file format design).
````

---

## DAY-077 — Define the Versioned Catalog Binary File Format and Integrity Rules

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-077
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Define the Versioned Catalog Binary File Format and Integrity Rules
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-076 complete.

### Objective
Specify the explicit binary layout of `.cat` catalog files (`include/tinydb/catalog/catalog_format.hpp` and `docs/storage-format.md`): magic `"TINYCAT\1"`, `catalog_version` (`1`), `next_table_id`, `next_index_id`, `table_count`, `index_count`, serialized table/column/index entries, and trailing whole-file CRC32 checksum.

### Concepts & Daily Learning
- **Concept:** Versioned binary metadata envelopes and trailing CRC32 checksums over variable-length files.
- **Why Database Engines Need It:** Because catalog metadata grows/shrinks with DDL (`CREATE`/`DROP`), the catalog file has variable length; placing a CRC32 checksum over the entire payload detects truncation or bit-rot immediately on startup!
- **How It Works:** Header (`magic[8]`, `u32 version`, `u32 next_table_id`, `u32 next_index_id`, `u32 table_count`, `u32 index_count`), followed by `table_count` table entries (each with columns) and `index_count` index entries, capped with a 4-byte little-endian `crc32` over all preceding bytes `[0..end-4)`.
- **Design Trade-offs:** Binary encoding with explicit `BinaryWriter`/`BinaryReader` avoids pulling an external JSON/YAML parser dependency into TinyDB while keeping strict bounds safety.
- **Common Mistakes:** Placing CRC32 only over the header (missing corruption inside table/column entries!) or omitting `catalog_version`.
- **Understanding Checkpoint:** Why is storing a whole-file trailing CRC32 at `[end-4..end)` ideal for a variable-length catalog file written sequentially?

### Component Context
- **Why It Exists:** Defines the exact persistent wire format for `.cat` files.
- **Module Relationships:** Implemented by `CatalogCodec` (`DAY-082`) and verified on startup (`DAY-084`).

### Files Expected to Change
- `include/tinydb/catalog/catalog_format.hpp` (new)
- `docs/storage-format.md` (add Section 5 Catalog File Format)
- `tests/unit/catalog/catalog_types_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define catalog magic (`{'T','I','N','Y','C','A','T','\1'}`), `kCatalogFormatVersion = 1`, max limits (`kMaxTables = 1024`, `kMaxIndexes = 4096`), and header constants.
2. Document Section 5 in `docs/storage-format.md`.

### Testing Instructions
- Build and run `ctest --output-on-failure`.

### Acceptance Criteria
- Catalog format constants and documentation are consistent and compile with zero warnings.

### Definition of Done
Catalog format specified and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-077: define versioned catalog binary file format`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-078` (`CatalogManager` in-memory core).
````

---

## DAY-078 — Implement `CatalogManager` In-Memory Registry and Monotonic ID Allocation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-078
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement `CatalogManager` In-Memory Registry and Monotonic ID Allocation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-077 complete.

### Objective
Create `tinydb::CatalogManager` (`include/tinydb/catalog/catalog_manager.hpp`, `src/catalog/catalog_manager.cpp`) managing in-memory maps (`tables_by_id_`, `table_ids_by_name_`, `indexes_by_id_`, `index_ids_by_name_`), monotonic `next_table_id_` / `next_index_id_`, path association, and thread-safe `std::mutex` protection.

### Concepts & Daily Learning
- **Concept:** Bidirectional secondary lookup indexes in memory (`ID -> Metadata` and `normalized_name -> ID`) kept synchronized as a single invariant.
- **Why Database Engines Need It:** SQL queries look up tables by `"Users"` / `"users"`, while internal metadata (`IndexMetadata::table_id`) links objects by immutable `TableId`; both maps must never go out of sync!
- **How It Works:** Store primary ownership in `tables_by_id_` (`unordered_map<TableId, TableEntry>`), secondary lookup in `table_ids_by_name_` (`unordered_map<string, TableId>`), and keep names normalized lowercase.
- **Design Trade-offs:** Keeping `TableMetadata` and live `std::unique_ptr<TableHeap>` associated cleanly inside `CatalogManager` lets callers obtain both schema and table heap from one authoritative place!
- **Common Mistakes:** Deleting from `tables_by_id_` without erasing from `table_ids_by_name_`, or storing raw pointers into `unordered_map` buckets that invalidate on rehash!
- **Understanding Checkpoint:** Why is storing `TableId` in `table_ids_by_name_` (and `TableMetadata` in `tables_by_id_`) safer than storing `TableMetadata*` pointers across two `unordered_map`s?

### Component Context
- **Why It Exists:** Central metadata and table/index handle registry for TinyDB.
- **Module Relationships:** Owns `TableHeap` instances; injects `IPageProvider&` / `DiskManager&`.

### Files Expected to Change
- `include/tinydb/catalog/catalog_manager.hpp` (new)
- `src/catalog/catalog_manager.cpp` (new)
- `tests/unit/catalog/catalog_manager_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `CatalogManager` API (`InitNew`, `Load`, `CreateTable`, `DropTable`, `GetTable`, `GetSchema`, `ListTables`, `CreateIndex`, `DropIndex`, `GetIndex`, `GetTableIndexes`, `UpdateIndexRootPage`).
2. Implement constructor, `InitNew`, and empty-catalog state tests.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Freshly initialized `CatalogManager` starts with `next_table_id = 1`, `next_index_id = 1`, and `0` tables/indexes.

### Definition of Done
Skeleton and registry tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-078: implement CatalogManager in-memory registry and ID counters`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-079` (`CreateTable`).
````

---

## DAY-079 — Implement `CatalogManager::CreateTable` (Validation, `TableHeap` Allocation, Persistence Hook)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-079
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement `CatalogManager::CreateTable` (Validation, `TableHeap` Allocation, Persistence Hook)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-078 complete.

### Objective
Implement `Result<TableId> CatalogManager::CreateTable(std::string_view name, Schema schema)` validating table identifier, rejecting duplicate table names (case-insensitive), allocating and initializing a new `TableHeap` via `IPageProvider`, and inserting synchronized metadata entries.

### Concepts & Daily Learning
- **Concept:** Coordinating logical catalog registration with physical `TableHeap` page allocation, and rollback on partial failure.
- **Why Database Engines Need It:** If `CreateTable` allocates `first_page_id = 3` in `DiskManager` and then fails validation (or persistence), leaving Page 3 orphaned leaks disk pages forever!
- **How It Works:** Validate table name (`IsValidIdentifier`) and check `!table_ids_by_name_.contains(norm_name)` BEFORE touching `IPageProvider`. Allocate `TableHeap::Create(provider_, schema)`, insert into maps, increment `next_table_id_` (and call `PersistLocked()` once implemented on `DAY-083`, rolling back map + `heap->Drop()` if persist fails!).
- **Design Trade-offs:** Validating everything cheap in memory before allocating disk pages minimizes error recovery complexity.
- **Common Mistakes:** Allowing `"Users"` and `"users"` as two separate tables or leaking `first_page_id` on duplicate-name error.
- **Understanding Checkpoint:** Why must duplicate table-name checking happen BEFORE `TableHeap::Create` allocates Page 1?

### Component Context
- **Why It Exists:** Implements DDL `CREATE TABLE` at the engine/catalog layer.
- **Module Relationships:** Called by `ExecutionEngine` / `Planner` (Phase 07).

### Files Expected to Change
- `include/tinydb/catalog/catalog_manager.hpp`
- `src/catalog/catalog_manager.cpp`
- `tests/unit/catalog/catalog_manager_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CatalogManager::CreateTable`.
2. Add tests verifying: creating multiple tables yields monotonic IDs (`1, 2, 3`), duplicate name (`"users"` vs `"USERS"`) returns `kAlreadyExists` without incrementing `PageCount()`, and invalid name (`"123bad"`) returns `kInvalidArgument`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Duplicate/invalid names fail cleanly with zero orphaned disk pages.

### Definition of Done
`CreateTable` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-079: implement CatalogManager::CreateTable with validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-080` (`DropTable`, `GetTable`, `GetSchema`, `ListTables`).
````

---

## DAY-080 — Implement `CatalogManager::DropTable`, `GetTable`, `GetSchema`, and `ListTables`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-080
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement `CatalogManager::DropTable`, `GetTable`, `GetSchema`, and `ListTables`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-079 complete.

### Objective
Implement `DropTable(name)` (reclaiming all `TableHeap` pages AND cascading deletion of all attached index metadata/pages!), `GetTable(name)`, `GetTableById(id)`, `GetSchema(name)`, `GetTableHeap(name)`, and deterministic sorted `ListTables()`.

### Concepts & Daily Learning
- **Concept:** Cascading catalog cleanup on `DROP TABLE` and deterministic ordering of metadata listings.
- **Why Database Engines Need It:** Dropping a table while leaving its `IndexMetadata` entries orphaned in the catalog creates dangling `table_id` references that crash `.schema` or `LoadCatalog()`!
- **How It Works:** `DropTable`: look up normalized name (`kNotFound` if absent), remove any indexes referencing `table_id` from `indexes_by_id_` and `index_ids_by_name_` (deallocating index pages once B+ Tree drop hook exists), drop `TableHeap` pages via `heap->Drop()`, and erase from both table maps. `ListTables` sorts alphabetically (or by `TableId`) for deterministic CLI `.tables` output!
- **Design Trade-offs:** Returning `TableHeap*` (non-owning pointer valid while `CatalogManager` holds no concurrent `DropTable`) or `Result<std::shared_ptr<TableHeap>>`? Using `std::shared_ptr<TableHeap>` or documented v1 single-active caller contract keeps lifetime safe even if a `DropTable` runs cleanly!
- **Common Mistakes:** Iterating `indexes_by_id_` while erasing entries inside a range-based `for` loop (classic iterator-invalidation UB!).
- **Understanding Checkpoint:** Why is erasing from `std::unordered_map` inside a `for (auto& [k, v] : map)` loop Undefined Behavior, and how do you avoid it cleanly?

### Component Context
- **Why It Exists:** Completes table DDL lifecycle and schema introspection APIs.
- **Module Relationships:** Powers `.tables`, `.schema`, `DROP TABLE`, and all DML executors.

### Files Expected to Change
- `include/tinydb/catalog/catalog_manager.hpp`
- `src/catalog/catalog_manager.cpp`
- `tests/unit/catalog/catalog_manager_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DropTable`, `GetTable`, `GetTableById`, `GetSchema`, `GetTableHeap`, and `ListTables`.
2. Add tests verifying: dropping table reclaims pages (`FreePageCount()` increases!), `GetTable` returns `kNotFound` afterward, recreating same table name gets a NEW `TableId`, and `ListTables` order is deterministic.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Dropping a table cleanly removes metadata + reclaims heap pages; recreating gets monotonic `TableId`.

### Definition of Done
Lookup and drop APIs tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-080: implement CatalogManager DropTable, GetTable, GetSchema, and ListTables`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-081` (`CreateIndex`, `DropIndex`, `GetTableIndexes`).
````

---

## DAY-081 — Implement `CatalogManager::CreateIndex`, `DropIndex`, and `GetTableIndexes`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-081
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement `CatalogManager::CreateIndex`, `DropIndex`, and `GetTableIndexes`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-080 complete.

### Objective
Implement `CreateIndex(index_name, table_name, column_name, root_page_id)`, `DropIndex(index_name)`, `GetIndex(index_name)`, `GetTableIndexes(table_id)`, and `UpdateIndexRootPage(index_id, new_root_page_id)` validating v1 single-column integer index rules (`INT` or `BIGINT` only!).

### Concepts & Daily Learning
- **Concept:** Index-to-table-column binding, type constraint enforcement (`INT`/`BIGINT` for v1), and persistent `root_page_id` tracking across B+ Tree root splits/shrinks!
- **Why Database Engines Need It:** When a B+ Tree's root splits (height grows) or shrinks (height drops), `root_page_id` changes! `CatalogManager` must record the updated `root_page_id` persistently so reopening the database finds the real root!
- **How It Works:** Verify valid `index_name`, uniqueness among indexes, `table_name` exists, `column_name` exists in table schema, `col.Type() == kInt || col.Type() == kBigInt`, store `IndexMetadata`, and allow `UpdateIndexRootPage`. Also verify `DropTable` removes all associated indexes!
- **Design Trade-offs:** Enforcing single-column `INT`/`BIGINT` inside `CreateIndex` keeps catalog & B+ Tree constraints identical everywhere.
- **Common Mistakes:** Allowing `CREATE INDEX` on a `VARCHAR` or `BOOL` column in v1 without rejecting it clearly; forgetting to verify `DropTable` cascades to attached indexes.
- **Understanding Checkpoint:** Why must `CatalogManager` support `UpdateIndexRootPage(index_id, new_root)` instead of treating `root_page_id` as immutable after `CREATE INDEX`?

### Component Context
- **Why It Exists:** Tracks secondary B+ Tree indexes attached to tables.
- **Module Relationships:** Used by Phase 05 (`BPlusTree`) and Phase 07 (`Planner`, `Insert`/`Update`/`DeleteExecutor`).

### Files Expected to Change
- `include/tinydb/catalog/catalog_manager.hpp`
- `src/catalog/catalog_manager.cpp`
- `tests/unit/catalog/catalog_index_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CreateIndex`, `DropIndex`, `GetIndex`, `GetTableIndexes`, `UpdateIndexRootPage`, and cascade removal in `DropTable`.
2. Add tests verifying valid `INT`/`BIGINT` indexes, duplicate index name rejection, nonexistent table/column rejection, `VARCHAR`/`BOOL` index rejection (`kUnsupported`), root update, and `DropTable` cascade cleanup!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Index creation enforces all constraints; `DropTable` leaves zero orphaned indexes.

### Definition of Done
Index catalog APIs tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-081: implement CatalogManager index metadata APIs and cascade drop`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-082` (Catalog serialization & deserialization).
````

---

## DAY-082 — Implement Explicit Catalog Serialization and Deserialization (`CatalogCodec`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-082
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement Explicit Catalog Serialization and Deserialization (`CatalogCodec`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-077, DAY-081 complete.

### Objective
Implement `CatalogCodec::Serialize` and `CatalogCodec::Deserialize` (`include/tinydb/catalog/catalog_codec.hpp`, `src/catalog/catalog_codec.cpp`) encoding/decoding `CatalogSnapshot` deterministically (sorted by ID!) with whole-payload CRC32 verification and strict bounds checks.

### Concepts & Daily Learning
- **Concept:** Canonical deterministic serialization of unordered maps (`sort by ID`), nested schema reconstruction, and relational cross-reference validation on decode (`index.table_id` must exist!).
- **Why Database Engines Need It:** Serializing `std::unordered_map` directly depends on hash bucket iteration order (non-deterministic!). Sorting tables and indexes by ID makes `.cat` bytes deterministic and easy to inspect/diff.
- **How It Works:** Encode header + sorted tables (calling `Column::Create` + `Schema::Create` during decode!) + sorted indexes + CRC32 over `[0..end-4)`. On decode, verify size (`>= header + 4`), magic, CRC32 FIRST, version, bounds (`table_count <= kMaxTables`, `index_count <= kMaxIndexes`), duplicate IDs/names, valid `first_page_id`, and `index.table_id` cross-references, and `reader.Remaining() == 0`!
- **Design Trade-offs:** Reusing `Column::Create` and `Schema::Create` during `Deserialize` re-enforces all Phase 02 invariants automatically on untrusted disk metadata.
- **Common Mistakes:** Checking CRC32 AFTER allocating vectors based on an untrusted `table_count = 0xFFFFFFFF`! (Always verify `size >= min`, magic, AND CRC32 BEFORE reading counts!)
- **Understanding Checkpoint:** Why must `CatalogCodec::Deserialize` verify the file-wide CRC32 checksum *before* reading `table_count` and reserving vector memory?

### Component Context
- **Why It Exists:** Converts catalog snapshots cleanly between RAM and `.cat` bytes.
- **Module Relationships:** Used by `CatalogManager` atomic persistence (`DAY-083`) and startup loader (`DAY-084`).

### Files Expected to Change
- `include/tinydb/catalog/catalog_codec.hpp` (new)
- `src/catalog/catalog_codec.cpp` (new)
- `tests/unit/catalog/catalog_codec_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CatalogCodec::Serialize` and `CatalogCodec::Deserialize`.
2. Add tests: empty catalog roundtrip, multi-table + multi-index roundtrip, deterministic byte identity independent of insertion order, truncated file at random byte offsets, bit-flip CRC failure, and cross-reference violation (`index` pointing to missing `table_id`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Roundtrip preserves all IDs, schemas, `first_page_id`s, and `root_page_id`s; every corrupt/truncated/inconsistent payload fails cleanly.

### Definition of Done
`CatalogCodec` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-082: implement deterministic CatalogCodec serialization and CRC32`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-083` (Atomic catalog persistence).
````

---

## DAY-083 — Implement Atomic Catalog Persistence (Temp File + `fsync` + Atomic `rename`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-083
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement Atomic Catalog Persistence (Temp File + `fsync` + Atomic `rename`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-011 (`os_io`), DAY-082 complete.

### Objective
Implement atomic `.cat` file writing (`PersistCatalogAtomic` using `<path>.tmp`, write all bytes, `Sync()`, atomic `std::filesystem::rename(tmp, final)`, and `SyncParentDirectory(final)`) and hook it into `CatalogManager` (`InitNew`, `CreateTable`, `DropTable`, `CreateIndex`, `DropIndex`, `UpdateIndexRootPage`).

### Concepts & Daily Learning
- **Concept:** POSIX atomic replace pattern (`write temp -> fsync temp -> rename -> fsync dir`) preventing half-written metadata on crash.
- **Why Database Engines Need It:** Truncating and overwriting `tinydb.cat` in place means a crash midway leaves a 0-byte or half-written `.cat` file—losing ALL table schemas in the database! POSIX `rename()` on the same filesystem atomically swaps directory pointers so readers see either the complete old file OR the complete new file.
- **How It Works:** Write serialized bytes to `catalog_path + ".tmp"` (same directory -> same filesystem!), call `tmp.Sync()` and `tmp.Close()`, call `std::filesystem::rename(tmp_path, catalog_path)`, call `SyncParentDirectory(catalog_path)`. Clean up `.tmp` if any pre-rename write fails!
- **Design Trade-offs:** Rewriting the entire catalog file per DDL command is slow if done 10,000 times/sec, which is completely fine since DDL (`CREATE`/`DROP`) happens rarely outside transactions in v1!
- **Common Mistakes:** Placing the temp file in `/tmp` (different mount/filesystem causes `EXDEV` cross-device rename failure or non-atomic copy!); forgetting `fsync(tmp)` BEFORE `rename()` (leaving an empty zeroed file after crash!).
- **Understanding Checkpoint:** Why MUST the temporary catalog file live in the *same directory* as the target `.cat` file, and why MUST `fsync(tmp_fd)` run *before* `rename()`?

### Component Context
- **Why It Exists:** Guarantees crash-safe atomic DDL catalog updates on disk.
- **Module Relationships:** Invoked on every `CatalogManager` DDL mutation.

### Files Expected to Change
- `include/tinydb/catalog/catalog_manager.hpp`
- `src/catalog/catalog_manager.cpp`
- `tests/unit/catalog/catalog_persistence_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `WriteCatalogFileAtomic(path, bytes)` helper and wire `PersistLocked()` into all `CatalogManager` DDL methods.
2. Add tests verifying: no `.tmp` leftover after success, old `.cat` preserved if a leftover stale `.tmp` exists on startup, and every `CreateTable`/`DropTable`/`CreateIndex` updates `.cat` durably on disk.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero `.tmp` leftovers after persistence; atomic overwrite leaves valid CRC32 `.cat` on disk after every DDL operation.

### Definition of Done
Atomic catalog persistence implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-083: implement atomic temp-fsync-rename catalog persistence`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-084` (Startup catalog loading & corrupt-catalog rejection).
````

---

## DAY-084 — Implement Startup Catalog Loading (`Load`), Version Validation, and Corruption Rejection

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-084
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Implement Startup Catalog Loading (`Load`), Version Validation, and Corruption Rejection
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-083 complete.

### Objective
Implement `Status CatalogManager::Load(const std::filesystem::path& catalog_path)` reading the `.cat` file cleanly (with bounded size check), deserializing `CatalogSnapshot`, reopening every table's `TableHeap` (`TableHeap::Open(provider_, schema, first_page_id)`) to verify page integrity, restoring `next_table_id_` / `next_index_id_`, and writing thorough corruption/version-mismatch tests.

### Concepts & Daily Learning
- **Concept:** Cross-file consistency validation on startup (`tinydb.cat` <-> `.tinydb` pages).
- **Why Database Engines Need It:** If a user mixes an old `.cat` file with a different `.tinydb` file (or corrupts `.cat`), `Load` must catch missing/invalid `first_page_id`s and corrupt `.cat` bytes at startup rather than crashing later during `SELECT`!
- **How It Works:** Read `.cat` bytes (reject empty or `> 16 MiB` sanity cap), deserialize via `CatalogCodec::Deserialize`, validate each table via `TableHeap::Open(provider_, schema, meta.first_page_id)`, validate each index's `root_page_id` (`kInvalidPageId` or `IsDataPageId`), and populate maps atomically (rollback `CatalogManager` to empty if ANY table fails to open!).
- **Design Trade-offs:** Validating `TableHeap::Open` for all tables on startup adds O(tables) page-0 checks on open while guaranteeing every loaded table handle is immediately valid.
- **Common Mistakes:** Mutating `this->tables_by_id_` halfway through `Load` before verifying all tables, leaving `CatalogManager` half-loaded when table #3 is corrupt!
- **Understanding Checkpoint:** Why should `CatalogManager::Load` build temporary maps first and `std::swap` them into member variables only after ALL tables and indexes validate successfully?

### Component Context
- **Why It Exists:** Safe startup reconstruction of database schemas from disk.
- **Module Relationships:** Called immediately after `DiskManager::Open` during DB startup.

### Files Expected to Change
- `src/catalog/catalog_manager.cpp`
- `tests/unit/catalog/catalog_load_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CatalogManager::Load` with atomic staging (all-or-nothing load).
2. Test: valid reload restoring 3 tables + 2 indexes + monotonic IDs; corrupt magic; unsupported catalog version (`99`); bit-flipped `.cat`; missing `.cat`; `.cat` referencing out-of-range `first_page_id` in `.tinydb` (failed load leaves `CatalogManager` empty!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Failed `Load` never leaves partial tables inside `CatalogManager`; valid reload restores exact schemas, IDs, and `TableHeap`s.

### Definition of Done
Startup loader and corruption tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-084: implement atomic CatalogManager::Load and corruption checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-085` (Phase 03 integration & sign-off).
````

---

## DAY-085 — Complete Phase 03 DDL Persistence Integration Suite and Sign Off Phase 03

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-085
- **Phase:** Phase 03 — Catalog Management
- **Session Title:** Complete Phase 03 DDL Persistence Integration Suite and Sign Off Phase 03
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-076 through DAY-084 complete.

### Objective
Write `tests/integration/phase03_catalog_integration_test.cpp` testing full DB lifecycle (`DiskManager` + `CatalogManager` + `TableHeap` rows) across multiple cold restarts (`CreateTable` -> insert rows -> `CreateIndex` metadata -> restart -> query schemas & scan rows -> `DropTable` -> restart -> recreate), verify `docs/storage-format.md` Section 5, and sign off Phase 03 (`66.0 / 220.0` hours).

### Concepts & Daily Learning
- **Concept:** Cross-module persistence verification (`DiskManager` + `TableHeap` + `CatalogManager`).
- **Why Database Engines Need It:** Proves schema catalog and table data files stay 100% synchronized across DDL mutations and process restarts.
- **How It Works:** Destroy both `CatalogManager` and `DiskManager` between restart stages, reopen from path, verify rows inserted before restart are still scannable by table name after restart, verify `DropTable` persisted across restart, and check `VerifyFreeListIntegrity()`.
- **Design Trade-offs:** Checking full Debug + Release + ASan/UBSan confirms Phase 03 introduces zero memory/FD leaks.
- **Common Mistakes:** Leaving stale `.cat` or `.tinydb` test files in workspace rather than `temp_directory_path()`.
- **Understanding Checkpoint:** Explain why DDL (`CREATE`/`DROP TABLE`) is kept outside SQL transactions in TinyDB v1 (`DD-004`) and how atomic catalog rename still prevents torn catalog files.

### Component Context
- **Why It Exists:** Completes Phase 03 (`8.0` hours).
- **Module Relationships:** Hands off a persistent `CatalogManager` ready for `BufferPoolManager` integration in Phase 04.

### Files Expected to Change
- `tests/integration/phase03_catalog_integration_test.cpp` (new)
- `docs/storage-format.md` (sync Section 5)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `phase03_catalog_integration_test.cpp`.
2. Audit `docs/storage-format.md` Section 5 against `catalog_format.hpp` and `catalog_codec.cpp`.
3. Run Debug, Release, and ASan+UBSan suites + `clang-format` check.
4. Update `PROGRESS.md` (`66.0` completed hours, Phase 03 complete) and `NEXT_SESSION.md` -> `DAY-086`.

### Testing Instructions
- Full Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format` check.

### Acceptance Criteria
- All Phase 00–03 unit and integration tests pass cleanly across all build configs.

### Definition of Done
Phase 03 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-085: complete Phase 03 catalog integration tests and sign off`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 04 (`DAY-086` — Buffer Pool architecture & `PageFrame`).
````
