# Phase 00 — Prerequisites and Architecture (`DAY-001` – `DAY-018`)

**Phase Budget:** 14.0 hours (16 × 45 min + 2 × 60 min = 18 sessions)
**Goal:** Master C++20 systems programming, tooling (CMake, ASan, UBSan, LLDB, `clang-format`), OS/database fundamentals, and implement TinyDB's `src/core/` foundations alongside learning.

---

## DAY-001 — Verify Toolchain, C++20 Compiler Warnings, Sanitizers, LLDB & `clang-format` Workflow

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-001
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Verify Toolchain, C++20 Compiler Warnings, Sanitizers, LLDB & `clang-format` Workflow
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Repository bootstrap complete (`CMakeLists.txt`, minimal `tinydb` binary, GoogleTest smoke test).

### Objective
Configure and verify the full developer workflow: Debug/Release CMake builds, AddressSanitizer + UndefinedBehaviorSanitizer (`TINYDB_ENABLE_SANITIZERS=ON`), strict compiler warnings (`TINYDB_WARNINGS_AS_ERRORS=ON`), `clang-format`, and `lldb` debugging on macOS. Add a concise developer tooling guide (`docs/tooling.md`) and verify the bootstrap commit SHA in `PROGRESS.md`.

### Concepts & Daily Learning
- **Concept:** Compiler warnings as static invariants, dynamic sanitizers (ASan/UBSan), source formatting, and debugger inspection.
- **Why Database Engines Need It:** Storage engines manipulate raw byte arrays, offsets, and file descriptors; silent signed overflow, dangling pointers, or buffer over-reads corrupt persistent data on disk.
- **How It Works:** `-fsanitize=address` instruments heap/stack allocations with shadow-memory redzones to trap out-of-bounds and use-after-free bugs immediately; `-fsanitize=undefined` inserts runtime traps for UB like signed integer overflow and misaligned loads.
- **Design Trade-offs:** Sanitizers add ~2x CPU/RAM overhead and belong in Debug/CI builds, whereas Release builds disable sanitizers for throughput.
- **Common Mistakes:** Assuming tests passing in non-sanitized Release mode proves memory safety; forgetting `-fno-omit-frame-pointer` (causing unreadable stack traces).
- **Understanding Checkpoint:** Explain why a C++ test that reads 1 byte past a 4096-byte buffer might "pass" without ASan, and how ASan catches it deterministically.

### Component Context
- **Why It Exists:** Establishes a bug-catching safety net before writing any byte-level storage code.
- **Module Relationships:** Governs every subsequent module (`core`, `storage`, `buffer`, `index`, `parser`, `execution`, `recovery`).

### Files Expected to Change
- `docs/tooling.md` (new)
- `CMakeLists.txt` (verify/add `cmake` sanitizer & `clang-format` helper target if clean)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Inspect `git status`, `git branch --show-current`, `git remote -v`, `PROGRESS.md`, and `NEXT_SESSION.md`.
2. Record the initial bootstrap commit SHA from `git log -n 1` into `PROGRESS.md`.
3. Write `docs/tooling.md` documenting exact commands for: Debug build, ASan+UBSan build, `-Werror` build, running `ctest`, invoking `/Library/Developer/CommandLineTools/usr/bin/clang-format` (or PATH `clang-format`), and launching `lldb ./build/tinydb_unit_tests`.
4. Run a brief intentional local experiment (outside committed code or in scratch, never committed!) or verify with `lldb` breakpoint on `GetVersionBanner` to confirm symbols and backtraces work.
5. Verify formatting on `include/` and `src/` with `clang-format --dry-run --Werror`.

### Testing Instructions
- Build and test standard Debug configuration: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DTINYDB_WARNINGS_AS_ERRORS=ON && cmake --build build -j && ctest --test-dir build --output-on-failure`.
- Build and test Sanitizer configuration: `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DTINYDB_ENABLE_SANITIZERS=ON -DTINYDB_WARNINGS_AS_ERRORS=ON && cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure`.

### Acceptance Criteria
- Both standard and ASan+UBSan builds succeed with zero warnings (`-Werror`).
- `ctest` passes all smoke tests in both configurations.
- `docs/tooling.md` accurately documents verified commands observed in this session.
- `clang-format` check exits 0 with zero formatting diffs.

### Definition of Done
Toolchain verified, `docs/tooling.md` created, `PROGRESS.md`, `SESSION-INDEX.md`, and `NEXT_SESSION.md` updated (`DAY-001` complete, `DAY-002` next), `git diff --check` clean, committed to `develop`, pushed and verified on `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-001: verify C++20 toolchain, sanitizers, lldb, and formatting workflow`
- Push `git push origin develop` and verify `git rev-parse origin/develop` matches `HEAD`.

### Next Session Dependency
Unlocks `DAY-002` (Core memory utilities & `std::span`).
````

---

## DAY-002 — C++20 Memory Model: Stack vs Heap, Pointers, References, and `std::span` in Core Utilities

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-002
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** C++20 Memory Model: Stack vs Heap, Pointers, References, and `std::span` in Core Utilities
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-001 complete.

### Objective
Implement non-owning byte-slice views and checked sub-span helpers (`include/tinydb/core/slice_utils.hpp`, `src/core/slice_utils.cpp`) using C++20 `std::span<const std::byte>` and `std::span<std::byte>` while contrasting stack, heap, pointer, and reference lifetimes.

### Concepts & Daily Learning
- **Concept:** Stack vs heap storage duration, non-nullable references vs nullable pointers, and `std::span` fat pointers (`pointer + size`).
- **Why Database Engines Need It:** Page buffers, tuples, and serialized records are raw byte regions passed across layers; raw `(const uint8_t* ptr, size_t len)` pairs invite mismatched lengths and buffer over-reads.
- **How It Works:** `std::span<std::byte>` stores a pointer and element count (`16` bytes on 64-bit) with zero heap allocation, enabling bounds-checked subview slicing (`SubSpan(span, offset, count)`).
- **Design Trade-offs:** `std::span` never owns memory (must not outlive the underlying array/vector), unlike `std::vector`, trading ownership for zero-allocation views.
- **Common Mistakes:** Returning a `std::span` pointing to a stack-local array; using `span.subspan(offset, count)` when `offset + count > span.size()` (UB unless explicitly checked!).
- **Understanding Checkpoint:** Why is `std::span<const std::byte>` strictly safer than `const uint8_t* data` for page parsing, and what lifetime rule must callers still uphold?

### Component Context
- **Why It Exists:** Forms the safe non-owning memory view primitive used by serialization, slotted pages, and WAL parsing.
- **Module Relationships:** Used directly by `core/endian`, `storage/page`, `storage/tuple`, and `recovery/log_record`.

### Files Expected to Change
- `include/tinydb/core/slice_utils.hpp` (new)
- `src/core/slice_utils.cpp` (new)
- `CMakeLists.txt` (register `src/core/slice_utils.cpp` and new unit test)
- `tests/unit/core/slice_utils_test.cpp` (new)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Inspect `CMakeLists.txt` and `PROGRESS.md`.
2. Create `include/tinydb/core/slice_utils.hpp` with `CheckedSubspan` (overloads for mutable and const `std::span<std::byte>`) returning `std::optional<std::span<...>>` when `offset <= s.size()` and `len <= s.size() - offset` (overflow-safe!).
3. Add `CopyInto(std::span<std::byte> dst, std::span<const std::byte> src)` returning `bool` (false if `src.size() > dst.size()`).
4. Add unit tests covering valid slices, empty slices, exact boundary slices, `SIZE_MAX` overflow traps, and out-of-bounds rejections.

### Testing Instructions
- Build and run `ctest` in both Debug and ASan+UBSan configurations.

### Acceptance Criteria
- `CheckedSubspan` rejects overflowed `offset + len` without unsigned wraparound UB.
- All unit tests pass cleanly under ASan+UBSan with zero warnings.

### Definition of Done
Code implemented, formatted, tested under ASan+UBSan, progress docs updated, `git diff --check` clean, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-002: add bounds-checked std::span core slice utilities`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-003` (RAII & ownership semantics).
````

---

## DAY-003 — RAII & Ownership Semantics: Rule of Five, `std::unique_ptr`, and `std::shared_ptr`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-003
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** RAII & Ownership Semantics: Rule of Five, `std::unique_ptr`, and `std::shared_ptr`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-002 complete.

### Objective
Implement a reusable RAII `ScopeExit` / `Defer` guard (`include/tinydb/core/scope_guard.hpp`) and ownership-checked aligned/pinned resource pattern tests demonstrating Rule of Five, exclusive `std::unique_ptr`, and shared `std::shared_ptr` semantics.

### Concepts & Daily Learning
- **Concept:** Resource Acquisition Is Initialization (RAII), Rule of Zero / Rule of Five, exclusive vs shared ownership (`std::unique_ptr` vs `std::shared_ptr`).
- **Why Database Engines Need It:** Every leaked buffer pool pin, unclosed file descriptor, or un-freed lock causes deadlocks, FD exhaustion, or silent data loss on error return paths.
- **How It Works:** Constructors acquire/bind a resource; destructors (`noexcept`) deterministically release it upon scope exit (normal return or exception), while non-copyable move-only types enforce single ownership.
- **Design Trade-offs:** `std::unique_ptr` has zero overhead over a raw pointer; `std::shared_ptr` adds atomic reference-count overhead and can mask unclear ownership hierarchies if overused.
- **Common Mistakes:** Copying a class that owns a raw handle without deleting copy operations (causing double-free); forgetting `Dismiss()` / `moved-from` state reset in move constructors.
- **Understanding Checkpoint:** Why should TinyDB's `BufferPoolManager` return `std::unique_ptr` or move-only `PageGuard`s rather than `std::shared_ptr<Page>`?

### Component Context
- **Why It Exists:** Provides deterministic cleanup on early returns in file I/O, buffer guards, and transaction guards.
- **Module Relationships:** Blueprint pattern for `FileHandle` (DAY-012), `PageGuard` (Phase 04), and transaction guards (Phase 08).

### Files Expected to Change
- `include/tinydb/core/scope_guard.hpp` (new)
- `tests/unit/core/raii_ownership_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `tinydb::ScopeGuard<Fn>` (non-copyable, movable, with `Dismiss()` and `MakeScopeGuard(fn)`), handling moved-from `active_` state properly.
2. Add unit tests verifying: cleanup on normal scope exit, cleanup on exception unwind, `Dismiss()` suppression, move transfer (runs exactly once!), and `std::unique_ptr` custom deleter behavior.

