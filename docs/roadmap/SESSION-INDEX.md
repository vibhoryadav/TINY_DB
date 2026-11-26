# TinyDB Master Session Index (`DAY-001` – `DAY-284`)

Total Budget: **220.0 hours** across **284 working sessions** (256 × 45 min + 28 × 60 min).
Status Legend: `Pending` | `In Progress` | `Completed` | `Incomplete` | `Blocked`

## Phase Totals

| Phase | Area | Sessions | Hours | Completed |
|---|---|---:|---:|---:|
| 00 | Prerequisites and Architecture | 18 | 14.0 | 1 / 18 |
| 01 | Disk Manager and Page Format | 31 | 24.0 | 0 / 31 |
| 02 | Records and Table Heap | 26 | 20.0 | 0 / 26 |
| 03 | Catalog Management | 10 | 8.0 | 0 / 10 |
| 04 | Buffer Pool | 26 | 20.0 | 0 / 26 |
| 05 | B+ Tree Indexing | 41 | 32.0 | 0 / 41 |
| 06 | SQL Lexer and Parser | 26 | 20.0 | 0 / 26 |
| 07 | Query Engine and CLI | 26 | 20.0 | 0 / 26 |
| 08 | Transactions and WAL | 36 | 28.0 | 0 / 36 |
| 09 | Security and Testing | 31 | 24.0 | 0 / 31 |
| 10 | Documentation and Release | 13 | 10.0 | 0 / 13 |
| **Total** | | **284** | **220.0** | **1 / 284** |

## Complete Session Schedule

