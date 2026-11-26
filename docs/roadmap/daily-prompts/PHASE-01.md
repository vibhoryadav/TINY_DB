# Phase 01 — Disk Manager and Page Format (`DAY-019` – `DAY-049`)

**Phase Budget:** 24.0 hours (28 × 45 min + 3 × 60 min = 31 sessions)
**Goal:** Build TinyDB's portable 4096-byte page storage foundation and `DiskManager` with explicit field serialization, CRC32 corruption detection, free-page list management, and verified persistence tests.

---

## DAY-019 — Design the Database File Header and Define Its Serialized Format

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-019
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Design the Database File Header and Define Its Serialized Format
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 00 complete (`DAY-018`).

### Objective
Define the exact binary layout and in-memory `FileHeader` struct (`include/tinydb/storage/file_header.hpp`) for Page 0 of every `.tinydb` file, documenting every byte offset, field width, magic bytes, format version, page size, page counts, free-list head, and header CRC32.

### Concepts & Daily Learning
- **Concept:** Self-describing binary file headers (Page 0 superblock) and versioned schemas.
- **Why Database Engines Need It:** A database opening an arbitrary path must immediately verify that the file is truly a TinyDB database of a compatible format version and page size—not a PNG, text file, or v2 format—before interpreting any page data.
- **How It Works:** Page 0 (`4096` bytes) reserves fixed byte offsets `[0..31]` (or `[0..35]`) for `magic[8]`, `format_version` (`u32`), `page_size` (`u32`), `page_count` (`u32`), `first_free_page_id` (`u32`), `free_page_count` (`u32`), `reserved` (`u32`), and `header_crc32` (`u32`), zero-padding the rest of Page 0 cleanly.
- **Design Trade-offs:** Dedicating all of Page 0 to the file header costs 4KB per database file while aligning all subsequent data pages (`page_id >= 1`) cleanly on 4KB disk boundaries (`offset = page_id * 4096`).
- **Common Mistakes:** Defining an unaligned C++ struct and `sizeof(FileHeader)`-dumping it to disk; storing `page_count` without checking overflow when multiplying by `page_size`.
- **Understanding Checkpoint:** Why is dedicating a full 4096-byte Page 0 to the file header cleaner than a 32-byte prefix before Page 1?

### Component Context
- **Why It Exists:** Acts as the root metadata anchor of every `.tinydb` database file.
- **Module Relationships:** Owned/persisted by `DiskManager`; validated on every `DiskManager::Open`.

### Files Expected to Change
- `include/tinydb/storage/file_header.hpp` (new)
- `docs/storage-format.md` (pin down exact Page 0 offsets & sizes)
- `tests/unit/storage/file_header_layout_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Create `include/tinydb/storage/file_header.hpp` defining `FileHeader` struct fields, magic byte array (`{'T','I','N','Y','D','B','\0','\1'}`), and exact `constexpr` offset constants (`kMagicOffset = 0`, `kFormatVersionOffset = 8`, ..., `kSerializedSize = 36`).
2. Update `docs/storage-format.md` Section 2 with the exact offset table.
3. Add static & unit tests validating `kSerializedSize <= config::kPageSize` and non-overlapping consecutive field offsets.

### Testing Instructions
- Build and run `ctest --output-on-failure`.

### Acceptance Criteria
- All Page 0 header offsets are contiguous, verified via tests, and documented in `docs/storage-format.md`.

### Definition of Done
Header layout defined, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-019: define Page 0 database FileHeader layout and offsets`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-020` (Magic-byte validation).
````

---

## DAY-020 — Implement Database Magic-Byte Validation (`"TINYDB\0\1"`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-020
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Database Magic-Byte Validation (`"TINYDB\0\1"`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-019 complete.

### Objective
Implement explicit magic-byte generation and constant-length validation helpers for `FileHeader` (`src/storage/file_header.cpp`), ensuring non-TinyDB files, empty files, and mismatched signatures fail immediately with `StatusCode::kCorruption`.

### Concepts & Daily Learning
- **Concept:** File format magic signatures (like SQLite's `"SQLite format 3\0"`).
- **Why Database Engines Need It:** Prevents accidentally overwriting or mis-parsing a user's unrelated file as a database file.
- **How It Works:** Compare the first 8 bytes against `FileHeader::kMagic` using `std::equal` / `std::ranges::equal`, returning a clear `Status::Corruption("Invalid TinyDB magic header")` on mismatch or truncated input (`< 8` bytes).
- **Design Trade-offs:** Including an explicit non-ASCII byte (`\0\1`) reduces false positives from plain text files starting with `"TINYDB"`.
- **Common Mistakes:** Using `strcmp` or C-string literals (which terminate early at `\0` and ignore bytes after `\0`!).
- **Understanding Checkpoint:** Why would `std::strncmp(buf, "TINYDB\0\1", 8)` be a subtle bug here?

### Component Context
- **Why It Exists:** First line of defense when opening any database file.
- **Module Relationships:** Called first inside `FileHeader::Deserialize` and `DiskManager::Open`.

### Files Expected to Change
- `include/tinydb/storage/file_header.hpp`
- `src/storage/file_header.cpp` (new)
- `tests/unit/storage/file_header_magic_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FileHeader::ValidateMagic(std::span<const std::byte> page_bytes)` and `FileHeader::WriteMagic(std::span<std::byte> page_bytes)`.
2. Write unit tests testing: exact magic match, differing 7th/8th byte (after `\0`!), empty span, 7-byte truncated span, and ASCII text file payload.

### Testing Instructions
- Build and test under Debug and ASan/UBSan.

### Acceptance Criteria
- Changing byte 7 (`\1` -> `\2`) triggers `StatusCode::kCorruption` (proving NUL-safe comparison).

### Definition of Done
Magic validation implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-020: implement NUL-safe FileHeader magic-byte validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-021` (Format-version and page-size validation).
````

---

## DAY-021 — Implement Database Format-Version and Page-Size Header Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-021
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Database Format-Version and Page-Size Header Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-020 complete.

### Objective
Implement explicit `FileHeader::ValidateCompatibility()` verifying supported `format_version`, exact `page_size == 4096`, and internal consistency of `page_count` and `free_page_count`.

### Concepts & Daily Learning
- **Concept:** Format versioning, forward/backward compatibility guards, and structural invariant validation.
- **Why Database Engines Need It:** Opening a file with `format_version = 99` or `page_size = 8192` using a 4096-byte engine would compute wrong page offsets and misread every record.
- **How It Works:** Enforce `format_version == config::kFormatVersion` (`1`), `page_size == config::kPageSize` (`4096`), `page_count >= 1` (Page 0 always exists!), and `free_page_count < page_count` (Page 0 cannot be on the free list!).
- **Design Trade-offs:** Returning `StatusCode::kUnsupported` for higher `format_version` vs `kCorruption` for impossible `page_size == 0` gives clear user-facing diagnostics.
- **Common Mistakes:** Allowing `page_count == 0` or `first_free_page_id == 0` or `first_free_page_id >= page_count` (when not `kInvalidPageId`).
- **Understanding Checkpoint:** Why must `free_page_count < page_count` and `first_free_page_id != 0` always hold for a valid TinyDB file header?

### Component Context
- **Why It Exists:** Protects `DiskManager` from operating on incompatible or logically inconsistent header metadata.
- **Module Relationships:** Invoked inside `FileHeader::Deserialize` and `DiskManager::Open`.

### Files Expected to Change
- `include/tinydb/storage/file_header.hpp`
- `src/storage/file_header.cpp`
- `tests/unit/storage/file_header_compat_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FileHeader::ValidateInvariants() const` returning `Status`: check magic, `format_version`, `page_size`, `page_count >= 1`, `free_page_count < page_count`, and valid `first_free_page_id` (`== kInvalidPageId` iff `free_page_count == 0`, otherwise `1 <= first_free_page_id < page_count`).
2. Add comprehensive unit tests checking each violated invariant individually.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every single inconsistent state (`version=0`, `version=2`, `page_size=8192`, `page_count=0`, `free_page_count >= page_count`, `first_free_page_id=0`) is explicitly rejected with the right `StatusCode`.