### Testing Instructions
- Compile with `-DTINYDB_ENABLE_SANITIZERS=ON -DTINYDB_WARNINGS_AS_ERRORS=ON` and run `ctest --output-on-failure`.

### Acceptance Criteria
- `ScopeGuard` executes its cleanup callback exactly once unless dismissed or moved-from.
- ASan detects zero leaks and zero double-cleanup across exception and move tests.

### Definition of Done
Tests pass under ASan/UBSan, formatting checked, roadmap docs updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-003: implement RAII ScopeGuard and ownership tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-004` (Move semantics & `ByteBuffer`).
````

---

## DAY-004 — Move Semantics & Value Categories: Implementing a Zero-Copy `ByteBuffer` Wrapper

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-004
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Move Semantics & Value Categories: Implementing a Zero-Copy `ByteBuffer` Wrapper
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-003 complete.

### Objective
Implement `tinydb::ByteBuffer` (`include/tinydb/core/byte_buffer.hpp`, `src/core/byte_buffer.cpp`) — a move-capable, explicit-copy buffer owning a byte array (such as a 4KB page or WAL payload) with `AsSpan()` views and Rule-of-Five move semantics.

### Concepts & Daily Learning
- **Concept:** Lvalues, rvalues, xvalues, `std::move`, `noexcept` move constructors/assignment operators, and `Clone()` vs implicit copy.
- **Why Database Engines Need It:** Copying 4096-byte pages or serialized tuples across function boundaries wastes memory bandwidth; moving ownership transfers a pointer in O(1) without touching the 4KB payload.
- **How It Works:** Move constructor steals the internal storage (`std::vector<std::byte>` or `std::unique_ptr<std::byte[]>`) and leaves the source in a valid empty state; deleting implicit copy constructor (while providing explicit `Clone()`) prevents accidental 4KB copies.
- **Design Trade-offs:** Disabling implicit copies avoids silent performance bugs while `Clone()` makes deep copies explicit in code reviews; marking moves `noexcept` lets `std::vector` relocate elements via move instead of copy.
- **Common Mistakes:** Reading from a moved-from object assuming its size is preserved; omitting `noexcept` on move operations; forgetting self-move-assignment guard.
- **Understanding Checkpoint:** Why does deleting the copy constructor while adding an explicit `Clone()` method improve performance safety in a database engine?

### Component Context
- **Why It Exists:** Safe owning byte storage complementing non-owning `std::span` views (`DAY-002`).
- **Module Relationships:** Used for WAL after-image buffers, serialized catalog buffers, and test fixtures.

### Files Expected to Change
- `include/tinydb/core/byte_buffer.hpp` (new)
- `src/core/byte_buffer.cpp` (new)
- `tests/unit/core/byte_buffer_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ByteBuffer`: explicit size constructor (zero-initialized), constructor from `std::span<const std::byte>`, deleted copy constructor/assignment, `noexcept` move constructor/assignment, explicit `Clone()`, `Data()`, `Size()`, `Empty()`, `AsSpan()`, `AsConstSpan()`, and `Clear()`.
2. Write unit tests verifying: pointer identity preservation on move (`data()` address unchanged!), valid empty moved-from state, deep independence of `Clone()`, and self-move safety.

### Testing Instructions
- Build and test with ASan+UBSan and `-Werror`.

### Acceptance Criteria
- `static_assert(!std::is_copy_constructible_v<tinydb::ByteBuffer>)` and `static_assert(std::is_nothrow_move_constructible_v<tinydb::ByteBuffer>)` pass.
- All `ByteBuffer` unit tests pass.

### Definition of Done
Implemented, formatted, ASan-tested, roadmap docs updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-004: implement move-only ByteBuffer with explicit Clone`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-005` (`Status` & `StatusCode` error handling).
````

---

## DAY-005 — Error Handling Architecture: Designing `tinydb::Status` and `tinydb::StatusCode`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-005
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Error Handling Architecture: Designing `tinydb::Status` and `tinydb::StatusCode`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-004 complete.

### Objective
Design and implement TinyDB's unified `Status` and `StatusCode` error model (`include/tinydb/core/status.hpp`, `src/core/status.cpp`), used across storage, catalog, buffer pool, parser, execution, and recovery instead of throwing uncaught exceptions for expected runtime/I/O errors.

### Concepts & Daily Learning
- **Concept:** Value-based error propagation (`Status`) vs C++ exceptions vs raw integer error codes.
- **Why Database Engines Need It:** Disk corruption, invalid SQL syntax, duplicate table names, and full buffer pools are expected runtime conditions; explicit `Status` return values force callers (`[[nodiscard]]`) to handle errors locally without hidden stack unwinding across lock/pin guards.
- **How It Works:** `Status` pairs a `StatusCode` enum (`kOk`, `kInvalidArgument`, `kNotFound`, `kAlreadyExists`, `kIoError`, `kCorruption`, `kOutOfRange`, `kResourceExhausted`, `kSyntaxError`, `kUnsupported`, `kInternal`) with a contextual message string (`std::string`).
- **Design Trade-offs:** Returning `Status` makes control flow visible at call sites (`if (!s.ok()) return s;`) at the cost of explicit propagation boilerplate, unlike exceptions which hide exit paths.
- **Common Mistakes:** Ignoring return values (mitigated via `[[nodiscard]]`); losing context by returning generic `false` instead of `Corruption("bad magic")` vs `IoError("short read")`.
- **Understanding Checkpoint:** Why do database engines like RocksDB, LevelDB, and SQLite prefer explicit status/error codes over throwing exceptions inside storage layers?

### Component Context
- **Why It Exists:** Single consistent error-reporting language across all TinyDB modules.
- **Module Relationships:** Returned by virtually every API in `DiskManager`, `TableHeap`, `CatalogManager`, `BufferPoolManager`, `BPlusTree`, `Parser`, `ExecutionEngine`, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/core/status.hpp` (new)
- `src/core/status.cpp` (new)
- `tests/unit/core/status_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `enum class StatusCode` covering all expected engine error classes.
2. Implement `class [[nodiscard]] Status` with static factory methods (`Ok()`, `InvalidArgument(...)`, `NotFound(...)`, `AlreadyExists(...)`, `IoError(...)`, `Corruption(...)`, `OutOfRange(...)`, `ResourceExhausted(...)`, `SyntaxError(...)`, `Unsupported(...)`, `Internal(...)`), `ok()`, `code()`, `message()`, `ToString()`, and `operator==`.
3. Add `TINYDB_RETURN_IF_ERROR(expr)` helper macro carefully avoiding double-evaluation.
4. Write unit tests verifying all status factories, `ToString()`, `Ok()` zero-allocation fast path, and macro propagation.

### Testing Instructions
- Build and run `ctest` with warnings-as-errors and ASan/UBSan enabled.

### Acceptance Criteria
- `Status::Ok().ok()` is `true`; all error constructors have `ok() == false` and preserve exact code + message.
- `TINYDB_RETURN_IF_ERROR` evaluates its argument exactly once.

### Definition of Done
`Status` tested and formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-005: design and implement Status and StatusCode error model`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-006` (Templates, C++20 concepts & `Result<T>`).
````

---

## DAY-006 — C++20 Templates & Concepts: Implementing `tinydb::Result<T>`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-006
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** C++20 Templates & Concepts: Implementing `tinydb::Result<T>`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-005 complete (`Status`).

### Objective
Implement `tinydb::Result<T>` (`include/tinydb/core/result.hpp`) using C++20 templates, `std::variant<T, Status>`, and C++20 `requires` constraints so functions returning a value or an error compose cleanly without out-parameters.

### Concepts & Daily Learning
- **Concept:** Function/class templates, C++20 `concepts` (`requires`), tagged unions (`std::variant`), and monadic/value-or-error types (`expected`/`Result<T>`).
- **Why Database Engines Need It:** Operations like `ParseStatement()`, `FetchPage()`, or `ReadHeader()` either produce a typed object OR fail with a diagnostic `Status`; combining them in `Result<T>` prevents uninitialized output parameters from being read after an error.
- **How It Works:** `Result<T>` holds `std::variant<T, Status>`, constraining `T` via `requires (!std::same_as<std::remove_cvref_t<T>, Status> && !std::is_reference_v<T>)` so variant alternatives are never ambiguous.
- **Design Trade-offs:** `Result<T>` avoids exceptions and null pointers, though callers must inspect `.ok()` before calling `.value()` (or extract via `TINYDB_ASSIGN_OR_RETURN`).
- **Common Mistakes:** Trying to instantiate `std::variant<Status, Status>`; returning a dangling reference inside `Result<const T&>`; calling `.value()` without checking `.ok()`.
- **Understanding Checkpoint:** Why must `T` be constrained away from `Status` and references in `Result<T>`, and what problem does `Result<T>` solve compared to `(T* out, Status return)`?

### Component Context
- **Why It Exists:** Eliminates unsafe uninitialized out-parameters across parsing, catalog lookup, and deserialization APIs.
- **Module Relationships:** Used by `DiskManager::Open`, `PageHeader::Deserialize`, `Parser::Parse`, and `ByteBuffer` factories.

### Files Expected to Change
- `include/tinydb/core/result.hpp` (new)
- `tests/unit/core/result_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `template <typename T> class [[nodiscard]] Result` with C++20 concept/requires checks, value constructor, `Status` error constructor (rejecting `Status::Ok()` via assertion/fallback to `Internal` so an error `Result` is never `Ok`!), `ok()`, `status()`, `value()` (const/mut/rvalue ref overloads), `value_or(default)`, and `TINYDB_ASSIGN_OR_RETURN(lhs, expr)`.
2. Write unit tests with `int`, `std::string`, and move-only `ByteBuffer` proving zero-copy move extraction works.

