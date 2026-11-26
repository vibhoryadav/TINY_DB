# Phase 09 — Security and Testing (`DAY-241` – `DAY-271`)

**Phase Budget:** 24.0 hours (28 × 45 min + 3 × 60 min = 31 sessions)
**Goal:** Conduct comprehensive security auditing, boundary & overflow verification, corrupt-file/page/catalog/B+ Tree/WAL fault suites, parser fuzzing, 50k-op B+ Tree stress testing, differential SQL testing against SQLite3, controlled crash matrix verification, ASan/UBSan/leak auditing, dependency/secret scanning, and regression test consolidation.

---

## DAY-241 — Conduct Security Threat Model and Attack Surface Audit

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-241
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Conduct Security Threat Model and Attack Surface Audit
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 08 complete (`DAY-240`).

### Objective
Document TinyDB v1's security threat model (`docs/security-audit.md`) cataloging untrusted input surfaces (SQL query text, `.tinydb` binary pages, `.cat` metadata file, `.wal` log file, CLI arguments/paths), security properties (bounds checking, integer overflow defense, memory safety, DoS resource limits, symlink/temp-file hygiene), and open findings checklist to drive `DAY-242`–`DAY-271`!

### Concepts & Daily Learning
- **Concept:** Threat modeling for local embedded/file-backed database engines (`untrusted SQL + untrusted DB/WAL/catalog files`).
- **Why Database Engines Need It:** Even embedded databases without a TCP server face serious risks when opening malformed/tampered `.db`/`.wal`/`.cat` files or running untrusted SQL queries (OOM, stack overflow, out-of-bounds read/write, infinite loops, symlink temp-file hijacking).
- **How It Works:** Map every entry point from user/filesystem into TinyDB, list invariants protecting each boundary, and inspect source code systematically to turn potential gaps into concrete regression tickets verified across Phase 09.
- **Design Trade-offs:** Treating on-disk `.tinydb`, `.cat`, and `.wal` bytes as *untrusted input* (just like network packets!) forces defensive parsing everywhere.
- **Common Mistakes:** Assuming disk files were always written by a clean version of TinyDB and skipping bounds validation on read!
- **Understanding Checkpoint:** Identify all 4 untrusted input boundaries entering TinyDB v1 and name the worst-case failure if each wasn't validated.

### Component Context
- **Why It Exists:** Drives structured security & QA hardening across Phase 09.
- **Module Relationships:** Audits all `src/` modules.

### Files Expected to Change
- `docs/security-audit.md` (new)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Inspect every `src/` directory and create `docs/security-audit.md` mapping assets, entry boundaries, invariants, and verification checklists for `DAY-242`–`DAY-270`.
2. Verify build/tests remain green.

### Testing Instructions
- Run `ctest --test-dir build --output-on-failure`.

### Acceptance Criteria
- `docs/security-audit.md` lists concrete auditable items across every subsystem.

### Definition of Done
Threat model documented, build green, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-241: document TinyDB v1 security threat model and audit checklist`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-242` (Integer overflow & bounds-checking audit).
````

---

## DAY-242 — Audit Integer Overflow and Bounds Checking Across All Storage, Index, and Recovery Layers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-242
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Audit Integer Overflow and Bounds Checking Across All Storage, Index, and Recovery Layers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-241 complete.

### Objective
Perform a targeted code audit across `src/core/`, `src/storage/`, `src/catalog/`, `src/index/`, and `src/recovery/` for unsigned wraparound (`offset + len <= size`), signed overflow, 32-bit `PageId * kPageSize` arithmetic, and `reserve()`/`resize()` from untrusted counts, adding targeted security unit tests in `tests/unit/security/overflow_bounds_test.cpp`.

### Concepts & Daily Learning
- **Concept:** Arithmetic wraparound vulnerabilities (`a + b < a`, `count * elem_size` overflow before `vector::resize`) and checked arithmetic helpers.
- **Why Database Engines Need It:** A classic CVE pattern in binary parsers is `if (offset + len <= buf.size())` overflowing `uint32_t`/`size_t` to wrap small (`5`), bypassing the check and causing heap buffer overflow!
- **How It Works:** Audit every addition/multiplication before buffer access or memory allocation; convert `offset + len <= total` everywhere to `offset <= total && len <= total - offset`, and cap counts against physical file/page remaining bytes *before* `reserve()`.
- **Design Trade-offs:** Writing `len <= total - offset` costs zero extra instructions and eliminates wraparound completely.
- **Common Mistakes:** Calling `vec.reserve(untrusted_count)` on a file claiming `1,000,000,000` entries inside a 100-byte file (`bad_alloc` / OOM DoS!).
- **Understanding Checkpoint:** How do you validate `untrusted_count` entries of `elem_size` bytes inside a remaining buffer of `remaining_bytes` without overflow OR OOM?

### Component Context
- **Why It Exists:** Eliminates arithmetic wraparound and untrusted-allocation DoS vectors.
- **Module Relationships:** Audits all binary decoders across TinyDB.

### Files Expected to Change
- `src/core/*.cpp`, `src/storage/*.cpp`, `src/catalog/*.cpp`, `src/index/*.cpp`, `src/recovery/*.cpp` (any wraparound/reserve hardenings)
- `tests/unit/security/overflow_bounds_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Grep/inspect all `reserve`, `resize`, `subspan`, `offset +`, and `* kPageSize` across `src/`.
2. Add unit tests passing `UINT32_MAX`, `SIZE_MAX`, and forged huge counts into every binary decoder under ASan+UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan (`-Werror`).

### Acceptance Criteria
- Every forged huge count/offset fails cleanly with `kCorruption`/`kOutOfRange` and zero OOM/UBSan alerts.

### Definition of Done
Overflow audit and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-242: audit and test integer overflow and untrusted allocation bounds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-243` (Corrupt DB file header & file-level testing).
````

---

## DAY-243 — Expand Corrupt Database File Rejection Suite (Headers, Truncations, Size Mismatches)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-243
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Expand Corrupt Database File Rejection Suite (Headers, Truncations, Size Mismatches)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-242 complete.

### Objective
Write comprehensive corrupt-file rejection tests (`tests/unit/security/corrupt_db_file_test.cpp`) verifying `Database::Open` and `DiskManager::Open` reject: 0-byte files, 1-byte/4095-byte files, non-DB text/binary files, wrong `format_version`, wrong `page_size` (`512`, `8192`), `page_count` mismatched with actual file bytes, corrupted Page 0 CRC32, and corrupted freelist chains—leaving zero leaked FDs or half-open handles!

### Concepts & Daily Learning
- **Concept:** Fail-closed startup validation and clean handle release on rejection.
- **Why Database Engines Need It:** Opening a corrupt or wrong file must never enter a partially-initialized engine state or leave stale lock/FD handles open.
- **How It Works:** Generate crafted `.tinydb` binaries for each failure mode, invoke `Database::Open` and `DiskManager::Open`, verify crisp error `Status`, and verify files remain untouched!
- **Design Trade-offs:** Rejecting corrupt files read-only without mutating user bytes preserves forensic recoverability.
- **Common Mistakes:** Modifying or truncating a user's file before confirming it actually has valid TinyDB magic bytes!
- **Understanding Checkpoint:** Why MUST `Database::Open` never modify `.tinydb` or `.wal` before verifying magic bytes and format headers?

### Component Context
- **Why It Exists:** Validates fail-closed file opening security.
- **Module Relationships:** Tests `DiskManager::Open` and `Database::Open`.

### Files Expected to Change
- `tests/unit/security/corrupt_db_file_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all corrupt DB file scenarios in `corrupt_db_file_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every corrupt DB file rejects cleanly with zero crashes, zero mutations to the bad file, and zero leaks.

### Definition of Done
Corrupt DB file suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-243: add exhaustive corrupt database file rejection tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-244` (Corrupt slotted-page & tuple tests).
````

---

## DAY-244 — Test Corrupt Slotted Pages, Overlapping Slots, and Crafted Tuple Payloads

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-244
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Test Corrupt Slotted Pages, Overlapping Slots, and Crafted Tuple Payloads
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-243 complete.

### Objective
Write adversarial slotted-page and tuple corruption tests (`tests/unit/security/corrupt_slotted_page_test.cpp`) injecting valid-CRC32 pages containing crafted corrupt `SlottedPage` / `Tuple` internals: `free_space_start > free_space_end`, `slot_count` exceeding page bytes, overlapping slot payload ranges, `offset` pointing into header (`< 40`), `offset + length > 4096`, `TableHeap` page-chain cycle (`page 1 -> page 2 -> page 1`), and crafted `Tuple` payloads (`bool = 9`, oversized `VARCHAR` len).

### Concepts & Daily Learning
- **Concept:** Defense-in-depth beyond CRC32 (what if disk corruption OR a crafted `.tinydb` file has a recomputed valid CRC32 but invalid internal invariants?!).
- **Why Database Engines Need It:** An attacker or a rogue tool could craft a `.tinydb` page with a valid CRC32 containing `slot.offset = 4090, slot.length = 500`! Structural invariants must still catch it without memory corruption!
- **How It Works:** Construct crafted `Page`s with valid `UpdateChecksum()`, test `SlottedPage::ValidateStructure`, `SlottedPage::GetTuple`, `TableHeap::GetTuple`, and `TableHeap::ScanIterator`!
- **Design Trade-offs:** Validating `SlottedPage` invariants on untrusted reads stops out-of-bounds memory access even with valid checksums!
- **Common Mistakes:** Assuming "CRC32 passed => every slot offset and tuple length inside `[32..4095]` is safe!"
- **Understanding Checkpoint:** Why is checking CRC32 necessary *yet insufficient* if a test/tool crafts an invalid `SlottedPage` with a matching CRC32?

### Component Context
- **Why It Exists:** Hardens intra-page record decoding against crafted pages.
- **Module Relationships:** Tests `SlottedPage`, `TupleCodec`, and `TableHeap`.

### Files Expected to Change
- `tests/unit/security/corrupt_slotted_page_test.cpp` (new)
- `src/storage/slotted_page.cpp`, `src/storage/table_heap.cpp` (harden any gap surfaced)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all crafted-CRC32 slotted-page and `TableHeap` cycle tests in `corrupt_slotted_page_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every crafted page/cycle rejects cleanly with `StatusCode::kCorruption` and zero ASan/UBSan violations.

### Definition of Done
Crafted slotted-page security suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-244: verify SlottedPage and TableHeap against crafted-CRC corruption`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-245` (Catalog integrity & corrupt `.cat` suite).
````

---