### Definition of Done
Compatibility checks implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-021: add FileHeader version, page-size, and invariant validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-022` (FileHeader serialization, deserialization & CRC32).
````

---

## DAY-022 — Implement Explicit FileHeader Serialization, Deserialization, and Header CRC32

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-022
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Explicit FileHeader Serialization, Deserialization, and Header CRC32
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-008 (`endian.hpp`), DAY-010 (`crc32.hpp`), DAY-021 complete.

### Objective
Implement `FileHeader::Serialize(std::span<std::byte>)` and `FileHeader::Deserialize(std::span<const std::byte>)` using explicit little-endian field encoding/decoding, computing and verifying the Page 0 CRC32 checksum.

### Concepts & Daily Learning
- **Concept:** Explicit deterministic serialization, zeroing unused page padding, and checksum-protected metadata blocks.
- **Why Database Engines Need It:** Writing uninitialized padding bytes leaks stack/heap memory onto disk and causes non-deterministic file hashes; missing checksums let silent header corruption pass unnoticed.
- **How It Works:** `Serialize` zeroes the 4096-byte page buffer, encodes fields `[0..31]` via `BinaryWriter`, computes CRC32 over `[0..32]` (or full `[0..4096)` with checksum zeroed—choose and document one consistently!), writes `header_crc32`, and `Deserialize` validates buffer size (`== 4096`), magic, CRC32, and invariants.
- **Design Trade-offs:** Hashing all 4096 bytes vs the serialized header bytes: hashing `[0..kHeaderCrcOffset)` is fast and protects all active header fields while hashing full 4096 bytes also catches corruption in Page 0 padding. Document the chosen range explicitly in `docs/storage-format.md`.
- **Common Mistakes:** Checking invariants BEFORE checking CRC32 or magic (giving confusing error messages); reading uninitialized bytes when computing CRC.
- **Understanding Checkpoint:** In what order should `Deserialize` validate (1) buffer length, (2) magic bytes, (3) CRC32 checksum, and (4) field invariants, and why?

### Component Context
- **Why It Exists:** Completes the persistent serialization boundary for Page 0.
- **Module Relationships:** Used directly by `DiskManager::Create`, `Open`, and `FlushHeader`.

### Files Expected to Change
- `include/tinydb/storage/file_header.hpp`
- `src/storage/file_header.cpp`
- `docs/storage-format.md`
- `tests/unit/storage/file_header_serde_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FileHeader::CreateDefault()` factory.
2. Implement `Status FileHeader::Serialize(std::span<std::byte> page_buf) const` and `Result<FileHeader> FileHeader::Deserialize(std::span<const std::byte> page_buf)`.
3. Verify exact roundtrip across non-default `page_count`, `first_free_page_id`, and `free_page_count`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Roundtrip `Serialize -> Deserialize` reproduces identical `FileHeader` fields.
- Zeroed padding leaves zero uninitialized bytes in the 4KB buffer.

### Definition of Done
Serialization/deserialization implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-022: implement FileHeader explicit serialization and CRC32`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-023` (Malformed FileHeader rejection tests).
````

---

## DAY-023 — Implement Malformed FileHeader and Corruption Rejection Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-023
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Malformed FileHeader and Corruption Rejection Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-022 complete.

### Objective
Write an exhaustive adversarial/corruption test suite (`tests/unit/storage/file_header_corruption_test.cpp`) verifying that single-bit flips, zeroed buffers, truncated pages, forged fields with invalid checksums, and forged valid-checksum-but-invalid-invariant headers are all cleanly rejected.

### Concepts & Daily Learning
- **Concept:** Negative testing, bit-flip fault injection, and defense-in-depth (checksum + semantic invariant checks).
- **Why Database Engines Need It:** Even if a bug or corrupt editor overwrites a header and somehow keeps a valid CRC (or corrupts a single bit), both syntactic checksums and semantic invariants must refuse to accept bad metadata.
- **How It Works:** Start from a valid serialized 4096-byte Page 0, mutate each byte/bit across the active header, and confirm `Deserialize` returns an error `Status` without crashing or asserting UB.
- **Design Trade-offs:** Returning graceful `Status::Corruption` rather than `assert()` ensures bad disk files never abort the caller unexpectedly in Release builds.
- **Common Mistakes:** Using `assert()` for disk-input validation (`assert()` disappears in `-DNDEBUG` Release builds!).
- **Understanding Checkpoint:** Why is using `assert()` to validate bytes read from disk a security and reliability bug?

### Component Context
- **Why It Exists:** Proves Page 0 parsing is immune to corrupt inputs before `DiskManager` depends on it.
- **Module Relationships:** Validates `FileHeader`.

### Files Expected to Change
- `tests/unit/storage/file_header_corruption_test.cpp` (new)
- `src/storage/file_header.cpp` (fix any uncovered edge cases found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write tests mutating every single byte in `[0..kSerializedSize)` one byte at a time and asserting `Deserialize` fails.
2. Add tests for: all-zero 4KB buffer, all-`0xFF` 4KB buffer, short buffer (`4095` bytes), oversized buffer (`4097` bytes), and valid-CRC32 + broken-invariant (`first_free_page_id == page_count`) injection.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every single corrupted byte position `[0..kSerializedSize)` and forged-CRC invariant violation fails cleanly with zero crashes or leaks.

### Definition of Done
All corruption tests pass under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-023: add comprehensive FileHeader corruption rejection tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-024` (`PageType` & `PageId` invariants).
````

---

## DAY-024 — Define `PageType` Enum, `PageId` Invariants, and Page-Type Validation Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-024
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Define `PageType` Enum, `PageId` Invariants, and Page-Type Validation Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-007 (`types.hpp`), DAY-023 complete.

### Objective
Define `enum class PageType : std::uint8_t` (`kInvalid`, `kFileHeader`, `kFree`, `kTableHeap`, `kBTreeInternal`, `kBTreeLeaf`) and safe conversion/validation helpers (`ToPageType`, `PageTypeToString`, `IsValidDataPageId`) in `include/tinydb/storage/page_type.hpp` and `src/storage/page_type.cpp`.

### Concepts & Daily Learning
- **Concept:** Scoped `enum class` with fixed underlying byte type, exhaustive raw-byte validation on decode, and `PageId` domain separation.
- **Why Database Engines Need It:** If a caller accidentally passes a B+ Tree leaf `PageId` into `TableHeap` or reads a raw byte `247` from disk as a `PageType`, casting `static_cast<PageType>(byte)` without validation creates undefined enum values and type confusion bugs!
- **How It Works:** `ToPageType(uint8_t raw)` explicitly switches on known enumerators and returns `StatusCode::kCorruption` for unknown bytes; `IsValidDataPageId(PageId id)` ensures `id != kInvalidPageId && id != kFileHeaderPageId`.
- **Design Trade-offs:** Explicit `switch` conversion requires updating one place when adding a new page type, while guaranteeing compile-time `-Wswitch` completeness and zero invalid enum values at runtime.
- **Common Mistakes:** Blindly doing `static_cast<PageType>(raw_byte)` on disk bytes; allowing `TableHeap` or `BPlusTree` to read/overwrite Page 0 (`kFileHeaderPageId`).
- **Understanding Checkpoint:** Why is `static_cast<PageType>(200)` dangerous when decoding an untrusted byte from disk?

### Component Context
- **Why It Exists:** Enforces strong runtime page-type safety across storage, buffer pool, and B+ Tree modules.
- **Module Relationships:** Embedded into `PageHeader` (`DAY-025`).

### Files Expected to Change
- `include/tinydb/storage/page_type.hpp` (new)
- `src/storage/page_type.cpp` (new)
- `tests/unit/storage/page_type_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `PageType : std::uint8_t` with explicit numeric values (`0..5`).
2. Implement `Result<PageType> DecodePageType(std::uint8_t raw)` and `std::string_view PageTypeName(PageType)`.
3. Implement `IsValidPageId(PageId)` and `IsDataPageId(PageId)`.
4. Write tests verifying every valid value (`0..5`) and rejecting unknown raw values (`6..255`).

### Testing Instructions
- Build and test under Debug + ASan/UBSan (`ctest --output-on-failure`).

### Acceptance Criteria
- Unknown raw bytes `6..255` return `kCorruption`; valid values decode cleanly.

### Definition of Done
Implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-024: define PageType enum and raw-byte validation helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-025` (Common 4096-byte page layout & `PageHeader`).
````

---

## DAY-025 — Define the Fixed 4096-Byte Page Layout and Common 32-Byte `PageHeader` Offsets

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-025
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Define the Fixed 4096-Byte Page Layout and Common 32-Byte `PageHeader` Offsets
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-024 complete.

### Objective
Specify TinyDB's fixed 32-byte common `PageHeader` layout (`include/tinydb/storage/page_header.hpp`) shared by all data/free pages (`page_id`, `page_type`, `flags`, `reserved`, `lsn`, `checksum`, `free_space_offset`, `next_page_id`, `prev_page_id`) and reserve bytes `[32..4095]` (`4064` bytes) for page-type-specific payloads.

### Concepts & Daily Learning
- **Concept:** Common page header vs typed payload separation, self-identifying pages (`page_id` inside header), and embedded `lsn` for idempotent recovery.
- **Why Database Engines Need It:** Embedding `page_id` inside every page detects OS/disk misdirected writes (getting Page 7 when asking for Page 4); embedding `lsn` lets crash recovery skip already-flushed pages without rewriting older WAL records.
- **How It Works:** Bytes `[0..31]` store fixed metadata: `page_id` (4B), `page_type` (1B), `flags` (1B), `reserved` (2B), `lsn` (8B), `checksum` (4B), `free_space_offset` (4B), `next_page_id` (4B), `prev_page_id` (4B) = `32` bytes total! Payload spans `[32..4095]`.
- **Design Trade-offs:** A uniform 32B header wastes ~8–12 unused bytes on page types that don't use `prev_page_id`, in exchange for allowing `DiskManager` and `RecoveryManager` to validate ANY page uniformly without decoding type-specific internals.
- **Common Mistakes:** Miscounting field offsets creating overlaps; exceeding 4096 bytes when computing payload capacity.
- **Understanding Checkpoint:** Why do both `DiskManager` (for checksums & `page_id` sanity) and `RecoveryManager` (for `lsn`) benefit from a common `PageHeader` across all page types?

### Component Context
- **Why It Exists:** Standard frame envelope for `FREE`, `TABLE_HEAP`, `BTREE_INTERNAL`, and `BTREE_LEAF` pages.
- **Module Relationships:** Used by `Page`, `DiskManager`, `SlottedPage`, `BPlusTree` nodes, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/storage/page_header.hpp` (new)
- `docs/storage-format.md` (update Section 3 exact offsets)
- `tests/unit/storage/page_header_layout_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `struct PageHeader` with default values (`kInvalidPageId`, `PageType::kInvalid`, `0`, `kInvalidLsn`, `0`, `kPageHeaderSize`, `kInvalidPageId`, `kInvalidPageId`).
2. Define exact offset constants (`kPageIdOffset = 0`, ..., `kPrevPageIdOffset = 28`, `kPageHeaderSize = 32`, `kPagePayloadSize = config::kPageSize - kPageHeaderSize = 4064`).
3. Verify byte offsets and capacity via `static_assert` and unit tests.

### Testing Instructions
- Build and run `ctest --output-on-failure`.

### Acceptance Criteria
- `kPageHeaderSize == 32`, `kPagePayloadSize == 4064`, and all field offsets sum contiguously to 32.

### Definition of Done
Header specification coded, documented in `docs/storage-format.md`, tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-025: define common 32-byte PageHeader layout and payload constants`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-026` (`PageHeader` serialization).
````

---