### Testing Instructions
- Build and test with Debug and ASan+UBSan (`ctest --output-on-failure`).

### Acceptance Criteria
- `Result<ByteBuffer>` moves cleanly without copying.
- Constructing an error `Result` preserves the exact `Status`; calling `value()` on error aborts/throws deterministically in Debug or is checked cleanly in tests (`EXPECT_DEATH` or documented contract).

### Definition of Done
`Result<T>` tested with move-only types, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-006: implement C++20 Result<T> template with concepts`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-007` (Fixed-width DB types & `config.hpp`).
````

---

## DAY-007 — Fixed-Width Database Types (`PageId`, `FrameId`, `TxnId`, `Lsn`) and `config.hpp`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-007
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Fixed-Width Database Types (`PageId`, `FrameId`, `TxnId`, `Lsn`) and `config.hpp`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-006 complete.

### Objective
Define TinyDB's canonical fixed-width integer aliases, invalid sentinel constants, and engine compile-time configuration constants in `include/tinydb/core/types.hpp` and `include/tinydb/core/config.hpp`.

### Concepts & Daily Learning
- **Concept:** Fixed-width integer types (`std::uint32_t`, `std::uint64_t`, `std::int32_t`, `std::int64_t`), sentinel values (`kInvalidPageId`), and ABI-independent sizing.
- **Why Database Engines Need It:** Platform types like `long` or `int` vary across 32/64-bit Windows/Linux/macOS (`long` is 4B on LLP64 Windows, 8B on LP64 macOS/Linux); disk formats require exact byte widths across every platform.
- **How It Works:** Defining `using PageId = std::uint32_t;`, `using FrameId = std::uint32_t;`, `using SlotId = std::uint16_t;`, `using TxnId = std::uint64_t;`, `using Lsn = std::uint64_t;`, `using TableId = std::uint32_t;`, `using IndexId = std::uint32_t;` with explicit sentinel constants (`kInvalidPageId = UINT32_MAX`, `kInvalidLsn = 0`, etc.) keeps sizes and limits uniform.
- **Design Trade-offs:** Type aliases give lightweight readability (though distinct strong wrapper types catch swapped `PageId` vs `FrameId` at compile time — we document this trade-off and use clear parameter naming + `static_assert` size checks).
- **Common Mistakes:** Using `0` as `kInvalidPageId` when Page 0 is the valid Database File Header Page! Using `size_t` in on-disk formats.
- **Understanding Checkpoint:** Why is `kInvalidPageId` set to `UINT32_MAX` instead of `0` in TinyDB, and why must `size_t` never appear in a serialized disk layout?