## DAY-245 — Test Catalog Integrity, Orphaned Temp Files, and Corrupt `.cat` Rejection

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-245
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Test Catalog Integrity, Orphaned Temp Files, and Corrupt `.cat` Rejection
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-244 complete.

### Objective
Write security tests (`tests/unit/security/corrupt_catalog_test.cpp`) verifying `.cat` resilience against: truncated `.cat`, huge `> 16 MiB` `.cat`, duplicate table/index names or IDs inside forged valid-CRC `.cat`, `next_table_id <= max_table_id` invariants, index referencing missing `table_id` or non-integer column, `first_page_id` pointing to `kFree` or `kBTreeLeaf` page in `.tinydb`, and stale `.cat.tmp` leftover recovery!

### Concepts & Daily Learning
- **Concept:** Cross-file catalog integrity checks (`Catalog <-> TableHeap <-> BPlusTree`) and atomic leftover temp-file hygiene.
- **Why Database Engines Need It:** If `.cat` and `.tinydb` ever get out of sync or `.cat` is tampered with, catching `next_table_id <= max_existing_id` or `index.column_type != INT/BIGINT` at `Load` time prevents runtime crashes or ID collisions later!
- **How It Works:** Add any missing semantic checks into `CatalogCodec::Deserialize` and `CatalogManager::Load`, and write unit tests exercising every crafted `.cat` anomaly!
- **Design Trade-offs:** Validating monotonic `next_table_id > max_id` and `first_page_id` page types at startup takes microseconds and secures catalog state completely.
- **Common Mistakes:** Only checking `table_count` bounds without checking `next_table_id > max(table.id)` or column index bounds on `IndexMetadata`!
- **Understanding Checkpoint:** What happens if a forged `.cat` file lists `table_id = 5` while `next_table_id = 3`, and why must `CatalogCodec::Deserialize` reject it?

### Component Context
- **Why It Exists:** Hardens `.cat` metadata security and cross-file validation.
- **Module Relationships:** Audits `CatalogCodec` and `CatalogManager::Load`.

### Files Expected to Change
- `src/catalog/catalog_codec.cpp`, `src/catalog/catalog_manager.cpp` (tighten cross-checks)
- `tests/unit/security/corrupt_catalog_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Harden `CatalogCodec` / `CatalogManager::Load` cross-checks (`next_id > max_id`, `index.column_index < table.ColumnCount()`, integer index type check, valid heap/index `PageType`).
2. Add all crafted `.cat` and leftover `.tmp` tests in `corrupt_catalog_test.cpp`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every forged or corrupted `.cat` rejects cleanly with `kCorruption`; valid `.cat` ignores orphaned `.cat.tmp` safely.

### Definition of Done
Catalog security suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-245: harden CatalogManager integrity checks and add crafted-cat tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-246` (B+ Tree pointer & runtime corruption tests).
````

---

## DAY-246 — Harden B+ Tree Runtime Pointer Validation Against Crafted Disk Pages

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-246
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Harden B+ Tree Runtime Pointer Validation Against Crafted Disk Pages
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-245 complete.

### Objective
Write security tests (`tests/unit/security/corrupt_btree_runtime_test.cpp`) proving runtime `BPlusTree` operations (`GetValue`, `Insert`, `Remove`, `RangeScan`)—not just `BPlusTreeValidator`!—safely reject crafted-CRC corrupt B+ Tree pages: invalid `child_page_id == 0`, child pointing to itself (`self-loop`), leaf `next_page_id` pointing to `kBTreeInternal` or `kTableHeap`, unsorted keys on load, and `parent_page_id` mismatch during `Insert`/`Remove`!

### Concepts & Daily Learning
- **Concept:** Runtime validation in tree operations vs offline-only validators.
- **Why Database Engines Need It:** `BPlusTreeValidator` runs in tests; in production, `GetValue`, `Insert`, and `Remove` run directly against disk pages—they must NEVER loop infinitely on a corrupted `parent_page_id` cycle or dereference an out-of-range child index!
- **How It Works:** Verify `BPlusTree` checks `ValidateLeaf` / `ValidateInternal` (lightweight bounds/type check!) on page decode, validates `parent_page_id != current_page_id` and depth/step caps when ascending `InsertIntoParent` / `CoalesceOrRedistribute`, and verify `ActivePinCount() == 0` on every corruption abort!
- **Design Trade-offs:** Checking `size <= max_size` and `IsDataPageId(child)` on every node access costs nanoseconds and prevents out-of-bounds memory corruption.
- **Common Mistakes:** Looping upward `while (!node.IsRoot())` via `parent_page_id` without checking `parent_page_id != node_id` or `depth <= kMaxTreeDepth`!
- **Understanding Checkpoint:** Why MUST ascending parent loops (`InsertIntoParent` / `CoalesceOrRedistribute`) ALSO bound step depth (`<= 32`), just like top-down `FindLeafPage`?

### Component Context
- **Why It Exists:** Hardens production B+ Tree query & mutation paths against malformed tree pages.
- **Module Relationships:** Hardens `BPlusTree` and `BPlusTreeIterator`.