| Session | Phase | Title | Duration | Prerequisites | Prompt File | Status |
|---|---|---|---:|---|---|---|
| `DAY-001` | Phase 00 | Verify Toolchain, C++20 Compiler Warnings, Sanitizers, LLDB & `clang-format` Workflow | 45m | Repository bootstrap complete (`CMakeLists.txt`, minimal `tinydb` binary, GoogleTest smoke test). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Completed |
| `DAY-002` | Phase 00 | C++20 Memory Model: Stack vs Heap, Pointers, References, and `std::span` in Core Utilities | 45m | DAY-001 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-003` | Phase 00 | RAII & Ownership Semantics: Rule of Five, `std::unique_ptr`, and `std::shared_ptr` | 45m | DAY-002 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-004` | Phase 00 | Move Semantics & Value Categories: Implementing a Zero-Copy `ByteBuffer` Wrapper | 45m | DAY-003 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-005` | Phase 00 | Error Handling Architecture: Designing `tinydb::Status` and `tinydb::StatusCode` | 45m | DAY-004 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-006` | Phase 00 | C++20 Templates & Concepts: Implementing `tinydb::Result<T>` | 45m | DAY-005 complete (`Status`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-007` | Phase 00 | Fixed-Width Database Types (`PageId`, `FrameId`, `TxnId`, `Lsn`) and `config.hpp` | 45m | DAY-006 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-008` | Phase 00 | Endianness & Explicit Binary Serialization Helpers (`endian.hpp`) | 45m | DAY-002 (`std::span`), DAY-006 (`Result<T>`), DAY-007 (`types.hpp`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-009` | Phase 00 | STL Containers, Iterators & Complexity Analysis (Lookup Benchmark Lab) | 45m | DAY-008 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-010` | Phase 00 | Data Integrity Foundations: Implementing CRC32 Checksum Utility | 45m | DAY-002 (`std::span`), DAY-008 (`endian.hpp`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-011` | Phase 00 | OS File Systems & Syscalls: File Descriptors, `fsync`/`F_FULLFSYNC`, and Durability Semantics | 45m | DAY-005 (`Status`), DAY-010 (`crc32`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-012` | Phase 00 | Binary File I/O: Implementing RAII `FileHandle` with Explicit Read, Write, Seek, and Sync | 45m | DAY-003 (RAII), DAY-011 (`os_io`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-013` | Phase 00 | Virtual Memory, OS Page Cache & Why Databases Manage Fixed 4096-Byte Pages | 45m | DAY-007 (`config.hpp`), DAY-012 (`FileHandle`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-014` | Phase 00 | C++20 Concurrency Primitives (`std::mutex`, `std::shared_mutex`, `std::scoped_lock`) & Thread-Safety Contract | 45m | DAY-003 (RAII), DAY-013 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-015` | Phase 00 | Relational Model Fundamentals: Tables, Schemas, Primary Keys & Normalization Design Notes | 45m | DAY-007 (`types.hpp`), DAY-008 (`endian.hpp`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-016` | Phase 00 | Indexing & B+ Tree Theory: Balanced Trees, 4KB Fanout Calculations & Binary Search Helpers | 45m | DAY-009 (`algorithms.hpp`), DAY-015. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-017` | Phase 00 | Transactions, ACID, Locking & WAL Mental Model: End-to-End Lifecycle Simulation Test | 60m | DAY-011 (`fsync`), DAY-013 (Buffer Pool), DAY-014 (`latch.hpp`). | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-018` | Phase 00 | Core Architecture Blueprint Review, Core Module Integration Tests & Phase 00 Sign-Off | 60m | DAY-001 through DAY-017 complete. | [`PHASE-00.md`](daily-prompts/PHASE-00.md) | Pending |
| `DAY-019` | Phase 01 | Design the Database File Header and Define Its Serialized Format | 45m | Phase 00 complete (`DAY-018`). | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-020` | Phase 01 | Implement Database Magic-Byte Validation (`"TINYDB\0\1"`) | 45m | DAY-019 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-021` | Phase 01 | Implement Database Format-Version and Page-Size Header Validation | 45m | DAY-020 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-022` | Phase 01 | Implement Explicit FileHeader Serialization, Deserialization, and Header CRC32 | 45m | DAY-008 (`endian.hpp`), DAY-010 (`crc32.hpp`), DAY-021 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-023` | Phase 01 | Implement Malformed FileHeader and Corruption Rejection Tests | 45m | DAY-022 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-024` | Phase 01 | Define `PageType` Enum, `PageId` Invariants, and Page-Type Validation Helpers | 45m | DAY-007 (`types.hpp`), DAY-023 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-025` | Phase 01 | Define the Fixed 4096-Byte Page Layout and Common 32-Byte `PageHeader` Offsets | 45m | DAY-024 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-026` | Phase 01 | Implement Explicit `PageHeader` Serialization (`PageHeader::Serialize`) | 45m | DAY-008 (`endian.hpp`), DAY-025 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-027` | Phase 01 | Implement Explicit `PageHeader` Deserialization and Bounds Validation (`PageHeader::Deserialize`) | 45m | DAY-026 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-028` | Phase 01 | Implement the `Page` Container Class (4096-Byte Aligned Buffer & Payload Views) | 45m | DAY-013 (`AlignedPageBuffer`), DAY-027 (`PageHeader`). | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-029` | Phase 01 | Implement Page-Level CRC32 Checksum Computation and Verification | 45m | DAY-010 (`crc32`), DAY-028 (`Page`). | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-030` | Phase 01 | Implement Malformed `PageHeader` and Bit-Flip Corruption Rejection Tests | 45m | DAY-029 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-031` | Phase 01 | Design Free-Page Management (Intrusive Free-Page Linked List Specification) | 45m | DAY-030 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-032` | Phase 01 | Implement Free-Page Node Serialization and Header Metadata Tracking | 45m | DAY-031 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-033` | Phase 01 | Implement `DiskManager` Class Skeleton and File Lifecycle State Machine | 45m | DAY-012 (`FileHandle`), DAY-014 (`latch.hpp`), DAY-022 (`FileHeader`), DAY-032 (`FreePage`). | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-034` | Phase 01 | Implement `DiskManager::Create` (Initialized Page 0 FileHeader and Durable Sync) | 45m | DAY-033 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-035` | Phase 01 | Implement `DiskManager::Open` (FileHeader Validation and Metadata Recovery) | 45m | DAY-034 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-036` | Phase 01 | Implement `DiskManager::Close` with Flush and RAII Destructor Cleanup | 45m | DAY-035 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-037` | Phase 01 | Implement Overflow-Safe Page Offset Calculation and File Boundary Validation | 45m | DAY-036 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-038` | Phase 01 | Implement `DiskManager::ReadPage` with Bounds Checks, Short-Read Guard, and Header Verification | 45m | DAY-030 (`Page::Validate`), DAY-037 (`ComputePageOffset`). | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-039` | Phase 01 | Implement `DiskManager::WritePage` with Size Validation, Checksum Update, and Explicit Seeking | 45m | DAY-038 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-040` | Phase 01 | Implement `DiskManager::FlushPage` and `DiskManager::FlushAll` (`fsync` Durability) | 45m | DAY-039 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-041` | Phase 01 | Implement `DiskManager::AllocatePage` (Reusing Free-List Pages Before File Extension) | 45m | DAY-032 (`FreePage`), DAY-040 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-042` | Phase 01 | Implement `DiskManager::DeallocatePage` (Pushing Freed Pages Safely onto Free List) | 45m | DAY-041 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-043` | Phase 01 | Reject Double-Free, Page-0 Freeing, and Out-of-Range `PageId`s in `DiskManager` | 45m | DAY-042 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-044` | Phase 01 | Implement Truncated-File and Torn-Page Corruption Detection Tests | 45m | DAY-043 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-045` | Phase 01 | Implement FileHeader and Free-List Persistence Tests Across Close/Reopen Restarts | 45m | DAY-044 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-046` | Phase 01 | Implement Multi-Page Allocate, Write, Deallocate, and Reuse Persistence Stress Tests | 60m | DAY-045 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-047` | Phase 01 | Add `DiskManager` I/O Statistics Counters (Reads, Writes, Flushes, Allocations) | 45m | DAY-046 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-048` | Phase 01 | Run ASan/UBSan Audit and Boundary-Hardening Sweep on `Page` and `DiskManager` | 60m | DAY-047 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-049` | Phase 01 | Complete Phase 01 Integration Suite, Finalize `docs/storage-format.md`, and Sign Off Phase 01 | 60m | DAY-048 complete. | [`PHASE-01.md`](daily-prompts/PHASE-01.md) | Pending |
| `DAY-050` | Phase 02 | Define `DataType` Enum (`INT`, `BIGINT`, `BOOL`, `VARCHAR`) and Type Helpers | 45m | Phase 01 complete (`DAY-049`). | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-051` | Phase 02 | Implement `Value` Tagged Variant Representation and Constructors | 45m | DAY-050 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-052` | Phase 02 | Implement `Value` Type Validation, Bounds Checks, and Comparison Operators | 45m | DAY-051 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-053` | Phase 02 | Implement `Column` Metadata Class (Name, Type, Max Length, Validation) | 45m | DAY-015 (`identifier.hpp`), DAY-052 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-054` | Phase 02 | Implement `Schema` Class (Column Lookup, Duplicate Checks, Page-Fit Validation) | 45m | DAY-053 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-055` | Phase 02 | Implement `Tuple` Representation and Schema-Checked Construction | 45m | DAY-054 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-056` | Phase 02 | Implement Explicit `Tuple` Serialization Format | 45m | DAY-008 (`endian.hpp`), DAY-055 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-057` | Phase 02 | Implement Explicit `Tuple` Deserialization with Overrun and Truncation Validation | 45m | DAY-056 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-058` | Phase 02 | Implement `Tuple` Comparison, Projection Helpers, and Roundtrip Tests | 45m | DAY-057 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-059` | Phase 02 | Implement `RowID` (`PageId`, `SlotId`), Sentinels, Serialization, and Hashing | 45m | DAY-007 (`types.hpp`), DAY-008 (`endian.hpp`), DAY-058 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-060` | Phase 02 | Design the 4096-Byte Slotted Page Layout and Slot Directory Specification | 45m | DAY-025 (`PageHeader`), DAY-059 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-061` | Phase 02 | Implement `SlottedPage` Initialization, Header, and `SlotEntry` Serialization | 45m | DAY-060 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-062` | Phase 02 | Implement `SlottedPage` Free-Space Tracking and Overflow-Safe Space Checks | 45m | DAY-061 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-063` | Phase 02 | Implement `SlottedPage::InsertTuple` (Slot Reuse, Append, and Boundary Protection) | 45m | DAY-062 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-064` | Phase 02 | Implement `SlottedPage::GetTuple` (Bounds Check, Tombstone Rejection, Safe Copy) | 45m | DAY-063 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-065` | Phase 02 | Implement `SlottedPage::MarkDelete` and Physical Tombstone Semantics | 45m | DAY-064 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-066` | Phase 02 | Implement `SlottedPage::UpdateTuple` (In-Place Fit vs Relocated Update) | 45m | DAY-065 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-067` | Phase 02 | Implement `SlottedPage::Compact` (Defragmenting Dead Space While Preserving `SlotId`s) | 60m | DAY-066 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-068` | Phase 02 | Implement `SlottedPage` Randomized Fuzz and Edge-Case Tests | 45m | DAY-067 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-069` | Phase 02 | Implement `TablePage` Chaining (`prev_page_id`, `next_page_id`) via `PageHeader` | 45m | DAY-068 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-070` | Phase 02 | Design and Implement `TableHeap` Skeleton and First-Page Creation | 45m | DAY-041 (`DiskManager::AllocatePage`), DAY-069 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-071` | Phase 02 | Implement `TableHeap::InsertTuple` (Multi-Page Traversal and New Page Allocation) | 45m | DAY-056 (`TupleCodec`), DAY-070 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-072` | Phase 02 | Implement `TableHeap::GetTuple` and `TableHeap::DeleteTuple` | 45m | DAY-071 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-073` | Phase 02 | Implement `TableHeap::UpdateTuple` (In-Place vs Relocated Cross-Page Update) | 45m | DAY-072 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-074` | Phase 02 | Implement `TableIterator` and Multi-Page Sequential Scan | 45m | DAY-073 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-075` | Phase 02 | Complete Phase 02 Persistence, Compaction & ASan/UBSan Integration Suite and Sign Off | 60m | DAY-050 through DAY-074 complete. | [`PHASE-02.md`](daily-prompts/PHASE-02.md) | Pending |
| `DAY-076` | Phase 03 | Design `TableMetadata`, `ColumnMetadata`, and `IndexMetadata` Structures | 45m | Phase 02 complete (`DAY-075`). | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-077` | Phase 03 | Define the Versioned Catalog Binary File Format and Integrity Rules | 45m | DAY-076 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-078` | Phase 03 | Implement `CatalogManager` In-Memory Registry and Monotonic ID Allocation | 45m | DAY-077 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-079` | Phase 03 | Implement `CatalogManager::CreateTable` (Validation, `TableHeap` Allocation, Persistence Hook) | 45m | DAY-078 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-080` | Phase 03 | Implement `CatalogManager::DropTable`, `GetTable`, `GetSchema`, and `ListTables` | 45m | DAY-079 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-081` | Phase 03 | Implement `CatalogManager::CreateIndex`, `DropIndex`, and `GetTableIndexes` | 45m | DAY-080 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-082` | Phase 03 | Implement Explicit Catalog Serialization and Deserialization (`CatalogCodec`) | 45m | DAY-077, DAY-081 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-083` | Phase 03 | Implement Atomic Catalog Persistence (Temp File + `fsync` + Atomic `rename`) | 45m | DAY-011 (`os_io`), DAY-082 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-084` | Phase 03 | Implement Startup Catalog Loading (`Load`), Version Validation, and Corruption Rejection | 60m | DAY-083 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-085` | Phase 03 | Complete Phase 03 DDL Persistence Integration Suite and Sign Off Phase 03 | 60m | DAY-076 through DAY-084 complete. | [`PHASE-03.md`](daily-prompts/PHASE-03.md) | Pending |
| `DAY-086` | Phase 04 | Design `BufferPoolManager` Architecture and `PageFrame` Structure | 45m | Phase 03 complete (`DAY-085`). | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-087` | Phase 04 | Define `Replacer` Interface and `LRUReplacer` Data Structure Design | 45m | DAY-086 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-088` | Phase 04 | Implement `LRUReplacer` in O(1) Time (`std::list` + `std::unordered_map`) | 45m | DAY-087 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-089` | Phase 04 | Write Exhaustive `LRUReplacer` Unit Tests | 45m | DAY-088 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-090` | Phase 04 | Implement `BufferPoolManager` Skeleton (Frames, `page_table_`, `free_list_`, Metrics) | 45m | DAY-089 complete, Phase 01 `DiskManager`. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-091` | Phase 04 | Implement `BufferPoolManager::FetchPage` Cache-Hit Path | 45m | DAY-090 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-092` | Phase 04 | Implement `BufferPoolManager::FetchPage` Cache-Miss Path | 45m | DAY-091 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-093` | Phase 04 | Implement Dirty-Page Writeback During Victim Eviction | 45m | DAY-092 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-094` | Phase 04 | Implement `BufferPoolManager::UnpinPage` with Double-Unpin Guard and Sticky Dirty Tracking | 45m | DAY-093 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-095` | Phase 04 | Implement `BufferPoolManager::NewPage` (Allocating and Pinning Fresh Disk Pages) | 45m | DAY-094 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-096` | Phase 04 | Implement `BufferPoolManager::DeletePage` (Pin Protection, Eviction, and Disk Reclamation) | 45m | DAY-095 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-097` | Phase 04 | Implement `BufferPoolManager::FlushPage` and `BufferPoolManager::FlushAllPages` | 45m | DAY-096 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-098` | Phase 04 | Harden All-Frames-Pinned Exhaustion and Zero-Eviction Guarantees | 45m | DAY-097 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-099` | Phase 04 | Design RAII `PageGuard` (`ReadPageGuard` / `WritePageGuard`) Ownership Model | 45m | DAY-098 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-100` | Phase 04 | Implement `PageGuard`, `ReadPageGuard`, and `WritePageGuard` | 45m | DAY-099 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-101` | Phase 04 | Write `PageGuard` Move, Overwrite, `Drop()`, and Double-Unpin Prevention Tests | 45m | DAY-100 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-102` | Phase 04 | Implement `BufferPoolManager::Shutdown` and Clean Destructor Handling | 45m | DAY-101 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-103` | Phase 04 | Integrate `TableHeap` with `BufferPoolManager` via `BufferPoolPageProvider` | 60m | DAY-070 (`IPageProvider`), DAY-102 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-104` | Phase 04 | Test Multi-Page `TableHeap` Under Tiny Buffer Pool (3 Frames) Forcing Constant Eviction | 45m | DAY-103 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-105` | Phase 04 | Test Buffer Pool Invalid `PageId` and Corrupt-Read Error Propagation | 45m | DAY-104 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-106` | Phase 04 | Audit Concurrency Safety and Run Multi-Threaded `BufferPoolManager` Stress Tests | 45m | DAY-105 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-107` | Phase 04 | Implement Buffer Pool Hit/Miss/Eviction Metrics and Diagnostic Inspection | 45m | DAY-106 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-108` | Phase 04 | Add Regression Tests for Sticky Dirty Flags Across Multiple Pin/Unpin Cycles | 45m | DAY-107 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-109` | Phase 04 | Add Regression Tests for Clean vs Dirty Eviction and Restart Durability | 45m | DAY-108 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-110` | Phase 04 | Implement Randomized Buffer Pool Read-Modify-Evict-Delete Fuzz Test vs Oracle | 60m | DAY-109 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-111` | Phase 04 | Finalize `docs/buffer-pool.md` and Sign Off Phase 04 | 45m | DAY-086 through DAY-110 complete. | [`PHASE-04.md`](daily-prompts/PHASE-04.md) | Pending |
| `DAY-112` | Phase 05 | Finalize 4KB B+ Tree Node Capacity Math and Composite Key Policy | 45m | Phase 04 complete (`DAY-111`), `DAY-016` fanout notes. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-113` | Phase 05 | Design `BPlusTreePage` Common Node Header (`node_type`, `size`, `max_size`, `parent_page_id`) | 45m | DAY-112 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-114` | Phase 05 | Implement Explicit `BPlusTreeNodeHeader` Serialization and Deserialization | 45m | DAY-113 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-115` | Phase 05 | Design `BPlusTreeLeafPage` Layout and Binary Search (`KeyIndex` / `Lookup`) | 45m | DAY-009 (`algorithms.hpp`), DAY-114 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-116` | Phase 05 | Implement `BPlusTreeLeafPage` Explicit Entry Serialization, Validation, and Mutation | 45m | DAY-115 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-117` | Phase 05 | Implement `BPlusTreeLeafPage` Split (`InsertAndSplitTo`) and Merge/Borrow Helpers | 45m | DAY-116 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-118` | Phase 05 | Design `BPlusTreeInternalPage` Layout and Child Lookup (`Lookup`) | 45m | DAY-117 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-119` | Phase 05 | Implement `BPlusTreeInternalPage` `PopulateNewRoot` and Structural Validation | 45m | DAY-118 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-120` | Phase 05 | Implement `BPlusTreeInternalPage` Insert, Push-Up Split, and Merge/Borrow Helpers | 45m | DAY-119 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-121` | Phase 05 | Implement B+ Tree Root Metadata Persistence Integration (`root_page_id` Updates) | 45m | Phase 03 (`CatalogManager::UpdateIndexRootPage`), DAY-120 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-122` | Phase 05 | Implement `BPlusTree::FindLeafPage` Traversal Using `PageGuard`s | 45m | DAY-121 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-123` | Phase 05 | Implement `BPlusTree::GetValue` Point Search (Single and Duplicate Matches) | 45m | DAY-122 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-124` | Phase 05 | Implement `BPlusTree::Insert` Leaf-Only Insertion (No Split) | 45m | DAY-123 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-125` | Phase 05 | Implement First Leaf Split and New Internal Root Creation (`InsertIntoParent`) | 45m | DAY-124 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-126` | Phase 05 | Implement Recursive Internal Node Splitting (`SplitInternal`) and Arbitrary-Height Growth | 60m | DAY-125 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-127` | Phase 05 | Verify Duplicate-Key Splitting Across Multi-Level B+ Trees | 60m | DAY-126 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-128` | Phase 05 | Implement `BPlusTreeIterator` and Range Search (`Begin`, `Begin(key)`, `RangeScan`) | 45m | DAY-127 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-129` | Phase 05 | Implement B+ Tree Structural Validator Part 1 (Node Bounds, Sorted Keys, Child Pointers) | 45m | DAY-128 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-130` | Phase 05 | Implement B+ Tree Structural Validator Part 2 (Separators, Leaf Siblings, Equal Depth, Indexed RowIDs) | 45m | DAY-129 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-131` | Phase 05 | Run Step-by-Step Invariant Validation Across Multi-Level Insertions | 60m | DAY-130 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-132` | Phase 05 | Implement `BPlusTree::Remove` Leaf Deletion Without Underflow | 45m | DAY-131 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-133` | Phase 05 | Implement Leaf Deletion Underflow: Sibling Redistribution (Borrow Left / Right) | 45m | DAY-132 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-134` | Phase 05 | Implement Leaf Deletion Underflow: Leaf Merging (`CoalesceLeaf`) and Sibling Pointer Updates | 60m | DAY-133 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-135` | Phase 05 | Implement Internal Node Redistribution (Pull-Down / Push-Up Separator Rotation) | 45m | DAY-134 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-136` | Phase 05 | Implement Internal Node Merging and Cascading Underflow to Root | 60m | DAY-135 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-137` | Phase 05 | Implement Root Shrinking (`AdjustRoot`: Promoting Sole Child and Reclaiming Old Root) | 45m | DAY-136 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-138` | Phase 05 | Test Sequential, Reverse, and Random Deletions with Stepwise Validator Checks | 45m | DAY-137 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-139` | Phase 05 | Test Interleaved Mixed Insert/Delete/Split/Merge Workloads Against `std::multimap` Oracle | 45m | DAY-138 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-140` | Phase 05 | Audit Buffer Pool Pinning Across Every `BPlusTree` Path (Zero Pin Leaks) | 45m | DAY-139 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-141` | Phase 05 | Stress-Test Height >= 3 B+ Tree Under Tiny 5-Frame Buffer Pool | 45m | DAY-140 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-142` | Phase 05 | Verify B+ Tree Persistence and Reload Across Full Database Restarts | 45m | DAY-141 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-143` | Phase 05 | Test Corrupt B+ Tree Node Rejection (Invalid Counts, Cycles, Wrong Page Types) | 45m | DAY-142 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-144` | Phase 05 | Run Heavy Duplicate-Key Stress Tests (1,000 Identical Keys Across Splits and Deletes) | 45m | DAY-143 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-145` | Phase 05 | Integrate `BPlusTree` Lifecycle with `CatalogManager` and `TableHeap` | 45m | DAY-144 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-146` | Phase 05 | Verify Full 4KB Page Fanout Scaling and I/O Reduction vs Sequential Scan | 45m | DAY-145 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-147` | Phase 05 | Implement 10,000-Op Randomized B+ Tree Fuzz Test (`Insert`, `Remove`, `GetValue`, `RangeScan`) | 45m | DAY-146 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-148` | Phase 05 | Run ASan/UBSan Audit and Boundary Hardening Across `src/index/` | 45m | DAY-147 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-149` | Phase 05 | Refactor and Clean Up `src/index/` Helper Methods and Docstrings | 45m | DAY-148 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-150` | Phase 05 | Finalize `docs/btree-design.md` Architecture and On-Disk Format Documentation | 45m | DAY-149 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-151` | Phase 05 | Implement End-to-End Table + B+ Tree Backfill and Sync Integration Test | 45m | DAY-150 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-152` | Phase 05 | Complete Phase 05 Acceptance Verification and Sign Off | 45m | DAY-112 through DAY-151 complete. | [`PHASE-05.md`](daily-prompts/PHASE-05.md) | Pending |
| `DAY-153` | Phase 06 | Define TinyDB v1 SQL Formal Grammar (EBNF) and `SourceLocation` Types | 45m | Phase 05 complete (`DAY-152`). | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-154` | Phase 06 | Define `TokenType` Enum and `Token` Representation | 45m | DAY-153 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-155` | Phase 06 | Implement `Lexer` Skeleton, Cursor Navigation, and Line/Column Tracking | 45m | DAY-154 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-156` | Phase 06 | Implement SQL Keyword Recognition and Identifier Scanning | 45m | DAY-155 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-157` | Phase 06 | Implement Integer Literal Scanning and Overflow Validation | 45m | DAY-156 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-158` | Phase 06 | Implement String Literal Scanning, Escape Sequences, and Unterminated Errors | 45m | DAY-157 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-159` | Phase 06 | Implement Operators, Punctuation, Invalid-Token Diagnostics, and `TokenizeAll` | 45m | DAY-158 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-160` | Phase 06 | Write Comprehensive `Lexer` Unit Test Suite | 45m | DAY-159 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-161` | Phase 06 | Design AST Base Hierarchy (`AstNode`, `Expression`, `Statement`) and Ownership Rules | 45m | DAY-160 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-162` | Phase 06 | Implement Expression AST Nodes (`LiteralExpr`, `ColumnRefExpr`, `BinaryOpExpr`) | 45m | DAY-161 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-163` | Phase 06 | Implement DDL Statement AST Nodes (`CreateTableStmt`, `DropTableStmt`, `CreateIndexStmt`) | 45m | DAY-162 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-164` | Phase 06 | Implement DML Statement AST Nodes (`InsertStmt`, `SelectStmt`, `UpdateStmt`, `DeleteStmt`) | 45m | DAY-163 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-165` | Phase 06 | Implement Recursive-Descent `Parser` Skeleton and Token Stream Helpers | 45m | DAY-164 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-166` | Phase 06 | Implement Expression Parsing, Parentheses, Literals, and Operator Precedence | 45m | DAY-165 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-167` | Phase 06 | Implement `CREATE TABLE` Statement Parsing and Type Bounds Validation | 45m | DAY-166 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-168` | Phase 06 | Implement `DROP TABLE` and `CREATE INDEX` Statement Parsing | 45m | DAY-167 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-169` | Phase 06 | Implement `INSERT INTO` Statement Parsing (Single & Multi-Row `VALUES`, Optional Columns) | 45m | DAY-168 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-170` | Phase 06 | Implement `SELECT` Statement Parsing (`*` vs Column List, `FROM`, Optional `WHERE`) | 45m | DAY-169 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-171` | Phase 06 | Implement `UPDATE` Statement Parsing (`SET col = expr`, Optional `WHERE`) | 45m | DAY-170 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-172` | Phase 06 | Implement `DELETE` Statement Parsing (`DELETE FROM <table> [WHERE <expr>]`) | 45m | DAY-171 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-173` | Phase 06 | Implement Multi-Statement Parsing and Semicolon Delimiter Rules | 45m | DAY-172 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-174` | Phase 06 | Enforce Parser Resource Limits (Max Query Bytes, Tokens, Depth, Columns, Statements) | 45m | DAY-173 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-175` | Phase 06 | Add Explicit Rejection & Diagnostics for Unsupported v1 SQL Constructs | 45m | DAY-174 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-176` | Phase 06 | Write Comprehensive Valid SQL Parser Unit Test Suite | 60m | DAY-175 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-177` | Phase 06 | Implement Malformed SQL Boundary and Fuzz Testing Suite | 60m | DAY-176 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-178` | Phase 06 | Finalize `docs/sql-spec.md` and Sign Off Phase 06 | 45m | DAY-153 through DAY-177 complete. | [`PHASE-06.md`](daily-prompts/PHASE-06.md) | Pending |
| `DAY-179` | Phase 07 | Design Pull-Based Volcano Executor Interface and `ExecutionContext` | 45m | Phase 06 complete (`DAY-178`). | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-180` | Phase 07 | Implement `QueryResult` and `ResultSet` Structures | 45m | DAY-179 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-181` | Phase 07 | Implement SQL Expression Evaluation and Type-Checking Engine | 45m | DAY-052 (`Value::Compare`), DAY-180 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-182` | Phase 07 | Implement `SequentialScanExecutor` | 45m | DAY-181 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-183` | Phase 07 | Implement `FilterExecutor` | 45m | DAY-182 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-184` | Phase 07 | Implement `ProjectionExecutor` | 45m | DAY-058 (`Tuple::Project`), DAY-183 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-185` | Phase 07 | Implement `InsertExecutor` (Validation, `TableHeap` Insert, and Multi-Index Maintenance) | 60m | DAY-184 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-186` | Phase 07 | Implement `DeleteExecutor` (Safe Target Collection and Multi-Index Removal) | 45m | DAY-185 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-187` | Phase 07 | Implement `UpdateExecutor` (Type Validation, Halloween Protection, and Index Sync) | 45m | DAY-186 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-188` | Phase 07 | Implement `IndexScanExecutor` (Point and Range Lookups via `BPlusTree`) | 45m | DAY-187 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-189` | Phase 07 | Implement DDL Execution (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX` + Backfill) | 60m | DAY-188 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-190` | Phase 07 | Implement Rule-Based `Planner` (Index Selection vs Sequential-Scan Fallback) | 45m | DAY-189 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-191` | Phase 07 | Implement `ExecutionEngine` Orchestrator (`ExecuteSql` and Statement Pipeline) | 45m | DAY-190 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-192` | Phase 07 | Test Type Validation and Runtime Semantic Error Reporting | 45m | DAY-191 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-193` | Phase 07 | Expand Isolated Unit Tests Across All 7 Volcano Executors | 45m | DAY-192 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-194` | Phase 07 | Write End-to-End Query Engine Integration Tests (`IndexScan` == `SeqScan`) | 45m | DAY-193 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-195` | Phase 07 | Implement Multi-Statement SQL Batch Execution and Error Isolation | 45m | DAY-194 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-196` | Phase 07 | Implement Readable Tabular Output Formatter | 45m | DAY-195 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-197` | Phase 07 | Implement Interactive CLI REPL Skeleton and Multi-Line SQL Buffering | 45m | DAY-196 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-198` | Phase 07 | Implement CLI Meta-Commands: `.exit`, `.quit`, and `.help` | 45m | DAY-197 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-199` | Phase 07 | Implement CLI Introspection Commands: `.tables` and `.schema [table]` | 45m | DAY-198 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-200` | Phase 07 | Write Real CLI Subprocess Piped Stdin/Stdout Automated Integration Tests | 45m | DAY-199 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-201` | Phase 07 | Test End-to-End SQL Persistence Across Multi-Session Restarts | 45m | DAY-200 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-202` | Phase 07 | Test Query Engine Edge Cases (Empty Tables, 0-Row Mutations, Special Strings, Extremes) | 45m | DAY-201 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-203` | Phase 07 | Run ASan/UBSan Audit Across Query Engine and CLI | 45m | DAY-202 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-204` | Phase 07 | Add Example SQL Walkthrough and Sign Off Phase 07 | 45m | DAY-179 through DAY-203 complete. | [`PHASE-07.md`](daily-prompts/PHASE-07.md) | Pending |
| `DAY-205` | Phase 08 | Finalize No-Steal, Redo-Only Transaction & WAL Architecture Specification | 45m | Phase 07 complete (`DAY-204`) — storage engine and query engine verified stable. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-206` | Phase 08 | Implement `Transaction` State Model and `TransactionManager` (Single Active Transaction) | 45m | DAY-205 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-207` | Phase 08 | Extend SQL Lexer, AST, and Parser for `BEGIN`, `COMMIT`, and `ROLLBACK` | 45m | DAY-206 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-208` | Phase 08 | Implement Auto-Commit vs Explicit Transaction Semantics in `ExecutionEngine` | 45m | DAY-207 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-209` | Phase 08 | Implement Strict No-Steal Policy in `BufferPoolManager` | 45m | DAY-208 complete, Phase 04 `BufferPoolManager`. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-210` | Phase 08 | Implement Transaction Modified-Page Tracking and Before-Image Capture | 45m | DAY-209 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-211` | Phase 08 | Implement Runtime `ROLLBACK` Across `TableHeap`, `BPlusTree`, and `BufferPoolManager` | 60m | DAY-210 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-212` | Phase 08 | Design the WAL File Format and `LogRecord` Binary Layout | 45m | DAY-211 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-213` | Phase 08 | Implement `LogRecord` Explicit Serialization, Deserialization, and CRC32 Verification | 45m | DAY-212 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-214` | Phase 08 | Implement `LogManager` Skeleton and WAL File Lifecycle (`Create`, `Open`, `Close`) | 45m | DAY-213 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-215` | Phase 08 | Implement `LogManager::AppendLogRecord` and Durable `Flush()` | 45m | DAY-214 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-216` | Phase 08 | Implement WAL Sequential Reader (`LogReader`) with Torn-Tail vs Corruption Detection | 45m | DAY-215 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-217` | Phase 08 | Implement `PageHeader::lsn` Stamping on WAL Log Append | 45m | DAY-216 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-218` | Phase 08 | Handle Page Allocations & Deallocations in WAL (`kFreePage` / Allocation Ordering) | 45m | DAY-217 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-219` | Phase 08 | Coordinate Index `root_page_id` Atomic Commit with WAL Transactions | 45m | DAY-218 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-220` | Phase 08 | Implement Durable `COMMIT` Sequence (WAL Append + `fsync` + Release No-Steal Holds) | 60m | DAY-219 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-221` | Phase 08 | Integrate `LogManager` and `TransactionManager` Across `Database` Lifecycle | 45m | DAY-220 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-222` | Phase 08 | Enforce and Test DDL-Outside-Transaction Rules Under WAL | 45m | DAY-221 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-223` | Phase 08 | Implement `RecoveryManager` Skeleton and Analysis Phase (Identifying Committed Txns) | 45m | DAY-216 (`LogReader`), DAY-222 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-224` | Phase 08 | Implement `RecoveryManager` Redo Phase with Idempotent `PageHeader::lsn` Checks | 60m | DAY-223 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-225` | Phase 08 | Wire `RecoveryManager` into `Database::Open` Startup Sequence | 45m | DAY-224 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-226` | Phase 08 | Build Controlled Crash Simulation Harness (`fork` / Fault-Injection Hooks) | 45m | DAY-225 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-227` | Phase 08 | Controlled Crash Test 1: Crash Mid-Transaction Before `COMMIT` | 45m | DAY-226 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-228` | Phase 08 | Controlled Crash Test 2: Crash After `ROLLBACK` | 45m | DAY-227 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-229` | Phase 08 | Controlled Crash Test 3: Crash After `COMMIT` WAL Flush Before Any DB Page Flush | 60m | DAY-228 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-230` | Phase 08 | Controlled Crash Test 4: Crash During Partial DB Page Flushing | 45m | DAY-229 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-231` | Phase 08 | Controlled Crash Test 5: Crash During Recovery (Recovery Idempotence) | 45m | DAY-230 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-232` | Phase 08 | Verify Multi-Transaction Interleaved Sequences Under Crash Recovery | 45m | DAY-231 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-233` | Phase 08 | Verify B+ Tree Structural Invariants Across All Crash Recovery Scenarios | 45m | DAY-232 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-234` | Phase 08 | Test WAL Torn Tail Truncation vs Mid-Log Corruption Rejection During Recovery | 45m | DAY-233 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-235` | Phase 08 | Test Multi-Page Transactions and CLI `BEGIN` / `COMMIT` / `ROLLBACK` Integration | 45m | DAY-234 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-236` | Phase 08 | Run ASan/UBSan and Memory-Leak Audit Across `transaction/` and `recovery/` | 45m | DAY-235 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-237` | Phase 08 | Add Multi-Wave Commit/Rollback/Recovery Endurance Stress Test | 45m | DAY-236 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-238` | Phase 08 | Refactor and Polish Transaction & Recovery Code Clarity and Docstrings | 45m | DAY-237 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-239` | Phase 08 | Finalize `docs/transaction-design.md` with Exact WAL Layout & Crash Matrix | 45m | DAY-238 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-240` | Phase 08 | Complete Phase 08 Acceptance Verification and Sign Off | 45m | DAY-205 through DAY-239 complete. | [`PHASE-08.md`](daily-prompts/PHASE-08.md) | Pending |
| `DAY-241` | Phase 09 | Conduct Security Threat Model and Attack Surface Audit | 45m | Phase 08 complete (`DAY-240`). | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-242` | Phase 09 | Audit Integer Overflow and Bounds Checking Across All Storage, Index, and Recovery Layers | 45m | DAY-241 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-243` | Phase 09 | Expand Corrupt Database File Rejection Suite (Headers, Truncations, Size Mismatches) | 45m | DAY-242 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-244` | Phase 09 | Test Corrupt Slotted Pages, Overlapping Slots, and Crafted Tuple Payloads | 45m | DAY-243 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-245` | Phase 09 | Test Catalog Integrity, Orphaned Temp Files, and Corrupt `.cat` Rejection | 45m | DAY-244 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-246` | Phase 09 | Harden B+ Tree Runtime Pointer Validation Against Crafted Disk Pages | 45m | DAY-245 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-247` | Phase 09 | Test WAL Integrity Validation and Crafted Log-Record Attacks | 45m | DAY-246 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-248` | Phase 09 | Harden SQL Parser Resource Limits Against Pathological Inputs | 45m | DAY-247 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-249` | Phase 09 | Run 50,000-Input Structured + Random Parser Fuzz Campaign | 60m | DAY-248 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-250` | Phase 09 | Stress-Test Buffer Pool Ownership and Pin Accounting Under Mixed Faults | 45m | DAY-249 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-251` | Phase 09 | Run 50,000-Op Multi-Seed Randomized B+ Tree Stress and Invariant Suite | 60m | DAY-250 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-252` | Phase 09 | Build Differential SQL Testing Harness Against SQLite3 | 45m | DAY-251 complete (`sqlite3` verified installed on host!). | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-253` | Phase 09 | Run Differential SQL Tests Against SQLite3: DDL, `INSERT`, `SELECT`, and `WHERE` | 45m | DAY-252 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-254` | Phase 09 | Run Differential SQL Tests Against SQLite3: `UPDATE`, `DELETE`, `CREATE INDEX`, and Transactions | 45m | DAY-253 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-255` | Phase 09 | Implement Randomized Differential SQL Generator Testing TinyDB vs SQLite3 | 60m | DAY-254 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-256` | Phase 09 | Execute Complete Controlled Crash Testing Matrix (`SIGKILL` + Instrumentation) | 45m | DAY-255 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-257` | Phase 09 | Run Full-Suite AddressSanitizer and Leak Detection (`LSan`) Audit | 45m | DAY-256 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-258` | Phase 09 | Run Full-Suite UndefinedBehaviorSanitizer (`UBSan`) Audit | 45m | DAY-257 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-259` | Phase 09 | Run Large-Dataset Stress Test (50,000+ Rows, Multi-MB File, Buffer Thrashing) | 45m | DAY-258 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-260` | Phase 09 | Audit File Permissions, Path Safety, and Atomic Temp-File Hygiene | 45m | DAY-259 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-261` | Phase 09 | Test Resource Exhaustion and Disk Write Failure Recovery Paths | 45m | DAY-260 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-262` | Phase 09 | Organize Dedicated Regression Test Suite for Edge and Historical Bugs | 45m | DAY-261 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-263` | Phase 09 | Execute Strict Compiler Warning Sweep (`-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`) | 45m | DAY-262 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-264` | Phase 09 | Execute Repository-Wide `clang-format` Audit and Enforce CI Formatting Check | 45m | DAY-263 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-265` | Phase 09 | Perform Secret Scanning and Dependency Audit | 45m | DAY-264 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-266` | Phase 09 | Measure Baseline Performance and Memory Footprint (Profiling Sanity Pass) | 45m | DAY-265 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-267` | Phase 09 | Run Full Data & Boundary Edge-Case Verification Sweep | 45m | DAY-266 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-268` | Phase 09 | Stress-Test Recovery Ordering Across Interleaved DDL, DML, Index Splits, and Crashes | 45m | DAY-267 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-269` | Phase 09 | Build Full-Stack End-to-End Automated Acceptance Integration Suite | 45m | DAY-268 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-270` | Phase 09 | Complete Security & QA Checklist Verification in `docs/security-audit.md` | 45m | DAY-269 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-271` | Phase 09 | Complete Phase 09 Verification Matrix and Sign Off | 45m | DAY-241 through DAY-270 complete. | [`PHASE-09.md`](daily-prompts/PHASE-09.md) | Pending |
| `DAY-272` | Phase 10 | Finalize `docs/architecture.md` and High-Resolution System Diagrams | 45m | Phase 09 complete (`DAY-271`). | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-273` | Phase 10 | Audit and Finalize `docs/storage-format.md` and `docs/buffer-pool.md` | 45m | DAY-272 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-274` | Phase 10 | Audit and Finalize `docs/btree-design.md` and `docs/sql-spec.md` | 45m | DAY-273 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-275` | Phase 10 | Finalize `docs/transaction-design.md`, `docs/design-decisions.md`, and Known Limitations | 45m | DAY-274 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-276` | Phase 10 | Implement Reproducible Benchmark Suite (`benchmarks/`) | 60m | DAY-275 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-277` | Phase 10 | Execute Benchmarks in Release Mode and Document Methodology & Real Results | 45m | DAY-276 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-278` | Phase 10 | Create SQL Example Scripts and Interactive Demo Walkthrough | 45m | DAY-277 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-279` | Phase 10 | Overhaul Root `README.md` (Install, Build, Usage, Architecture, Benchmarks, Limitations) | 45m | DAY-278 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-280` | Phase 10 | Polish GitHub Actions CI Workflow (Matrix, Sanitizers, Benchmarks Compile, Format & Hygiene) | 45m | DAY-279 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-281` | Phase 10 | Bump Version to `1.0.0` and Execute Clean Release Readiness Audit | 45m | DAY-280 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-282` | Phase 10 | Prepare `CHANGELOG.md` and `v1.0.0` Release Notes | 45m | DAY-281 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-283` | Phase 10 | Merge `develop` into `main` and Verify Stable Release Branch | 45m | DAY-282 complete. | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
| `DAY-284` | Phase 10 | Tag `v1.0.0`, Push Release Tag, and Complete Final 220-Hour Sign-Off | 45m | DAY-283 complete (`main` merged & verified). | [`PHASE-10.md`](daily-prompts/PHASE-10.md) | Pending |