### Component Context
- **Why It Exists:** Single source of truth for page size (`4096`), limits, and core identifier types.
- **Module Relationships:** Included by every storage, buffer, catalog, index, and recovery header.

### Files Expected to Change
- `include/tinydb/core/types.hpp` (new)
- `include/tinydb/core/config.hpp` (new)
- `include/tinydb/build_info.hpp` (align `kPageSizeBytes` to reference `config::kPageSize`)
- `tests/unit/core/types_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Create `include/tinydb/core/config.hpp` defining `kPageSize = 4096`, `kMaxVarcharLength = 4000`, `kDefaultBufferPoolSize = 128`, `kMaxSqlQueryBytes = 65536`, `kMaxAstDepth = 64`, `kFormatVersion = 1`.
2. Create `include/tinydb/core/types.hpp` defining `PageId`, `FrameId`, `SlotId`, `TxnId`, `Lsn`, `TableId`, `IndexId`, and sentinels (`kInvalidPageId`, `kInvalidFrameId`, `kInvalidSlotId`, `kInvalidTxnId`, `kInvalidLsn`, `kFileHeaderPageId = 0`), plus `static_assert` byte-width invariants.
3. Add unit tests verifying all constant widths and sentinel invariants.

### Testing Instructions
- Build and test with Debug and ASan/UBSan (`ctest --output-on-failure`).

### Acceptance Criteria
- `static_assert` confirms exact byte widths (`PageId == 4`, `Lsn == 8`, `SlotId == 2`).
- `kFileHeaderPageId == 0` and `kInvalidPageId != kFileHeaderPageId`.

### Definition of Done
Types and config implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-007: define core fixed-width database types and config constants`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-008` (Endianness & binary serialization helpers).
````

---

## DAY-008 — Endianness & Explicit Binary Serialization Helpers (`endian.hpp`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-008
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Endianness & Explicit Binary Serialization Helpers (`endian.hpp`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-002 (`std::span`), DAY-006 (`Result<T>`), DAY-007 (`types.hpp`).

### Objective
Implement explicit little-endian encoding/decoding functions (`ReadU16`, `ReadU32`, `ReadU64`, `ReadI32`, `ReadI64`, `WriteU16`, `WriteU32`, `WriteU64`, `WriteI32`, `WriteI64`) plus a sequential `BinaryWriter` / `BinaryReader` cursor over `std::span` in `include/tinydb/core/endian.hpp` and `src/core/endian.cpp`.

### Concepts & Daily Learning
- **Concept:** CPU endianness (little-endian vs big-endian), alignment safety, two's complement signed encoding, and bounds-checked cursor readers/writers.
- **Why Database Engines Need It:** Casting a `char*` buffer to `uint32_t*` invokes C++ Undefined Behavior (strict aliasing + unaligned pointer access) and silently breaks portability across CPU architectures.
- **How It Works:** Bit-shifting (`b[0] | (b[1] << 8) | ...` using `std::to_integer<uint64_t>`) reads/writes bytes in fixed little-endian order regardless of host CPU endianness and without requiring pointer alignment.
- **Design Trade-offs:** Byte-by-byte shift assembly is slightly more verbose than `memcpy` (though modern compilers optimize little-endian shifts into single `LDR`/`STR` instructions!) and guarantees identical byte order on big- and little-endian hosts.
- **Common Mistakes:** Shifting a `uint8_t` left by `>= 24` bits before widening to `uint64_t` (int promotion / signed shift UB!); reading past end of span without bounds check.
- **Understanding Checkpoint:** Why is `static_cast<uint64_t>(b[7]) << 56` required instead of `b[7] << 56`? What UB does `reinterpret_cast<const uint32_t*>(ptr)` risk on arbitrary offsets?

### Component Context
- **Why It Exists:** Enforces TinyDB's core rule: zero raw C++ struct serialization to disk.
- **Module Relationships:** Used by `FileHeader`, `PageHeader`, `Tuple`, `SlottedPage`, `BPlusTree` nodes, `CatalogManager`, and WAL `LogRecord`.

### Files Expected to Change
- `include/tinydb/core/endian.hpp` (new)
- `src/core/endian.cpp` (new)
- `tests/unit/core/endian_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement standalone checked `ReadU16/U32/U64/I32/I64` and `WriteU16/U32/U64/I32/I64` operating on `std::span` + byte offset (returning `Status` / `Result<T>` on out-of-bounds).
2. Implement `BinaryWriter` and `BinaryReader` classes with `WriteBytes`/`ReadBytes`, `WriteString`/`ReadString` (`uint16_t` length prefix), `Remaining()`, and `Offset()`.
3. Test roundtrips on unaligned offsets (`offset = 1, 3, 7`), `INT32_MIN/MAX`, `INT64_MIN/MAX`, `UINT64_MAX`, exact byte sequences matching little-endian spec, and truncated-buffer error checks.

### Testing Instructions
- Build and run `ctest` with UBSan+ASan (`-DTINYDB_ENABLE_SANITIZERS=ON`) to prove zero alignment or shift UB!

### Acceptance Criteria
- Unaligned offset reads/writes trigger zero UBSan warnings.
- Out-of-bounds reads/writes return `StatusCode::kOutOfRange` / `kCorruption` cleanly.

### Definition of Done
Endian helpers and cursor classes tested under UBSan, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-008: implement little-endian binary reader/writer helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-009` (STL containers, iterators & complexity lab) and `DAY-010` (CRC32).
````

---

## DAY-009 — STL Containers, Iterators & Complexity Analysis (Lookup Benchmark Lab)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-009
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** STL Containers, Iterators & Complexity Analysis (Lookup Benchmark Lab)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-008 complete.

### Objective
Implement reusable binary-search and ordered-container helper utilities (`include/tinydb/core/algorithms.hpp`) and write a comparative complexity test/study (`docs/notes/dsa-complexity.md`) evaluating `std::vector` linear scan vs sorted binary search vs `std::unordered_map` vs `std::list` + map (LRU precursor).

### Concepts & Daily Learning
- **Concept:** Contiguous `std::vector` cache locality, `std::lower_bound` / `std::upper_bound`, `std::unordered_map` bucket hashing, `std::list` iterator stability, and Big-O vs real cache behavior.
- **Why Database Engines Need It:** A Buffer Pool uses `unordered_map` + doubly-linked `list` for O(1) lookup and O(1) LRU move-to-front; B+ Tree pages use binary search (`O(log N)`) over contiguous sorted key arrays; sequential scans use linear iteration.
- **How It Works:** `std::lower_bound` finds the first element `>= key` in `O(log N)` comparisons on random-access iterators; `std::list::splice` moves a node in `O(1)` without invalidating iterators stored in a hash map.
- **Design Trade-offs:** Hash maps offer `O(1)` point lookups but zero range-query support (`WHERE k > 10`); balanced trees offer `O(log N)` point AND range scans.
- **Common Mistakes:** Invalidating iterators/references on `std::vector::push_back` reallocation; using `std::map` (red-black tree with poor pointer-chasing locality) inside a 4KB disk page instead of contiguous sorted arrays.
- **Understanding Checkpoint:** Why can `std::list` iterators remain valid inside an `unordered_map<FrameId, list::iterator>` when other elements are spliced or erased, whereas `vector` indices/iterators cannot?

### Component Context
- **Why It Exists:** Supplies tested binary search bounds helpers (`LowerBoundIndex`, `UpperBoundIndex`) later reused by B+ Tree nodes.
- **Module Relationships:** Direct precursor to `LRUReplacer` (Phase 04) and `BPlusTreeLeafPage`/`InternalPage` binary search (Phase 05).

### Files Expected to Change
- `include/tinydb/core/algorithms.hpp` (new)
- `tests/unit/core/algorithms_test.cpp` (new)
- `docs/notes/dsa-complexity.md` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement generic `LowerBoundIndex(std::span<const T>, const Key&, Comparator)` and `UpperBoundIndex(...)` returning `std::size_t`.
2. Add tests verifying exact indices on empty spans, duplicates, missing keys, smallest/largest boundaries, plus a small test exercising `std::list::splice` iterator stability for LRU cache mechanics.
3. Document Big-O complexities and cache/iterator rules concisely in `docs/notes/dsa-complexity.md`.

### Testing Instructions
- Build and run `ctest --output-on-failure` (Debug + ASan/UBSan).

### Acceptance Criteria
- Binary search helpers match `std::lower_bound`/`std::upper_bound` across duplicate and edge cases.
- Iterator stability test verifies zero dangling accesses under ASan.

### Definition of Done
Implemented, tested, documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-009: add binary search core helpers and DSA complexity notes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-010` (CRC32 checksum utility).
````