### Files Expected to Change
- `src/index/bplus_tree.cpp`, `src/index/bplus_tree_iterator.cpp`
- `tests/unit/security/corrupt_btree_runtime_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit upward/downward/horizontal loops in `src/index/*.cpp` for cycle bounds and `PageType` checks.
2. Add tests in `corrupt_btree_runtime_test.cpp` asserting clean `kCorruption` rejection and `ActivePinCount() == 0`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Runtime B+ Tree methods reject every crafted anomaly cleanly without hangs, crashes, or pin leaks.

### Definition of Done
B+ Tree runtime pointer hardening and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-246: harden runtime BPlusTree pointer and cycle checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-247` (WAL integrity & forged-record tests).
````

---

## DAY-247 — Test WAL Integrity Validation and Crafted Log-Record Attacks

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-247
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Test WAL Integrity Validation and Crafted Log-Record Attacks
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-246 complete.

### Objective
Write security tests (`tests/unit/security/corrupt_wal_security_test.cpp`) testing crafted WAL payloads (even with valid CRC32!): `total_len = 0`, `total_len = 0xFFFFFFFF`, forged `kPageImage` containing a corrupt/mismatched `PageHeader` inside the 4KB image (`page_id != record.page_id`!), `record.page_id == 0` (`kFileHeader` overwrite attempt!), `lsn == 0`, duplicate/non-monotonic `lsn`, and `kIndexRoot` targeting non-existent `IndexId`!

### Concepts & Daily Learning
- **Concept:** Validating **semantics inside** WAL `PAGE_IMAGE` and `kIndexRoot` records before applying them to `.tinydb` or `.cat`!
- **Why Database Engines Need It:** If a crafted/corrupt WAL `kPageImage` carries `record.page_id = 0` or an embedded `PageHeader` whose `page_id = 99 != record.page_id` or invalid CRC32, blindly writing those 4096 bytes straight onto `.tinydb` would overwrite Page 0 FileHeader or inject corrupt pages onto disk!
- **How It Works:** Inside `RecoveryManager`: before `ApplyRecoveredPage(record.page_id, page)`, validate `IsDataPageId(record.page_id)` AND `page.Validate(record.page_id)` (checking internal page CRC32 and `page_id` match!) AND validate `lsn` strictly increasing!
- **Design Trade-offs:** Verifying the embedded 4KB page's own `Page::Validate(record.page_id)` inside `RecoveryManager` gives dual-checksum protection (WAL record CRC32 + inner Page CRC32!).
- **Common Mistakes:** Allowing a WAL `kPageImage` with `page_id = 0` to bypass `DiskManager`'s Page-0 protection!
- **Understanding Checkpoint:** Why should `RecoveryManager` validate `IsDataPageId(record.page_id)` AND `inner_page.Validate(record.page_id)` before writing a replayed WAL `PAGE_IMAGE` onto `.tinydb`?

### Component Context
- **Why It Exists:** Protects recovery replay from crafted or internally inconsistent WAL records.
- **Module Relationships:** Hardens `LogReader` and `RecoveryManager`.

### Files Expected to Change
- `src/recovery/recovery_manager.cpp`, `src/recovery/log_reader.cpp`
- `tests/unit/security/corrupt_wal_security_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Harden `RecoveryManager` checks on `kPageImage`, `kFreePage`, and `kIndexRoot`.
2. Add all crafted-WAL attack tests in `corrupt_wal_security_test.cpp`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every crafted WAL attack fails cleanly with `kCorruption` before altering a single byte of `.tinydb` or `.cat`.

### Definition of Done
WAL security hardening and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-247: harden WAL recovery validation against crafted log records`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-248` (Parser input limits & DoS hardening).
````

---

## DAY-248 — Harden SQL Parser Resource Limits Against Pathological Inputs

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-248
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Harden SQL Parser Resource Limits Against Pathological Inputs
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-247 complete.

### Objective
Write targeted DoS & pathological input tests (`tests/unit/security/parser_dos_test.cpp`) throwing giant queries, huge comment blocks, deeply nested parentheses (`((((...`), deeply chained `AND` trees, massive `INSERT` value lists, and 10,000-byte strings against `Lexer`, `Parser`, AND `ExecutionEngine::ExecuteSql` + `CliSession` REPL buffering!

### Concepts & Daily Learning
- **Concept:** End-to-end input bounding across CLI line buffer -> `Lexer` -> `Parser` -> `AST` teardown.
- **Why Database Engines Need It:** Even if `Parser` rejects `65537`-byte queries, what if a user pipes a 100 MB single line into `CliSession` (`std::getline`) without bounding `sql_buffer_` size?! Bounding `CliSession::sql_buffer_` too prevents memory exhaustion at the CLI boundary!
- **How It Works:** Enforce `sql_buffer_.size() <= config::kMaxSqlQueryBytes` inside `CliSession` while accumulating multi-line input (clearing buffer cleanly on overflow!), verify `DepthGuard` + `kMaxAndTerms`, and test all limits under ASan+UBSan!
- **Design Trade-offs:** Rejecting oversized multi-line buffers early in `CliSession` keeps interactive memory bounded.
- **Common Mistakes:** Checking `kMaxSqlQueryBytes` in `Lexer` while letting `CliSession`'s multi-line loop append gigabytes before calling `ExecuteSql`!
- **Understanding Checkpoint:** Why must `CliSession` enforce `kMaxSqlQueryBytes` during line accumulation rather than waiting for `;` to arrive?

### Component Context
- **Why It Exists:** Protects both engine and CLI against resource exhaustion inputs.
- **Module Relationships:** Hardens `CliSession`, `Lexer`, and `Parser`.

### Files Expected to Change
- `src/cli/cli_session.cpp` (bounded `sql_buffer_` check)
- `tests/unit/security/parser_dos_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add buffer cap check in `CliSession` multi-line accumulation.
2. Add pathological DoS tests across CLI, Lexer, and Parser under ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Pathological oversized/deep inputs reject immediately with zero OOM or stack overflow under ASan.

### Definition of Done
DoS bounds verified across CLI and Parser, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-248: enforce CLI buffer caps and pathological SQL DoS tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-249` (50,000-input parser fuzz campaign).
````

---

## DAY-249 — Run 50,000-Input Structured + Random Parser Fuzz Campaign

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-249
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run 50,000-Input Structured + Random Parser Fuzz Campaign
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-248 complete.

### Objective
Upgrade `tests/stress/parser_fuzz_test.cpp` into a high-coverage 50,000-input grammar-mutating + byte-mutating fuzz campaign covering ALL Phase 06 + Phase 08 SQL constructs (`CREATE TABLE/INDEX`, `DROP`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`, `BEGIN/COMMIT/ROLLBACK`), executing mutated valid queries directly against `Database::ExecuteSql` too (fuzzing semantic binding + planner + executor error paths!) under ASan+UBSan!

### Concepts & Daily Learning
- **Concept:** Full-pipeline SQL fuzzing (fuzzing not only `Parser`, but also valid-syntax / invalid-semantics queries hitting `Planner` + `ExecutionEngine`!).
- **Why Database Engines Need It:** Many database bugs lie just *past* the parser—e.g., syntactically valid SQL like `UPDATE t SET a = 'x' WHERE (1 = 1) AND (b < -9223372036854775808)` crashing `Planner` or `ExprEvaluator`!
- **How It Works:** Mutate both strings/tokens (30,000 inputs into `Parser::ParseAll`) AND valid-syntax random AST/SQL combinations against a live `Database` instance (20,000 statements with random valid/invalid column names, literal types, operators, and numbers!).
- **Design Trade-offs:** Splitting 30k parse-only + 20k full-execution statements keeps runtime fast (~2–3 seconds) while testing all frontend & planner paths!
- **Common Mistakes:** Only fuzzing the lexer/parser and leaving `Planner`/`ExprEvaluator` unfuzzed against weird valid-syntax expressions!
- **Understanding Checkpoint:** Why does fuzzing *syntactically valid but semantically weird* SQL (`SELECT * FROM t WHERE 5 < 10 AND 'a' = 1`) expose bugs that raw byte fuzzing never reaches?

### Component Context
- **Why It Exists:** High-volume stability verification for SQL frontend and planner.
- **Module Relationships:** Fuzzes `Lexer`, `Parser`, `Planner`, `ExprEvaluator`, and `ExecutionEngine`.

### Files Expected to Change
- `tests/stress/parser_fuzz_test.cpp`
- `src/parser/*.cpp`, `src/execution/*.cpp` (fix any edge case uncovered)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Expand `parser_fuzz_test.cpp` with 50,000 combined parse + execution fuzz cases across seeded PRNG runs.
2. Run under Debug + ASan/UBSan and Release.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 50,000 fuzzed inputs complete cleanly with zero crashes, zero leaks, and zero UBSan violations.

### Definition of Done
50k SQL fuzz campaign passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-249: run 50k-input SQL parser and planner fuzz campaign`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-250` (Buffer Pool memory ownership & pin-leak stress).
````

---

## DAY-250 — Stress-Test Buffer Pool Ownership and Pin Accounting Under Mixed Faults

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-250
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Stress-Test Buffer Pool Ownership and Pin Accounting Under Mixed Faults
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-249 complete.

### Objective
Write `tests/stress/buffer_pool_ownership_stress_test.cpp` running 10,000+ rapid `NewPageGuarded`, `FetchPageRead`, `FetchPageWrite`, move-assign swaps, `MarkUncommittedHold`/`Release`, `DeletePage` (including rejecting pinned deletes!), intentional disk faults, and tiny-pool (`pool_size = 3`) thrashing under ASan+LSan+UBSan, verifying `ActivePinCount() == 0` and exact frame accounting at every checkpoint!

### Concepts & Daily Learning
- **Concept:** Memory ownership invariant verification (`FreeFrames + CachedPages == PoolSize`, `ActivePinCount == 0`) under faulted churn.
- **Why Database Engines Need It:** A single frame leaked during an error path (`DeletePage` failing or `ReadPage` failing) accumulates silently over long uptimes until the pool deadlocks.
- **How It Works:** Exercise interleaved valid and invalid requests (fetching invalid IDs, deleting pinned frames, overwriting `PageGuard`s, toggling No-Steal holds) across 10,000 steps while asserting pool frame conservation invariants continuously!
- **Design Trade-offs:** Checking exact `FreeFrameCount() + CachedPageCount() == PoolSize()` catches 1-frame leaks immediately!
- **Common Mistakes:** Only checking `ActivePinCount()` at teardown instead of verifying frame conservation during active churn.
- **Understanding Checkpoint:** What invariants must ALWAYS hold on `BufferPoolManager` between operations (`FreeFrameCount`, `CachedPageCount`, `ActivePinCount`, `LRUReplacer::Size`)?

### Component Context
- **Why It Exists:** High-churn verification of Buffer Pool resource ownership.
- **Module Relationships:** Stress-tests `BufferPoolManager` + `PageGuard` + `LRUReplacer`.

### Files Expected to Change
- `tests/stress/buffer_pool_ownership_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement 10,000-op mixed valid/faulted buffer pool ownership stress test.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Frame conservation and `ActivePinCount() == 0` hold across all 10,000 ops under ASan/LSan.

### Definition of Done
Buffer pool ownership stress suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-250: add 10k-op BufferPool ownership and fault stress suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-251` (50,000-op multi-seed B+ Tree stress suite).
````

---

## DAY-251 — Run 50,000-Op Multi-Seed Randomized B+ Tree Stress and Invariant Suite

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-251
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run 50,000-Op Multi-Seed Randomized B+ Tree Stress and Invariant Suite
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-250 complete.

### Objective
Build a multi-seed 50,000-op randomized B+ Tree stress test (`tests/stress/bplus_tree_50k_stress_test.cpp`) covering 5 distinct seeds, even/odd node sizes (`4, 5, 7`) + production 4KB nodes, duplicate-heavy and unique workloads, tiny buffer pool eviction (`pool_size = 5`), cold restarts mid-workload, `RangeScan` equivalence, and `BPlusTreeValidator::Validate`!

### Concepts & Daily Learning
- **Concept:** Deep multi-seed randomized invariant checking bridging small-order and full 4KB trees under tight memory.
- **Why Database Engines Need It:** Certain edge cases (like a height-4 internal merge immediately following a duplicate-key root split on a 5-frame buffer pool!) require tens of thousands of mixed operations to stumble upon.
- **How It Works:** Run 5 seeds × 10,000 ops (including periodic full `RangeScan` and `Validate` checks plus cold restarts), checking `std::set<IndexEntryKey>` oracle equivalence after every batch!
- **Design Trade-offs:** Keeping 5 deterministic seeds gives wide state-space exploration while staying 100% reproducible when debugging!
- **Common Mistakes:** Only testing in Release (slowing CI under Debug+ASan? Keep `Validate` cadence smart, e.g., every 100 ops, so 50k ops run cleanly in a few seconds even under ASan!).
- **Understanding Checkpoint:** Why is running randomized B+ Tree stress across BOTH `order = 4/5` AND full 4KB capacity necessary?

### Component Context
- **Why It Exists:** High-confidence stress proof for Phase 05 indexing under Phase 09 QA.
- **Module Relationships:** Exercises `BPlusTree`, `BPlusTreeIterator`, `BPlusTreeValidator`, and `BufferPoolManager`.

### Files Expected to Change
- `tests/stress/bplus_tree_50k_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `bplus_tree_50k_stress_test.cpp` across 5 seeds and multiple configs.
2. Run under Debug + ASan/UBSan and Release!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 50,000+ ops pass across all seeds and configs with zero validator, oracle, or sanitizer failures.

### Definition of Done
50k B+ Tree stress test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-251: add 50k-op multi-seed B+ Tree stress and validator suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-252` (Differential SQL testing harness vs SQLite3).
````

---

## DAY-252 — Build Differential SQL Testing Harness Against SQLite3

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-252
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Build Differential SQL Testing Harness Against SQLite3
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-251 complete (`sqlite3` verified installed on host!).

### Objective
Build a reusable differential testing harness (`tests/integration/sqlite_diff_harness.hpp`, `src/test_utils` or test-only helper) that executes identical SQL statements (within TinyDB's supported subset!) against BOTH TinyDB AND `/usr/bin/sqlite3` (using `sqlite3` CLI or `libsqlite3` if cleanly linked—checking CMake `find_package(SQLite3)` or invoking `/usr/bin/sqlite3 -batch -csv` cleanly via pipe!), normalizing row order for `SELECT` comparisons, and asserting identical results!

### Concepts & Daily Learning
- **Concept:** **Differential testing** (comparing our database implementation against an industry gold-standard oracle like SQLite3 on identical SQL inputs!).
- **Why Database Engines Need It:** Writing expected answers manually only catches discrepancies you thought of; comparing TinyDB directly against SQLite3 on hundreds of SQL statements automatically uncovers subtle edge cases in comparisons, escapes, boolean handling, `WHERE` logic, and `UPDATE`/`DELETE` behavior!
- **How It Works:** Check `find_package(SQLite3)` (macOS ships `<sqlite3.h>` and `/usr/lib/libsqlite3.tbd`! If `SQLite3_FOUND`, link test-only target cleanly—zero production dependency in `tinydb` binary! Otherwise skip or invoke `sqlite3` CLI!). Using `SQLite3` C API in tests (guarded by `find_package(SQLite3 QUIET)`!) runs in-memory `:memory:` comparisons in microseconds with zero subprocess overhead!
- **Design Trade-offs:** Linking `SQLite::SQLite3` ONLY to a dedicated differential test executable keeps TinyDB itself 100% dependency-free while delivering ultra-fast differential oracle testing! Note documented semantic differences clearly (e.g., TinyDB enforces strict `INT`/`VARCHAR` types while SQLite uses dynamic affinity—so differential tests stick to strictly typed valid v1 SQL!).
- **Common Mistakes:** Linking `sqlite3` into `tinydb_core` (never!), or forgetting SQLite treats `TRUE`/`FALSE` as `1`/`0` in output (normalize bool rendering `"true"`/`"false"` vs `"1"`/`"0"` cleanly in the comparison layer!).
- **Understanding Checkpoint:** Why is dynamic typing in SQLite ("type affinity") an important difference to account for when writing differential tests between TinyDB (strict types) and SQLite3?

### Component Context
- **Why It Exists:** Oracle testing framework for `DAY-253`–`DAY-255`.
- **Module Relationships:** Test-only harness comparing `Database` vs SQLite3.

### Files Expected to Change
- `CMakeLists.txt` (`find_package(SQLite3 QUIET)` for test-only diff target!)
- `tests/integration/sqlite_diff_harness.hpp` (new)
- `tests/integration/sqlite_diff_smoke_test.cpp` (new)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `find_package(SQLite3 QUIET)` (fallback: `/usr/bin/sqlite3` or skip cleanly if absent) in `CMakeLists.txt` ONLY for `tinydb_diff_tests`.
2. Implement `SqliteOracle` + `AssertQueriesMatch(tinydb, sqlite, sql, sort_rows)` normalizing booleans (`1/0` <-> `true/false`) and sorting unordered `SELECT` rows.
3. Add smoke test verifying `CREATE TABLE + INSERT + SELECT` matches SQLite3!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Differential harness compiles cleanly, runs SQL against both engines, and reports exact mismatches if forced.

### Definition of Done
Differential harness built and smoke-tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-252: build SQLite3 differential SQL testing harness`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-253` (Differential SQL tests: DDL, `INSERT`, `SELECT`, `WHERE`).
````

---

## DAY-253 — Run Differential SQL Tests Against SQLite3: DDL, `INSERT`, `SELECT`, and `WHERE`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-253
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Differential SQL Tests Against SQLite3: DDL, `INSERT`, `SELECT`, and `WHERE`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-252 complete.

### Objective
Write differential test suites (`tests/integration/sqlite_diff_select_test.cpp`) verifying TinyDB matches SQLite3 identically across: `CREATE TABLE`, single/multi-row `INSERT`, reordered column `INSERT`, `SELECT *`, `SELECT col2, col1`, compound `WHERE` (`=`, `!=`, `<>`, `<`, `<=`, `>`, `>=`, `AND`, parentheses), string comparisons, negative integer boundaries, and indexed vs unindexed tables!

### Concepts & Daily Learning
- **Concept:** Cross-engine query semantics equivalence (`TinyDB(seq) == TinyDB(indexed) == SQLite3`).
- **Why Database Engines Need It:** Catches subtle comparison precedence, `<>` vs `!=`, lexicographical `VARCHAR` comparison, or empty-range `IndexScan` discrepancies against a mature SQL engine!
- **How It Works:** Run 40+ curated SQL queries covering tricky `WHERE` predicates (inclusive/exclusive bounds, contradictions `a > 5 AND a < 3`, negative numbers, empty strings `''`, escaped quotes `'it''s'`) against TinyDB (both with and without `CREATE INDEX`) AND SQLite3!
- **Design Trade-offs:** Testing both unindexed and indexed TinyDB instances against SQLite3 simultaneously catches BOTH general SQL bugs AND index-planner bugs!
- **Common Mistakes:** Passing queries relying on SQLite quirks (like comparing string `'10' > 2`) instead of well-typed v1 SQL.
- **Understanding Checkpoint:** If TinyDB `SELECT * FROM t WHERE name > 'b'` differs from SQLite3, what bug could cause it (e.g., signed `char` vs `unsigned char` comparison or escape decoding)?

### Component Context
- **Why It Exists:** Validates query semantics of `CREATE`/`INSERT`/`SELECT`/`WHERE` against SQLite3.
- **Module Relationships:** Tests `Parser`, `Planner`, `ExprEvaluator`, `SeqScan`, `IndexScan`, `Filter`, `Projection`.

### Files Expected to Change
- `tests/integration/sqlite_diff_select_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `sqlite_diff_select_test.cpp` comparing TinyDB (unindexed + indexed) vs SQLite3 across all query patterns.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 100% of queries return identical sorted multisets across TinyDB (unindexed), TinyDB (indexed), and SQLite3.

### Definition of Done
Differential `SELECT`/`WHERE` tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-253: verify INSERT, SELECT, and WHERE against SQLite3 oracle`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-254` (Differential SQL tests: `UPDATE`, `DELETE`, `CREATE INDEX`, transactions).
````

---

## DAY-254 — Run Differential SQL Tests Against SQLite3: `UPDATE`, `DELETE`, `CREATE INDEX`, and Transactions

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-254
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Differential SQL Tests Against SQLite3: `UPDATE`, `DELETE`, `CREATE INDEX`, and Transactions
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-253 complete.

### Objective
Write `tests/integration/sqlite_diff_dml_test.cpp` comparing TinyDB (with multiple B+ Tree indexes) against SQLite3 across interleaved `INSERT`, `CREATE INDEX` (on populated tables), `UPDATE` (multi-column, `WHERE`-filtered, unconditional, `VARCHAR` growth), `DELETE`, `DROP TABLE`, AND `BEGIN ... COMMIT` / `BEGIN ... ROLLBACK` sequences!

### Concepts & Daily Learning
- **Concept:** Stateful mutation + transaction differential testing (`affected_rows` AND resulting `SELECT *` state match SQLite3 after every statement!).
- **Why Database Engines Need It:** `UPDATE` affected-row counts, `ROLLBACK` state restoration, and multi-index synchronization bugs surface immediately when checked step-by-step against SQLite3!
- **How It Works:** Execute identical multi-step scripts (`BEGIN; INSERT ...; UPDATE ...; ROLLBACK; SELECT *; BEGIN; DELETE ...; COMMIT; SELECT *;`) on both engines, comparing BOTH affected row counts AND final sorted `SELECT *` rows!
- **Design Trade-offs:** Comparing `sqlite3_changes()` vs TinyDB `affected_rows` verifies mutation-count semantics match standard SQL!
- **Common Mistakes:** Confusing `UPDATE t SET a = 1 WHERE a = 1` affected row count (standard SQL & SQLite count matched rows updated, `1`, even if value stayed `1`!).
- **Understanding Checkpoint:** In SQL, if 3 rows match `WHERE status = 1` and you run `UPDATE t SET status = 1 WHERE status = 1`, what does `affected_rows` equal?

### Component Context
- **Why It Exists:** Validates mutation and transaction equivalence against SQLite3.
- **Module Relationships:** Tests `UpdateExecutor`, `DeleteExecutor`, `TransactionManager`, `BPlusTree`.

### Files Expected to Change
- `tests/integration/sqlite_diff_dml_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `sqlite_diff_dml_test.cpp` checking `affected_rows` and `SELECT *` parity across `UPDATE`, `DELETE`, `CREATE INDEX`, and `BEGIN`/`COMMIT`/`ROLLBACK`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All mutation counts, final tables, and `ROLLBACK`/`COMMIT` states match SQLite3 100%, with `BPlusTreeValidator` passing on every TinyDB index.

### Definition of Done
Differential DML/TCL tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-254: verify UPDATE, DELETE, CREATE INDEX, and transactions vs SQLite3`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-255` (Randomized differential SQL generator vs SQLite3).
````

---

## DAY-255 — Implement Randomized Differential SQL Generator Testing TinyDB vs SQLite3

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-255
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Implement Randomized Differential SQL Generator Testing TinyDB vs SQLite3
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-254 complete.

### Objective
Write a seeded randomized SQL generator (`tests/stress/sqlite_random_diff_test.cpp`) generating 1,000+ random valid v1 SQL statements (`INSERT`, `SELECT` with random compound `WHERE` + projections, `UPDATE`, `DELETE`, `BEGIN`/`COMMIT`/`ROLLBACK`, plus `CREATE INDEX` mid-stream!), running every statement identically on BOTH TinyDB and SQLite3, asserting 100% parity AND running `BPlusTreeValidator` periodically!

### Concepts & Daily Learning
- **Concept:** Grammar-driven random SQL differential testing ("SQLsmith-style" equivalence testing).
- **Why Database Engines Need It:** Random combinations of `IndexScan` + residual `Filter` + `Projection` + `UPDATE` + `ROLLBACK` uncover subtle interaction bugs that hand-written tests never imagine!
- **How It Works:** Maintain schema metadata in the generator; generate random well-typed predicates (`(col op lit) AND (col2 op2 lit2)`), random `UPDATE`s, random `DELETE`s, and random `COMMIT`/`ROLLBACK` blocks (across 3 seeds × 400 statements = 1,200+ statements!). Verify every `SELECT` and every `affected_rows` matches SQLite3 byte-for-byte!
- **Design Trade-offs:** Generating well-typed literals matching column types keeps differential testing focused on real semantic bugs rather than intentional strict-type differences!
- **Common Mistakes:** Including `OR` or `NULL` or un-parenthesized undefined behavior in the SQL generator!
- **Understanding Checkpoint:** Why is a grammar-driven random SQL generator paired with SQLite3 one of the highest-ROI testing techniques in database engineering?

### Component Context
- **Why It Exists:** High-coverage automated SQL equivalence oracle.
- **Module Relationships:** Exercises the entire SQL, Planner, Execution, B+ Tree, and Transaction stack against SQLite3.

### Files Expected to Change
- `tests/stress/sqlite_random_diff_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `sqlite_random_diff_test.cpp` (1,200+ generated SQL statements across 3 seeds, periodic cold restarts & `BPlusTreeValidator` checks).
2. Run under Debug + ASan/UBSan and Release.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 1,200+ generated SQL statements match SQLite3 100% across results, row counts, and restarts.

### Definition of Done
Randomized SQL differential suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-255: add randomized SQL generator differential tests against SQLite3`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-256` (Controlled crash testing matrix verification).
````

---

## DAY-256 — Execute Complete Controlled Crash Testing Matrix (`SIGKILL` + Instrumentation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-256
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Execute Complete Controlled Crash Testing Matrix (`SIGKILL` + Instrumentation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-255 complete.

### Objective
Build `tests/recovery/crash_matrix_test.cpp` combining REAL POSIX `SIGKILL` (`kill(pid, SIGKILL)`) on live `tinydb` subprocesses / forked workers AND every instrumented `CrashPoint` (`kBeforeCommitWalFlush`, `kAfterCommitWalFlush`, `kDuringBufferFlush`, `kDuringRecovery`) across multi-table + multi-index workloads, documenting verified outcomes in `docs/security-audit.md`.

### Concepts & Daily Learning
- **Concept:** Unmaskable `SIGKILL` subprocess killing vs deterministic crash hooks.
- **Why Database Engines Need It:** `SIGKILL` tests real OS-level pipe/process death (`tinydb` binary receiving `SIGKILL` mid-script!) alongside deterministic hook timing inside exact flush/commit windows!
- **How It Works:** Spawn `./tinydb` in child process feeding a continuous SQL stream (`CREATE` -> committed txns -> ongoing uncommitted work), wait for an explicit synchronization marker on stdout (e.g., `"COMMIT_DONE_1"` or sentinel `SELECT`), send `SIGKILL`, reopen DB with `./tinydb`, and verify committed rows survived and uncommitted rows did not!
- **Design Trade-offs:** Combining deterministic `CrashPoint` tests (exact sub-millisecond timing) with `SIGKILL` subprocess tests (real compiled `tinydb` binary) covers both exact boundaries and real OS signals!
- **Common Mistakes:** Sleeping blindly (`sleep(1)`) in subprocess crash tests (flaky under CI load!) instead of synchronizing on an exact flushed stdout sentinel line before sending `SIGKILL`!
- **Understanding Checkpoint:** Why is synchronizing on a flushed stdout response line before sending `SIGKILL` deterministic whereas `usleep(10000)` is flaky?

### Component Context
- **Why It Exists:** Completes Phase 09's controlled crash verification requirement.
- **Module Relationships:** Exercises `tinydb` binary + `RecoveryManager` under `SIGKILL`.

### Files Expected to Change
- `tests/recovery/crash_matrix_test.cpp` (new)
- `docs/security-audit.md` (record crash matrix verification)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement synchronized `SIGKILL` crash tests on the `tinydb` binary + instrumented matrix checks in `crash_matrix_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every `SIGKILL` and instrumented crash case converges cleanly with zero lost committed transactions and zero resurrected uncommitted rows.

### Definition of Done
Crash matrix verified and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-256: verify full controlled SIGKILL and instrumented crash matrix`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-257` (ASan & LeakSanitizer full-suite audit).
````

---

## DAY-257 — Run Full-Suite AddressSanitizer and Leak Detection (`LSan`) Audit

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-257
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Full-Suite AddressSanitizer and Leak Detection (`LSan`) Audit
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-256 complete.

### Objective
Execute the ENTIRE TinyDB test suite (unit, integration, stress, fuzz, differential, recovery, CLI subprocess) under AddressSanitizer + leak detection (`ASAN_OPTIONS=detect_leaks=1` where supported on platform / macOS `leaks` check if LSan isn't default on Apple Clang!), verify zero heap/stack OOB, zero use-after-free, and zero memory leaks across normal and error paths!

### Concepts & Daily Learning
- **Concept:** Memory-leak detection (`LSan` on Linux, macOS `leaks --atExit` / ASan LSan support on arm64 macOS) and verifying ASan flags actually execute across every binary!
- **Why Database Engines Need It:** Long-running database processes leak memory silently when error paths drop pointers or cyclic `shared_ptr`s form between `CatalogManager` and `TableHeap`/`BPlusTree`!
- **How It Works:** Inspect whether Apple Clang arm64 supports `ASAN_OPTIONS=detect_leaks=1` or macOS `leaks -atExit -- ./build-asan/tinydb_unit_tests`; verify zero cycles (`shared_ptr` vs `unique_ptr`), test `Database` open/error/close loops (`100` cycles) checking RSS growth & zero leaks, and log exact observed command results in `docs/security-audit.md`.
- **Design Trade-offs:** Checking `leaks` on macOS AND keeping `detect_leaks` enabled in Linux CI ensures full cross-platform leak safety.
- **Common Mistakes:** Assuming `-fsanitize=address` on macOS always enables LSan automatically without checking!
- **Understanding Checkpoint:** How do `std::shared_ptr` reference cycles occur in C++, and how do we ensure `CatalogManager`, `TableHeap`, and `BPlusTree` never form a `shared_ptr` cycle?

### Component Context
- **Why It Exists:** Verifies memory & leak safety across the entire test suite.
- **Module Relationships:** Audits all TinyDB libraries and binaries.

### Files Expected to Change
- `tests/unit/security/leak_cycle_test.cpp` (new — 100-iteration open/error/close cycle test)
- `docs/security-audit.md` (record observed ASan/leak check output)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add `leak_cycle_test.cpp` running 100 iterations of `Create`, error `Open`, CRUD + `CREATE INDEX` + `DROP`, and `Close`.
2. Run full ASan test suite + `leaks` check (observing actual platform tool support honestly!) and record exact results in `docs/security-audit.md`.

### Testing Instructions
- Full ASan `ctest --output-on-failure` + `leaks` verification.

### Acceptance Criteria
- Zero ASan errors and zero detected leaks across all tests and 100-cycle lifecycle stress.

### Definition of Done
ASan/leak audit verified and recorded, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-257: audit full suite with ASan and memory leak detection`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-258` (UBSan full-suite audit).
````

---

## DAY-258 — Run Full-Suite UndefinedBehaviorSanitizer (`UBSan`) Audit

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-258
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Full-Suite UndefinedBehaviorSanitizer (`UBSan`) Audit
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-257 complete.

### Objective
Run the ENTIRE TinyDB suite with strict UBSan flags (`-fsanitize=undefined -fno-sanitize-recover=all` so ANY runtime UB immediately aborts with exact stacktrace rather than silently continuing!), testing signed shifts, misaligned access, null-pointer dereferences, enum bounds, and signed overflow!

### Concepts & Daily Learning
- **Concept:** `-fno-sanitize-recover=all` making every UB trap fatal in tests, plus signed-vs-unsigned UB hazards in C++20.
- **Why Database Engines Need It:** By default, `-fsanitize=undefined` prints a warning to stderr and *keeps running*, which CTest can treat as passing if the test returned 0! Adding `-fno-sanitize-recover=all` guarantees ANY UB fails the test immediately!
- **How It Works:** Add `-fno-sanitize-recover=all` to `CMakeLists.txt` sanitizer flags (or verify!), compile & run every single CTest suite, fix any latent warning/UB if triggered, and document verification in `docs/security-audit.md`.
- **Design Trade-offs:** Making UBSan non-recoverable in Debug/CI catches silent warnings immediately.
- **Common Mistakes:** Leaving UBSan in default recoverable mode where warnings scroll past unnoticed in passing CTest runs!
- **Understanding Checkpoint:** Why should `-fno-sanitize-recover=all` ALWAYS accompany `-fsanitize=undefined` in CMake sanitizer builds?

### Component Context
- **Why It Exists:** Guarantees zero C++ Undefined Behavior across the entire codebase.
- **Module Relationships:** Tightens `CMakeLists.txt` sanitizer flags and CI safety.

### Files Expected to Change
- `CMakeLists.txt` (add `-fno-sanitize-recover=all` under `TINYDB_ENABLE_SANITIZERS`)
- `docs/security-audit.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Update `CMakeLists.txt` with `-fno-sanitize-recover=all` for sanitizer builds.
2. Rebuild and run the ENTIRE CTest suite (`build-asan`), fixing any UBSan trap immediately.

### Testing Instructions
- `cmake -S . -B build-asan -DTINYDB_ENABLE_SANITIZERS=ON -DTINYDB_WARNINGS_AS_ERRORS=ON && cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure`.

### Acceptance Criteria
- 100% of CTest targets pass under strict non-recoverable ASan+UBSan.

### Definition of Done
Strict UBSan audit passed and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-258: enforce non-recoverable UBSan across full test suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-259` (Large-dataset 50k+ row multi-page stress test).
````

---

## DAY-259 — Run Large-Dataset Stress Test (50,000+ Rows, Multi-MB File, Buffer Thrashing)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-259
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Large-Dataset Stress Test (50,000+ Rows, Multi-MB File, Buffer Thrashing)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-258 complete.

### Objective
Write `tests/stress/large_dataset_stress_test.cpp` inserting 50,000 rows (`~3–5 MiB` `.tinydb` across ~1,000 pages!) with two B+ Tree indexes (`pool_size = 32` forcing continuous buffer thrashing!), running point queries, range queries, bulk updates, bulk deletes, cold reopen, and `BPlusTreeValidator` verification!

### Concepts & Daily Learning
- **Concept:** Realistic multi-megabyte disk pressure, multi-hundred-page heap chains, and height-3+ 4KB B+ Trees under constrained buffer RAM (`~128 KiB` pool vs `4+ MiB` dataset).
- **Why Database Engines Need It:** Small unit tests fit completely in RAM; running a multi-MB database inside a 128 KiB buffer pool exercises real disk-eviction cycles across heap and index pages together!
- **How It Works:** Batch inserts inside transactions (e.g., 500 rows per transaction = 100 commits!) for realistic fast WAL throughput, verify indexed point/range lookups, delete 20,000 rows, restart DB cold, and verify exact count (`30,000`) + `BPlusTreeValidator`!
- **Design Trade-offs:** Batching 500 rows per transaction avoids 50,000 separate `fsync` calls (`50k * 2ms = 100s`) so the stress test finishes cleanly in seconds!
- **Common Mistakes:** Running 50,000 individual auto-commit `fsync`s inside a unit test (timing out CI!).
- **Understanding Checkpoint:** Why does grouping 500 `INSERT`s into one `BEGIN ... COMMIT` run orders of magnitude faster on SSDs than 50,000 single-row auto-commits?

### Component Context
- **Why It Exists:** Proves TinyDB scales smoothly to tens of thousands of rows and ~1,000+ pages.
- **Module Relationships:** Exercises full stack under real I/O churn.

### Files Expected to Change
- `tests/stress/large_dataset_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `large_dataset_stress_test.cpp` (50k rows, 2 indexes, `pool_size = 32`, cold restart, validator check).
2. Run in Release and Debug+ASan/UBSan verifying reasonable runtime!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Release and Debug+ASan/UBSan.

### Acceptance Criteria
- 50,000-row multi-MB lifecycle passes under both Release and ASan+UBSan cleanly.

### Definition of Done
Large-dataset stress test passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-259: add 50k-row multi-MB buffer-thrashing stress test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-260` (File permissions, path safety & temp-file hygiene).
````

---

## DAY-260 — Audit File Permissions, Path Safety, and Atomic Temp-File Hygiene

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-260
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Audit File Permissions, Path Safety, and Atomic Temp-File Hygiene
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-259 complete.

### Objective
Audit POSIX file creation modes (`0600` / `0644`—never world-writable `0666`/`0777`!), symlink safety (`O_NOFOLLOW` where appropriate on temp files!), non-existent parent directory handling, empty/trailing-slash DB paths, and read-only permission error handling in `tests/unit/security/file_safety_test.cpp`.

### Concepts & Daily Learning
- **Concept:** Least-privilege file permissions (`0600` vs `0644`), symlink TOCTOU defense on temp files (`.cat.tmp`), and clean path validation.
- **Why Database Engines Need It:** Opening `.cat.tmp` with `O_CREAT | O_EXCL` (or exclusive flags rather than overwriting an existing symlink!) prevents symlink attacks and avoids leaving world-writable database files (`0666`) on multi-user machines!
- **How It Works:** Verify `FileHandle::CreateNew` / `OpenOrCreate` uses restrictive `0600` (owner rw only!) or `0644`, check `.cat.tmp` creation uses `O_CREAT | O_TRUNC` cleanly after unlinking stale `.tmp` or `O_EXCL | O_NOFOLLOW`, reject empty DB paths or directory paths (`db_path` is a directory!), and test read-only directory failure recovery cleanly!
- **Design Trade-offs:** Using `0600` for database/catalog/WAL files protects user data on shared systems without affecting normal single-user usage.
- **Common Mistakes:** Passing `0777` or forgetting the `mode_t` argument when calling `::open(..., O_CREAT)` (resulting in random stack garbage file permissions—classic POSIX C bug!!).
- **Understanding Checkpoint:** Why is calling `::open(path, O_RDWR | O_CREAT)` WITHOUT the 3rd `mode_t` argument a security bug in C/C++?

### Component Context
- **Why It Exists:** Hardens OS file creation and path validation boundaries.
- **Module Relationships:** Audits `os_io.cpp`, `file_handle.cpp`, `catalog_manager.cpp`, and `main.cpp`.

### Files Expected to Change
- `src/core/file_handle.cpp`, `src/catalog/catalog_manager.cpp`, `src/execution/execution_engine.cpp`
- `tests/unit/security/file_safety_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit all `::open` calls for `0600` mode, `O_NOFOLLOW` on temp files, and empty/directory path checks.
2. Add tests verifying restrictive file modes (`stat` mode check!), directory-as-db-path rejection, empty path rejection, and unwritable parent directory failure leaving zero garbage files!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All created `.tinydb`/`.cat`/`.wal` files have safe permissions; invalid/directory paths fail cleanly.

### Definition of Done
File-safety audit and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-260: audit file permissions, temp-file safety, and path validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-261` (Resource exhaustion & I/O failure simulation).
````

---

## DAY-261 — Test Resource Exhaustion and Disk Write Failure Recovery Paths

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-261
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Test Resource Exhaustion and Disk Write Failure Recovery Paths
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-260 complete.

### Objective
Write resource-exhaustion and I/O fault tests (`tests/unit/security/resource_exhaustion_test.cpp`) verifying graceful recovery without crashes or invariant breaks when: `BufferPoolManager` saturates (`pool_size = 2` during complex operations), `RLIMIT_FSIZE` / simulated write error fails `WriteAt` / `LogManager::Flush`, or max table/column/statement limits are exceeded!

### Concepts & Daily Learning
- **Concept:** Fault-injected I/O failures (`ENOSPC` / write error hooks) verifying transactional commit fails safely without claiming `COMMIT` succeeded!
- **Why Database Engines Need It:** If disk fills up during `LogManager::Flush()` inside `COMMIT`, the database MUST NOT mark the transaction committed or release uncommitted pages to `.tinydb`—it must propagate `kIoError`, keep/allow safe rollback, and protect existing committed data!
- **How It Works:** Add a lightweight optional test-only `IoFaultInjector` (or `DiskFull` write-failure hook inside `os_io.cpp` / `LogManager`!), fail `LogManager::Flush()` on a `COMMIT`, verify `Commit()` returns `kIoError`, verify `ROLLBACK` recovers clean prior state, and verify `.wal` stays valid without corrupt partial records!
- **Design Trade-offs:** A small test hook in `os_io.cpp` tests real disk-full error handling deterministically without actually filling up the user's Mac SSD!
- **Common Mistakes:** Filling up a real tmpfs/SSD in tests (dangerous!) instead of injecting deterministic `EIO`/`ENOSPC` inside the I/O wrapper!
- **Understanding Checkpoint:** What should happen inside `TransactionManager::Commit()` if `LogManager::Flush()` fails with `ENOSPC` (`kIoError`)?

### Component Context
- **Why It Exists:** Verifies error resilience under resource exhaustion and disk-full conditions.
- **Module Relationships:** Exercises `os_io`, `LogManager`, `TransactionManager`, and `BufferPoolManager`.

### Files Expected to Change
- `include/tinydb/core/os_io.hpp`, `src/core/os_io.cpp` (lightweight test fault hook)
- `tests/unit/security/resource_exhaustion_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add clean `IoFaultHook` (active only when armed in tests) for simulating write/sync failures.
2. Add tests verifying failed WAL `Flush()` during `COMMIT` returns `kIoError` cleanly without corrupting prior committed DB or `.wal`, and `BufferPool` saturation leaves `ActivePinCount() == 0`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Simulated write/sync errors fail cleanly without data corruption or pin leaks.

### Definition of Done
Resource-exhaustion and I/O fault tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-261: verify resource exhaustion and simulated disk-write failure handling`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-262` (Regression test suite organization).
````

---

## DAY-262 — Organize Dedicated Regression Test Suite for Edge and Historical Bugs

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-262
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Organize Dedicated Regression Test Suite for Edge and Historical Bugs
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-261 complete.

### Objective
Consolidate and expand a permanent named regression suite (`tests/unit/regression/tinydb_regression_test.cpp`) documenting every subtle bug class prevented across Phases 01–08 (`REG-001` NUL-in-magic, `REG-002` 32-bit `PageId` offset overflow, `REG-003` sticky dirty `Unpin(false)`, `REG-004` cross-leaf `(key, {0,0})` boundary, `REG-005` Halloween `UPDATE`, `REG-006` `UPDATE` non-indexed `VARCHAR` updating `RowID` in index, `REG-007` `ROLLBACK` restoring prior committed `was_dirty`, `REG-008` `kIndexRoot` WAL crash, `REG-009` `INT64_MIN` lexer/parser).

### Concepts & Daily Learning
- **Concept:** Permanent regression suites keyed by explicit `REG-XXX` scenario IDs.
- **Why Database Engines Need It:** Subtle bugs (like `REG-006` cross-page `RowID` relocation updating untouched indexes, or `REG-007` `ROLLBACK` preserving earlier committed dirty state) are easily re-introduced during future optimization passes unless permanently locked by named regression tests!
- **How It Works:** Write clean, isolated tests `TEST(RegressionSuite, REG001_...)` .. `TEST(RegressionSuite, REG010_...)` (including any real bugs discovered during Phases 01–09!), each with a 2-line docstring explaining the invariant!
- **Design Trade-offs:** Fast, laser-focused regression tests provide instant CI signal whenever core files change.
- **Common Mistakes:** Leaving bug fixes verified only inside giant slow fuzz tests instead of extracting minimal fast regression unit tests!
- **Understanding Checkpoint:** Pick any 3 `REG-XXX` scenarios above and explain why a naive implementation would fail each one.

### Component Context
- **Why It Exists:** Permanent insurance policy against re-introducing subtle database engine bugs.
- **Module Relationships:** Cross-cuts all TinyDB modules.

### Files Expected to Change
- `tests/unit/regression/tinydb_regression_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `tinydb_regression_test.cpp` covering `REG-001` through `REG-010` (plus any real bugs encountered during earlier sessions!).
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every regression case passes cleanly in milliseconds under ASan/UBSan.

### Definition of Done
Regression suite created and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-262: create named REG-001..010 regression test suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-263` (Compiler warning sweep).
````

---

## DAY-263 — Execute Strict Compiler Warning Sweep (`-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-263
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Execute Strict Compiler Warning Sweep (`-Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-262 complete.

### Objective
Audit `CMakeLists.txt` warning flags (enabling additional strict warnings like `-Wold-style-cast`, `-Wnull-dereference`, `-Wnon-virtual-dtor`, `-Woverloaded-virtual` cleanly on TinyDB targets—careful not to apply `-Werror` to third-party GTest headers via `SYSTEM` includes!), fix every single warning across all `src/`, `include/`, and `tests/`, and verify zero warnings in Debug, Release, and ASan+UBSan!

### Concepts & Daily Learning
- **Concept:** Static warning discipline as lightweight static analysis (`-Wold-style-cast`, `-Wnon-virtual-dtor`, `-Wconversion`, `-Wshadow`).
- **Why Database Engines Need It:** `-Wnon-virtual-dtor` prevents polymorphic AST/Executor leaks; `-Wconversion` catches silent 64-to-32-bit `PageId` / `size_t` truncation; `-Wshadow` prevents accidentally shadowing `page_id` inside nested blocks!
- **How It Works:** Add `-Wnon-virtual-dtor`, `-Woverloaded-virtual`, `-Wnull-dereference` (and `-Wold-style-cast` if clean across C++ code!) to `tinydb_compile_options`, run clean rebuilds in Debug, ASan+UBSan, and Release (`-O3` activates additional GCC/Clang uninitialized/null-dereference warnings!).
- **Design Trade-offs:** Using `SYSTEM` on external test headers keeps strict flags focused purely on TinyDB's own code!
- **Common Mistakes:** Suppressing warnings with broad `#pragma`s instead of fixing narrowing types cleanly with `static_cast` or `gsl::narrow`/checked helpers!
- **Understanding Checkpoint:** Why does compiling in **Release (`-O3`)** with `-Werror` sometimes reveal warnings (like `-Wuninitialized` or `-Wnull-dereference`) that `-O0` Debug builds miss?

### Component Context
- **Why It Exists:** Elevates static code quality across the entire repository.
- **Module Relationships:** Applies to `CMakeLists.txt` and all source/test targets.

### Files Expected to Change
- `CMakeLists.txt`
- Any `include/`, `src/`, `tests/` files with latent warnings
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Enable strict warnings in `CMakeLists.txt` and compile fresh Debug, ASan+UBSan, and Release builds with `-DTINYDB_WARNINGS_AS_ERRORS=ON`.
2. Fix any warnings cleanly and run full `ctest`.

### Testing Instructions
- Clean builds & `ctest --output-on-failure` across Debug, ASan+UBSan, and Release with `-DTINYDB_WARNINGS_AS_ERRORS=ON`.

### Acceptance Criteria
- Zero warnings across all 3 build modes with `-Werror` enabled.

### Definition of Done
Warning sweep complete and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-263: enforce expanded compiler warnings with -Werror across all builds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-264` (`clang-format` audit).
````

---

## DAY-264 — Execute Repository-Wide `clang-format` Audit and Enforce CI Formatting Check

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-264
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Execute Repository-Wide `clang-format` Audit and Enforce CI Formatting Check
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-263 complete.

### Objective
Format every `.hpp` and `.cpp` file across `include/`, `src/`, `tests/`, and `benchmarks/` using `.clang-format`, verify `--dry-run --Werror` exits `0`, and add an automated `clang-format` check job to `.github/workflows/ci.yml`!

### Concepts & Daily Learning
- **Concept:** Automated code-style enforcement in CI eliminating style drift across 280+ sessions.
- **Why Database Engines Need It:** Consistent formatting across 50+ C++ headers and files improves code review readability and interview presentation.
- **How It Works:** Run `clang-format -i` across all project `.hpp`/`.cpp` files (excluding `build*/`!), verify `clang-format --dry-run --Werror`, and update `.github/workflows/ci.yml` cleanly!
- **Design Trade-offs:** Enforcing formatting in CI keeps PRs/commits uniformly styled without manual nitpicking.
- **Common Mistakes:** Running `clang-format` over `build/_deps` FetchContent directories! Always scope strictly to `include/`, `src/`, `tests/`, `benchmarks/`.
- **Understanding Checkpoint:** Why is keeping formatting edits in their own dedicated commit (`day-264`) better than mixing formatting sweeps with logic edits?

### Component Context
- **Why It Exists:** Enforces uniform style across the entire C++20 codebase.
- **Module Relationships:** Updates `.github/workflows/ci.yml` and formats `include/`, `src/`, `tests/`.

### Files Expected to Change
- `include/**/*.hpp`, `src/**/*.cpp`, `tests/**/*.hpp`, `tests/**/*.cpp` (formatting only if needed)
- `.github/workflows/ci.yml` (add `clang-format` CI step)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Format all TinyDB C++ files with `clang-format`.
2. Add formatting check step to `.github/workflows/ci.yml`.
3. Re-run `ctest` to verify zero unintended changes.

### Testing Instructions
- `clang-format --dry-run --Werror` across all `include/`, `src/`, `tests/` files + `ctest --output-on-failure`.

### Acceptance Criteria
- `clang-format --dry-run --Werror` exits `0` across 100% of TinyDB C++ sources; all tests pass.

### Definition of Done
Formatting audit and CI step complete, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-264: format all C++ sources and add clang-format CI check`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-265` (Secret scanning & dependency audit).
````

---

## DAY-265 — Perform Secret Scanning and Dependency Audit

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-265
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Perform Secret Scanning and Dependency Audit
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-264 complete.

### Objective
Audit the entire repository and Git history (`git log`, `CMakeLists.txt`, `.github/workflows/`, `src/`, `docs/`) for: accidental secrets/tokens/credentials/hardcoded local paths, unpinned third-party dependencies (`FetchContent` tag/hash, GitHub Actions SHA/major versions), and unneeded external libraries—recording concrete findings in `docs/security-audit.md` and adding a lightweight automated check script (`scripts/check_hygiene.py` or CI check)!

### Concepts & Daily Learning
- **Concept:** Supply-chain hygiene, secret scanning across working tree + Git history, and minimal dependency footprint.
- **Why Database Engines Need It:** TinyDB's production `tinydb` executable must depend ONLY on standard C++20 + OS POSIX APIs (zero unnecessary external libs!), while `CMakeLists.txt` (`GoogleTest`) and `.github/workflows/` must pin explicit trusted versions and avoid hardcoded user paths like `/Users/...`!
- **How It Works:** Scan all tracked files + commit history for credential patterns, `/Users/` hardcoded paths, `DEBUG` print leaks, and verify `ldd`/`otool -L ./build/tinydb` shows ONLY standard OS/C++ runtime libraries! Document exact verified `otool -L` output in `docs/security-audit.md`.
- **Design Trade-offs:** Checking `otool -L` / `ldd` proves the production binary stays dependency-minimal even though GoogleTest/SQLite are linked in test binaries!
- **Common Mistakes:** Accidentally linking `GTest` or `SQLite3` into `tinydb_core` or `tinydb` executable!
- **Understanding Checkpoint:** Run `otool -L` on `tinydb` and explain why `tinydb` must link ZERO test or third-party database libraries.

### Component Context
- **Why It Exists:** Verifies security hygiene and zero-bloat dependency boundaries.
- **Module Relationships:** Audits `CMakeLists.txt`, `.github/workflows/ci.yml`, and Git history.

### Files Expected to Change
- `scripts/check_hygiene.py` (new — scans for secrets, hardcoded home paths, formatting/CRLF hygiene)
- `.github/workflows/ci.yml` (wire hygiene check)
- `docs/security-audit.md` (record observed `otool -L` and history scan results)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Inspect `otool -L ./build/tinydb`, `CMakeLists.txt` (`FetchContent` URL/hash pinning), and Git history for secrets or hardcoded paths.
2. Create `scripts/check_hygiene.py` checking tracked files cleanly (no secrets, no `/Users/` hardcoded paths in `src/`/`include/`/`tests/`, no trailing whitespace errors).
3. Record exact observed results in `docs/security-audit.md`.

### Testing Instructions
- Run `python3 scripts/check_hygiene.py`, `otool -L ./build/tinydb`, and `ctest --test-dir build --output-on-failure`.

### Acceptance Criteria
- `tinydb` links only standard OS/C++ libraries; zero secrets or hardcoded local paths exist in tracked code or history.

### Definition of Done
Secret & dependency audit completed and recorded, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-265: perform secret/dependency audit and add repo hygiene check`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-266` (Performance & memory profiling baseline).
````

---

## DAY-266 — Measure Baseline Performance and Memory Footprint (Profiling Sanity Pass)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-266
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Measure Baseline Performance and Memory Footprint (Profiling Sanity Pass)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-265 complete.

### Objective
Run a genuine Release-mode timing and memory/I/O sanity check (`DiskStats`, `BufferPoolStats`, RSS/wall-clock timing on 10,000 rows for `SeqScan` vs `IndexScan`, batched `INSERT`, and WAL recovery) without premature micro-optimizations—fixing ONLY clear algorithmic bottlenecks if discovered (e.g., an accidental `O(N^2)` loop!) and documenting baseline observations in `docs/notes/perf-baseline.md`.

### Concepts & Daily Learning
- **Concept:** Empirical measurement before optimization ("measure, don't guess") and detecting accidental `O(N^2)` hotspots.
- **Why Database Engines Need It:** An unnoticed linear scan inside `TableHeap::InsertTuple` or re-serializing `PageHeader` unnecessarily or `O(N)` `CatalogManager` lookups can hide 100x slowdowns until measured at 10,000 rows!
- **How It Works:** Run Release (`-O3`) measurements on a temporary workload, inspect `DiskStats` + `BufferPoolStats` + elapsed milliseconds, verify zero obvious `O(N^2)` hotspots (`TableHeap` tail hint working! `BPlusTree` O(log N) working!), and document measured numbers honestly.
- **Design Trade-offs:** Fixing only genuine algorithmic/bug hotspots (not sacrificing code clarity for premature bit-hacks!) matches Section 12 guidelines.
- **Common Mistakes:** Benchmarking Debug (`-O0`) or ASan builds and misinterpreting sanitizer overhead as engine bottlenecks!
- **Understanding Checkpoint:** Why MUST performance measurements always run on `-DCMAKE_BUILD_TYPE=Release` with sanitizers `OFF`?

### Component Context
- **Why It Exists:** Identifies real bottlenecks before Phase 10 formal benchmarks.
- **Module Relationships:** Profiles Release performance of `ExecutionEngine`, `BPlusTree`, `BufferPool`, `DiskManager`.

### Files Expected to Change
- `docs/notes/perf-baseline.md` (new — observed Release timing & I/O counts)
- Any targeted `O(N^2)` bug fix if genuinely discovered (rare)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Measure Release-mode (`-O3`) timing and I/O stats for 10,000-row insert (batched vs auto-commit), `SeqScan`, `IndexScan`, and WAL recovery.
2. Record real measured numbers and environment details honestly in `docs/notes/perf-baseline.md`.

### Testing Instructions
- Run Release `ctest` and verify any adjustment under ASan+UBSan too.

### Acceptance Criteria
- Real Release-build measurements recorded cleanly; zero `O(N^2)` anomalies in `TableHeap` or `BPlusTree`.

### Definition of Done
Baseline profiling documented, tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-266: measure Release baseline performance and I/O sanity metrics`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-267` (Edge-case verification sweep).
````

---

## DAY-267 — Run Full Data & Boundary Edge-Case Verification Sweep

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-267
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Run Full Data & Boundary Edge-Case Verification Sweep
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-266 complete.

### Objective
Write `tests/integration/phase09_edge_matrix_test.cpp` verifying every tricky cross-layer boundary together under transactions + WAL + B+ Trees: empty DB restart, empty table with 3 indexes, max-width `VARCHAR` (`kMaxVarcharLength`) inside `INSERT`/`UPDATE`/`SELECT`/WAL/recovery, `INT32_MIN/MAX` and `INT64_MIN/MAX` indexed keys across `IndexScan`, `64`-column wide table, and `DROP TABLE` followed by `VACUUM`-like reuse on reopen!

### Concepts & Daily Learning
- **Concept:** Cross-subsystem boundary composition (testing two edge conditions simultaneously: e.g., max-width `VARCHAR` + cross-page `UPDATE` + `BPlusTree` + WAL crash recovery!).
- **Why Database Engines Need It:** Edge cases often pass individually in unit tests and fail ONLY when combined (e.g., a 64-column schema + max-size tuple + WAL `kPageImage`!).
- **How It Works:** Exercise all combined extremes end-to-end through SQL + crash recovery + reopen!
- **Design Trade-offs:** Combined end-to-end extreme tests guard subtle interactions between tuple sizing, slotted pages, B+ Trees, and WAL.
- **Common Mistakes:** Testing 64-column schemas only in `SchemaTest` without inserting, updating, indexing, and crash-recovering a 64-column row!
- **Understanding Checkpoint:** Verify that a `64`-column row or a `VARCHAR(4000)` row fits within `SlottedPage::kMaxSingleTupleBytes` (`4050` bytes) AND recovers cleanly via 4096-byte WAL `kPageImage`!

### Component Context
- **Why It Exists:** Proves all v1 limits hold seamlessly across the full vertical stack.
- **Module Relationships:** Exercises Phases 01–08 at max boundary limits.

### Files Expected to Change
- `tests/integration/phase09_edge_matrix_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `phase09_edge_matrix_test.cpp` combining max columns, max `VARCHAR`, min/max integers, empty tables, and crash recovery.
2. Run under Debug + ASan/UBSan and Release.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All cross-layer extreme boundary tests pass with zero warnings or leaks.