## DAY-026 — Implement Explicit `PageHeader` Serialization (`PageHeader::Serialize`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-026
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Explicit `PageHeader` Serialization (`PageHeader::Serialize`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-008 (`endian.hpp`), DAY-025 complete.

### Objective
Implement `PageHeader::Serialize(std::span<std::byte> page_bytes)` in `src/storage/page_header.cpp`, writing all 32 bytes explicitly in little-endian order without modifying payload bytes `[32..4095]`.

### Concepts & Daily Learning
- **Concept:** Partial-header serialization preserving the surrounding 4KB payload buffer.
- **Why Database Engines Need It:** When `SlottedPage` or `BPlusTree` updates a header field (or `DiskManager` updates `checksum`), `PageHeader::Serialize` must overwrite ONLY bytes `[0..31]` without wiping the payload in `[32..4095]`.
- **How It Works:** Validate `page_bytes.size() >= kPageHeaderSize` (and typically `== 4096`), encode each field via `BinaryWriter` into `page_bytes.subspan(0, kPageHeaderSize)`, writing `reserved = 0` explicitly.
- **Design Trade-offs:** Having `SerializeHeaderOnly` vs full-page serialization keeps header updates clean while allowing a separate `UpdateChecksum` pass over `[0..4095]`.
- **Common Mistakes:** Calling `std::fill(..., 0)` on the whole 4096-byte span inside `PageHeader::Serialize`, accidentally wiping the page's tuples!
- **Understanding Checkpoint:** Why must `PageHeader::Serialize` never zero bytes `[32..4095]`?

### Component Context
- **Why It Exists:** Explicit encoder for the first 32 bytes of every page.
- **Module Relationships:** Paired with `PageHeader::Deserialize` (`DAY-027`) and `Page` (`DAY-028`).

### Files Expected to Change
- `include/tinydb/storage/page_header.hpp`
- `src/storage/page_header.cpp` (new)
- `tests/unit/storage/page_header_serde_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Status PageHeader::Serialize(std::span<std::byte> page_buf) const`.
2. Reject spans smaller than `kPageHeaderSize` with `StatusCode::kInvalidArgument`.
3. Write unit tests verifying exact little-endian byte output for every field AND verifying sentinel payload bytes `[32..4095]` remain untouched.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Exact little-endian byte checks pass for all fields; payload bytes `[32..4095]` remain 100% preserved.

### Definition of Done
Implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-026: implement explicit PageHeader serialization`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-027` (`PageHeader` deserialization).
````

---

## DAY-027 — Implement Explicit `PageHeader` Deserialization and Bounds Validation (`PageHeader::Deserialize`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-027
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Explicit `PageHeader` Deserialization and Bounds Validation (`PageHeader::Deserialize`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-026 complete.

### Objective
Implement `Result<PageHeader> PageHeader::Deserialize(std::span<const std::byte> page_bytes)` with strict `page_type` decoding, `free_space_offset` range checking, and `reserved` checks.

### Concepts & Daily Learning
- **Concept:** Untrusted binary decoding and semantic bounds checking (`kPageHeaderSize <= free_space_offset <= kPageSize`).
- **Why Database Engines Need It:** Corrupt `free_space_offset = 99999` or `0` in a `PageHeader` would cause subsequent payload reads/writes to index out of the 4096-byte buffer.
- **How It Works:** Read all 32 bytes via `BinaryReader`, decode `PageType` via `DecodePageType`, validate `free_space_offset` (`kPageHeaderSize <= free_space_offset <= kPageSize`) when applicable, and return `Result<PageHeader>`.
- **Design Trade-offs:** Validating structural header ranges at decode time fails fast at the I/O boundary before higher layers touch corrupted offsets.
- **Common Mistakes:** Accepting `free_space_offset < 32` (pointing back into the header!) or `> 4096`.
- **Understanding Checkpoint:** What security/safety failures occur if a corrupt `free_space_offset = 10` or `65000` is accepted during deserialization?

### Component Context
- **Why It Exists:** Safe decoder for every non-header page loaded from disk.
- **Module Relationships:** Used by `Page`, `DiskManager::ReadPage`, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/storage/page_header.hpp`
- `src/storage/page_header.cpp`
- `tests/unit/storage/page_header_serde_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Result<PageHeader> PageHeader::Deserialize(std::span<const std::byte> page_buf)`.
2. Enforce `page_buf.size() >= kPageHeaderSize`, valid `PageType`, and `kPageHeaderSize <= free_space_offset <= config::kPageSize`.
3. Add roundtrip and invalid-field unit tests (`free_space_offset = 0`, `31`, `4097`, invalid `PageType = 99`, truncated buffer).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Roundtrip `Serialize -> Deserialize` passes across all `PageType`s; invalid bounds/types fail with `StatusCode::kCorruption`.

### Definition of Done
Implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-027: implement PageHeader deserialization and bounds checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-028` (`Page` container class).
````

---

## DAY-028 — Implement the `Page` Container Class (4096-Byte Aligned Buffer & Payload Views)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-028
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement the `Page` Container Class (4096-Byte Aligned Buffer & Payload Views)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-013 (`AlignedPageBuffer`), DAY-027 (`PageHeader`).

### Objective
Implement the `tinydb::Page` class (`include/tinydb/storage/page.hpp`, `src/storage/page.cpp`) encapsulating a 4096-byte page buffer, typed header read/write accessors, payload `std::span` slices (`[32..4095]`), and `Reset()`/`Initialize(PageId, PageType)`.

### Concepts & Daily Learning
- **Concept:** Separating the raw 4096-byte storage representation (`Page`) from buffer-frame metadata (`pin_count`, `is_dirty`, which belong in `PageFrame` in Phase 04).
- **Why Database Engines Need It:** `DiskManager` deals strictly in 4096-byte raw `Page`s (no awareness of LRU or pin counts), keeping storage cleanly decoupled from buffer pool caching.
- **How It Works:** `Page` owns `AlignedPageBuffer`, exposes `Data()` / `ConstData()` (`std::span<std::byte, 4096>`), `Payload()` (`std::span<std::byte, 4064>`), `ReadHeader()`, and `WriteHeader(const PageHeader&)`.
- **Design Trade-offs:** Storing `PageHeader` inside the serialized bytes (and parsing via `ReadHeader()` / writing via `WriteHeader()`) rather than duplicating shadow header fields prevents divergence between in-memory fields and raw page bytes.
- **Common Mistakes:** Allowing raw `memcpy` over the whole 4KB buffer including header without preserving or validating `PageId`.
- **Understanding Checkpoint:** Why does keeping `pin_count` and `is_dirty` out of `Page` (in Phase 04's `PageFrame`) yield a cleaner architecture?

### Component Context
- **Why It Exists:** Canonical in-memory 4KB page container passed to/from `DiskManager`.
- **Module Relationships:** Held inside `PageFrame` (Phase 04), wrapped by `SlottedPage` (Phase 02) and `BPlusTreePage` (Phase 05).

### Files Expected to Change
- `include/tinydb/storage/page.hpp` (new)
- `src/storage/page.cpp` (new)
- `tests/unit/storage/page_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Page` with `Reset()`, `Initialize(PageId, PageType)`, `ReadHeader()`, `WriteHeader(const PageHeader&)`, `Data()`, `ConstData()`, `Payload()`, `ConstPayload()`.
2. Write tests verifying 4096-byte raw access, 4064-byte payload offset starting at `Data() + 32`, `Initialize`, and `Reset()`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `Payload().data() == Data().data() + 32` and `Payload().size() == 4064`.
- `Initialize(5, PageType::kTableHeap)` persists valid header bytes immediately readable via `ReadHeader()`.

### Definition of Done
`Page` class implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-028: implement 4096-byte Page container and payload views`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-029` (Page CRC32 checksum computation and verification).
````

---

## DAY-029 — Implement Page-Level CRC32 Checksum Computation and Verification

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-029
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Page-Level CRC32 Checksum Computation and Verification
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-010 (`crc32`), DAY-028 (`Page`).

### Objective
Add `Page::ComputeChecksum()`, `Page::UpdateChecksum()`, and `Page::VerifyChecksum()` covering all 4096 bytes of a `Page` (treating the 4-byte `checksum` field at offset `[16..19]` as `0x00000000` during computation).

### Concepts & Daily Learning
- **Concept:** Whole-page CRC32 calculation over `[0..4095]` excluding/zeroing the embedded checksum field.
- **Why Database Engines Need It:** Data payload corruption inside `[32..4095]` is far more common than header-only corruption; whole-page CRC32 catches bit-rot anywhere in the 4096 bytes.
- **How It Works:** Feed `[0..16)`, four zero bytes `{0,0,0,0}`, and `[20..4096)` sequentially into incremental `UpdateCrc32` + `FinalizeCrc32`. `VerifyChecksum()` reads the stored `uint32_t` at `[16..19]` and compares it to `ComputeChecksum()`.
- **Design Trade-offs:** Feeding three slices avoids mutating or copying the 4KB buffer during read-only `VerifyChecksum()`!
- **Common Mistakes:** Mutating the buffer temporarily to zero `[16..19]` inside a `const` `VerifyChecksum()` method (breaks const/thread-safety!); forgetting to call `UpdateChecksum()` after modifying payload bytes before writing to disk.
- **Understanding Checkpoint:** How does `ComputeChecksum()` avoid allocating or mutating the `const Page&` buffer while treating bytes `[16..19]` as zero?

### Component Context
- **Why It Exists:** Protects all 4096 bytes of every page on disk.
- **Module Relationships:** Invoked automatically by `DiskManager::WritePage` and `DiskManager::ReadPage`.

### Files Expected to Change
- `include/tinydb/storage/page.hpp`
- `src/storage/page.cpp`
- `tests/unit/storage/page_checksum_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ComputeChecksum() const`, `UpdateChecksum()`, and `VerifyChecksum() const` on `Page`.
2. Test: `Initialize` -> `UpdateChecksum` -> `VerifyChecksum` is true; modifying any payload byte or header byte invalidates `VerifyChecksum()`; re-running `UpdateChecksum` fixes it.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `ComputeChecksum()` is non-mutating (`const`) and catches single-bit flips in both header (`!= checksum` bytes) and payload `[32..4095]`.

### Definition of Done
Page checksum methods implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-029: implement non-mutating whole-page CRC32 checksum verification`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-030` (Malformed PageHeader & bit-flip tests).
````

---

## DAY-030 — Implement Malformed `PageHeader` and Bit-Flip Corruption Rejection Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-030
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Malformed `PageHeader` and Bit-Flip Corruption Rejection Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-029 complete.

### Objective
Write exhaustive validation tests (`tests/unit/storage/page_corruption_test.cpp`) plus `Page::Validate(PageId expected_page_id)` verifying page-ID match, header validity, and whole-page CRC32 integrity.

### Concepts & Daily Learning
- **Concept:** Self-identifying page validation (`expected_page_id == header.page_id`) + checksum defense against torn or misdirected disk writes.
- **Why Database Engines Need It:** If a bug or OS quirk swaps Page 3 and Page 4 on disk, each page still has a valid internal CRC32—only comparing `header.page_id == expected_page_id` catches the swap!
- **How It Works:** `Page::Validate(expected_id)` checks: (1) `PageHeader::Deserialize` succeeds, (2) `VerifyChecksum()` passes, and (3) `header.page_id == expected_id`.
- **Design Trade-offs:** Returning descriptive `Status::Corruption` messages pinpoints whether a failure was CRC mismatch, invalid `PageType`, or wrong `page_id`.
- **Common Mistakes:** Only checking CRC32 without checking `header.page_id == expected_id`.
- **Understanding Checkpoint:** What specific hardware/software bug does `header.page_id == expected_id` catch that whole-page CRC32 alone cannot catch?

### Component Context
- **Why It Exists:** Single authoritative page-integrity check called on every `DiskManager::ReadPage`.
- **Module Relationships:** Used by `DiskManager::ReadPage` and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/storage/page.hpp`
- `src/storage/page.cpp`
- `tests/unit/storage/page_corruption_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Result<PageHeader> Page::Validate(PageId expected_page_id) const`.
2. Write tests verifying: swapped-page (`expected=3`, `actual=4`) rejection, bit-flip sweep across sampled offsets `[0..4095]`, torn-page tail (last 512 bytes zeroed), and invalid `PageType` even with recalculated CRC32.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every injected bit-flip, torn tail, swapped `PageId`, and forged header fails `Page::Validate` with `StatusCode::kCorruption`.

### Definition of Done
`Page::Validate` and corruption suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-030: add Page::Validate and whole-page corruption tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-031` (Free-page management design).
````

---

## DAY-031 — Design Free-Page Management (Intrusive Free-Page Linked List Specification)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-031
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Design Free-Page Management (Intrusive Free-Page Linked List Specification)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-030 complete.

### Objective
Design TinyDB's intrusive singly-linked free-page list (LIFO freelist), documenting invariants, cycle/double-free safety, and crash-consistency considerations in `docs/storage-format.md` and defining `FreePage` helper structures (`include/tinydb/storage/free_page.hpp`).

### Concepts & Daily Learning
- **Concept:** Intrusive freelist vs free-space bitmap pages for disk-page reclamation.
- **Why Database Engines Need It:** Dropping tables, deleting B+ Tree nodes, and compacting heaps free 4KB pages; without reclamation, database files grow unboundedly even when records are deleted.
- **How It Works:** When a page becomes free, it requires zero payload data—its own `PageHeader` (`page_type = kFree`, `next_page_id = old_first_free_page_id`) stores the link to the next free page! `FileHeader` stores `first_free_page_id` and `free_page_count`.
- **Design Trade-offs:** Intrusive freelists use O(1) push/pop and zero extra metadata pages when empty, whereas bitmap pages enable contiguous multi-page allocation and easier corruption recovery (choosing intrusive freelist for clean v1 simplicity, documented in `design-decisions.md`).
- **Common Mistakes:** Allowing a free page to retain stale user data instead of zeroing payload; forgetting to update `FileHeader` when popping/pushing; forming a cycle by double-freeing a page.
- **Understanding Checkpoint:** Compare intrusive free-page linked lists vs free-space bitmap pages in terms of allocation cost, space overhead, and double-free detection.

### Component Context
- **Why It Exists:** Enables `DiskManager::AllocatePage` and `DeallocatePage` to reclaim freed pages across restarts.
- **Module Relationships:** Uses `Page` (`PageType::kFree`) and `FileHeader` (`first_free_page_id`, `free_page_count`).

### Files Expected to Change
- `include/tinydb/storage/free_page.hpp` (new)
- `docs/storage-format.md` (add Free-Page List specification section)
- `docs/design-decisions.md` (add `DD-008` Free-Page Linked List decision)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Specify the `FreePage` header/payload invariants (`page_type == PageType::kFree`, payload zeroed on free, `next_page_id` points to next free page or `kInvalidPageId`).
2. Add `DD-008` in `docs/design-decisions.md` and detailed freelist state transitions in `docs/storage-format.md`.

### Testing Instructions
- Verify project compiles cleanly (`cmake --build build && ctest --test-dir build --output-on-failure`).

### Acceptance Criteria
- Design specifications and invariants for push, pop, empty list, and double-free prevention are fully documented and header interfaces compile cleanly.

### Definition of Done
Design spec and `free_page.hpp` interface written, build/tests clean, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-031: design intrusive free-page linked list format`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-032` (Free-page node serialization).
````

---

## DAY-032 — Implement Free-Page Node Serialization and Header Metadata Tracking

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-032
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Free-Page Node Serialization and Header Metadata Tracking
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-031 complete.

### Objective
Implement `FreePageCodec` (`src/storage/free_page.cpp`) to format an arbitrary `Page` into a checksummed `PageType::kFree` node (clearing stale data!), extract `next_free_page_id` safely, and verify free-list pointer invariants.

### Concepts & Daily Learning
- **Concept:** Data sanitization upon deallocation and type-validated freelist traversal.
- **Why Database Engines Need It:** If a deleted table's sensitive bytes remain inside a freed page, reading a raw disk file exposes deleted data; also, popping a page whose `page_type != kFree` indicates severe corruption.
- **How It Works:** `FormatFreePage(Page& page, PageId id, PageId next_free)` resets `page`, writes `PageHeader` with `kFree` and `next_page_id = next_free`, and updates CRC32; `ReadNextFreePage(const Page& page, PageId expected_id)` validates CRC32, `page_id`, and `page_type == kFree` before returning `next_page_id`.
- **Design Trade-offs:** Zeroing the 4064-byte payload on `DeallocatePage` costs one `memset` per free while protecting privacy and making checksum behavior predictable.
- **Common Mistakes:** Not verifying `page_type == PageType::kFree` when popping from `first_free_page_id`—potentially handing out an active B+ Tree page!
- **Understanding Checkpoint:** Why must `ReadNextFreePage` reject a page whose `page_type` is `kTableHeap` even if its CRC32 is valid?

### Component Context
- **Why It Exists:** Separates freelist page encoding/validation cleanly from `DiskManager` file I/O logic.
- **Module Relationships:** Used directly by `DiskManager::AllocatePage` and `DeallocatePage`.

### Files Expected to Change
- `include/tinydb/storage/free_page.hpp`
- `src/storage/free_page.cpp` (new)
- `tests/unit/storage/free_page_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FormatFreePage` and `ReadNextFreePage` in `src/storage/free_page.cpp`.
2. Write unit tests: formatting wipes non-zero stale payload, roundtrip preserves `next_free_page_id`, non-`kFree` page fails with `kCorruption`, and `next_free == expected_id` (self-loop) fails with `kCorruption`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Stale payload is verified zeroed; self-loops and wrong `PageType`s are rejected.

### Definition of Done
Implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-032: implement FreePage node serialization and validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-033` (`DiskManager` skeleton).
````

---

## DAY-033 — Implement `DiskManager` Class Skeleton and File Lifecycle State Machine

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-033
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager` Class Skeleton and File Lifecycle State Machine
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-012 (`FileHandle`), DAY-014 (`latch.hpp`), DAY-022 (`FileHeader`), DAY-032 (`FreePage`).

### Objective
Create `DiskManager` class declaration (`include/tinydb/storage/disk_manager.hpp`) and initial lifecycle state machine (`src/storage/disk_manager.cpp`): managing `FileHandle`, cached `FileHeader`, `std::mutex`, open/closed state invariants, and read-only accessors (`GetHeader()`, `IsOpen()`, `Path()`).

### Concepts & Daily Learning
- **Concept:** Explicit lifecycle state machines (`Unopened -> Open -> Closed`) and mutex-guarded I/O components.
- **Why Database Engines Need It:** Calling `ReadPage` or `AllocatePage` on a closed `DiskManager` must fail deterministically with `Status` rather than passing `fd = -1` to the OS or crashing.
- **How It Works:** `DiskManager` is non-copyable (owns a `FileHandle`, `std::filesystem::path`, `FileHeader`, and `std::mutex`), constructed cleanly via static `Create`/`Open` factories or default-constructed + `Open`/`Close` with guarded state checks.
- **Design Trade-offs:** Returning `std::unique_ptr<DiskManager>` or `Result<DiskManager>` (Wait: since `std::mutex` is non-movable, either wrap `DiskManager` in `std::unique_ptr` via static factory `Result<std::unique_ptr<DiskManager>> Create(...)` OR make a default-constructible `DiskManager` with `Status Create/Open/Close`!). Let's use `Status Create(...)` / `Open(...)` / `Close()` (plus optional `unique_ptr` factory if helpful) cleanly and document why `std::mutex` makes a class non-movable!
- **Common Mistakes:** Embedding a `std::mutex` inside a class and wondering why `Result<DiskManager>` fails to compile because `std::mutex` cannot be moved!
- **Understanding Checkpoint:** Why can't a class containing a `std::mutex` member directly be moved or returned by value in `Result<DiskManager>`? How do `std::unique_ptr<DiskManager>` or `Status Create/Open` methods solve this cleanly?

### Component Context
- **Why It Exists:** Central persistent page store interface for the entire database engine.
- **Module Relationships:** Wraps `FileHandle`, `FileHeader`, `Page`, `FreePage`; consumed by `BufferPoolManager` and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp` (new)
- `src/storage/disk_manager.cpp` (new)
- `tests/unit/storage/disk_manager_state_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Declare the full public API of `DiskManager` (`Create`, `Open`, `Close`, `ReadPage`, `WritePage`, `FlushPage`, `FlushAll`, `AllocatePage`, `DeallocatePage`, `PageCount`, `FreePageCount`, `IsOpen`).
2. Implement constructor/destructor, state guards (operations while `!is_open_` return `StatusCode::kInvalidArgument` or `kFailedPrecondition`/`kIoError`), and stub unimplemented methods cleanly until `DAY-034`–`DAY-042`.
3. Write state-guard unit tests on an unopened `DiskManager`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Unopened `DiskManager` rejects page operations gracefully without touching disk.

### Definition of Done
Skeleton compiles cleanly with `-Werror`, state tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-033: define DiskManager API and file lifecycle state guards`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-034` (`DiskManager::Create`).
````

---

## DAY-034 — Implement `DiskManager::Create` (Initialized Page 0 FileHeader and Durable Sync)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-034
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::Create` (Initialized Page 0 FileHeader and Durable Sync)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-033 complete.

### Objective
Implement `DiskManager::Create(const std::filesystem::path& db_path)` creating a brand-new `.tinydb` file exclusively (`CreateNew`), writing the serialized 4096-byte Page 0 `FileHeader`, and syncing both the file and parent directory for durability.

### Concepts & Daily Learning
- **Concept:** Exclusive file creation (`O_CREAT | O_EXCL`), atomic initialization of Page 0, and crash-safe directory syncing.
- **Why Database Engines Need It:** If `Create` silently overwrote an existing database file, running `Create` twice on an existing DB path would destroy user data! Also, a newly created DB file must be exactly 4096 bytes with a valid Page 0 header on disk before returning success.
- **How It Works:** Guard `!is_open_`, call `FileHandle::CreateNew(db_path)`, serialize `FileHeader::CreateDefault()` into a 4096-byte buffer, `WriteAt(0, buf)`, `Sync()`, `SyncParentDirectory(db_path)`, and set `is_open_ = true`. Clean up partial file if writing Page 0 fails!
- **Design Trade-offs:** Failing if the file exists (rather than truncating) prioritizes safety over convenience; tests/CLI explicitly decide when to delete or open existing files.
- **Common Mistakes:** Leaving a half-created 0-byte file on disk if writing Page 0 fails; forgetting to sync the parent directory so the directory entry itself could disappear after power loss.
- **Understanding Checkpoint:** Why does `DiskManager::Create` call `SyncParentDirectory` in addition to syncing the database file descriptor?

### Component Context
- **Why It Exists:** Initializes valid empty TinyDB database files on disk.
- **Module Relationships:** Paired with `DiskManager::Open` (`DAY-035`).

### Files Expected to Change
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_lifecycle_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DiskManager::Create` in `src/storage/disk_manager.cpp`.
2. Verify in tests: created file exists on disk with exact size `4096` bytes, Page 0 bytes decode cleanly via `FileHeader::Deserialize`, calling `Create` on an already-existing file returns `kAlreadyExists`, and calling `Create` on an already-open `DiskManager` fails safely.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Created `.tinydb` file is exactly `4096` bytes with valid Page 0 CRC32 and `page_count == 1`.

### Definition of Done
`Create` tested and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-034: implement DiskManager::Create with durable Page 0 FileHeader`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-035` (`DiskManager::Open`).
````

---

## DAY-035 — Implement `DiskManager::Open` (FileHeader Validation and Metadata Recovery)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-035
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::Open` (FileHeader Validation and Metadata Recovery)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-034 complete.

### Objective
Implement `DiskManager::Open(const std::filesystem::path& db_path)` opening an existing `.tinydb` file, validating file size alignment (`size >= 4096 && size % 4096 == 0`), reading & deserializing Page 0 `FileHeader`, and verifying `file_size >= header.page_count * 4096`.

### Concepts & Daily Learning
- **Concept:** Startup file validation, checking physical file length against logical `header.page_count`, and rejecting truncated or non-page-aligned database files.
- **Why Database Engines Need It:** If a database crashed while extending the file or someone truncated it, `header.page_count` could claim 100 pages exist while the file only holds 50 pages—leading to unexpected EOF errors later during queries.
- **How It Works:** Open existing file, inspect `FileSize()`, verify `size >= 4096` and `size % 4096 == 0`, read Page 0, run `FileHeader::Deserialize`, verify `size == header.page_count * kPageSize` (or `>=` with clear policy—requiring exact match `size == page_count * 4096` catches trailing torn allocations!).
- **Design Trade-offs:** Strict `file_size == header.page_count * 4096` catches half-written page allocations immediately, while `AllocatePage` must ensure Page 0 is updated cleanly.
- **Common Mistakes:** Not checking `file_size % 4096 == 0`; leaving `file_` open inside `DiskManager` if `FileHeader::Deserialize` fails!
- **Understanding Checkpoint:** If `header.page_count` is `10` (`40,960` bytes) but `FileSize()` is `38,000` bytes, what likely happened and why must `Open` reject it?

### Component Context
- **Why It Exists:** Safely reattaches `DiskManager` to persisted database files after process restart.
- **Module Relationships:** Pairs with `Create` (`DAY-034`) and `Close` (`DAY-036`).

### Files Expected to Change
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_lifecycle_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DiskManager::Open`. Ensure `file_.Close()` runs on any error before returning `Status` so `DiskManager` stays cleanly unopened.
2. Add tests: `Create` -> close/destroy -> `Open` succeeds; opening nonexistent file fails `kNotFound`; opening non-TinyDB file fails `kCorruption`; opening truncated/non-multiple-of-4096 file fails `kCorruption`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Failed `Open` leaves `IsOpen() == false` with zero leaked FDs.
- Valid DB reopens with exact recovered `PageCount()` and `FreePageCount()`.

### Definition of Done
`Open` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-035: implement DiskManager::Open with size and FileHeader validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-036` (`DiskManager::Close`).
````

---

## DAY-036 — Implement `DiskManager::Close` with Flush and RAII Destructor Cleanup

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-036
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::Close` with Flush and RAII Destructor Cleanup
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-035 complete.

### Objective
Implement `DiskManager::Close()` (persisting latest `FileHeader`, calling `Sync()`, closing `FileHandle`, resetting internal state) and verify `~DiskManager()` invokes cleanup safely without throwing or double-closing.

### Concepts & Daily Learning
- **Concept:** Graceful database shutdown vs destructor safety (`noexcept`), and idempotent close operations.
- **Why Database Engines Need It:** Unclean shutdown leaves OS dirty pages unsynced or FDs open; calling `Close()` explicitly lets callers observe any flush error while `~DiskManager()` provides an RAII safety net.
- **How It Works:** Under `latch_`, if `!is_open_` return `Status::Ok()` (idempotent) or documented behavior; flush `FileHeader` to Page 0, `file_.Sync()`, `file_.Close()`, reset `is_open_ = false` and `header_ = {}`. Destructor checks `if (is_open_) { (void)Close(); }`.
- **Design Trade-offs:** Making `Close()` idempotent (or explicit status) prevents errors when callers explicitly `Close()` and then `~DiskManager()` runs at scope exit.
- **Common Mistakes:** Calling `Close()` without acquiring `latch_` inside destructor while another thread still holds a reference (lifetime bug), or double-closing raw FD.
- **Understanding Checkpoint:** Why should `~DiskManager()` never throw an exception if `Close()` encounters an I/O error during stack unwinding?

### Component Context
- **Why It Exists:** Completes the `Create -> Open -> Close` lifecycle triad.
- **Module Relationships:** Called by `BufferPoolManager::Shutdown` and engine shutdown.

### Files Expected to Change
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_lifecycle_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `WriteHeaderLocked()` helper, `DiskManager::Close()`, and `~DiskManager()`.
2. Write unit tests verifying: explicit `Close()` followed by destructor works cleanly; calling `Close()` twice is safe; after `Close()`, `DiskManager` can `Open()` the same or a different database path cleanly.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero FD leaks, zero crashes across repeated `Create -> Close -> Open -> Close` cycles.

### Definition of Done
`Close` and RAII cleanup tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-036: implement DiskManager::Close with header flush and RAII destructor`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-037` (Overflow-safe page offset calculation).
````

---

## DAY-037 — Implement Overflow-Safe Page Offset Calculation and File Boundary Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-037
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Overflow-Safe Page Offset Calculation and File Boundary Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-036 complete.

### Objective
Implement dedicated overflow-safe page offset calculation and bounds validation helpers (`ComputePageOffset(PageId)`, `ValidateDataPageId(PageId, page_count)`) so `ReadPage`, `WritePage`, `AllocatePage`, and `DeallocatePage` share one fortified boundary check.

### Concepts & Daily Learning
- **Concept:** 32-bit multiplication overflow (`uint32_t * 4096` overflows at `page_id >= 1,048,576` -> 4GB!) and off-by-one boundary traps (`page_id == 0` vs `page_id >= page_count`).
- **Why Database Engines Need It:** Computing `page_id * 4096` in 32-bit `uint32_t` silently wraps around back to offset `0` (overwriting Page 0 FileHeader!) once a file exceeds 4 GiB or if a corrupt `page_id` like `0x00100000` is read!
- **How It Works:** Cast `static_cast<std::uint64_t>(page_id) * static_cast<std::uint64_t>(config::kPageSize)` (cannot overflow `uint64_t`), explicitly forbid `kInvalidPageId`, forbid `page_id == 0` for data operations, and require `page_id < header_.page_count`.
- **Design Trade-offs:** Centralizing boundary checks in a single helper eliminates duplicated `if` guards across 4 methods.
- **Common Mistakes:** Writing `off_t offset = page_id * kPageSize;` where both operands are `uint32_t`, overflowing BEFORE conversion to 64-bit `off_t`!
- **Understanding Checkpoint:** What happens in C++ if both `PageId` (`uint32_t`) and `kPageSize` (`uint32_t`) are `uint32_t` and you write `uint64_t off = page_id * kPageSize;` for `page_id = 1048576`?

### Component Context
- **Why It Exists:** Eliminates a classic C++ integer-overflow vulnerability across all disk seeks.
- **Module Relationships:** Used by `ReadPage`, `WritePage`, `FlushPage`, and `DeallocatePage`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_bounds_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `static Result<std::uint64_t> ComputePageOffset(PageId page_id)` and `ValidateDataPageAccessLocked(PageId page_id) const`.
2. Add unit tests verifying: `page_id = 1048576` computes `4294967296ULL` (`4 GiB`, NOT `0`!), `page_id = 0` is rejected, `kInvalidPageId` is rejected, and `page_id >= page_count` is rejected.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `ComputePageOffset(1048576U)` equals `4294967296ULL` with zero UBSan warnings.
- Page 0 and out-of-range `PageId`s are rejected with `StatusCode::kOutOfRange` / `kInvalidArgument`.

### Definition of Done
Overflow-safe offset helpers implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-037: implement overflow-safe page offset calculation and bounds validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-038` (`DiskManager::ReadPage`).
````

---

## DAY-038 — Implement `DiskManager::ReadPage` with Bounds Checks, Short-Read Guard, and Header Verification

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-038
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::ReadPage` with Bounds Checks, Short-Read Guard, and Header Verification
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-030 (`Page::Validate`), DAY-037 (`ComputePageOffset`).

### Objective
Implement `Status DiskManager::ReadPage(PageId page_id, Page& out_page)` validating `page_id`, reading 4096 bytes at the computed offset, detecting truncated short reads, and validating the page header, `page_id` match, and whole-page CRC32.

### Concepts & Daily Learning
- **Concept:** Read-path verification pipeline (`Bounds -> Exact I/O -> Integrity Check -> Caller Delivery`).
- **Why Database Engines Need It:** Preventing corrupt or freed pages from silently entering the Buffer Pool stops cascading bugs in `SlottedPage` or `BPlusTree` before memory corruption happens.
- **How It Works:** Under `latch_`, validate `is_open_` and `page_id < page_count`, compute offset, call `file_.ReadAt(offset, out_page.Data())`, call `out_page.Validate(page_id)`, and reject reading `PageType::kFree` as an active data page (Wait: provide `ReadRawPage` internally for freelist inspection if needed, while `ReadPage` verifies integrity and rejects `kInvalid`!). Let's explicitly enforce `page_type != kFree` in `ReadPage` (or validate `kFree` cleanly via internal helper `ReadPageAllowFreeLocked` for `AllocatePage`!). That prevents use-after-free page reads!
- **Design Trade-offs:** Rejecting `PageType::kFree` in public `ReadPage` catches dangling `PageId` references immediately!
- **Common Mistakes:** Reading into `out_page` and leaving partial corrupt data inside `out_page` when validation fails (reset `out_page` on failure or validate into temp buffer if desired; resetting on failure is clean and zero-alloc!).
- **Understanding Checkpoint:** Why should public `DiskManager::ReadPage` reject reading a page whose on-disk `PageType` is `kFree`?

### Component Context
- **Why It Exists:** Primary disk-to-memory read entry point for `BufferPoolManager` and `RecoveryManager`.
- **Module Relationships:** Calls `FileHandle::ReadAt` and `Page::Validate`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_io_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ReadPage` and private `ReadPageRawLocked` (used later by freelist traversal).
2. On validation error, `out_page.Reset()` so callers never consume partially decoded junk.
3. Write tests for invalid `page_id` (`0`, `>= page_count`, `kInvalidPageId`) and unopened `DiskManager`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Out-of-bounds `ReadPage` fails with `kOutOfRange`/`kInvalidArgument` and leaves `out_page` zeroed.

### Definition of Done
`ReadPage` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-038: implement DiskManager::ReadPage with bounds and integrity checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-039` (`DiskManager::WritePage`).
````

---

## DAY-039 — Implement `DiskManager::WritePage` with Size Validation, Checksum Update, and Explicit Seeking

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-039
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::WritePage` with Size Validation, Checksum Update, and Explicit Seeking
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-038 complete.

### Objective
Implement `Status DiskManager::WritePage(PageId page_id, Page& page)` (or `const Page&` copying/updating checksum cleanly) validating `page_id`, header `page_id == page_id`, disallowed `kFree`/`kFileHeader` misuse, updating whole-page CRC32, and writing all 4096 bytes at `ComputePageOffset(page_id)`.

### Concepts & Daily Learning
- **Concept:** Write-path invariant enforcement (`header.page_id == target_page_id`), checksum stamping before disk write, and avoiding silent page-0 overwrite.
- **Why Database Engines Need It:** If a buggy caller accidentally passes Page 5's buffer when calling `WritePage(2, page)`, validating `header.page_id == page_id` stops the overwrite BEFORE Page 2 is destroyed on disk!
- **How It Works:** Under `latch_`, validate open state and `page_id`, parse `page.ReadHeader()`, assert `header.page_id == page_id` and `header.page_type != PageType::kInvalid && header.page_type != PageType::kFileHeader && header.page_type != PageType::kFree`, call `page.UpdateChecksum()`, and invoke `file_.WriteAt(offset, page.ConstData())`.
- **Design Trade-offs:** Taking `Page&` (updating `checksum` in-place) vs `const Page&` (requiring temporary 4KB copy or caller pre-stamping): taking `Page&` avoids a 4KB copy on every write while guaranteeing every written page has a fresh valid checksum!
- **Common Mistakes:** Writing a modified page to disk without calling `UpdateChecksum()`, causing the next `ReadPage` to fail with CRC32 corruption!
- **Understanding Checkpoint:** Why must `WritePage` verify `header.page_id == target_page_id` AND forbid `PageType::kFileHeader`?

### Component Context
- **Why It Exists:** Safe persistent page-write boundary.
- **Module Relationships:** Paired with `ReadPage` (`DAY-038`) and `FlushPage` (`DAY-040`).

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_io_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DiskManager::WritePage(PageId page_id, Page& page)`.
2. Test: writing a valid initialized page (tested with an allocated or raw test page once `AllocatePage` exists—or test rejection of Page 0, `page_id >= page_count`, mismatched `header.page_id`, and `kInvalid` type cleanly now, plus test roundtrip after `AllocatePage` or test-helper setup!). Note: we can verify `WritePage` rejection cases immediately and test roundtrip via `Create` + `AllocatePage` in `DAY-041` (or write a small internal friend/test or implement clean roundtrip on `DAY-041`—actually wait: can we test `WritePage` roundtrip using `ReadPage`? Until `AllocatePage` exists on `DAY-041`, `page_count` is `1`, so `DAY-039` tests error validation OR we test valid write if `page_count` > 1? Even cleaner: test all `WritePage` validation rejections thoroughly today AND keep `AllocatePage` cleanly scoped for `DAY-041`!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `WritePage` rejects `page_id == 0`, out-of-range `page_id`, mismatched `header.page_id`, and `PageType::kInvalid` / `kFree` before writing a single byte to disk.

### Definition of Done
`WritePage` implemented and error paths tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-039: implement DiskManager::WritePage with validation and checksum stamping`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-040` (`FlushPage` & `FlushAll`).
````

---

## DAY-040 — Implement `DiskManager::FlushPage` and `DiskManager::FlushAll` (`fsync` Durability)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-040
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::FlushPage` and `DiskManager::FlushAll` (`fsync` Durability)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-039 complete.

### Objective
Implement `DiskManager::FlushPage(PageId page_id, Page& page)` (atomic write + durable `Sync()`) and `DiskManager::FlushAll()` (flushing `FileHeader` + `Sync()`), distinguishing buffered `WritePage` from durable `Flush`.

### Concepts & Daily Learning
- **Concept:** Buffered writes (kernel page cache) vs durable flushes (`fsync` / `F_FULLFSYNC`), and write-coalescing trade-offs.
- **Why Database Engines Need It:** Calling `fsync` on every individual page write would cripple bulk-load throughput (100x slower); separating `WritePage` from `FlushAll` lets `BufferPoolManager` batch dirty writes and sync once at checkpoint/commit!
- **How It Works:** `FlushPage(id, page)` writes the page via `WritePageLocked` and calls `file_.Sync()`. `FlushAll()` writes the latest `FileHeader` to Page 0 and calls `file_.Sync()`.
- **Design Trade-offs:** POSIX `fsync` flushes the entire file descriptor rather than a single 4KB block (`sync_file_range` is Linux-only and not always durable for metadata); documenting this honestly keeps v1 portable.
- **Common Mistakes:** Assuming `WritePage` alone survives power loss without `Sync()`.
- **Understanding Checkpoint:** Why do database engines separate `WritePage` from `FlushAll`/`Sync` instead of calling `fsync` inside every `WritePage`?

### Component Context
- **Why It Exists:** Explicit durability boundary for `BufferPoolManager::FlushPage` / `FlushAllPages` and `RecoveryManager`.
- **Module Relationships:** Calls `WritePageLocked`, `WriteHeaderLocked`, and `FileHandle::Sync`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_io_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FlushPage(PageId, Page&)` and `FlushAll()`.
2. Write unit tests verifying `FlushAll()` succeeds on an open DB and fails on an unopened DB, and `FlushPage(0, ...)` rejects Page 0 cleanly.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `FlushPage` and `FlushAll` enforce open state, bounds, and durable `file_.Sync()`.

### Definition of Done
`FlushPage` and `FlushAll` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-040: implement DiskManager::FlushPage and FlushAll durability`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-041` (`DiskManager::AllocatePage`).
````

---

## DAY-041 — Implement `DiskManager::AllocatePage` (Reusing Free-List Pages Before File Extension)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-041
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::AllocatePage` (Reusing Free-List Pages Before File Extension)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-032 (`FreePage`), DAY-040 complete.

### Objective
Implement `Result<PageId> DiskManager::AllocatePage(PageType initial_type)` which pops from the freelist if `free_page_count > 0` or appends a newly zeroed, initialized page at the end of the file (`page_count++`), updating Page 0 `FileHeader` persistently.

### Concepts & Daily Learning
- **Concept:** Free-list LIFO pop before file extension, pre-initializing allocated pages on disk, and keeping Page 0 consistent with physical file length.
- **Why Database Engines Need It:** If `AllocatePage` increments `page_count` in Page 0 without writing the new 4KB page to disk (or writes the page without updating Page 0), `file_size == page_count * 4096` breaks on reopen!
- **How It Works:** Under `latch_`:
  - Branch A (freelist non-empty): Read `first_free_page_id`, validate `PageType::kFree` and extract `next_free`, format `page` with `initial_type`, write `page`, update `header_.first_free_page_id = next_free`, `--header_.free_page_count`, write `header_` to Page 0.
  - Branch B (freelist empty): `new_id = header_.page_count`, check overflow (`new_id == kInvalidPageId`), format `page` with `initial_type`, write `page` at `new_id * 4096`, `++header_.page_count`, write `header_` to Page 0.
- **Design Trade-offs:** Writing the new blank initialized page AND Page 0 during `AllocatePage` ensures `ReadPage(new_id)` immediately succeeds even before the caller writes data to it!
- **Common Mistakes:** Leaving the newly allocated page un-written on disk so `ReadPage(new_id)` gets EOF or old stale `kFree` header! Forgetting to flush/write `FileHeader` after `page_count++`.
- **Understanding Checkpoint:** Why does `AllocatePage` write an initialized page header (with `initial_type` and valid CRC32) to disk immediately upon allocation?

### Component Context
- **Why It Exists:** Supplies new persistent `PageId`s for `TableHeap`, `BPlusTree`, and `BufferPoolManager::NewPage`.
- **Module Relationships:** Uses `FreePageCodec` and `WriteHeaderLocked`; enables full `WritePage`/`ReadPage` roundtrip testing!

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_io_test.cpp` (add full `AllocatePage -> WritePage -> ReadPage` roundtrip tests!)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Result<PageId> DiskManager::AllocatePage(PageType initial_type)` (rejecting `kInvalid`, `kFileHeader`, and `kFree` as `initial_type`).
2. Add tests verifying sequential allocation (`1, 2, 3`), file size growing by `4096` bytes per page, writing custom payload via `WritePage` and reading it back cleanly via `ReadPage`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Allocating N pages grows file size to `(1 + N) * 4096` bytes; `WritePage -> ReadPage` payload roundtrips cleanly.

### Definition of Done
`AllocatePage` and `WritePage`/`ReadPage` roundtrip tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-041: implement DiskManager::AllocatePage and Write/Read roundtrips`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-042` (`DiskManager::DeallocatePage`).
````

---

## DAY-042 — Implement `DiskManager::DeallocatePage` (Pushing Freed Pages Safely onto Free List)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-042
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement `DiskManager::DeallocatePage` (Pushing Freed Pages Safely onto Free List)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-041 complete.

### Objective
Implement `Status DiskManager::DeallocatePage(PageId page_id)` pushing valid active pages onto the freelist head (`PageType::kFree`, wiping payload, updating `FileHeader`) so subsequent `AllocatePage` calls reuse them in LIFO order.

### Concepts & Daily Learning
- **Concept:** LIFO freelist push, zeroing freed payloads, and `FreePageCount` maintenance.
- **Why Database Engines Need It:** Reclaiming deleted B+ Tree and table heap pages stops unbounded disk growth without needing expensive immediate file compaction (`ftruncate`).
- **How It Works:** Validate `page_id` is in `[1, header_.page_count)`, read current page header to verify it is NOT already `kFree`, format page as `kFree` with `next_page_id = header_.first_free_page_id`, write page to disk, set `header_.first_free_page_id = page_id`, increment `header_.free_page_count`, and write updated `FileHeader` to Page 0.
- **Design Trade-offs:** Reading the target page before freeing costs 1 page read per deallocation, in exchange for deterministic double-free detection (`page_type == kFree`)!
- **Common Mistakes:** Failing to check if the page is already `kFree` before pushing it onto the freelist (which creates a self-cycle or corrupted `free_page_count`!).
- **Understanding Checkpoint:** Walk through how `DeallocatePage(2)` -> `DeallocatePage(4)` -> `AllocatePage()` -> `AllocatePage()` allocates `4` then `2` without extending the file size.

### Component Context
- **Why It Exists:** Completes page reclamation for `BufferPoolManager::DeletePage` and `DROP TABLE`.
- **Module Relationships:** Pairs symmetrically with `AllocatePage` (`DAY-041`).

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_alloc_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DiskManager::DeallocatePage(PageId page_id)`.
2. Write unit tests verifying: deallocated page's file bytes have zeroed payload (`[32..4095]`), `FreePageCount()` increments, subsequent `AllocatePage()` returns the freed `PageId` in LIFO order without increasing file size, and `ReadPage` on a freed page is rejected!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- File size remains constant while allocating/deallocating/reallocating freed pages.
- Freed pages cannot be read via `ReadPage` until reallocated.

### Definition of Done
`DeallocatePage` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-042: implement DiskManager::DeallocatePage and LIFO reuse`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-043` (Double-free & invalid PageId rejection).
````

---

## DAY-043 — Reject Double-Free, Page-0 Freeing, and Out-of-Range `PageId`s in `DiskManager`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-043
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Reject Double-Free, Page-0 Freeing, and Out-of-Range `PageId`s in `DiskManager`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-042 complete.

### Objective
Harden `DeallocatePage`, `AllocatePage`, `ReadPage`, and `WritePage` against double-frees, Page-0 deallocation, `kInvalidPageId`, out-of-range IDs, and corrupted freelist cycles (`ValidateFreeList` test/audit helper).

### Concepts & Daily Learning
- **Concept:** Double-free corruption (cycles in linked lists), sentinel protection, and structural freelist verification.
- **Why Database Engines Need It:** Double-freeing Page 3 makes Page 3 point to itself (`next_page_id = 3`) or corrupts `free_page_count`, causing the same page to be allocated twice to two different tables or an infinite loop!
- **How It Works:** `DeallocatePage` rejects `page_id == 0`, `kInvalidPageId`, `>= page_count`, and pages whose current `page_type == PageType::kFree`; additionally, `AllocatePage` rejects cycles (`next_free == popped_id` or `next_free >= page_count`), and a `VerifyFreeListIntegrity()` diagnostic method traverses the list checking visited set + exact count match.
- **Design Trade-offs:** Adding an optional diagnostic `VerifyFreeListIntegrity()` method gives tests and Phase 09 an O(free_pages) structural check without slowing down production O(1) allocation.
- **Common Mistakes:** Only rejecting double-free when the page is the current `first_free_page_id`, missing double-free of a page deeper in the chain!
- **Understanding Checkpoint:** Why does checking `page_type == PageType::kFree` inside `DeallocatePage` catch double-frees at ANY depth of the freelist, not just the head?

### Component Context
- **Why It Exists:** Ensures freelist invariants hold under bug/fault conditions.
- **Module Relationships:** Hardens `DiskManager` for `BufferPoolManager::DeletePage`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_alloc_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add `Status VerifyFreeListIntegrity() const` (or `VerifyFreeListIntegrity()` with lock) verifying visited set has no cycles, all nodes have `PageType::kFree`, and visited count == `free_page_count`.
2. Add tests attempting: double-free at head, double-free deep in chain (`free 1, free 2, free 3, free 1 again!`), freeing Page 0, freeing `kInvalidPageId`, and freeing `page_count + 5`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All invalid deallocations return an error `Status` without mutating `free_page_count` or `first_free_page_id`.

### Definition of Done
Double-free and cycle defenses verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-043: harden DiskManager against double-free and invalid PageIds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-044` (Truncated-file & torn-page tests).
````

---

## DAY-044 — Implement Truncated-File and Torn-Page Corruption Detection Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-044
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Truncated-File and Torn-Page Corruption Detection Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-043 complete.

### Objective
Write targeted fault-injection tests (`tests/unit/storage/disk_manager_corruption_test.cpp`) simulating real-world disk failures: truncated DB files, appended garbage bytes, torn 512-byte sectors inside data pages, swapped pages on disk, and corrupted freelist nodes.

### Concepts & Daily Learning
- **Concept:** Torn writes (partial 4096-byte page write across 512-byte physical disk sectors), file truncation, and misdirected sector writes.
- **Why Database Engines Need It:** Hardware disks write in 512B/4KB sectors; a crash during a 4096B write can leave the first 2KB new and the last 2KB old (torn page!).
- **How It Works:** After closing a valid multi-page `DiskManager`, tests open the raw `.tinydb` file via `std::fstream` / POSIX I/O, inject each specific fault (truncate by 100 bytes, flip bytes in Page 2 payload, swap raw bytes of Page 1 and Page 2, corrupt `next_page_id` of a free page), reopen `DiskManager`, and verify exact detection.
- **Design Trade-offs:** Fixing corruption on raw files in tests verifies the engine's actual on-disk contract rather than mocking `DiskManager`.
- **Common Mistakes:** Testing only in-memory buffers instead of actual bytes on the filesystem.
- **Understanding Checkpoint:** When Page 1 and Page 2 raw bytes are swapped on disk, why does `DiskManager::ReadPage(1)` still reject Page 1 even though its CRC32 matches its contents?

### Component Context
- **Why It Exists:** Proves `DiskManager` catches real filesystem corruption cleanly.
- **Module Relationships:** Tests `DiskManager`, `FileHeader`, `Page`, and `FreePage`.

### Files Expected to Change
- `tests/unit/storage/disk_manager_corruption_test.cpp` (new)
- `src/storage/disk_manager.cpp` (any edge-case refinement revealed by tests)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all 5 fault-injection tests in `disk_manager_corruption_test.cpp`.
2. Ensure zero crashes, zero leaked FDs, and `StatusCode::kCorruption` returned in every scenario.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Truncated DB, trailing-garbage DB, torn-page, swapped-page, and corrupt-freelist tests all reject with `kCorruption`.

### Definition of Done
All fault-injection tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-044: add truncated-file and torn-page DiskManager corruption tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-045` (Persistence across restarts).
````

---

## DAY-045 — Implement FileHeader and Free-List Persistence Tests Across Close/Reopen Restarts

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-045
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement FileHeader and Free-List Persistence Tests Across Close/Reopen Restarts
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-044 complete.

### Objective
Write restart-persistence unit tests (`tests/unit/storage/disk_manager_persistence_test.cpp`) proving that `page_count`, `free_page_count`, the entire free-page chain, and modified page payloads + LSNs survive multiple `Close -> Destroy DiskManager -> Construct Fresh DiskManager -> Open` cycles.

### Concepts & Daily Learning
- **Concept:** Cold-restart state reconstruction solely from on-disk bytes (no hidden static/global RAM state!).
- **Why Database Engines Need It:** If any metadata lived only in a `DiskManager` member variable without being serialized to Page 0 or data pages, a restart would silently lose pages or corrupt allocations.
- **How It Works:** Destroy the `DiskManager` object completely (`{ DiskManager dm; ... }`) between phases of a test, reopen from path, verify `PageCount()`, `FreePageCount()`, `VerifyFreeListIntegrity()`, `ReadPage()` payloads, `lsn`, and continue allocating from the persisted freelist.
- **Design Trade-offs:** Putting tests in distinct lexical scopes guarantees zero accidental reuse of in-memory state.
- **Common Mistakes:** Testing persistence on the SAME `DiskManager` instance without destroying and recreating it.
- **Understanding Checkpoint:** What bugs can testing persistence on a *newly constructed* `DiskManager` catch that calling `Close() -> Open()` on the *same* instance might miss?

### Component Context
- **Why It Exists:** Verifies full cold-restart persistence of Phase 01.
- **Module Relationships:** Exercises `DiskManager::Create/Open/Close/Allocate/Deallocate/Read/Write/FlushAll`.

### Files Expected to Change
- `tests/unit/storage/disk_manager_persistence_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement multi-restart persistence tests covering: (1) allocate 10 pages + write distinct byte patterns + LSNs -> restart -> read & verify all 10, (2) free 5 pages -> restart -> verify `FreePageCount() == 5` and allocate 5 pages reuses exact freed IDs without growing file size, (3) free all pages -> restart -> verify integrity.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All cold-restart persistence assertions pass with zero ASan/UBSan errors and temporary test files cleaned up via RAII.

### Definition of Done
Persistence tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-045: add cold-restart FileHeader and free-list persistence tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-046` (Multi-page stress & persistence tests).
````

---

## DAY-046 — Implement Multi-Page Allocate, Write, Deallocate, and Reuse Persistence Stress Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-046
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Implement Multi-Page Allocate, Write, Deallocate, and Reuse Persistence Stress Tests
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-045 complete.

### Objective
Write deterministic seeded randomized model-checking tests (`tests/unit/storage/disk_manager_stress_test.cpp`) running 1,000+ interleaved `AllocatePage`, `WritePage`, `ReadPage`, `DeallocatePage`, and `Close/Open` restart steps against an oracle `std::map<PageId, std::array<std::byte, 4064>>`.

### Concepts & Daily Learning
- **Concept:** Oracle/model-based randomized testing for storage engines.
- **Why Database Engines Need It:** Hand-written test cases only exercise sequences the author imagined; a deterministic PRNG (`std::mt19937`) discovers subtle state interaction bugs between allocations, frees, overwrites, and restarts.
- **How It Works:** Maintain a simple `std::unordered_map<PageId, Payload>` oracle of all currently live pages. On each step, pick randomly: allocate + fill deterministic payload, overwrite an existing live page, verify `ReadPage` matches oracle, deallocate a live page (verifying subsequent `ReadPage` fails), or restart `DiskManager` periodically.
- **Design Trade-offs:** Using a fixed seed (`seed = 202601`) makes stress tests 100% deterministic and reproducible in CI while sweeping hundreds of state combinations.
- **Common Mistakes:** Using `std::random_device` directly as seed in unit tests (producing non-reproducible CI failures!).
- **Understanding Checkpoint:** Why is comparing `DiskManager` against a simple in-memory `unordered_map` oracle with a fixed PRNG seed so effective for storage debugging?

### Component Context
- **Why It Exists:** Stress-validates `DiskManager` before `TableHeap` and `BufferPoolManager` rely on it.
- **Module Relationships:** Exercises all `DiskManager` operations under heavy churn.

### Files Expected to Change
- `tests/unit/storage/disk_manager_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement the oracle-based randomized stress test (`1,000` operations, periodic restarts every `150` steps, `VerifyFreeListIntegrity()` after every restart).
2. Also verify final live pages after reopening from scratch.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan. Confirm test runtime stays fast (`< 2s`).

### Acceptance Criteria
- 1,000-op stress test + restarts passes deterministically under ASan/UBSan with zero mismatches.

### Definition of Done
Stress suite passing quickly, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-046: add oracle-verified randomized DiskManager stress test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-047` (Disk I/O statistics instrumentation).
````

---

## DAY-047 — Add `DiskManager` I/O Statistics Counters (Reads, Writes, Flushes, Allocations)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-047
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Add `DiskManager` I/O Statistics Counters (Reads, Writes, Flushes, Allocations)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-046 complete.

### Objective
Instrument `DiskManager` with `DiskStats` (`page_reads`, `page_writes`, `header_writes`, `flushes`, `allocations`, `deallocations`, `freelist_reuses`) and `GetStats()`/`ResetStats()` for observability and Buffer Pool hit-ratio verification in Phase 04.

### Concepts & Daily Learning
- **Concept:** Engine observability and quantitative testing via I/O counters.
- **Why Database Engines Need It:** In Phase 04 (`BufferPoolManager`) and Phase 05 (`BPlusTree`), you cannot verify that caching actually reduced disk reads or that a B+ Tree search touched only 3 pages instead of 1,000 without exact I/O counters!
- **How It Works:** Increment `DiskStats` counters inside `DiskManager` under `latch_` only after successful syscalls, expose snapshot `GetStats()` by value, and `ResetStats()`.
- **Design Trade-offs:** Returning `DiskStats` by value under `latch_` avoids torn struct reads across threads with negligible cost.
- **Common Mistakes:** Incrementing `page_reads` before validation fails or counting `ReadPage` cache hits inside `BufferPoolManager` as disk reads.
- **Understanding Checkpoint:** How will `DiskManager::GetStats().page_reads` prove in Phase 04 that `BufferPoolManager` serves hot pages from RAM without hitting disk?

### Component Context
- **Why It Exists:** Enables quantitative I/O testing in Buffer Pool, B+ Tree, and Benchmarks.
- **Module Relationships:** Directly inspected by Phase 04, Phase 05, and Phase 10 tests/benchmarks.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`
- `src/storage/disk_manager.cpp`
- `tests/unit/storage/disk_manager_stats_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `struct DiskStats` and implement `GetStats()` / `ResetStats()` in `DiskManager`.
2. Verify exact counts for `Create`, `AllocatePage` (fresh vs reused!), `WritePage`, `ReadPage`, `DeallocatePage`, and `FlushAll`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `freelist_reuses` increments iff `AllocatePage` popped from the freelist; `page_reads` and `page_writes` match exact operations.

### Definition of Done
Stats instrumentation and tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-047: instrument DiskManager with DiskStats I/O counters`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-048` (ASan/UBSan audit & boundary hardening).
````

---

## DAY-048 — Run ASan/UBSan Audit and Boundary-Hardening Sweep on `Page` and `DiskManager`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-048
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Run ASan/UBSan Audit and Boundary-Hardening Sweep on `Page` and `DiskManager`
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-047 complete.

### Objective
Audit every line of `include/tinydb/storage/*` and `src/storage/*` for compiler warnings (`-Wconversion -Wsign-conversion -Wshadow -Werror`), integer overflow edges, thread-safety latch coverage, and edge-case tests (`tests/unit/storage/storage_boundary_test.cpp`).

### Concepts & Daily Learning
- **Concept:** Security-minded code auditing: implicit narrowing conversions, unclosed lock paths, and boundary edge testing.
- **Why Database Engines Need It:** Most subtle storage bugs sit at boundary conditions: `PageId = 0`, `PageId = UINT32_MAX - 1`, `free_space_offset == 32` vs `4096`, or silent implicit signed/unsigned conversions.
- **How It Works:** Inspect every `src/storage/*.cpp` function, check every arithmetic expression for overflow, ensure every public `DiskManager` method holds `latch_` cleanly, and write explicit regression tests for each checked edge case.
- **Design Trade-offs:** Fixing minor warnings and subtle conversions now prevents technical debt across 200+ future sessions.
- **Common Mistakes:** Calling one public `DiskManager` method that acquires `std::mutex latch_` from another public method that already holds `latch_` (instant self-deadlock!).
- **Understanding Checkpoint:** Why must `DiskManager` split public `Lock`-acquiring methods (`ReadPage`) from private `...Locked` helpers (`ReadPageLocked`)?

### Component Context
- **Why It Exists:** Hardens Phase 01 before Phase 02 (`Records & TableHeap`) builds atop it.
- **Module Relationships:** Audits `FileHeader`, `PageHeader`, `Page`, `FreePage`, `DiskManager`.

### Files Expected to Change
- `include/tinydb/storage/*.hpp`, `src/storage/*.cpp` (hardening fixes if any found)
- `tests/unit/storage/storage_boundary_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Review all `src/storage/` headers and sources for self-deadlock risks, narrowing casts, and missing error resets.
2. Write boundary tests covering `PageType` boundaries, `PageHeader::lsn == UINT64_MAX`, `free_space_offset == 32` and `4096`, and `DiskManager` re-opening after failed operations.
3. Build with `-DTINYDB_ENABLE_SANITIZERS=ON -DTINYDB_WARNINGS_AS_ERRORS=ON` and verify zero issues.

### Testing Instructions
- Full Debug + ASan/UBSan + Release test suites + `clang-format` check.

### Acceptance Criteria
- Zero compiler warnings with `-Werror`, zero ASan/UBSan errors, zero self-deadlocks.

### Definition of Done
Audit complete, boundary tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-048: audit storage module boundaries under ASan and UBSan`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-049` (Phase 01 sign-off).
````

---

## DAY-049 — Complete Phase 01 Integration Suite, Finalize `docs/storage-format.md`, and Sign Off Phase 01

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-049
- **Phase:** Phase 01 — Disk Manager and Page Format
- **Session Title:** Complete Phase 01 Integration Suite, Finalize `docs/storage-format.md`, and Sign Off Phase 01
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-048 complete.

### Objective
Write an integration test (`tests/integration/phase01_disk_storage_test.cpp`) validating complete Phase 01 requirements end-to-end, bring `docs/storage-format.md` into 100% alignment with implemented byte offsets and checksums, and sign off Phase 01 (`38.0 / 220.0` hours).

### Concepts & Daily Learning
- **Concept:** Specification-to-implementation synchronization and phase sign-off verification.
- **Why Database Engines Need It:** Documentation that drifts from actual on-disk bytes misleads developers and interviewers alike; verifying every documented byte offset against real test bytes ensures the spec is truth.
- **How It Works:** Inspect `docs/storage-format.md` against `FileHeader`, `PageHeader`, and `FreePage` constants; run an integration test creating a DB, allocating/writing/freeing/reusing pages, closing, inspecting raw file size/headers, reopening, and verifying all stats and contents.
- **Design Trade-offs:** Investing one dedicated session at phase boundaries guarantees a rock-solid base before building Phase 02.
- **Common Mistakes:** Marking a phase complete before testing clean Release + ASan builds and verifying docs match code byte-for-byte.
- **Understanding Checkpoint:** Summarize every byte of Page 0 and a generic data page in TinyDB from memory and explain how corruption is detected at each layer.

### Component Context
- **Why It Exists:** Gate checkpoint concluding Phase 01 (`24.0` hours).
- **Module Relationships:** Hands off a verified `DiskManager` + `Page` storage layer to Phase 02 (`Records & TableHeap`).

### Files Expected to Change
- `tests/integration/phase01_disk_storage_test.cpp` (new)
- `docs/storage-format.md` (sync Sections 1–3 with verified implementation)
- `CMakeLists.txt` (add integration test target if distinct or register cleanly in CTest)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Create `tests/integration/phase01_disk_storage_test.cpp` and register in `CMakeLists.txt`.
2. Audit `docs/storage-format.md` Sections 1–3 against `file_header.hpp` and `page_header.hpp`.
3. Run Debug, Release, and ASan+UBSan CTest suites.
4. Update `PROGRESS.md` (`38.0` completed hours, Phase 01 complete, Phase 02 active) and `NEXT_SESSION.md` -> `DAY-050`.

### Testing Instructions
- Full Debug, Release, and ASan+UBSan `ctest --output-on-failure` + `clang-format` check.

### Acceptance Criteria
- Every unit and integration test passes across Debug, Release, and ASan+UBSan; `docs/storage-format.md` matches code 100%.

### Definition of Done
Phase 01 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-049: complete Phase 01 disk storage integration tests and sign off`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 02 (`DAY-050` — `DataType` enum and type traits).
````