---

## DAY-010 — Data Integrity Foundations: Implementing CRC32 Checksum Utility

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-010
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Data Integrity Foundations: Implementing CRC32 Checksum Utility
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-002 (`std::span`), DAY-008 (`endian.hpp`).

### Objective
Implement standard IEEE 802.3 CRC32 (`0xEDB88320` reflected polynomial) in `include/tinydb/core/crc32.hpp` and `src/core/crc32.cpp` with `constexpr` lookup table generation and `ComputeCrc32` over `std::span<const std::byte>`.

### Concepts & Daily Learning
- **Concept:** Cyclic Redundancy Check (CRC32) error-detecting codes vs cryptographic hashes vs weak additive checksums.
- **Why Database Engines Need It:** Disk controllers, cables, and partial power-loss writes ("torn pages") can flip or zero bytes silently; without checksums, a database reads garbage bytes as valid tuple lengths or page IDs and crashes or returns wrong data.
- **How It Works:** Treats the byte stream as coefficients of a polynomial modulo a generator polynomial (`0x04C11DB7`, reflected `0xEDB88320`), precomputing a 256-entry `constexpr` lookup table at compile time so hashing a 4096-byte page takes ~4096 table lookups.
- **Design Trade-offs:** CRC32 is fast and catches burst/single-bit hardware corruption (though not adversarial tampering like SHA-256, which isn't needed for storage integrity).
- **Common Mistakes:** Including the checksum field's own old bytes when recomputing the checksum (must zero the checksum slot or exclude it consistently!); confusing reflected vs non-reflected polynomial (`"123456789"` must equal `0xCBF43926`!).
- **Understanding Checkpoint:** Why must TinyDB zero out the 4-byte checksum field (or compute CRC over `[0..offset) + zero(4) + [offset+4..end)`) before verifying a page checksum?

### Component Context
- **Why It Exists:** Core corruption-detection engine for FileHeader, Pages, Catalog, and WAL records.
- **Module Relationships:** Used directly by `storage/file_header`, `storage/page`, `catalog/catalog_manager`, and `recovery/log_record`.

### Files Expected to Change
- `include/tinydb/core/crc32.hpp` (new)
- `src/core/crc32.cpp` (new)
- `tests/unit/core/crc32_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `constexpr` 256-entry CRC32 table and `ComputeCrc32(std::span<const std::byte> data)` plus incremental `UpdateCrc32(std::uint32_t current, std::span<const std::byte> data)` + `FinalizeCrc32`.
2. Verify against the canonical test vector: `"123456789"` -> `0xCBF43926`, empty span -> `0x00000000`, single-bit flip detection across a 4096-byte buffer, and incremental multi-chunk equivalence.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `ComputeCrc32("123456789") == 0xCBF43926`.
- Flipping any single bit in a 4096-byte test page alters the CRC32 output.

### Definition of Done
CRC32 implemented and verified against standard test vectors, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-010: implement IEEE CRC32 checksum utility`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-011` & `DAY-012` (OS file I/O & `FileHandle`).
````

---

## DAY-011 — OS File Systems & Syscalls: File Descriptors, `fsync`/`F_FULLFSYNC`, and Durability Semantics

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-011
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** OS File Systems & Syscalls: File Descriptors, `fsync`/`F_FULLFSYNC`, and Durability Semantics
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-005 (`Status`), DAY-010 (`crc32`).

### Objective
Build portable POSIX I/O error-mapping and platform durability helper functions (`include/tinydb/core/os_io.hpp`, `src/core/os_io.cpp`) covering `errno` -> `Status` conversion, `EINTR`-safe `pread`/`pwrite` wrappers, `fsync`/`F_FULLFSYNC` (on macOS), and parent directory sync notes (`docs/notes/os-durability.md`).

### Concepts & Daily Learning
- **Concept:** POSIX file descriptors (`int fd`), kernel page cache, `write()` vs `pwrite()`, `EINTR` partial I/O loops, `fsync()` vs macOS `fcntl(fd, F_FULLFSYNC)`, and directory `fsync` after `rename()`.
- **Why Database Engines Need It:** `std::ofstream::flush()` only pushes userspace buffers to the OS kernel page cache—not to physical storage! Power loss after `write()` without `fsync()` loses committed transactions.
- **How It Works:** `pread`/`pwrite` perform atomic offset-based reads/writes without mutating a shared file cursor; looping on `EINTR` or short reads handles signal interruptions; `SyncFile(fd)` calls `fcntl(fd, F_FULLFSYNC)` on Apple platforms (falling back to `fsync` if unsupported) and `fsync(fd)` on POSIX/Linux.
- **Design Trade-offs:** `F_FULLFSYNC` flushes drive write caches (true durability on macOS) at much higher latency than plain `fsync`—crucial for WAL commits, unnecessary for temporary scratch writes.
- **Common Mistakes:** Assuming `write(fd, buf, 4096)` always writes all 4096 bytes in one call; ignoring `errno` before another syscall overwrites it; forgetting to `fsync` the parent directory after an atomic file `rename()`.
- **Understanding Checkpoint:** What is the difference between `std::fflush`, `write()`, `fsync()`, and macOS `F_FULLFSYNC`? Why does `pread` simplify offset I/O compared to `lseek` + `read`?

### Component Context
- **Why It Exists:** Isolates OS-specific POSIX syscalls (`<unistd.h>`, `<fcntl.h>`) cleanly inside `src/core/os_io.cpp`.
- **Module Relationships:** Foundation for `FileHandle` (DAY-012), `DiskManager` (Phase 01), `CatalogManager` atomic rename (Phase 03), and `LogManager` WAL flush (Phase 08).

### Files Expected to Change
- `include/tinydb/core/os_io.hpp` (new)
- `src/core/os_io.cpp` (new)
- `docs/notes/os-durability.md` (new)
- `tests/unit/core/os_io_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `StatusFromErrno(int err, std::string_view context)` mapping `ENOENT` -> `kNotFound`, `EEXIST` -> `kAlreadyExists`, `EACCES`/`EPERM` -> `kIoError`, etc.
2. Implement `PreadExact(int fd, std::span<std::byte> buf, std::uint64_t offset)` (detecting EOF short reads distinctly!) and `PwriteAll(int fd, std::span<const std::byte> buf, std::uint64_t offset)` looping over partial writes and `EINTR`.
3. Implement `SyncFd(int fd)` and `SyncParentDirectory(const std::filesystem::path& file_path)`.
4. Write unit tests using temporary files in `std::filesystem::temp_directory_path()` verifying offset writes/reads, short-read EOF detection, `StatusFromErrno`, and `SyncFd`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Partial-write and offset `PreadExact`/`PwriteAll` work accurately without racey `lseek`.
- EOF/truncated file reads return a clear `Status` error rather than zero-filled garbage.

### Definition of Done
OS I/O helpers and durability notes verified and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-011: implement POSIX pread/pwrite, fsync helpers, and errno mapping`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-012` (`FileHandle` RAII class).
````

---

## DAY-012 — Binary File I/O: Implementing RAII `FileHandle` with Explicit Read, Write, Seek, and Sync

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-012
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Binary File I/O: Implementing RAII `FileHandle` with Explicit Read, Write, Seek, and Sync
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-003 (RAII), DAY-011 (`os_io`).

### Objective
Implement `tinydb::FileHandle` (`include/tinydb/core/file_handle.hpp`, `src/core/file_handle.cpp`) — a move-only RAII wrapper around an OS file descriptor providing `CreateNew`, `OpenExisting`, `OpenOrCreate`, `ReadAt`, `WriteAt`, `Append`, `Size`, `Truncate`, `Sync`, and `Close`.

### Concepts & Daily Learning
- **Concept:** RAII file descriptor lifecycle, move-only handle ownership, `O_CREAT | O_EXCL` atomic exclusive creation, and explicit close vs destructor cleanup.
- **Why Database Engines Need It:** Raw `int fd` variables easily leak on error branches or get double-closed (a catastrophic bug in multithreaded code where a reused FD number closes someone else's file!).
- **How It Works:** `FileHandle` holds `int fd_ = -1`. Copy operations are deleted; move operations transfer `fd_` and set `other.fd_ = -1`; destructor calls `::close(fd_)` if `fd_ >= 0`. Explicit `Close()` returns any `close()` I/O error `Status` before destruction.
- **Design Trade-offs:** Returning `Status` from explicit `Close()` lets callers detect delayed write errors that POSIX `close()` surfaces, while the destructor guarantees zero FD leaks if an error path exits early.
- **Common Mistakes:** Ignoring `close()` failure on an explicit flush/close path; allowing `FileHandle` copies; leaving `other.fd_` non-negative after move.
- **Understanding Checkpoint:** Why should `FileHandle` have BOTH an explicit `Status Close()` method AND a `~FileHandle()` destructor?

### Component Context
- **Why It Exists:** Safe low-level file abstraction used by `DiskManager`, `CatalogManager`, and `LogManager`.
- **Module Relationships:** Built directly atop `os_io`; directly owned by `DiskManager` (Phase 01) and `LogManager` (Phase 08).

### Files Expected to Change
- `include/tinydb/core/file_handle.hpp` (new)
- `src/core/file_handle.cpp` (new)
- `tests/unit/core/file_handle_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FileHandle` factory methods (`CreateNew`, `OpenExisting`, `OpenOrCreate`) returning `Result<FileHandle>`.
2. Implement `ReadAt(offset, span)`, `WriteAt(offset, span)`, `FileSize()`, `Sync()`, `Close()`, `IsOpen()`, `Release()`, and move constructor/assignment.
3. Write unit tests verifying: exclusive `CreateNew` fails with `kAlreadyExists` on existing file, `OpenExisting` fails with `kNotFound` on missing file, `ReadAt`/`WriteAt` roundtrip, move transfer, and safe double-`Close()` idempotency.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero file descriptor leaks across repeated open/move/close cycles.
- Truncated `ReadAt` returns error `Status` cleanly.

### Definition of Done
`FileHandle` implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-012: implement move-only RAII FileHandle for binary file I/O`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-013` (Virtual memory & 4096B page management) and `DiskManager` in Phase 01.
````

---

## DAY-013 — Virtual Memory, OS Page Cache & Why Databases Manage Fixed 4096-Byte Pages

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-013
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Virtual Memory, OS Page Cache & Why Databases Manage Fixed 4096-Byte Pages
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-007 (`config.hpp`), DAY-012 (`FileHandle`).

### Objective
Implement an aligned 4096-byte raw memory buffer primitive (`AlignedPageStorage` in `include/tinydb/core/aligned_page.hpp`) and document the architectural rationale for DB-managed Buffer Pools vs `mmap` / OS page cache (`docs/notes/os-vs-db-caching.md`).

### Concepts & Daily Learning
- **Concept:** Virtual vs physical memory, TLB/page tables, OS page cache, double buffering, `mmap` pitfalls (`SIGBUS`, uncontrollable dirty writeback ordering), and cache-line/page alignment (`alignas(4096)`).
- **Why Database Engines Need It:** If a DBMS relies on `mmap` or OS page caching, the OS can evict and flush dirty uncommitted pages out of order before the WAL is written—breaking transactional atomicity and durability!
- **How It Works:** A DBMS allocates a fixed-size pool of aligned 4KB byte frames in userspace and explicitly controls **when** pages are read, pinned, evicted, and flushed relative to WAL LSNs.
- **Design Trade-offs:** Using standard `pread`/`pwrite` + userspace buffer pool causes double buffering (OS cache + DB buffer pool) unless `O_DIRECT`/`F_NOCACHE` is enabled, trading slightly higher RAM footprint for portable, explicit write ordering.
- **Common Mistakes:** Assuming `mmap` makes a DBMS faster and simpler without handling `SIGBUS` on I/O errors or dirty-page writeback ordering.
- **Understanding Checkpoint:** Name three concrete reasons why serious disk-oriented databases (like PostgreSQL and SQLite) manage their own buffer pool rather than relying purely on `mmap`.

### Component Context
- **Why It Exists:** Connects OS virtual memory concepts directly to TinyDB's fixed 4096-byte page buffer representation.
- **Module Relationships:** Feeds directly into `Page` (`Phase 01`) and `PageFrame` (`Phase 04`).

### Files Expected to Change
- `include/tinydb/core/aligned_page.hpp` (new)
- `docs/notes/os-vs-db-caching.md` (new)
- `tests/unit/core/aligned_page_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `AlignedPageBuffer` holding `alignas(config::kPageSize) std::array<std::byte, config::kPageSize>` (or heap-allocated `std::unique_ptr` with `kPageSize` alignment so stack frames stay small!) plus `Zero()`, `AsSpan()`, `AsConstSpan()`, and alignment assertions.
2. Write unit tests verifying `sizeof`, alignment (`reinterpret_cast<uintptr_t>(buf.Data()) % 4096 == 0`), zero-initialization, and move behavior.
3. Write `docs/notes/os-vs-db-caching.md` clearly explaining virtual memory, OS page cache, and `mmap` vs Buffer Pool trade-offs.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `AlignedPageBuffer` guarantees `4096`-byte size and alignment without blowing stack space on heap allocation.
- All unit tests pass cleanly under ASan/UBSan.

### Definition of Done
Code and notes created, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-013: add aligned 4KB page buffer and OS caching notes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-014` (Concurrency primitives).
````

---

## DAY-014 — C++20 Concurrency Primitives (`std::mutex`, `std::shared_mutex`, `std::scoped_lock`) & Thread-Safety Contract

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-014
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** C++20 Concurrency Primitives (`std::mutex`, `std::shared_mutex`, `std::scoped_lock`) & Thread-Safety Contract
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-003 (RAII), DAY-013 complete.

### Objective
Define TinyDB's module thread-safety contract (`docs/notes/concurrency-contract.md`) and implement a lightweight thread-safe `ThreadSafeCounter` / `RwLatch` wrapper (`include/tinydb/core/latch.hpp`) with multi-threaded unit tests verifying race-free lock acquisition.

### Concepts & Daily Learning
- **Concept:** Data races (C++ UB), `std::mutex`, `std::unique_lock` vs `std::scoped_lock`, `std::shared_mutex` (readers-writer latch), and lock ordering to prevent deadlocks.
- **Why Database Engines Need It:** Even in a single-active-transaction v1 engine, background/test callers and `BufferPoolManager` / `DiskManager` stats benefit from clear mutex invariants and deadlock-free latching rules.
- **How It Works:** `std::scoped_lock` locks one or more mutexes using deadlock-avoidance ordering; `std::shared_mutex` allows multiple concurrent readers (`std::shared_lock`) OR one exclusive writer (`std::unique_lock`).
- **Design Trade-offs:** Coarse single mutexes (`BufferPoolManager::latch_`) are trivial to reason about and deadlock-free in v1, trading parallel throughput (a v2 optimization via lock striping/crabbing) for correctness.
- **Common Mistakes:** Calling `lock()`/`unlock()` manually instead of RAII lock guards; invoking disk I/O or recursive callbacks while holding a shared mutex inconsistently; acquiring two locks in opposite orders across functions.
- **Understanding Checkpoint:** What is the difference between a database **lock** (transaction-level logical isolation across queries) and a **latch** (short-lived mutex protecting an in-memory data structure like a page frame)?

### Component Context
- **Why It Exists:** Establishes explicit latch conventions before building `DiskManager` and `BufferPoolManager`.
- **Module Relationships:** Used by `DiskManager` (Phase 01), `LRUReplacer` & `BufferPoolManager` (Phase 04), and `TransactionManager` (Phase 08).

### Files Expected to Change
- `include/tinydb/core/latch.hpp` (new)
- `docs/notes/concurrency-contract.md` (new)
- `tests/unit/core/latch_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ReaderWriterLatch` (wrapping `std::shared_mutex` with `WLock()`, `WUnlock()`, `RLock()`, `RUnlock()`, plus RAII `ReadGuard` and `WriteGuard`) and non-copyable semantics.
2. Spawn 4+ `std::jthread` / `std::thread` test workers exercising concurrent shared reads and exclusive writes under ASan+UBSan.
3. Document TinyDB v1.0's thread-safety contract (which classes are thread-safe vs externally synchronized) in `docs/notes/concurrency-contract.md`.

### Testing Instructions
- Build and test with `ctest --output-on-failure` (Debug and ASan+UBSan).

### Acceptance Criteria
- Multi-threaded unit test executes cleanly with zero lost updates, zero deadlocks, and zero sanitizer errors.

### Definition of Done
Latch wrapper, test, and concurrency contract completed, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-014: add ReaderWriterLatch and v1 concurrency contract`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-015` (Relational model & schema design foundations).
````

---

## DAY-015 — Relational Model Fundamentals: Tables, Schemas, Primary Keys & Normalization Design Notes

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-015
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Relational Model Fundamentals: Tables, Schemas, Primary Keys & Normalization Design Notes
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-007 (`types.hpp`), DAY-008 (`endian.hpp`).

### Objective
Bridge relational database theory to physical engine design by writing `docs/notes/relational-fundamentals.md` (relations, tuples, attributes, primary keys, 1NF–3NF normalization, logical-vs-physical separation) and implementing core identifier validation helpers (`IsValidIdentifier`) in `include/tinydb/core/identifier.hpp`.

### Concepts & Daily Learning
- **Concept:** Relations, schemas, tuples, candidate/primary keys, 1NF/2NF/3NF normalization anomalies, and logical schema vs physical row/page storage.
- **Why Database Engines Need It:** SQL tables represent unordered multisets/relations logically, but disk files are flat 1D sequences of 4KB pages; understanding how logical columns map to physical bytes prevents brittle schemas and inconsistent identifiers.
- **How It Works:** Every table has a fixed ordered `Schema` of typed `Column`s; physical rows (`Tuple`s) encode values according to that schema and live at physical `RowID(page_id, slot_id)` locations independent of logical key values.
- **Design Trade-offs:** Separating physical `RowID` from primary key (heap organization) vs clustered primary-key B+ Trees (deferred; heap + secondary integer B+ Tree keeps updates/inserts clean in v1).
- **Common Mistakes:** Coupling `RowID` to row contents so updates break indexes without careful `RowID` maintenance; accepting arbitrary unvalidated table/column identifiers (`""`, control chars, >64 chars).
- **Understanding Checkpoint:** Why can a database not rely on a row's byte offset in a file staying fixed forever, and how does `(page_id, slot_id)` solve half of that problem?

### Component Context
- **Why It Exists:** Shared identifier validation (`[A-Za-z_][A-Za-z0-9_]*`, max length 64) used by schemas, catalog, and SQL parser.
- **Module Relationships:** Reused by `Schema` (Phase 02), `CatalogManager` (Phase 03), and `Parser` (Phase 06).

### Files Expected to Change
- `include/tinydb/core/identifier.hpp` (new)
- `src/core/identifier.cpp` (new)
- `docs/notes/relational-fundamentals.md` (new)
- `tests/unit/core/identifier_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `IsValidIdentifier(std::string_view name)` and `NormalizeIdentifier(std::string_view name)` (ASCII lowercase normalization for case-insensitive table/column names, or exact policy documented!).
2. Write comprehensive unit tests covering valid identifiers, empty names, leading digits, embedded NUL/spaces, and >64-char overflows.
3. Document relational theory + normalization examples in `docs/notes/relational-fundamentals.md`.

### Testing Instructions
- Build and run `ctest --output-on-failure` (Debug and ASan/UBSan).

### Acceptance Criteria
- Identifier validator rejects empty, overlong, or punctuation-containing names cleanly.

### Definition of Done
Identifier utilities, tests, and relational notes verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-015: add SQL identifier validation and relational model notes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-016` (B+ Tree & fanout theory).
````

---

## DAY-016 — Indexing & B+ Tree Theory: Balanced Trees, 4KB Fanout Calculations & Binary Search Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-016
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Indexing & B+ Tree Theory: Balanced Trees, 4KB Fanout Calculations & Binary Search Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-009 (`algorithms.hpp`), DAY-015.

### Objective
Derive exact 4096-byte page fanout capacities and tree height formulas for B+ Trees vs BSTs/AVL trees (`docs/notes/btree-math.md`) and build an educational in-memory sorted-array page-fanout simulation test in `tests/unit/core/btree_fanout_test.cpp`.

### Concepts & Daily Learning
- **Concept:** Binary search trees, AVL/Red-Black trees (fanout 2) vs B-Trees vs B+ Trees (high fanout `M`), leaf sibling chains, and I/O complexity `O(log_M N)`.
- **Why Database Engines Need It:** A 1-million-key binary tree has height `~20` (20 random disk reads = ~100ms on HDD / milliseconds on SSD); a 4KB B+ Tree with fanout `~250` has height `3` (`250^3 = 15.6M`), keeping root & upper levels pinned in the Buffer Pool for 1–2 disk reads!
- **How It Works:** Internal nodes store only separator keys and child `PageId`s (maximizing fanout), while all `RowID` values live in doubly/singly-linked leaf nodes for fast range scans.
- **Design Trade-offs:** B+ Tree vs B-Tree (B+ Tree sacrifices keeping values in internal nodes to maximize internal fanout and make sequential range scans follow leaf pointers without tree backtracking).
- **Common Mistakes:** Confusing tree order `M` with bytes vs entries; forgetting header bytes (`32B` `PageHeader` + node metadata) when calculating `max_size`.
- **Understanding Checkpoint:** With a 4096-byte page, a 64-byte header, 8-byte `int64_t` keys, and 4-byte `PageId`s, compute the exact max fanout of an internal node and max entries of a leaf node (`8B key + 6B/8B RowID`).

### Component Context
- **Why It Exists:** Locks down exact B+ Tree capacity math before writing disk-backed node serialization in Phase 05.
- **Module Relationships:** Guides `docs/btree-design.md` and Phase 05 node sizes.

### Files Expected to Change
- `docs/notes/btree-math.md` (new)
- `tests/unit/core/btree_fanout_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `docs/notes/btree-math.md` showing exact byte-budget arithmetic for TinyDB's 4096-byte internal and leaf nodes, plus I/O comparison against BST and linear scan.
2. Write `tests/unit/core/btree_fanout_test.cpp` verifying compile-time byte-budget formulas (`header + max_entries * entry_size <= 4096`) and exercising `LowerBoundIndex` on leaf-sized sorted arrays.

### Testing Instructions
- Build and test with `ctest --output-on-failure` (Debug + ASan/UBSan).

### Acceptance Criteria
- Fanout sanity assertions hold statically (`<= 4096` bytes with zero overflow) and tests pass.

### Definition of Done
Math verified, tests pass, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-016: document 4KB B+ Tree fanout math and verify capacity bounds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-017` (ACID, transactions & WAL mental model).
````

---

## DAY-017 — Transactions, ACID, Locking & WAL Mental Model: End-to-End Lifecycle Simulation Test

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-017
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Transactions, ACID, Locking & WAL Mental Model: End-to-End Lifecycle Simulation Test
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-011 (`fsync`), DAY-013 (Buffer Pool), DAY-014 (`latch.hpp`).

### Objective
Document TinyDB's ACID guarantees, no-steal/redo-only recovery model, and locking vs latching rules (`docs/notes/acid-and-wal-model.md`), and build a minimal in-memory WAL/crash-replay conceptual simulation test (`tests/unit/core/wal_concept_test.cpp`) verifying why WAL flush must precede commit ack and why LSN checks make redo idempotent.

### Concepts & Daily Learning
- **Concept:** Atomicity, Consistency, Isolation, Durability (ACID), Steal vs No-Steal, Force vs No-Force, Write-Ahead Logging (WAL), and Page LSN idempotence.
- **Why Database Engines Need It:** Updating a 4KB B+ Tree leaf AND table page cannot happen atomically in one hardware instruction; a crash midway leaves corrupted pointers unless changes are first logged durably to an append-only WAL.
- **How It Works:** Under TinyDB's v1 **No-Steal, Redo-Only** model: uncommitted pages stay in RAM (never flushed to disk), runtime `ROLLBACK` restores before-images in RAM, `COMMIT` writes after-images + `COMMIT` record to WAL and calls `fsync`, and crash recovery replays only committed LSNs where `page_lsn < record_lsn`.
- **Design Trade-offs:** No-Steal simplifies v1 recovery (no disk UNDO needed) at the cost of requiring RAM to hold a single transaction's dirty pages—acceptable for an educational v1 engine, upgraded to full ARIES in v2.
- **Common Mistakes:** Reporting `COMMIT` success before `fsync(wal_fd)`; reapplying an older WAL record over a newer flushed page without checking `page_lsn < record_lsn`.
- **Understanding Checkpoint:** Trace what happens if the server crashes (a) right before WAL `fsync`, (b) right after WAL `fsync` before page flushing, and (c) halfway through flushing dirty pages to `.tinydb`.

### Component Context
- **Why It Exists:** Ensures complete conceptual clarity of Phase 08 invariants from Day 17 onward so earlier modules (`PageHeader::lsn`, `BufferPoolManager` pin/dirty rules) support WAL seamlessly.
- **Module Relationships:** Guides `PageHeader` (Phase 01), `BufferPoolManager` (Phase 04), and Phase 08 (`TransactionManager`, `LogManager`, `RecoveryManager`).

### Files Expected to Change
- `docs/notes/acid-and-wal-model.md` (new)
- `tests/unit/core/wal_concept_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `docs/notes/acid-and-wal-model.md` tracing all 3 crash timing scenarios step by step.
2. Write `tests/unit/core/wal_concept_test.cpp` simulating an in-memory `DiskState` + `WalLog` verifying: uncommitted txn lost on crash, committed txn recovered via Redo, and double-recovery with `page_lsn >= record_lsn` remains idempotent.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Simulation test proves idempotent Redo across partial flush + double recovery.

### Definition of Done
Conceptual WAL/ACID test passes, notes complete, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-017: document ACID/WAL model and add idempotent redo concept test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-018` (Phase 00 sign-off).
````

---

## DAY-018 — Core Architecture Blueprint Review, Core Module Integration Tests & Phase 00 Sign-Off

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-018
- **Phase:** Phase 00 — Prerequisites and Architecture
- **Session Title:** Core Architecture Blueprint Review, Core Module Integration Tests & Phase 00 Sign-Off
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-001 through DAY-017 complete.

### Objective
Audit all `src/core/` utilities (`slice_utils`, `scope_guard`, `byte_buffer`, `status`, `result`, `types`, `config`, `endian`, `algorithms`, `crc32`, `os_io`, `file_handle`, `aligned_page`, `latch`, `identifier`), write an end-to-end `core_integration_test.cpp` combining file I/O + endian serialization + CRC32 + aligned pages, verify `docs/architecture.md`, and formally complete Phase 00.

### Concepts & Daily Learning
- **Concept:** Integration testing across core primitives, API boundary contracts, and clean phase-gate engineering reviews.
- **Why Database Engines Need It:** Unit tests validate parts in isolation; integration tests catch mismatches at seams (e.g., endian cursor + CRC32 calculation + POSIX `FileHandle` offset writes).
- **How It Works:** Write a 4KB aligned page containing encoded header fields and CRC32 using `BinaryWriter` into a `FileHandle`, `Sync()`, reopen in a fresh `FileHandle`, read back, verify CRC32, and test `BinaryReader` extraction.
- **Design Trade-offs:** Fixing interface friction in `core/` now avoids costly refactors when 10+ storage/buffer files depend on them.
- **Common Mistakes:** Marking Phase 00 complete without verifying ASan+UBSan+`-Werror` across ALL tests; leaving unused scratch files behind.
- **Understanding Checkpoint:** Walk through every layer of `docs/architecture.md` and explain how `core/` primitives support Phase 01's `DiskManager` without raw struct writes or memory leaks.

### Component Context
- **Why It Exists:** Completes the foundational `tinydb_core` library before Phase 01 starts building the on-disk engine.
- **Module Relationships:** `tinydb_core` becomes the clean dependency base for `storage`, `buffer`, `catalog`, `index`, `parser`, `execution`, `transaction`, and `recovery`.

### Files Expected to Change
- `tests/unit/core/core_integration_test.cpp` (new)
- `docs/architecture.md` (review & sync any updated core filenames)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Review all headers in `include/tinydb/core/` for consistent naming, `[[nodiscard]]`, docstrings, and `clang-format` compliance.
2. Add `tests/unit/core/core_integration_test.cpp` exercising `AlignedPageBuffer` + `BinaryWriter` + `ComputeCrc32` + `FileHandle` write/sync/reopen/verify.
3. Run full Debug, Release, and ASan+UBSan test suites with `-DTINYDB_WARNINGS_AS_ERRORS=ON`.
4. Mark Phase 00 complete in `PROGRESS.md` (14.0 / 14.0 hours completed) and point `NEXT_SESSION.md` to `DAY-019`.

### Testing Instructions
- Full matrix: Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format` dry-run check.

### Acceptance Criteria
- 100% of Phase 00 unit and integration tests pass under ASan+UBSan with zero warnings and zero leaks.
- All `core/` headers match `docs/architecture.md`.

### Definition of Done
Phase 00 verified and signed off, `PROGRESS.md` shows `14.0 hours` completed, `NEXT_SESSION.md` points to `DAY-019`, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-018: complete Phase 00 core integration tests and sign off`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 01 (`DAY-019` — Database file header design).
````