### Definition of Done
Boundary matrix passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-267: verify cross-layer data and schema boundary extremes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-268` (Recovery ordering & multi-crash stress).
````

---

## DAY-268 — Stress-Test Recovery Ordering Across Interleaved DDL, DML, Index Splits, and Crashes

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-268
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Stress-Test Recovery Ordering Across Interleaved DDL, DML, Index Splits, and Crashes
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-267 complete.

### Objective
Write `tests/recovery/recovery_ordering_stress_test.cpp` verifying WAL recovery ordering when a single database experiences multiple sessions combining standalone DDL (`CREATE TABLE`, `CREATE INDEX`, `DROP TABLE`), multi-page transactions, buffer evictions, `kIndexRoot` splits/shrinks, `kFreePage` reuses, and repeated crashes!

### Concepts & Daily Learning
- **Concept:** Recovery ordering across standalone DDL (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`), DML transactions, freelist page reuse, and crashes.
- **Why Database Engines Need It:** Suppose Session 1 inserts into Table A (`PAGE_IMAGE(1)` at LSN 2 in `.wal`), runs `DROP TABLE A` (reclaiming Page 1 to `kFree` and updating `.cat`), and crashes BEFORE `Close()` (or creates Table B reusing Page 1 and crashes). Without flushing/checkpointing around standalone DDL, WAL still holds old Table A's LSN 2 record targeting Page 1, risking resurrection of dropped-table bytes!
- **How It Works:** Execute `BufferPoolManager::FlushAllPages()` + `LogManager::ResetCheckpoint()` cleanly around standalone DDL so `.cat` DDL state, `.tinydb` freelist, and `.wal` stay strictly synchronized across subsequent crashes.
- **Design Trade-offs:** Checkpointing `.wal` during rare standalone DDL eliminates complex DDL log-replay logic in v1 while keeping `DROP TABLE -> reuse -> crash` 100% consistent.
- **Common Mistakes:** Leaving stale `PAGE_IMAGE` records in `.wal` referencing pages that a subsequent standalone `DROP TABLE` already returned to the freelist!
- **Understanding Checkpoint:** Trace `INSERT INTO A (Page 1) -> DROP TABLE A -> CRASH`: why does checkpointing `.wal` during `DROP TABLE` prevent `RecoveryManager` from replaying old Table A data onto freed Page 1?

### Component Context
- **Why It Exists:** Catches subtle DDL + WAL + freelist ordering bugs across crashes.
- **Module Relationships:** Coordinates `ExecutionEngine` DDL checkpointing + `RecoveryManager`.

### Files Expected to Change
- `src/execution/execution_engine.cpp` (ensure clean buffer flush + WAL checkpoint around standalone DDL!)
- `tests/recovery/recovery_ordering_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Ensure standalone DDL cleanly syncs dirty buffers and checkpoints `.wal` so `.cat` DDL state and `.tinydb`/`.wal` never diverge across subsequent crashes!
2. Implement `recovery_ordering_stress_test.cpp` testing `INSERT A -> DROP A -> CRASH`, `INSERT A -> DROP A -> CREATE B (reuse pages!) -> INSERT B -> CRASH`, and `CREATE INDEX -> COMMIT -> CRASH`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `DROP TABLE` followed by immediate crash (or page reuse + crash) leaves zero resurrected pages and 100% consistent `.cat`, `.tinydb`, and freelist!

### Definition of Done
DDL/WAL ordering crash tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-268: checkpoint WAL around standalone DDL and test DDL+crash ordering`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-269` (End-to-end automated CLI+SQL+Index+WAL integration suite).
````

---

## DAY-269 — Build Full-Stack End-to-End Automated Acceptance Integration Suite

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-269
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Build Full-Stack End-to-End Automated Acceptance Integration Suite
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-268 complete.

### Objective
Create `tests/integration/v1_end_to_end_acceptance_test.cpp` acting as TinyDB v1.0's master automated acceptance suite exercising all 8 core specifications from Section 3 in one realistic multi-session story (creating/dropping tables, persistent records, `SeqScan` & `IndexScan`, B+ Trees across restarts, bounded Buffer Pool evictions, transactions `BEGIN`/`COMMIT`/`ROLLBACK`, crash recovery, and CLI `.tables`/`.schema`/SQL execution!).

### Concepts & Daily Learning
- **Concept:** Acceptance Test-Driven Validation against Section 3's 8 official v1.0 objectives.
- **Why Database Engines Need It:** Provides a single clear, readable test file proving every required v1.0 capability works harmoniously together!
- **How It Works:** Write 8 named `TEST(TinyDBV1Acceptance, Objective1_...)` .. `Objective8_...` test cases covering every requirement end-to-end!
- **Design Trade-offs:** Organizing acceptance tests by the 8 official v1.0 objectives makes release verification transparent and interview-demonstrable.
- **Common Mistakes:** Leaving any objective verified only in isolated mocks instead of full end-to-end execution.
- **Understanding Checkpoint:** Verify all 8 objectives from Section 3 map 1-to-1 onto executable passing tests in `v1_end_to_end_acceptance_test.cpp`.

### Component Context
- **Why It Exists:** Master integration acceptance suite for TinyDB v1.0.
- **Module Relationships:** Exercises the entire project end-to-end.

### Files Expected to Change
- `tests/integration/v1_end_to_end_acceptance_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all 8 objective acceptance tests in `v1_end_to_end_acceptance_test.cpp`.
2. Run under Debug, ASan+UBSan, and Release!

### Testing Instructions
- Build and run `ctest --output-on-failure` across Debug, ASan+UBSan, and Release.

### Acceptance Criteria
- All 8 acceptance objectives pass cleanly across all 3 build modes.

### Definition of Done
End-to-end acceptance suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-269: add master v1.0 8-objective end-to-end acceptance test suite`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-270` (Security & QA checklist completion report).
````

---

## DAY-270 — Complete Security & QA Checklist Verification in `docs/security-audit.md`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-270
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Complete Security & QA Checklist Verification in `docs/security-audit.md`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-269 complete.

### Objective
Review every security & testing item required in Phase 09 (`bounds checking`, `overflow protection`, `memory ownership`, `invalid page IDs`, `corrupt DB/catalog/B+ Tree/WAL rejection`, `parser limits`, `recovery ordering`, `secret scan`, `dependency audit`, `fuzzing`, `differential SQLite`, `ASan/UBSan/leak`, `crash matrix`), verify each item links to its passing test file and actual observed run output, and finalize `docs/security-audit.md`!

### Concepts & Daily Learning
- **Concept:** Traceable security verification matrix (Requirement -> Code Guard -> Automated Test -> Verified Output).
- **Why Database Engines Need It:** Audits mean nothing without reproducible evidence linking every security claim to actual test names and observed commands!
- **How It Works:** Walk item-by-item through Phase 09's checklist, run `scripts/check_hygiene.py`, `clang-format`, and `ctest`, and record exact passing test counts and observations in `docs/security-audit.md` (zero fabricated claims!).
- **Design Trade-offs:** Honest traceable evidence ensures full confidence before Phase 10 documentation & release.
- **Common Mistakes:** Copying unverified boilerplate summaries instead of actual observed file names, test counts, and command outputs.
- **Understanding Checkpoint:** Review `docs/security-audit.md` and confirm every single Phase 09 checklist item cites a verified file and test run from this repository.

### Component Context
- **Why It Exists:** Closes out every security/QA check cleanly before Phase 10.
- **Module Relationships:** Summarizes `tests/` + `docs/security-audit.md`.

### Files Expected to Change
- `docs/security-audit.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Run `python3 scripts/check_hygiene.py`, `clang-format --dry-run --Werror`, and `ctest` on Debug, ASan+UBSan, and Release.
2. Record exact observed counts, binary dependency checks, and test mappings in `docs/security-audit.md`.

### Testing Instructions
- Full Debug, ASan+UBSan, Release `ctest --output-on-failure` + formatting + hygiene checks.

### Acceptance Criteria
- Every Phase 09 checklist item is checked off with verified, accurate evidence.

### Definition of Done
Security audit finalized, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-270: finalize Phase 09 security and QA verification report`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-271` (Phase 09 sign-off).
````

---

## DAY-271 — Complete Phase 09 Verification Matrix and Sign Off

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-271
- **Phase:** Phase 09 — Security and Testing
- **Session Title:** Complete Phase 09 Verification Matrix and Sign Off
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-241 through DAY-270 complete.

### Objective
Execute one final clean-build verification from scratch (`rm -rf build-verify && cmake ...`) across Debug+ASan+UBSan and Release (`-Werror`), verify zero untracked files or regressions, update `PROGRESS.md` (`210.0 / 220.0` hours completed!), and formally sign off Phase 09!

### Concepts & Daily Learning
- **Concept:** Clean-room build verification before entering the final Release phase (`Phase 10`).
- **Why Database Engines Need It:** Stale CMake cache entries can hide missing source files or build-order bugs; building from a fresh empty directory proves anyone cloning the repo can compile and pass 100% of tests cleanly!
- **How It Works:** Configure fresh temporary build directories, compile with `-DTINYDB_WARNINGS_AS_ERRORS=ON`, run full `ctest`, remove temporary build folders (or keep `.gitignore`d cleanly), update `PROGRESS.md` and `NEXT_SESSION.md` (`DAY-272`).
- **Design Trade-offs:** Clean-room verification takes minutes and prevents last-minute release surprises.
- **Common Mistakes:** Relying solely on incremental builds when `CMakeLists.txt` or headers changed heavily.
- **Understanding Checkpoint:** Why is running a clean-directory CMake build (`rm -rf build-clean && cmake -S . -B build-clean`) essential before signing off a major QA phase?

### Component Context
- **Why It Exists:** Completes Phase 09 (`24.0` hours).
- **Module Relationships:** Hands off a hardened, audited engine to Phase 10 (`Documentation and Release`).

### Files Expected to Change
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Run clean-directory ASan+UBSan and Release builds + full `ctest --output-on-failure` + `clang-format` + `scripts/check_hygiene.py`.
2. Update `PROGRESS.md` (`210.0 / 220.0` hours, Phase 09 complete, Phase 10 active) and `NEXT_SESSION.md` -> `DAY-272`.

### Testing Instructions
- Fresh Debug+ASan+UBSan and Release builds + full `ctest`.

### Acceptance Criteria
- 100% of unit, integration, stress, fuzz, differential, and crash recovery tests pass from clean build.

### Definition of Done
Phase 09 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-271: verify clean-room build matrix and sign off Phase 09`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 10 (`DAY-272` — Architecture diagram & `docs/architecture.md` finalization).
````
