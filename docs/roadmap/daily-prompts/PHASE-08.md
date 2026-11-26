# Phase 08 — Transactions and WAL (`DAY-205` – `DAY-240`)

**Phase Budget:** 28.0 hours (32 × 45 min + 4 × 60 min = 36 sessions)
**Goal:** Implement single-active transactions (`BEGIN`, `COMMIT`, `ROLLBACK`), runtime undo before-images, strict No-Steal buffer-pool enforcement, checksummed WAL records (`LogManager`), durable WAL commit flushing (`fsync`), idempotent redo-only `RecoveryManager`, and controlled crash testing before/after commit and during recovery.

---

## DAY-205 — Finalize No-Steal, Redo-Only Transaction & WAL Architecture Specification

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-205
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Finalize No-Steal, Redo-Only Transaction & WAL Architecture Specification
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 07 complete (`DAY-204`) — storage engine and query engine verified stable.

### Objective
Finalize `docs/transaction-design.md` specifying TinyDB v1's exact No-Steal, Redo-Only architecture: single active transaction rules, in-memory page before-images (for `ROLLBACK`), Buffer Pool No-Steal dirty-page pin/hold rules, binary WAL file & `LogRecord` layout (`PAGE_IMAGE` after-images + `BEGIN`/`COMMIT`/`ABORT`), `PageHeader::lsn` rules, and DDL-outside-transaction guards.

### Concepts & Daily Learning
- **Concept:** **No-Steal / No-Force** (with WAL Force at commit) simplified Redo-Only recovery vs full Steal/No-Force ARIES.
- **Why Database Engines Need It:** If uncommitted dirty pages could be evicted/written to `.tinydb` ("Steal"), a crash before `COMMIT` would leave uncommitted changes on disk requiring physical/logical disk UNDO and CLRs! With **No-Steal** + **Single Active Transaction**, uncommitted changes NEVER touch `.tinydb`, eliminating disk UNDO completely while still teaching real WAL durability and idempotent Redo!
- **How It Works:**
  1. On `BEGIN`, allocate monotonic `TxnId`.
  2. Before modifying any page inside an active txn, snapshot its pre-transaction 4096B image (`before_images_`) in `Transaction` AND pin/protect that frame as **No-Steal** (`uncommitted_dirty_ = true` so `BufferPoolManager` never evicts or flushes it to `.tinydb`!).
  3. On `ROLLBACK`, restore all modified pages from `before_images_` cleanly in RAM (restoring `TableHeap` + `BPlusTree` + `root_page_id` instantaneously!), release No-Steal pins, and discard!
  4. On `COMMIT`, append WAL `PAGE_IMAGE` after-image records (stamping `PageHeader::lsn = record.lsn` on each page!), append WAL `COMMIT` record, call `LogManager::Flush()` (`fsync`!), and ONLY THEN release No-Steal holds so pages can flush/evict to `.tinydb`!
- **Design Trade-offs:** Notice how snapshotting 4KB page `before_images_` in `Transaction` makes runtime `ROLLBACK` 100% exact across BOTH `TableHeap` AND `BPlusTree` (including splits, merges, and `root_page_id`!) without fragile inverse B+ Tree merge logic! (Plus tracking allocated pages to deallocate on rollback, or restoring `FileHeader`—let's document exact `NewPage`/`DeletePage` handling inside a transaction cleanly!).
- **Common Mistakes:** Forgetting what happens if a `Transaction` allocates a NEW page (`NewPage`) or changes `BPlusTree::root_page_id` and then runs `ROLLBACK`! (Saving `initial_root_page_ids` + `newly_allocated_pages` inside `Transaction` lets `ROLLBACK` free newly allocated pages cleanly too! Note: disallow `DeletePage` flushing/freelist writes during uncommitted txn by deferring `DeletePage` until commit or restoring pages—even cleaner!).
- **Understanding Checkpoint:** Trace why storing `before_images_` (4KB page snapshots) + `deferred_deallocations_` + `saved_index_roots_` inside `Transaction` restores `TableHeap`, `BPlusTree` splits, AND `CatalogManager` root pointers cleanly on `ROLLBACK`!

### Component Context
- **Why It Exists:** Locks down exact contracts between `BufferPoolManager`, `TransactionManager`, `LogManager`, and `RecoveryManager` before coding.
- **Module Relationships:** Blueprint for `DAY-206`–`DAY-240`.

### Files Expected to Change
- `docs/transaction-design.md` (complete exact binary & state-machine spec)
- `docs/design-decisions.md` (confirm/expand `DD-006`)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Finalize all binary offsets, state diagrams, and `NewPage`/`DeletePage` rollback rules in `docs/transaction-design.md`.
2. Verify build stays green (`ctest --test-dir build --output-on-failure`).

### Testing Instructions
- Verify existing test suite runs clean.

### Acceptance Criteria
- Every edge case (`NewPage` inside txn, `DeletePage` inside txn, `root_page_id` change inside txn, DDL rejection inside txn) has an explicit design rule in `docs/transaction-design.md`.

### Definition of Done
Transaction & WAL specification finalized, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-205: finalize No-Steal Redo-Only transaction and WAL architecture spec`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-206` (`Transaction` & `TransactionManager` skeleton).
````

---

## DAY-206 — Implement `Transaction` State Model and `TransactionManager` (Single Active Transaction)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-206
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `Transaction` State Model and `TransactionManager` (Single Active Transaction)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-205 complete.

### Objective
Implement `Transaction` (`txn_id`, `TxnState`: `kActive`, `kCommitted`, `kAborted`, `prev_lsn`, `is_autocommit`) and `TransactionManager` (`include/tinydb/transaction/transaction.hpp`, `transaction_manager.hpp`, `src/transaction/*.cpp`) enforcing **at most one active transaction at a time** with monotonic `next_txn_id_`.

### Concepts & Daily Learning
- **Concept:** Transaction state transitions (`kActive -> kCommitted` / `kAborted`), monotonic `TxnId` allocation, and single-active-transaction mutual exclusion.
- **Why Database Engines Need It:** Prevents accidental nested `BEGIN` inside an open transaction (`"Transaction already active"`) or calling `COMMIT`/`ROLLBACK` when no transaction is active!
- **How It Works:** `TransactionManager` holds `std::unique_ptr<Transaction> active_txn_` and `TxnId next_txn_id_`. `Begin()` checks `active_txn_ == nullptr`, allocates `next_txn_id_++`, returns `Transaction*`. `HasActiveTransaction()` and `ActiveTxn()` expose status cleanly.
- **Design Trade-offs:** Returning explicit error `Status::InvalidArgument("Transaction already active")` on nested `BEGIN` matches TinyDB's v1 single-transaction spec (`SAVEPOINT`/nested txns deferred).
- **Common Mistakes:** Silently treating a second `BEGIN` as a no-op or resetting the active transaction without rolling back!
- **Understanding Checkpoint:** Why must `TransactionManager::Begin()` reject a nested `BEGIN` rather than overwriting `active_txn_`?

### Component Context
- **Why It Exists:** Tracks the lifecycle of the single active transaction.
- **Module Relationships:** Owned by `Database` / `ExecutionEngine`; extended in `DAY-209`–`DAY-211`.

### Files Expected to Change
- `include/tinydb/transaction/transaction.hpp` (new)
- `include/tinydb/transaction/transaction_manager.hpp` (new)
- `src/transaction/transaction.cpp` (new)
- `src/transaction/transaction_manager.cpp` (new)
- `tests/unit/transaction/transaction_manager_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Transaction` and `TransactionManager` (initial `Begin`, `Commit` state, `Abort` state stubs ready for WAL/undo wiring).
2. Add unit tests verifying monotonic `TxnId`s, nested `BEGIN` rejection, `COMMIT`/`ROLLBACK` with no active txn rejection, and post-commit/abort new `BEGIN` succeeding!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- State transitions (`Active -> Committed/Aborted`) and single-active exclusion pass all unit tests.

### Definition of Done
`Transaction` and `TransactionManager` skeleton tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-206: implement Transaction state model and single-active TransactionManager`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-207` (SQL `BEGIN`, `COMMIT`, `ROLLBACK` parsing).
````

---

## DAY-207 — Extend SQL Lexer, AST, and Parser for `BEGIN`, `COMMIT`, and `ROLLBACK`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-207
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Extend SQL Lexer, AST, and Parser for `BEGIN`, `COMMIT`, and `ROLLBACK`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-206 complete.

### Objective
Add `BEGIN`, `COMMIT`, `ROLLBACK` keywords (`TokenType::kBegin`, `kCommit`, `kRollback`), AST statement nodes (`BeginStmt`, `CommitStmt`, `RollbackStmt`), and `Parser` rules (`BEGIN [TRANSACTION];`, `COMMIT [TRANSACTION];`, `ROLLBACK [TRANSACTION];` or concise `BEGIN;`/`COMMIT;`/`ROLLBACK;`), updating `docs/sql-spec.md`.

### Concepts & Daily Learning
- **Concept:** Extending the SQL frontend cleanly for transaction control language (TCL) statements.
- **Why Database Engines Need It:** Users and tests drive transactions via standard SQL `BEGIN; ... COMMIT;` and `BEGIN; ... ROLLBACK;` inside `ExecuteSql` and CLI REPL!
- **How It Works:** Add keywords to `Lexer` lookup table, add `StmtKind::kBegin`, `kCommit`, `kRollback` AST nodes, parse cleanly in `ParseStatement()`, reject unexpected trailing arguments before `;`/`EOF`.
- **Design Trade-offs:** Supporting optional `TRANSACTION` keyword (`BEGIN;` or `BEGIN TRANSACTION;`) if simple—or exact `BEGIN;`/`COMMIT;`/`ROLLBACK;` documented clearly in `docs/sql-spec.md`.
- **Common Mistakes:** Forgetting to update `TokenTypeName` or `Parser::ParseStatement` switch coverage (`-Wswitch` catches this immediately!).
- **Understanding Checkpoint:** Verify that `BEGIN`, `COMMIT`, `ROLLBACK` are case-insensitive in `Lexer` and parse cleanly both standalone and inside multi-statement batches (`BEGIN; INSERT ...; COMMIT;`).

### Component Context
- **Why It Exists:** Exposes transaction control to SQL callers and CLI.
- **Module Relationships:** Extends `src/parser/`; consumed by `ExecutionEngine` (`DAY-208`).

### Files Expected to Change
- `include/tinydb/parser/token.hpp`, `src/parser/token.cpp`, `src/parser/lexer.cpp`
- `include/tinydb/parser/ast.hpp`, `include/tinydb/parser/ast_stmt.hpp`, `src/parser/ast.cpp`, `src/parser/parser.cpp`
- `docs/sql-spec.md`
- `tests/unit/parser/parser_txn_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Extend `TokenType`, `Lexer`, `AST`, and `Parser` for `BEGIN`, `COMMIT`, `ROLLBACK`.
2. Add parser unit tests (single statement, case-insensitive, batch `BEGIN; INSERT ...; COMMIT;`, trailing garbage rejection) and run parser fuzz test too!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All transaction statements parse cleanly across batches; invalid trailing tokens fail cleanly.

### Definition of Done
TCL lexer/AST/parser tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-207: add BEGIN, COMMIT, and ROLLBACK to SQL Lexer, AST, and Parser`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-208` (Auto-commit vs explicit transaction semantics).
````

---

## DAY-208 — Implement Auto-Commit vs Explicit Transaction Semantics in `ExecutionEngine`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-208
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement Auto-Commit vs Explicit Transaction Semantics in `ExecutionEngine`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-207 complete.

### Objective
Wire `BEGIN`, `COMMIT`, and `ROLLBACK` dispatch plus auto-commit wrapping into `ExecutionEngine`: standalone DML statements outside an explicit `BEGIN` auto-begin an implicit transaction and auto-commit on success (or auto-rollback on error!), while DDL (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`) is cleanly rejected inside an active explicit transaction per v1 spec!

### Concepts & Daily Learning
- **Concept:** Auto-commit mode vs explicit transaction blocks, and non-transactional v1 DDL isolation (`DD-004`).
- **Why Database Engines Need It:** In Phase 07, standalone `INSERT`/`UPDATE` worked without `BEGIN`; auto-commit wraps each standalone DML in an implicit `[Begin -> Execute -> Commit/Rollback]` so EVERY durable modification passes through the WAL uniformly! And rejecting DDL inside `BEGIN ... COMMIT` prevents mixing non-transactional `.cat` renames with rollbackable DML!
- **How It Works:** When `ExecuteStatement` sees `kBegin`, `kCommit`, `kRollback`, delegate to `TransactionManager`. When it sees DML (`Insert`, `Update`, `Delete`) and `!txn_mgr.HasActiveTransaction()`, start an implicit `autocommit_txn`, pass it via `ExecutionContext`, commit on `ok()` or abort on error! When it sees DDL while `HasActiveTransaction() && !is_autocommit`, return `Status::Unsupported("DDL statements cannot run inside an explicit transaction in TinyDB v1")`.
- **Design Trade-offs:** Notice also what happens on `Database::Close()` / CLI `.exit` if an explicit transaction is still uncommitted (`kActive`)! Auto-rollback uncommitted active transactions on clean shutdown so uncommitted changes are NEVER left dirty!
- **Common Mistakes:** Leaving an uncommitted explicit transaction hanging when `Database::Close()` runs—or accidentally flushing its dirty pages via `Shutdown()`!!
- **Understanding Checkpoint:** What MUST `Database::Close()` do if the user runs `BEGIN; INSERT INTO t VALUES (1); .exit` WITHOUT typing `COMMIT;`?

### Component Context
- **Why It Exists:** Connects SQL statements to `TransactionManager` cleanly.
- **Module Relationships:** Integrates `ExecutionEngine`, `CliSession`, and `TransactionManager`.

### Files Expected to Change
- `include/tinydb/execution/execution_engine.hpp`, `src/execution/execution_engine.cpp`
- `tests/unit/execution/txn_semantics_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `BEGIN`/`COMMIT`/`ROLLBACK` routing, auto-commit DML wrapping, DDL-in-txn rejection, and auto-abort on `Database::Close()` when an explicit txn remains active.
2. Add tests verifying: nested `BEGIN` fails, `COMMIT`/`ROLLBACK` without `BEGIN` fails, DDL inside `BEGIN` fails cleanly while leaving txn active or safe, and auto-commit works transparently for earlier SQL queries!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Earlier SQL tests continue passing via auto-commit; explicit `BEGIN`/`COMMIT`/`ROLLBACK` and DDL rejection pass cleanly.

### Definition of Done
Auto-commit and explicit transaction dispatch tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-208: wire BEGIN/COMMIT/ROLLBACK and auto-commit in ExecutionEngine`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-209` (No-Steal enforcement in `BufferPoolManager`).
````

---

## DAY-209 — Implement Strict No-Steal Policy in `BufferPoolManager`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-209
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement Strict No-Steal Policy in `BufferPoolManager`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-208 complete, Phase 04 `BufferPoolManager`.

### Objective
Add explicit No-Steal protection to `PageFrame` and `BufferPoolManager` (`MarkPageUncommitted(PageId)`, `ReleaseUncommittedHolds(span<PageId>)` or `PinUncommitted` / `FlushAllCommittedPages()`): preventing `LRUReplacer` from evicting AND preventing `FlushPage`/`FlushAllPages`/`Shutdown` from writing ANY uncommitted transaction page to `.tinydb` until `COMMIT` finishes flushing the WAL!

### Concepts & Daily Learning
- **Concept:** **No-Steal** buffer pool rule ("uncommitted dirty frames never leave RAM for disk").
- **Why Database Engines Need It:** Even if `FlushAllPages()` or LRU eviction runs while a transaction is still active (before `COMMIT`), writing an uncommitted page to `.tinydb` breaks Redo-Only recovery (if power fails right after, `.tinydb` would contain uncommitted bytes!).
- **How It Works:** Add `bool uncommitted_hold_ = false` on `PageFrame` (distinct from normal `pin_count_` so operators still acquire/release `PageGuard`s normally while the frame stays held in `replacer_` as non-evictable!). Specifically:
  - When `MarkUncommitted(page_id)` sets `uncommitted_hold_ = true` (after capturing before-image!), `UnpinPage` will ONLY put `fid` into `LRUReplacer` when `pin_count_ == 0 && !uncommitted_hold_`!
  - `FlushPage`/`FlushAllPages`/`Shutdown` SKIP (or refuse!) flushing any frame with `uncommitted_hold_ == true`!
  - On `COMMIT` (post-WAL-flush!) or `ROLLBACK` (post-before-image-restore!), `ReleaseUncommitted(page_id, is_dirty_after)` clears `uncommitted_hold_ = false` and puts `fid` back in `LRUReplacer` if `pin_count_ == 0`!
- **Design Trade-offs:** Separating `uncommitted_hold_` from `pin_count_` keeps `ActivePinCount()` accurate (`PageGuard` leaks are STILL caught!) while enforcing 100% No-Steal until commit/rollback! And if an uncommitted transaction tries to dirty more pages than `pool_size` frames, `NewPage`/`FetchPage` returns `StatusCode::kResourceExhausted` cleanly rather than stealing uncommitted pages!
- **Common Mistakes:** Allowing `FlushAllPages()` or `~BufferPoolManager()` to flush an `uncommitted_hold_` frame!
- **Understanding Checkpoint:** Why is adding a separate `uncommitted_hold_` boolean on `PageFrame` cleaner than artificially incrementing `pin_count_` for No-Steal protection?

### Component Context
- **Why It Exists:** Enforces the foundational invariant of Redo-Only recovery.
- **Module Relationships:** Links `BufferPoolManager` to `TransactionManager`.

### Files Expected to Change
- `include/tinydb/buffer/page_frame.hpp`, `src/buffer/page_frame.cpp`
- `include/tinydb/buffer/buffer_pool_manager.hpp`, `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/buffer/buffer_pool_nosteal_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `MarkUncommittedHold(PageId)`, `ReleaseUncommittedHold(PageId, bool keep_dirty)`, and No-Steal guards in `UnpinPage`, `FlushPage`, `FlushAllPages`, and `DeletePage`!
2. Write unit tests verifying: dirty page marked `uncommitted_hold_` is NEVER evicted even when `pin_count == 0`, NEVER written on `FlushAllPages()` (`DiskStats::page_writes == 0`!), and becomes evictable + flushable right after `ReleaseUncommittedHold`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Uncommitted frames incur `0` disk writes under eviction pressure and `FlushAllPages()`; releasing hold restores normal eviction/flush.

### Definition of Done
No-Steal buffer enforcement implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-209: enforce strict No-Steal uncommitted hold in BufferPoolManager`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-210` (Modified-page tracking & before-images inside `Transaction`).
````

---

## DAY-210 — Implement Transaction Modified-Page Tracking and Before-Image Capture

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-210
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement Transaction Modified-Page Tracking and Before-Image Capture
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-209 complete.

### Objective
Equip `Transaction` (`include/tinydb/transaction/transaction.hpp`, `src/transaction/transaction.cpp`) with `BeforeImageMap` (`unordered_map<PageId, ByteBuffer>` capturing the *first* pre-modification 4096-byte page snapshot only!), `modified_pages_` order list, `allocated_pages_` set, `deferred_deletes_` set, and `saved_index_roots_` map (`IndexId -> old_root_page_id`), plus `BufferPoolManager::CaptureBeforeImageIfNeeded(txn, page_id)` / hook!

### Concepts & Daily Learning
- **Concept:** First-touch page before-image capture (`if (!before_images_.contains(pid)) snapshot(pid);`) and tracking allocations/roots for exact undo.
- **Why Database Engines Need It:** If a transaction modifies Page 4 three times (`INSERT` row 1, `INSERT` row 2, `UPDATE` row 1), saving the snapshot *only on the first modification* preserves the exact pre-transaction Page 4 state (`before_image`), whereas overwriting the snapshot on the 2nd modification would lose the original pre-txn state!
- **How It Works:** Before mutating `page_id` in a `Transaction`:
  - If `page_id` was newly allocated inside THIS txn (`allocated_pages_.contains(pid)`), no disk before-image needed (it didn't exist before `BEGIN`—rollback will deallocate it!).
  - Otherwise, if `!before_images_.contains(pid)`: copy the 4096-byte page BEFORE modification into `before_images_[pid]` AND call `bpm.MarkUncommittedHold(pid)`!
  - When an index `root_page_id` changes inside a transaction: record `saved_index_roots_.try_emplace(index_id, old_root)` and defer `.cat` persistence until `COMMIT`!
- **Design Trade-offs:** Deferring `CatalogManager::UpdateIndexRootPage` persistence until `COMMIT` (keeping pending root updates inside `Transaction`, writing `.cat` atomically ONLY on `COMMIT`, and restoring old `root_page_id` in memory on `ROLLBACK`) keeps `.cat` and `.tinydb` 100% consistent if a transaction splits a B+ Tree root and then aborts or crashes!
- **Common Mistakes:** Writing `.cat` immediately during an uncommitted B+ Tree split—leaving `.cat` pointing at a nonexistent `root_page_id` if the transaction later rolls back or crashes!
- **Understanding Checkpoint:** Why MUST a `Transaction` capture a page's before-image *only the very first time* the transaction touches that page, and why must `CatalogManager` root-page updates during DML wait until `COMMIT` before persisting to `.cat`?

### Component Context
- **Why It Exists:** Captures complete undo state in memory for runtime `ROLLBACK` and clean after-image list for WAL `COMMIT`.
- **Module Relationships:** Hooks into `TableHeap`, `BPlusTree`, and `BufferPoolManager` when `txn != nullptr`.

### Files Expected to Change
- `include/tinydb/transaction/transaction.hpp`, `src/transaction/transaction.cpp`
- `include/tinydb/transaction/txn_context.hpp` (new — helper coordinating `BeforeModifyPage`, `OnAllocatePage`, `OnDeallocatePage`, `OnIndexRootChange`)
- `tests/unit/transaction/before_image_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `before_images_`, `allocated_pages_`, `deferred_deletes_`, `pending_index_roots_` inside `Transaction` and `TxnContext` helpers.
2. Write unit tests proving repeated writes to the same `PageId` preserve the *initial* pre-txn before-image, newly allocated pages track in `allocated_pages_`, and index root changes track initial `old_root`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- First-touch snapshot invariant holds across multi-write sequences; zero disk writes occur on uncommitted pages.

### Definition of Done
Before-image and page tracking implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-210: implement Transaction before-image capture and allocation tracking`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-211` (Runtime `ROLLBACK`).
````

---

## DAY-211 — Implement Runtime `ROLLBACK` Across `TableHeap`, `BPlusTree`, and `BufferPoolManager`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-211
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement Runtime `ROLLBACK` Across `TableHeap`, `BPlusTree`, and `BufferPoolManager`
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-210 complete.

### Objective
Wire `TxnContext` into `TableHeap` and `BPlusTree` (plus `BufferPoolManager::RestorePageImage` / `DeallocateUncommittedPage`) and implement runtime `TransactionManager::Rollback()` restoring all modified pages to their exact `before_images_`, freeing `allocated_pages_`, restoring old `BPlusTree::root_page_id_`s, discarding `deferred_deletes_`, and releasing all No-Steal holds!

### Concepts & Daily Learning
- **Concept:** Physical page-level runtime `ROLLBACK` restoring both heap pages and B+ Tree pages atomically without disk UNDO logs.
- **Why Database Engines Need It:** A transaction might insert 50 rows, trigger 3 B+ Tree splits, relocate a `TableHeap` tuple across pages, delete 10 rows, and then execute `ROLLBACK;` (or fail validation on statement #4)—`ROLLBACK` must revert EVERY heap and index change cleanly!
- **How It Works:**
  1. Ensure `TableHeap` and `BPlusTree` notify `TxnContext` (when active) *before* mutating any page, *before* `NewPage` allocations, and defer `DeletePage` into `deferred_deletes_` during active transactions!
  2. In `Rollback()`:
     - For each `(page_id, saved_buf)` in `before_images_`: overwrite the resident frame's `Page` bytes with `saved_buf` (`BufferPoolManager::RestoreFrameBytes`), release No-Steal hold (keeping `is_dirty = false` if page wasn't dirty before txn, or preserving prior dirty state—tracking `was_dirty_before` in `before_images_` is super clean!).
     - For each `alloc_pid` in `allocated_pages_`: release No-Steal hold and call `bpm.DeletePage(alloc_pid)`!
     - Restore any modified `BPlusTree::root_page_id_` in memory to `old_root` (no `.cat` write needed!).
     - Clear `deferred_deletes_` (those pages stay untouched!).
- **Design Trade-offs:** Notice tracking `bool was_dirty_before` alongside each `before_image` means rolling back a previously clean page resets `is_dirty = false` (zero wasted disk writes!) while rolling back a page that ALREADY had committed un-flushed dirty data keeps `is_dirty = true`!! What a clean invariant!
- **Common Mistakes:** Restoring `before_image` and setting `is_dirty = false` on a page that had *earlier committed* un-flushed changes sitting in the Buffer Pool!! (Recording `was_dirty` at capture time avoids losing prior committed changes!)
- **Understanding Checkpoint:** Suppose Txn 1 commits changes to Page 2 (still dirty in RAM, not flushed to disk yet). Txn 2 modifies Page 2 and then executes `ROLLBACK`. Why MUST `Rollback()` restore Page 2's `before_image` AND keep `frame.is_dirty = true`?

### Component Context
- **Why It Exists:** Delivers complete transactional `ROLLBACK` atomicity in RAM.
- **Module Relationships:** Wires `TransactionManager` <-> `BufferPoolManager` <-> `TableHeap` <-> `BPlusTree`.

### Files Expected to Change
- `include/tinydb/buffer/buffer_pool_manager.hpp`, `src/buffer/buffer_pool_manager.cpp`
- `src/storage/table_heap.cpp`, `src/index/bplus_tree.cpp`
- `src/transaction/transaction_manager.cpp`, `src/execution/execution_engine.cpp`
- `tests/integration/phase08_rollback_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `TxnContext` hooks into `TableHeap` and `BPlusTree` (`Insert`, `Update`, `Delete`, `NewPage`, deferred `DeletePage`, deferred `root_page_id` persist).
2. Implement full `TransactionManager::Rollback()`.
3. Test `ROLLBACK` across: multi-row `INSERT`, `UPDATE` with cross-page move, `DELETE` triggering B+ Tree merges, `ROLLBACK` after an earlier committed-but-unflushed txn on the SAME page, and verify `BPlusTreeValidator` + `DiskStats::page_writes == 0` during uncommitted work!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Rolled-back changes disappear completely from `SELECT`s, `TableHeap`, `BPlusTree`, and disk; prior committed dirty pages stay intact!

### Definition of Done
Runtime `ROLLBACK` verified end-to-end across heap & indexes, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-211: implement full runtime ROLLBACK across TableHeap, BPlusTree, and BufferPool`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-212` (WAL file format design).
````

---

## DAY-212 — Design the WAL File Format and `LogRecord` Binary Layout

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-212
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Design the WAL File Format and `LogRecord` Binary Layout
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-211 complete.

### Objective
Define the Write-Ahead Log file header (magic `"TINYWAL\1"`, `wal_version = 1`, `page_size = 4096`) and `LogRecord` binary layout (`LogRecordType`: `kBegin`, `kCommit`, `kAbort`, `kPageImage`, `kFreePage`; fields: `total_len`, `lsn`, `prev_lsn`, `txn_id`, `type`, `page_id`, optional 4096B payload, trailing `crc32`) in `include/tinydb/recovery/log_record.hpp` and `docs/transaction-design.md`.

### Concepts & Daily Learning
- **Concept:** Self-framing binary log records (`total_len` + payload + trailing `crc32`) for linear forward scanning and torn-tail detection.
- **Why Database Engines Need It:** When the OS crashes mid-`write()` to `.wal`, the tail record may be half-written; checking exact length bounds AND per-record CRC32 lets `RecoveryManager` verify every record cleanly and stop safely at a torn tail!
- **How It Works:**
  - WAL File Header (`16` bytes): `magic[8]` + `u32 version` + `u32 page_size`.
  - Fixed LogRecord header (`32` bytes): `u32 total_len`, `u64 lsn`, `u64 prev_lsn`, `u64 txn_id`, `u8 type`, `u8 flags`, `u16 reserved` (actually `4+8+8+8+1+3 = 32` + `u32 page_id` -> `36` bytes header!).
  - Optional body: for `kPageImage`, exact `4096` bytes of the modified page after-image!
  - Trailing `u32 crc32` covering `[0 .. total_len - 4)`!
- **Design Trade-offs:** Full 4096-byte page after-images (`PAGE_IMAGE` records) trade larger WAL volume per modified page for crystal-clear idempotent redo recovery without physiological tuple-offset coupling!
- **Common Mistakes:** Writing `LogRecord` fields without `total_len` or trailing `crc32` (making recovery unable to distinguish valid records from torn trailing bytes!).
- **Understanding Checkpoint:** Why is placing `crc32` at the **end** of each `LogRecord` (after the 4096-byte page image) important for catching partial/torn writes to the WAL tail?

### Component Context
- **Why It Exists:** Persistent durability wire format for TinyDB crash recovery.
- **Module Relationships:** Encoded/decoded in `DAY-213`–`DAY-214`, managed by `LogManager`, replayed by `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/recovery/log_record.hpp` (new)
- `docs/transaction-design.md` (pin down exact byte offsets of WAL header & `LogRecord`)
- `tests/unit/recovery/log_record_layout_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `LogRecordType`, WAL header constants, `LogRecord` struct, and exact serialized byte offset constants in `log_record.hpp`.
2. Update `docs/transaction-design.md` and add layout unit tests.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- WAL header and `LogRecord` byte offsets are contiguous, documented, and compile cleanly.

### Definition of Done
WAL format designed and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-212: define WAL file header and LogRecord binary layout`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-213` (`LogRecord` serialization & deserialization).
````

---

## DAY-213 — Implement `LogRecord` Explicit Serialization, Deserialization, and CRC32 Verification

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-213
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `LogRecord` Explicit Serialization, Deserialization, and CRC32 Verification
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-212 complete.

### Objective
Implement `LogRecordCodec::Serialize` and `LogRecordCodec::Deserialize` (`src/recovery/log_record.cpp`) encoding/decoding `kBegin`, `kCommit`, `kAbort`, `kPageImage`, `kFreePage` explicitly with `BinaryWriter`/`BinaryReader`, validating `total_len`, `lsn >= 1`, `LogRecordType`, `page_id` bounds, and trailing CRC32 checksum!

### Concepts & Daily Learning
- **Concept:** Framing self-describing variable-length records (`total_len`) with checksum validation before trusting inner fields.
- **Why Database Engines Need It:** Prevents corrupt `total_len = 0` (infinite loop in WAL scanner!) or `total_len = 0xFFFFFFFF` (out-of-bounds read/huge allocation!) from crashing crash recovery!
- **How It Works:** Enforce `kMinRecordSize <= total_len <= kMaxRecordSize` (`36 + 4 = 40` to `36 + 4096 + 4 = 4136`). Verify buffer contains `total_len` bytes; verify CRC32 over `[0 .. total_len - 4)` equals trailing `u32` FIRST; decode `LogRecordType`, verify exact expected `total_len` for that `type` (`40` for `Begin`/`Commit`/`Abort`/`FreePage`, `4136` for `PageImage`!) and valid `lsn != kInvalidLsn`.
- **Design Trade-offs:** Distinguishing `kIncompleteTail` (fewer bytes remaining in file than `total_len` or EOF mid-record) from `kCorruption` helps `RecoveryManager` handle torn tails gracefully!
- **Common Mistakes:** Allowing `total_len == 0` (causing `offset += total_len` to loop forever!) or mismatching `total_len` vs `type`!
- **Understanding Checkpoint:** Why must `Deserialize` verify BOTH `total_len >= kMinRecordSize` AND `total_len == ExpectedLength(type)`?

### Component Context
- **Why It Exists:** Safe encoder/decoder for WAL bytes.
- **Module Relationships:** Driven by `LogManager` (`DAY-215`+) and `RecoveryManager` (`DAY-223`+).

### Files Expected to Change
- `include/tinydb/recovery/log_record.hpp`
- `src/recovery/log_record.cpp` (new)
- `tests/unit/recovery/log_record_codec_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `LogRecordCodec::Serialize` and `Deserialize` (plus WAL header serialize/deserialize!).
2. Add unit tests: roundtrips for all `LogRecordType`s, `total_len = 0` rejected, `total_len > max` rejected, bit-flipped payload CRC failure, truncated buffer detection, and `kPageImage` with wrong `total_len = 40` rejected!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All valid records roundtrip byte-exact; every malformed/truncated/bit-flipped record fails cleanly.

### Definition of Done
`LogRecord` codec implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-213: implement LogRecord explicit serialization, decoding, and CRC32`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-214` (`LogManager` skeleton & WAL open/create).
````

---

## DAY-214 — Implement `LogManager` Skeleton and WAL File Lifecycle (`Create`, `Open`, `Close`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-214
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `LogManager` Skeleton and WAL File Lifecycle (`Create`, `Open`, `Close`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-213 complete.

### Objective
Create `tinydb::LogManager` (`include/tinydb/recovery/log_manager.hpp`, `src/recovery/log_manager.cpp`) managing the `.wal` file (`FileHandle`), WAL header validation, monotonic `next_lsn_` and `persistent_lsn_` tracking, in-memory append buffer, and clean `Create` / `Open` / `Close` / `Reset` lifecycle!

### Concepts & Daily Learning
- **Concept:** Monotonic `next_lsn_` vs `persistent_lsn_` (flushed LSN) tracking and WAL file initialization.
- **Why Database Engines Need It:** `next_lsn_` tells the engine the next LSN to assign in RAM, while `persistent_lsn_` tracks the highest LSN actually synced to disk via `fsync`; `BufferPoolManager` / `Commit` checks `persistent_lsn_ >= commit_lsn` before reporting durable commit success!
- **How It Works:** `Create(wal_path)` writes the 16-byte WAL header and syncs file + parent directory. `Open(wal_path)` validates header magic/version/page_size and computes initial `write_offset_`. `next_lsn_` starts at `1` (or `max_valid_lsn + 1` after recovery!).
- **Design Trade-offs:** Buffering appended bytes in `log_buffer_` (`std::vector<std::byte>`) until `Flush()` batches multiple records (`PAGE_IMAGE`s + `COMMIT`) into a single contiguous write + single `fsync`!
- **Common Mistakes:** Resetting `next_lsn_` back to `1` when reopening a WAL that already contains records `1..50` (must advance `next_lsn_ = last_lsn + 1` after recovery scan!).
- **Understanding Checkpoint:** Explain the difference between `next_lsn_` and `persistent_lsn_` inside `LogManager`.

### Component Context
- **Why It Exists:** Append-only WAL writer and durability manager.
- **Module Relationships:** Used by `TransactionManager::Commit`, `Database`, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/recovery/log_manager.hpp` (new)
- `src/recovery/log_manager.cpp` (new)
- `tests/unit/recovery/log_manager_lifecycle_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `LogManager` (`Create`, `Open`, `Close`, `SetNextLsn`, `PersistentLsn`, `NextLsn`, `IsOpen`, `ResetLog` for clean checkpoint truncation).
2. Add unit tests verifying WAL creation (`16` bytes), open validation, invalid magic/version rejection, and `Close` safety.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- WAL file creates cleanly with valid 16B header; malformed WAL headers reject with `kCorruption`.

### Definition of Done
`LogManager` skeleton and lifecycle tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-214: implement LogManager skeleton and WAL file lifecycle`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-215` (`AppendLogRecord` & `Flush`).
````

---

## DAY-215 — Implement `LogManager::AppendLogRecord` and Durable `Flush()`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-215
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `LogManager::AppendLogRecord` and Durable `Flush()`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-214 complete.

### Objective
Implement `Result<Lsn> LogManager::Append(LogRecord record)` (assigning monotonic `lsn = next_lsn_++`, encoding via `LogRecordCodec::Serialize` into `log_buffer_`) and `Status LogManager::Flush()` (writing `log_buffer_` atomically at `write_offset_`, calling `file_.Sync()`, advancing `write_offset_` and `persistent_lsn_` ONLY AFTER `Sync()` succeeds!).

### Concepts & Daily Learning
- **Concept:** Atomic WAL append + `fsync` durability barrier, updating `persistent_lsn_` strictly post-`Sync()`.
- **Why Database Engines Need It:** If `Flush()` crashes or `Sync()` fails midway, advancing `persistent_lsn_` prematurely would falsely tell `COMMIT` the log is durable; updating `persistent_lsn_` *only* after `Sync()` returns `Ok` preserves the WAL durability invariant!
- **How It Works:** Under `latch_`, assign `lsn = next_lsn_++`, append serialized bytes into `log_buffer_`, track `buffered_max_lsn_`. On `Flush()`: if `log_buffer_.empty()`, call `file_.Sync()` (if needed) and return `Ok`. Write `log_buffer_` at `write_offset_`, call `file_.Sync()`, then advance `write_offset_ += log_buffer_.size()`, `persistent_lsn_ = buffered_max_lsn_`, `log_buffer_.clear()`.
- **Design Trade-offs:** Group/batch flushing `log_buffer_` once per `COMMIT` does 1 `pwrite` + 1 `fsync` for an entire multi-page transaction rather than `K` separate `fsync`s!
- **Common Mistakes:** Clearing `log_buffer_` before `WriteAt` succeeds; forgetting `write_offset_` progression after reopening a pre-existing WAL!
- **Understanding Checkpoint:** Why does batching `PAGE_IMAGE` records + `COMMIT` record into `log_buffer_` and calling `Flush()` once at commit give identical durability with vastly fewer `fsync` syscalls?

### Component Context
- **Why It Exists:** Core durable log append engine.
- **Module Relationships:** Called by `TransactionManager::Commit` (`DAY-220`).

### Files Expected to Change
- `include/tinydb/recovery/log_manager.hpp`
- `src/recovery/log_manager.cpp`
- `tests/unit/recovery/log_manager_append_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Append` and `Flush` (plus optional `FlushUpTo(Lsn)`).
2. Add tests verifying monotonic LSNs (`1, 2, 3...`), `PersistentLsn()` staying `0` until `Flush()`, exact byte lengths written to `.wal` after `Flush()`, and reading back appended records via `LogRecordCodec::Deserialize`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `PersistentLsn()` advances strictly on `Flush()`; appended records decode cleanly byte-for-byte from `.wal`.

### Definition of Done
`Append` and `Flush` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-215: implement LogManager::Append and durable fsync Flush`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-216` (WAL sequential reader & torn-tail vs mid-file corruption).
````

---

## DAY-216 — Implement WAL Sequential Reader (`LogReader`) with Torn-Tail vs Corruption Detection

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-216
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement WAL Sequential Reader (`LogReader`) with Torn-Tail vs Corruption Detection
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-215 complete.

### Objective
Implement `LogReader` (`include/tinydb/recovery/log_reader.hpp`, `src/recovery/log_reader.cpp`) scanning `.wal` records sequentially from offset `16` (`kWalHeaderSize`) to EOF, cleanly distinguishing:
1. Clean EOF (`std::nullopt`)
2. Torn/partial trailing record at EOF (no remaining complete bytes after it -> stop gracefully with `torn_tail_offset`!)
3. Mid-log corruption (bad CRC or invalid `total_len` when more bytes exist or inside header -> fail cleanly with `StatusCode::kCorruption`!)

### Concepts & Daily Learning
- **Concept:** Torn-tail tolerance vs mid-log corruption rejection in WAL recovery.
- **Why Database Engines Need It:** If a crash occurred while writing record `N` at the very end of `.wal`, trailing incomplete bytes (or CRC mismatch right at the end with no subsequent records) are expected torn tail bytes—whereas a bit-flip inside record #2 (followed by valid records #3, #4, #5!) or forged `total_len = 0` indicates genuine WAL corruption!
- **How It Works:** Read `4`-byte `total_len` at `offset`:
  - If `0` bytes left: clean EOF!
  - If `< 4` bytes left: torn tail at `offset`!
  - Validate `kMinRecordSize <= total_len <= kMaxRecordSize`; if `offset + total_len > file_size`, it's a torn tail!
  - Read `total_len` bytes, verify `LogRecordCodec::Deserialize`. If CRC fails AND `offset + total_len == file_size` (or trailing bytes look torn at EOF—document exact rule clearly!), treat carefully vs mid-file (`offset + total_len < file_size` -> strict `kCorruption`!). Also check monotonic LSN (`record.lsn > last_lsn`!).
- **Design Trade-offs:** Strict `kCorruption` on mid-log errors combined with safe tail truncation stops silent data skipping while surviving real torn writes!
- **Common Mistakes:** Silently skipping a bad record in the middle of the WAL and replaying later records after it (violating transaction atomicity/ordering!) OR crashing recovery on 5 trailing incomplete crash bytes at EOF!
- **Understanding Checkpoint:** Why must `LogReader` handle an incomplete record at the very end of `.wal` differently from a CRC mismatch in the middle of `.wal`?

### Component Context
- **Why It Exists:** Foundation scanner used by `RecoveryManager` (`DAY-223`–`DAY-226`).
- **Module Relationships:** Reads `.wal` files written by `LogManager`.

### Files Expected to Change
- `include/tinydb/recovery/log_reader.hpp` (new)
- `src/recovery/log_reader.cpp` (new)
- `tests/unit/recovery/log_reader_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `LogReader` (`Open`, `ReadNext`, `ValidEndOffset`).
2. Add tests covering: clean multi-record read, torn tail (`< 4` trailing bytes, incomplete `4136`-byte `kPageImage` truncated at `2000` bytes), mid-log bit-flip (`kCorruption`!), `total_len = 0` (`kCorruption`!), and out-of-order LSN (`kCorruption`!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Torn tail at EOF halts gracefully returning valid prefix records; mid-file corruption fails explicitly with `kCorruption`.

### Definition of Done
`LogReader` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-216: implement LogReader with torn-tail and mid-log corruption checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-217` (Page LSN stamping & WAL integration).
````

---

## DAY-217 — Implement `PageHeader::lsn` Stamping on WAL Log Append

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-217
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `PageHeader::lsn` Stamping on WAL Log Append
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-216 complete.

### Objective
Add helper methods `Page::SetLsn(Lsn)` / `Page::GetLsn()` (preserving payload & updating header cleanly!) and `BufferPoolManager::SetPageLsn(PageId, Lsn)` + `ReadPageImage(PageId)` so every `PAGE_IMAGE` log record stamps its assigned `lsn` directly inside the page's `PageHeader::lsn` BEFORE serializing the after-image into the WAL AND updating the resident buffer frame!

### Concepts & Daily Learning
- **Concept:** Synchronizing `PageHeader::lsn` between the in-memory `PageFrame`, the WAL `PAGE_IMAGE` payload, and eventually `.tinydb`.
- **Why Database Engines Need It:** During Redo recovery, `RecoveryManager` compares `disk_page.GetLsn() < log_record.lsn`. If the WAL `PAGE_IMAGE` didn't carry its own `lsn` stamped inside `[8..15]` of the 4096-byte page image, replaying that image onto disk would leave `disk_page.lsn == 0`—making subsequent recovery think the page wasn't updated yet!
- **How It Works:** When logging `page_id` at commit: allocate `lsn = log_manager.NextLsn()` (or inside `LogManager::AppendPageImage`: pass mutable/stamped `Page` buffer where `SetLsn(lsn)` writes `lsn` into `PageHeader` AND `page.UpdateChecksum()` BEFORE computing the `LogRecord` CRC32 and saving the stamped page back into the buffer pool frame!).
- **Design Trade-offs:** Stamping `lsn` + `UpdateChecksum()` on the page BEFORE copying it into `LogRecord::page_image` guarantees that replaying those exact 4096 bytes straight onto `.tinydb` produces a checksum-valid page carrying the exact `lsn`!
- **Common Mistakes:** Stamping `lsn` in RAM *after* serializing the WAL record (leaving `lsn = 0` inside the WAL copy!), or forgetting `UpdateChecksum()` after changing `PageHeader::lsn`!
- **Understanding Checkpoint:** Why MUST `Page::SetLsn(lsn)` + `Page::UpdateChecksum()` happen *before* copying the 4096 bytes into `LogRecord::page_image`?

### Component Context
- **Why It Exists:** Ensures WAL page images are self-consistent checksummed 4KB disk images carrying their exact LSN.
- **Module Relationships:** Bridges `Page` (`Phase 01`), `BufferPoolManager` (`Phase 04`), and `LogManager` (`Phase 08`).

### Files Expected to Change
- `include/tinydb/storage/page.hpp`, `src/storage/page.cpp`
- `include/tinydb/buffer/buffer_pool_manager.hpp`, `src/buffer/buffer_pool_manager.cpp`
- `tests/unit/recovery/page_lsn_stamp_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Page::GetLsn()` / `SetLsn(Lsn)` (updating `PageHeader::lsn` cleanly) and `BufferPoolManager::SnapshotAndStampPageLsn(...)`.
2. Add unit tests verifying `SetLsn(42)` preserves `Page::Validate(page_id)` (`UpdateChecksum` valid!) and stamps exact `lsn == 42` in `[8..15]`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Stamped 4096-byte page images validate cleanly via `Page::Validate` and carry exact `lsn`.

### Definition of Done
Page LSN stamping tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-217: implement PageHeader::lsn stamping and checksum-safe page snapshots`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-218` (Handling `PageType::kFree` & `FileHeader` consistency in WAL).
````

---

## DAY-218 — Handle Page Allocations & Deallocations in WAL (`kFreePage` / Allocation Ordering)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-218
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Handle Page Allocations & Deallocations in WAL (`kFreePage` / Allocation Ordering)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-217 complete.

### Objective
Ensure transactions that allocate new pages (`NewPage`), deallocate pages (`DeletePage` deferred until commit!), or update `FileHeader` (`page_count` / freelist) remain crash-consistent during Redo recovery: specifically, log `kFreePage` / ensure `RecoveryManager` can cleanly extend `.tinydb` up to `record.page_id + 1` if a committed `PAGE_IMAGE` targets an allocated page whose `FileHeader` wasn't flushed yet, AND log `kFreePage` for deferred page deletions on commit!

### Concepts & Daily Learning
- **Concept:** File-extension & freelist crash consistency during Redo recovery (`RecoveryManager` extending `.tinydb` when replaying `PAGE_IMAGE` for a newly allocated `PageId >= current_page_count`).
- **Why Database Engines Need It:** Suppose Txn 1 runs `NewPage` (allocating Page 5) and writes rows to Page 5 + commits to WAL, then crashes BEFORE `Page 0` (`page_count = 6`) or Page 5 flushes! If `.tinydb` on disk still says `page_count = 5` (or 4 pages) during recovery, `RecoveryManager` trying to replay `PAGE_IMAGE(page_id=5)` would fail with `kOutOfRange` unless recovery extends `.tinydb` cleanly AND updates Page 0 (`FileHeader::page_count`)!
- **How It Works:**
  - Log `kBegin`, then `kPageImage` (for every modified + allocated page, including Page 0 `FileHeader` if needed—OR even cleaner: let `RecoveryManager::RedoPageImage(page_id, bytes)` automatically extend `.tinydb` up to `page_id + 1` and reconcile Page 0 `page_count` & `freelist` cleanly if `page_id >= disk.PageCount()`! AND log `kFreePage(pid)` on commit for `deferred_deletes_`!).
  - Wait: checking Page 0 + `kFreePage` vs logging Page 0: remember Page 0 is small (`4096B`), and `RecoveryManager` knowing `EnsurePageExistsForRecovery(page_id)` + `DeallocatePage` for `kFreePage` handles BOTH file growth AND freelist reclamation accurately!
- **Design Trade-offs:** Extending `.tinydb` cleanly during recovery when `record.page_id >= disk.PageCount()` guarantees `NewPage` inside a committed transaction recovers 100% faithfully after a crash!
- **Common Mistakes:** Assuming every `PAGE_IMAGE` in the WAL targets a `page_id` that already existed in `.tinydb` before the crash!
- **Understanding Checkpoint:** Trace what happens if Txn 1 allocates Page 5 via `NewPage`, commits to WAL, and crashes before `.tinydb` extends on disk—how does `RecoveryManager` recreate Page 5 cleanly?

### Component Context
- **Why It Exists:** Makes `NewPage` and `DeletePage` inside transactions crash-recoverable!
- **Module Relationships:** Extends `DiskManager` recovery helper + `LogRecord`.

### Files Expected to Change
- `include/tinydb/storage/disk_manager.hpp`, `src/storage/disk_manager.cpp` (`ApplyRecoveredPage` / `EnsureCapacityForRecovery`)
- `tests/unit/recovery/disk_recovery_extend_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add recovery-only `DiskManager::ApplyRecoveredPage(PageId, const Page&)` (extending `.tinydb` safely if `page_id >= header_.page_count`, writing valid checksummed page, and rebuilding/updating `FileHeader::page_count`) and `ApplyRecoveredFreePage(PageId)`.
2. Test extending a 2-page DB to Page 5 via `ApplyRecoveredPage(5, ...)` and reopening cleanly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `ApplyRecoveredPage` safely extends `.tinydb` and leaves `FileHeader::page_count` and CRC32s 100% consistent on reopen!

### Definition of Done
Recovery page-extension and free-page helpers tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-218: support recovery page extension and free-page replay in DiskManager`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-219` (Catalog `root_page_id` durability on `COMMIT` + `Recovery`).
````

---

## DAY-219 — Coordinate Index `root_page_id` Atomic Commit with WAL Transactions

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-219
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Coordinate Index `root_page_id` Atomic Commit with WAL Transactions
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-218 complete.

### Objective
Ensure `CatalogManager` root-page updates caused by B+ Tree root splits/shrinks during an active transaction (`pending_index_roots_`) are flushed atomically ONLY AFTER the WAL `COMMIT` record is synced (or logged via WAL `kSetIndexRoot` record? Wait—logging `kSetIndexRoot(index_id, root_page_id)` inside the WAL OR persisting `.cat` cleanly!).
WAIT — think about crash recovery right after `COMMIT` WAL flush BEFORE `.cat` updates!
Suppose Txn 1 inserts rows causing a B+ Tree root split (`root_page_id` changes from `4` to `7`), logs WAL `PAGE_IMAGE`s + `COMMIT` + `fsync(wal)`, and crashes BEFORE updating `.cat`!
If `kSetIndexRoot(index_id, new_root_page_id)` is ALSO logged inside the WAL before `COMMIT`, then `RecoveryManager` restores BOTH the B+ Tree pages AND the exact committed `root_page_id` in `CatalogManager`!!
WHAT A GREAT CATCH!! Let's include `LogRecordType::kIndexRoot` (`index_id`, `new_root_page_id`) cleanly in `LogRecord`!

### Concepts & Daily Learning
- **Concept:** Avoiding cross-file split-brain between `.wal`/`.tinydb` and `.cat` when a transactional insert splits a B+ Tree root!
- **Why Database Engines Need It:** If WAL commits `Page 7` as the new B+ Tree root, and crashes before `.cat` updates from `4` to `7`, recovery replays `Page 7` onto `.tinydb` while `.cat` still points at `Page 4`! Recording `kIndexRoot` inside the WAL guarantees `.cat` and `.tinydb` agree 100% after crash recovery!
- **How It Works:** Add `LogRecordType::kIndexRoot` (storing `index_id` in `page_id` / header and `new_root_page_id` in header/payload). During `COMMIT`: log `kPageImage`s, `kFreePage`s, and `kIndexRoot`s -> log `kCommit` -> `Flush()` WAL -> apply `CatalogManager::UpdateIndexRootPage` -> release No-Steal holds! During `RecoveryManager`: replay `kIndexRoot` into `CatalogManager`!
- **Design Trade-offs:** Making `kIndexRoot` a 40-byte WAL record keeps `.cat` and `.tinydb` 100% crash-consistent with zero complex two-phase file protocols!
- **Common Mistakes:** Updating `.cat` *before* WAL commit (breaks `ROLLBACK` and pre-commit crash!) OR forgetting to log `root_page_id` changes in WAL (breaks post-commit crash recovery!).
- **Understanding Checkpoint:** Trace what breaks on crash recovery if a transaction splits a B+ Tree root from Page 4 to Page 7, commits to WAL, and crashes before `.cat` writes `root = 7`—and how `kIndexRoot` in the WAL solves it completely!

### Component Context
- **Why It Exists:** Guarantees B+ Tree root pointers survive crashes atomically with tree pages.
- **Module Relationships:** Connects `LogRecord`, `TransactionManager::Commit`, `CatalogManager`, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/recovery/log_record.hpp`, `src/recovery/log_record.cpp` (add `kIndexRoot`)
- `docs/transaction-design.md`
- `tests/unit/recovery/log_record_codec_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Add `LogRecordType::kIndexRoot` (`index_id` + `root_page_id`) to `LogRecord` and `LogRecordCodec`.
2. Add roundtrip and bounds validation tests in `log_record_codec_test.cpp`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `kIndexRoot` encodes/decodes cleanly with CRC32 verification and updates `docs/transaction-design.md`.

### Definition of Done
`kIndexRoot` WAL record implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-219: add kIndexRoot WAL record for crash-safe root pointer durability`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-220` (Full durable `COMMIT` sequence!).
````

---

## DAY-220 — Implement Durable `COMMIT` Sequence (WAL Append + `fsync` + Release No-Steal Holds)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-220
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement Durable `COMMIT` Sequence (WAL Append + `fsync` + Release No-Steal Holds)
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-219 complete.

### Objective
Implement the complete durable `TransactionManager::Commit()` sequence:
1. Append `kBegin` (if not already appended when first mutation occurred—lazy begin logging skips WAL writes for read-only transactions!).
2. For each modified/allocated page: allocate `lsn`, stamp `PageHeader::lsn`, append `kPageImage` to `LogManager`.
3. Append any `kFreePage` and `kIndexRoot` records.
4. Append `kCommit` record.
5. Call `LogManager::Flush()` (`fsync` durable on disk **before** reporting commit success!).
6. Apply deferred catalog `UpdateIndexRootPage`s and deferred `DeletePage`s, and release all No-Steal holds in `BufferPoolManager`!

### Concepts & Daily Learning
- **Concept:** Write-Ahead Logging rule ("WAL records + `COMMIT` MUST hit durable storage BEFORE dirty pages are allowed to flush or `COMMIT` returns success") and lazy WAL logging for read-only transactions!
- **Why Database Engines Need It:** If a `SELECT`-only transaction wrote `BEGIN` + `COMMIT` to WAL, every read query would do an `fsync`! Skipping WAL writes when a transaction modified 0 pages keeps reads fast while guaranteeing 100% durability for every mutating transaction!
- **How It Works:** In `Commit()`: if `modified_pages_` and `allocated_pages_` and `deferred_deletes_` are empty (read-only!), simply transition `kCommitted` and finish with 0 WAL I/O! Otherwise execute steps 1–6 strictly in order. If `LogManager::Flush()` fails, roll back cleanly and propagate error!
- **Design Trade-offs:** Appending `kBegin` + `kPageImage`s + `kCommit` together at commit (under No-Steal!) writes zero wasted WAL bytes if the transaction runs `ROLLBACK`!
- **Common Mistakes:** Releasing No-Steal holds *before* `LogManager::Flush()` finishes (breaking WAL ordering if eviction flushes an un-logged page!); writing `COMMIT` for read-only `SELECT`s!
- **Understanding Checkpoint:** Why does our No-Steal design allow deferring `kBegin` + `kPageImage` WAL appends until `Commit()`, and what happens if the process crashes *after* `Commit()`'s `LogManager::Flush()` succeeds?

### Component Context
- **Why It Exists:** Guarantees ACID Durability and Atomicity for every committed transaction.
- **Module Relationships:** Coordinates `TransactionManager`, `LogManager`, `BufferPoolManager`, and `CatalogManager`.

### Files Expected to Change
- `include/tinydb/transaction/transaction_manager.hpp`, `src/transaction/transaction_manager.cpp`
- `tests/integration/phase08_durable_commit_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement complete `TransactionManager::Commit()` wired with `LogManager` inside `Database`/`ExecutionEngine`.
2. Add tests verifying: read-only `BEGIN; SELECT ...; COMMIT;` writes `0` bytes to `.wal`; mutating `COMMIT` advances `PersistentLsn()`, stamps `PageHeader::lsn` on RAM pages, updates `.wal` with `kBegin + kPageImage + kCommit`, AND keeps `.tinydb` clean until flush/eviction!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- WAL contains complete `kBegin -> kPageImage(s) -> kCommit` sequence before `Commit()` returns `Ok`; `PageHeader::lsn` matches WAL LSNs.

### Definition of Done
Durable `COMMIT` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-220: implement durable WAL COMMIT sequence and No-Steal release`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-221` (Full `ExecutionEngine` + `LogManager` + `TransactionManager` wiring).
````

---

## DAY-221 — Integrate `LogManager` and `TransactionManager` Across `Database` Lifecycle

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-221
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Integrate `LogManager` and `TransactionManager` Across `Database` Lifecycle
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-220 complete.

### Objective
Wire `LogManager` (`<db_prefix>.wal`) and `TransactionManager` cleanly into `Database` (`Create`, `Open`, `Close` -> auto-rollback active txn -> `BufferPoolManager::Shutdown` -> clean WAL checkpoint reset -> `DiskManager::Close`), ensuring all Phase 00–07 tests still pass seamlessly alongside WAL!

### Concepts & Daily Learning
- **Concept:** Unified database file triad (`.tinydb`, `.cat`, `.wal`) lifecycle and clean checkpointing on graceful `Database::Close()`.
- **Why Database Engines Need It:** On a clean shutdown (`Close()`), after `BufferPoolManager::Shutdown()` has flushed and synced all committed dirty pages to `.tinydb` and `CatalogManager` has synced `.cat`, truncating/resetting `.wal` cleanly avoids endlessly replaying old committed history on future normal restarts! (While leaving `.wal` intact whenever the DB crashes without `Close()`!).
- **How It Works:** `Database::Create` creates `.tinydb`, `.cat`, and `.wal`. `Database::Open` opens `.tinydb`, `.cat`, opens `.wal` (invoking `RecoveryManager` once implemented on `DAY-226`!). `Database::Close`: 1) rollback active uncommitted txn, 2) `BufferPoolManager::Shutdown()` (syncing `.tinydb`), 3) `LogManager::ResetCheckpoint()` (only AFTER `.tinydb` + `.cat` are synced!), 4) `LogManager::Close()`, `DiskManager::Close()`.
- **Design Trade-offs:** Truncating `.wal` ONLY *after* `BufferPoolManager::Shutdown()` succeeds guarantees `.wal` isn't cleared prematurely before dirty committed pages reach `.tinydb`!
- **Common Mistakes:** Clearing `.wal` *before* `BufferPoolManager::Shutdown()` flushes dirty pages (crash between those two steps would lose committed data!!).
- **Understanding Checkpoint:** Why MUST `LogManager::ResetCheckpoint()` happen *strictly after* `BufferPoolManager::Shutdown()` syncs `.tinydb` to disk?

### Component Context
- **Why It Exists:** Integrates WAL cleanly into `Database` and `tinydb` CLI.
- **Module Relationships:** Connects `Database`, `LogManager`, `TransactionManager`, `BufferPoolManager`, `DiskManager`, and `CatalogManager`.

### Files Expected to Change
- `include/tinydb/execution/execution_engine.hpp`, `src/execution/execution_engine.cpp`
- `tests/unit/execution/database_wal_lifecycle_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `.wal` creation/open/close/reset inside `Database`.
2. Run ALL existing Phase 00–07 tests + new lifecycle test verifying `.wal` exists while open, grows on committed DML, and resets cleanly on graceful `Close()`.

### Testing Instructions
- Build and run entire `ctest --output-on-failure` under Debug + ASan/UBSan!

### Acceptance Criteria
- 100% of earlier tests pass with WAL enabled; graceful `Close()` leaves all dirty data flushed before `.wal` resets.

### Definition of Done
`Database` + `LogManager` lifecycle wired and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-221: wire LogManager WAL lifecycle into Database Create/Open/Close`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-222` (Verify DDL-outside-transaction rules under WAL).
````

---

## DAY-222 — Enforce and Test DDL-Outside-Transaction Rules Under WAL

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-222
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Enforce and Test DDL-Outside-Transaction Rules Under WAL
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-221 complete.

### Objective
Write targeted tests (`tests/unit/transaction/ddl_txn_guard_test.cpp`) verifying DDL (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`) outside transactions remains safe (flushing prior committed dirty pages cleanly before `DROP TABLE`/`CREATE INDEX` if needed!) and DDL inside an active explicit `BEGIN` transaction fails cleanly with `kUnsupported` without corrupting or aborting the active transaction's ongoing DML!

### Concepts & Daily Learning
- **Concept:** Interaction boundary between transactional DML and non-transactional DDL (`DD-004`).
- **Why Database Engines Need It:** If a user runs `BEGIN; INSERT INTO a VALUES (1); CREATE TABLE b (x INT);` and our guard rejects `CREATE TABLE b`, the already-inserted row in `a` inside the active transaction must STILL remain cleanly commitable or rollbackable!
- **How It Works:** Verify `ExecuteStatement` rejects DDL before mutating `.cat` or `Transaction`, preserving `active_txn_` untouched! Also ensure standalone `DROP TABLE` / `CREATE INDEX` runs safely even when earlier auto-commit transactions have dirty pages in `BufferPoolManager` (`FlushAllPages` or clean `DeletePage`!).
- **Design Trade-offs:** Rejecting DDL inside active transactions cleanly matches Section 6 ("Keep DDL operations outside SQL transactions in v1").
- **Common Mistakes:** Rolling back or destroying `active_txn_` silently when rejecting an invalid DDL command inside `BEGIN`, OR letting `DROP TABLE` leave WAL `PAGE_IMAGE`s targeting dropped pages without flushing/syncing! (Flush dirty pages before DDL `DROP TABLE` / `CREATE INDEX`—super clean!).
- **Understanding Checkpoint:** Why is calling `BufferPoolManager::FlushAllPages()` cleanly before executing standalone `DROP TABLE` or `CREATE TABLE` helpful when WAL is enabled?

### Component Context
- **Why It Exists:** Hardens the DDL/DML transaction boundary.
- **Module Relationships:** Verifies `ExecutionEngine` + `TransactionManager` + `CatalogManager`.

### Files Expected to Change
- `src/execution/execution_engine.cpp`
- `tests/unit/transaction/ddl_txn_guard_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Ensure standalone DDL guards and active-txn DDL rejections behave cleanly.
2. Add tests verifying `BEGIN; INSERT ...; CREATE TABLE ... (fails); COMMIT;` commits the valid `INSERT` cleanly without creating the rejected table, and `BEGIN; INSERT ...; CREATE TABLE ... (fails); ROLLBACK;` rolls back cleanly!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Rejected DDL inside `BEGIN` leaves `active_txn_` intact and uncorrupted; standalone DDL persists safely.

### Definition of Done
DDL/transaction boundary tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-222: enforce and verify DDL-outside-transaction boundary under WAL`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-223` (`RecoveryManager` skeleton & Analysis phase).
````

---

## DAY-223 — Implement `RecoveryManager` Skeleton and Analysis Phase (Identifying Committed Txns)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-223
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `RecoveryManager` Skeleton and Analysis Phase (Identifying Committed Txns)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-216 (`LogReader`), DAY-222 complete.

### Objective
Create `tinydb::RecoveryManager` (`include/tinydb/recovery/recovery_manager.hpp`, `src/recovery/recovery_manager.cpp`) and implement **Phase 1 (Analysis)** using `LogReader` to scan `.wal` from offset 16, identify `committed_txns_` (`unordered_set<TxnId>` containing every txn with a valid `kCommit` record!), collect records in LSN order, detect `max_valid_lsn` and `torn_tail_offset`, and verify `Analyze` unit tests!

### Concepts & Daily Learning
- **Concept:** Recovery Analysis phase (building the set of committed transactions before applying a single byte to `.tinydb`!).
- **Why Database Engines Need It:** If a crash happened mid-flush of a `LogBuffer` (or if an aborted/uncommitted record ever landed in WAL), Analysis guarantees we NEVER redo any record belonging to a transaction without a complete, CRC-verified `kCommit` record!
- **How It Works:** Scan all valid records via `LogReader` into a structured `AnalysisResult`: `committed_txns` (every `txn_id` that logged `kCommit`), `records` (or replay metadata referencing WAL offsets/records so Redo doesn't blow memory on huge WALs—or vector of metadata `{offset, type, lsn, txn_id, page_id}`!), `max_lsn`, `valid_end_offset`. Reject mid-log corruption immediately BEFORE touching `.tinydb`!
- **Design Trade-offs:** Storing `{file_offset, type, lsn, txn_id, page_id}` during Analysis and reading 4KB payloads sequentially on Redo keeps Analysis memory `O(records * 32B)` instead of buffering every 4KB `PAGE_IMAGE` simultaneously!
- **Common Mistakes:** Redoing `PAGE_IMAGE` records on the fly during the first pass *before* verifying that the transaction actually has a valid `kCommit` record!
- **Understanding Checkpoint:** Why MUST Analysis complete pass #1 over `.wal` (finding `kCommit` records AND confirming no mid-log corruption) *before* Redo modifies a single page in `.tinydb`?

### Component Context
- **Why It Exists:** First phase of crash recovery (`Analyze -> Redo`).
- **Module Relationships:** Uses `LogReader`; feeds `Redo` (`DAY-224`–`DAY-225`).

### Files Expected to Change
- `include/tinydb/recovery/recovery_manager.hpp` (new)
- `src/recovery/recovery_manager.cpp` (new)
- `tests/unit/recovery/recovery_analysis_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `RecoveryManager::Analyze(const std::filesystem::path& wal_path)` returning `Result<AnalysisResult>`.
2. Write unit tests verifying: committed txn identified, uncommitted txn (with `kBegin` + `kPageImage` but NO `kCommit` or torn `kCommit`!) excluded from `committed_txns`, `max_lsn` recorded accurately, and mid-file WAL corruption rejected!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Only txns with complete CRC-verified `kCommit` records appear in `committed_txns`; torn uncommitted tails are excluded cleanly.

### Definition of Done
Analysis phase implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-223: implement RecoveryManager Analysis pass over WAL`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-224` (Redo phase & `PageHeader::lsn` idempotence).
````

---

## DAY-224 — Implement `RecoveryManager` Redo Phase with Idempotent `PageHeader::lsn` Checks

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-224
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Implement `RecoveryManager` Redo Phase with Idempotent `PageHeader::lsn` Checks
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-223 complete.

### Objective
Implement **Phase 2 (Redo)** inside `RecoveryManager::Recover(...)`: iterating committed records in strict ascending LSN order:
- For `kPageImage`: if `page_id < disk.PageCount()` and disk page is readable AND `disk_page.GetLsn() >= record.lsn`, **skip** (already on disk — idempotent!); otherwise invoke `disk.ApplyRecoveredPage(page_id, image)`.
- For `kFreePage`: if page isn't already `kFree` (check `page_lsn` or state safely!), invoke `disk.ApplyRecoveredFreePage(page_id)`.
- Collect committed `kIndexRoot` updates to apply to `CatalogManager`!
- Finally call `disk.FlushAll()`, update `LogManager::SetNextLsn(max_lsn + 1)` (and truncate torn tail if any!), and return recovery stats (`replayed_pages`, `skipped_pages`, `committed_txns`)!

### Concepts & Daily Learning
- **Concept:** LSN-guarded idempotent Redo (`page_lsn >= record_lsn -> skip`) and handling newly extended pages (`page_id >= PageCount()`).
- **Why Database Engines Need It:** Suppose Txn 1 (LSN 1..3) flushed Page 2 (`lsn = 3`) to disk, Txn 2 (LSN 4..6) later modified Page 2 (`lsn = 6`) and ALSO flushed to disk, and then a crash occurred before WAL truncation! Without checking `disk_page.lsn >= record.lsn`, replaying Txn 1's older `PAGE_IMAGE(lsn=3)` would OVERWRITE Txn 2's newer Page 2 (`lsn=6`) with stale old data!!
- **How It Works:** Read disk page header at `page_id`; if `page_id < PageCount()`, validate CRC32 and `GetLsn()`, skipping ONLY when the disk page validates cleanly AND `disk_page.GetLsn() >= record.lsn`!
- **Design Trade-offs:** Notice that if `.tinydb` suffered a **torn page** from crashing mid-write after `COMMIT`, `ReadPage(page_id)` fails CRC32—and because the WAL holds the complete checksummed `PAGE_IMAGE`, `RecoveryManager` overwrites the torn disk page with the valid WAL image and **heals the torn page** instead of skipping it!
- **Common Mistakes:** Skipping a page solely by inspecting raw un-checksummed `lsn` bytes without running `Page::Validate` first (failing to heal torn disk pages!), OR omitting `disk_lsn >= record.lsn` (overwriting newer commits with older WAL images!).
- **Understanding Checkpoint:** Explain TWO critical scenarios where `RecoveryManager::Redo` MUST check `disk_page.Validate() && disk_page.lsn >= record.lsn`: (1) why skipping when `>=` prevents overwriting newer commits, and (2) why NOT skipping a *corrupt/torn* disk page allows WAL to heal torn database writes!

### Component Context
- **Why It Exists:** Restores committed database state idempotently after any crash.
- **Module Relationships:** Drives `DiskManager` recovery methods; hooked into `Database::Open` (`DAY-225`).

### Files Expected to Change
- `include/tinydb/recovery/recovery_manager.hpp`
- `src/recovery/recovery_manager.cpp`
- `tests/unit/recovery/recovery_redo_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `RecoveryManager::Recover(DiskManager&, const std::filesystem::path& wal_path, ...)` returning `Result<RecoveryReport>`.
2. Write unit tests verifying: skipped when `disk_lsn >= record_lsn`, applied when `disk_lsn < record_lsn`, torn disk page healed by valid WAL image, uncommitted WAL records ignored, and running `Recover` twice in a row skips 100% on pass 2 (idempotence!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Pass 1 recovers committed pages (`replayed > 0`); Pass 2 skips all (`replayed == 0`, `skipped == total`); torn disk pages heal cleanly.

### Definition of Done
Redo recovery and idempotence tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-224: implement idempotent WAL Redo recovery with torn-page healing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-225` (Startup recovery integration in `Database::Open`).
````

---

## DAY-225 — Wire `RecoveryManager` into `Database::Open` Startup Sequence

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-225
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Wire `RecoveryManager` into `Database::Open` Startup Sequence
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-224 complete.

### Objective
Integrate `RecoveryManager::Recover` into `Database::Open` BEFORE `CatalogManager::Load` and `BufferPoolManager` startup (and apply recovered `kIndexRoot` updates cleanly into `CatalogManager`!), truncate any torn tail bytes in `.wal` safely, advance `LogManager::SetNextLsn(report.max_lsn + 1)`, and verify end-to-end startup recovery!

### Concepts & Daily Learning
- **Concept:** Startup ordering (`Open DiskManager -> Recover WAL onto .tinydb -> Sync .tinydb -> Load CatalogManager -> Apply recovered kIndexRoot updates -> Start BufferPoolManager -> Resume LogManager`).
- **Why Database Engines Need It:** Notice the order! `CatalogManager::Load` validates `TableHeap::Open` for every table—if a newly committed `TableHeap` page hadn't flushed to `.tinydb` yet, running `RecoveryManager` *before* `CatalogManager::Load` guarantees all table pages exist on disk when `CatalogManager::Load` opens them! And applying recovered `kIndexRoot`s ensures `CatalogManager` uses the post-crash root page!
- **How It Works:** In `Database::Open`:
  1. `DiskManager::Open`
  2. If `.wal` exists: run `RecoveryManager::Recover(disk_, wal_path)` (recording any `recovered_index_roots`!).
  3. Open `BufferPoolManager` + `CatalogManager::Load`!
  4. Apply any `recovered_index_roots` via `CatalogManager::UpdateIndexRootPage`!
  5. Open `LogManager` at valid end offset with `next_lsn = report.max_lsn + 1`!
- **Design Trade-offs:** Notice if `.wal` is missing on opening an existing clean DB, create/reinitialize `.wal` safely (`OpenOrCreate`); if `.wal` exists, replay before loading catalog!
- **Common Mistakes:** Initializing `BufferPoolManager` BEFORE `RecoveryManager` modifies `.tinydb` directly via `DiskManager` (causing `BufferPoolManager` to cache stale pre-recovery pages!).
- **Understanding Checkpoint:** Why MUST `RecoveryManager::Recover` finish modifying `.tinydb` *before* `BufferPoolManager` is started and *before* `CatalogManager::Load` runs?

### Component Context
- **Why It Exists:** Completes automatic crash recovery upon reopening any TinyDB database.
- **Module Relationships:** Hooks `RecoveryManager` into `Database::Open` (`src/execution/execution_engine.cpp`).

### Files Expected to Change
- `src/execution/execution_engine.cpp`
- `src/recovery/log_manager.cpp` (truncate torn tail safely on reopen)
- `tests/integration/phase08_startup_recovery_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `RecoveryManager` into `Database::Open`.
2. Add tests simulating crash (writing committed WAL without flushing `BufferPoolManager`, reopening via `Database::Open`), verifying rows, indexes, and root page updates recover transparently and `next_lsn` continues monotonically!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Reopening a DB with un-flushed committed WAL records automatically replays them and restores SQL query accuracy.

### Definition of Done
Startup recovery wired and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-225: wire automatic WAL RecoveryManager into Database::Open`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-226` (Controlled crash simulation harness).
````

---

## DAY-226 — Build Controlled Crash Simulation Harness (`fork` / Fault-Injection Hooks)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-226
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Build Controlled Crash Simulation Harness (`fork` / Fault-Injection Hooks)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-225 complete.

### Objective
Build a clean crash-testing harness (`include/tinydb/recovery/crash_point.hpp`, `src/recovery/crash_point.cpp`, `tests/recovery/crash_harness.hpp`) supporting deterministic crash injection (invoking `std::_Exit(99)` inside an isolated `fork()`ed child process at named crash points: `kBeforeWalFlush`, `kAfterWalCommitBeforePageFlush`, `kDuringPageFlush`, `kDuringRecovery`, etc.) without leaking or affecting the parent test process!

### Concepts & Daily Learning
- **Concept:** Process-level crash simulation (`fork()` + `std::_Exit()` / `SIGKILL`) vs fake in-memory destruction!
- **Why Database Engines Need It:** Calling `~BufferPoolManager()` runs cleanup destructors! A real power failure or `SIGKILL` skips ALL C++ destructors and leaves userspace RAM buffers completely vanished! `fork()` + `std::_Exit()` tests genuine OS-level abrupt process termination while keeping ASan/GoogleTest in the parent test process clean!
- **How It Works:** Compile lightweight `CrashHook` singleton/registry (no-op unless armed in test process!) with named points: `kBeforeCommitWalFlush`, `kAfterCommitWalFlush`, `kAfterPartialPageFlush`, `kDuringRecoveryAfterOnePage`. In tests: `pid_t pid = fork();` -> child arms hook, runs SQL transactions, exits abruptly via `_Exit(99)` at the target crash point -> parent `waitpid` checks child exited at exact crash point, then parent opens a fresh `Database` and verifies recovery!
- **Design Trade-offs:** Using `_Exit` inside child skips `atexit`/destructors identically to `SIGKILL` (testing real OS file durability!) without triggering spurious ASan leak-detector failures in the intentionally crashed child!
- **Common Mistakes:** Testing "crash recovery" by simply letting `Database` go out of scope (which runs `~Database()` -> `Close()` -> flushes everything!!); forgetting `std::fflush(nullptr)` before `fork()` causing duplicated GoogleTest stdout!
- **Understanding Checkpoint:** Why does testing crash recovery by letting a C++ `Database` object go out of scope NOT actually test a crash?

### Component Context
- **Why It Exists:** Enables authentic crash testing in `DAY-227`–`DAY-232`.
- **Module Relationships:** Instrumented cleanly in `TransactionManager`, `BufferPoolManager`, and `RecoveryManager`.

### Files Expected to Change
- `include/tinydb/recovery/crash_point.hpp` (new)
- `src/recovery/crash_point.cpp` (new)
- `src/transaction/transaction_manager.cpp`, `src/buffer/buffer_pool_manager.cpp`, `src/recovery/recovery_manager.cpp`
- `tests/recovery/crash_harness.hpp` (new)
- `tests/recovery/crash_harness_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CrashInjector` (`Arm(CrashPoint, count)` -> `MaybeCrash(CrashPoint)` calling `_Exit(99)`) and `RunInCrashChild(...)` helper (`fflush`, `fork`, `waitpid`).
2. Add smoke test verifying child exits abruptly with status `99` at armed crash point while parent cleanly proceeds!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Child terminates abruptly at armed crash point without running destructors (`Close()` bypassed); parent verifies cleanly.

### Definition of Done
Crash harness built and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-226: build fork/_Exit controlled crash injection harness`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-227` (Crash Test 1: mid-transaction before `COMMIT`).
````

---

## DAY-227 — Controlled Crash Test 1: Crash Mid-Transaction Before `COMMIT`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-227
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Controlled Crash Test 1: Crash Mid-Transaction Before `COMMIT`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-226 complete.

### Objective
Write crash recovery tests (`tests/recovery/crash_before_commit_test.cpp`) verifying:
1. Committed Txn 1 survives when Txn 2 begins, performs `INSERT`/`UPDATE`/`DELETE` (including B+ Tree splits!), and crashes BEFORE `COMMIT`.
2. Txn 2 crashing DURING `Commit()` right BEFORE `LogManager::Flush()` (`kBeforeCommitWalFlush`!) ALSO leaves 0 uncommitted rows on disk!
3. Both `.tinydb` and B+ Tree indexes match Txn 1's exact pre-Txn-2 state after reopen + recovery!

### Concepts & Daily Learning
- **Concept:** Atomicity of uncommitted transactions across abrupt process death (`SIGKILL`/`_Exit`).
- **Why Database Engines Need It:** Proves both No-Steal (no uncommitted bytes touched `.tinydb`) AND WAL durability barriers (un-flushed WAL records don't corrupt recovery or leak uncommitted rows!).
- **How It Works:** In `fork()` child: commit Txn 1, begin Txn 2, run multiple inserts/updates/deletes spanning pages & splits, crash either before `COMMIT` or at `kBeforeCommitWalFlush`. Parent reopens DB, runs `SELECT`s and `BPlusTreeValidator`, asserting exact Txn 1 state!
- **Design Trade-offs:** Testing BOTH "crash mid-transaction" AND "crash inside `COMMIT` before `fsync`" covers the entire pre-durability window!
- **Common Mistakes:** Allowing `TableHeap` or `BPlusTree` to steal/flush a page during Txn 2's splits!
- **Understanding Checkpoint:** Why do NONE of Txn 2's B+ Tree splits survive in `.tinydb` when Txn 2 crashes mid-transaction?

### Component Context
- **Why It Exists:** Proves Atomicity on pre-commit crashes.
- **Module Relationships:** Exercises No-Steal + `RecoveryManager` Analysis.

### Files Expected to Change
- `tests/recovery/crash_before_commit_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement pre-commit and `kBeforeCommitWalFlush` crash tests in `crash_before_commit_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Uncommitted Txn 2 changes vanish 100%; committed Txn 1 data & indexes remain intact and pass `BPlusTreeValidator`.

### Definition of Done
Pre-commit crash tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-227: add controlled crash tests before COMMIT and WAL flush`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-228` (Crash Test 2: crash after `ROLLBACK`).
````

---

## DAY-228 — Controlled Crash Test 2: Crash After `ROLLBACK`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-228
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Controlled Crash Test 2: Crash After `ROLLBACK`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-227 complete.

### Objective
Write crash recovery tests (`tests/recovery/crash_after_rollback_test.cpp`) verifying that when Txn 1 commits, Txn 2 runs DML + `ROLLBACK;`, and the process crashes BEFORE graceful shutdown (or after Txn 3 commits!), rolled-back Txn 2 changes NEVER reappear after WAL recovery!

### Concepts & Daily Learning
- **Concept:** Guaranteeing rolled-back transactions stay dead across subsequent crash recovery (including when earlier/later transactions share the same pages!).
- **Why Database Engines Need It:** If `ROLLBACK` touched the WAL carelessly or broke `was_dirty` / `PageHeader::lsn` on a shared page previously touched by Txn 1, crashing right after `ROLLBACK` could lose Txn 1 or resurrect Txn 2!
- **How It Works:** In `fork()` child: commit Txn 1 (modifying Page 1, NOT flushed to `.tinydb`), run Txn 2 modifying Page 1 + splitting index + `ROLLBACK;`, then crash abruptly (`_Exit(99)`). Also test `Txn 1 commit -> Txn 2 rollback -> Txn 3 commit -> crash`! Reopen in parent and verify exact `(Txn 1)` and `(Txn 1 + Txn 3)` states!
- **Design Trade-offs:** Keeping `ROLLBACK` strictly in RAM (zero `PAGE_IMAGE` written to WAL) avoids even needing `ABORT` replay records while keeping WAL clean.
- **Common Mistakes:** `Rollback()` accidentally clearing `PageHeader::lsn` to `0` on a page whose `before_image` had `lsn = 3` from Txn 1! (Actually `before_image` captures the entire 4KB page INCLUDING its `lsn = 3`, preserving Txn 1's LSN automatically!)
- **Understanding Checkpoint:** After `Txn 1` commits (writing `Page 1` at `lsn = 3`), `Txn 2` modifies `Page 1` and executes `ROLLBACK`, and then the DB crashes—what happens during recovery?

### Component Context
- **Why It Exists:** Validates `ROLLBACK` + WAL interaction under crash.
- **Module Relationships:** Exercises `Rollback()` + `RecoveryManager`.

### Files Expected to Change
- `tests/recovery/crash_after_rollback_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement post-rollback crash tests in `crash_after_rollback_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Rolled-back changes remain 100% absent; earlier and subsequent committed transactions recover cleanly.

### Definition of Done
Post-rollback crash tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-228: add controlled crash tests after explicit ROLLBACK`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-229` (Crash Test 3: crash AFTER `COMMIT` WAL flush BEFORE DB page flush).
````

---

## DAY-229 — Controlled Crash Test 3: Crash After `COMMIT` WAL Flush Before Any DB Page Flush

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-229
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Controlled Crash Test 3: Crash After `COMMIT` WAL Flush Before Any DB Page Flush
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-228 complete.

### Objective
Write the flagship Durability crash test (`tests/recovery/crash_after_commit_test.cpp`) verifying that when transactions commit (performing inserts, updates, deletes, new page allocations, B+ Tree root splits, and deferred page frees) and crash IMMEDIATELY at `kAfterCommitWalFlush` (zero dirty pages flushed to `.tinydb`!), `RecoveryManager` replays the WAL on reopen and restores 100% of committed rows, new pages, root pointers, and valid `BPlusTree` invariants!

### Concepts & Daily Learning
- **Concept:** Proving WAL **Force-at-Commit** + **No-Force** page flushing durability (`COMMIT` survives even when `0` modified `.tinydb` pages reached disk!).
- **Why Database Engines Need It:** This is the essence of WAL: committing fast with a single sequential WAL `fsync` without flushing dozens of random 4KB heap/B+ Tree pages at commit time, yet never losing a committed row!
- **How It Works:** In `fork()` child: create table + index, commit Txn 1 (200 rows triggering multi-level B+ Tree root split + multiple `TableHeap` pages!), arm `kAfterCommitWalFlush` on Txn 2 (`UPDATE` + `DELETE` + `INSERT`), crash immediately inside `Commit()` right after `LogManager::Flush()`. Parent reopens DB cold and verifies EVERY single row and index via SQL + `BPlusTreeValidator`!
- **Design Trade-offs:** Verifying across BOTH `TableHeap` and `BPlusTree` root splits exercises `kPageImage`, `kIndexRoot`, and `ApplyRecoveredPage` extension together!
- **Common Mistakes:** Failing to update `FileHeader` or extend `.tinydb` when `NewPage` pages were committed only in WAL!
- **Understanding Checkpoint:** When Txn 2 crashes right after `LogManager::Flush()` with zero `.tinydb` writes, trace every record type `RecoveryManager` replays to reconstruct both `.tinydb` and `.cat`.

### Component Context
- **Why It Exists:** Flagship ACID Durability proof for TinyDB v1.
- **Module Relationships:** Exercises full `Commit -> Crash -> RecoveryManager -> Catalog -> SQL` stack.

### Files Expected to Change
- `tests/recovery/crash_after_commit_test.cpp` (new)
- `src/recovery/recovery_manager.cpp` (fix any root/extension edge uncovered)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `crash_after_commit_test.cpp` testing inserts, splits, cross-page updates, and deletes crashing at `kAfterCommitWalFlush`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero `.tinydb` writes occurred before crash yet 100% of committed data and B+ Tree indexes recover accurately on reopen!

### Definition of Done
Post-commit durability crash tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-229: verify durable recovery when crashing right after WAL COMMIT flush`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-230` (Crash Test 4: crash DURING DB page flushing).
````

---

## DAY-230 — Controlled Crash Test 4: Crash During Partial DB Page Flushing

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-230
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Controlled Crash Test 4: Crash During Partial DB Page Flushing
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-229 complete.

### Objective
Write crash tests (`tests/recovery/crash_during_flush_test.cpp`) simulating a crash **midway** through `BufferPoolManager::FlushAllPages()` (or partial eviction): some committed pages from Txn 1 & Txn 2 HAVE flushed to `.tinydb` (carrying higher `lsn`), some flushed with an older `lsn` (and one even simulated torn on disk!), and remaining pages did NOT flush yet—verifying `RecoveryManager` skips up-to-date pages, replays stale/missing pages, heals torn disk pages, and converges to 100% exact committed state!

### Concepts & Daily Learning
- **Concept:** Partial-checkpoint / mid-flush crash convergence via `PageHeader::lsn` comparisons.
- **Why Database Engines Need It:** `FlushAllPages()` writes pages sequentially over milliseconds; crashing halfway leaves `.tinydb` in a mixed temporal state (Page 1 at LSN 12, Page 2 at LSN 4, Page 3 at LSN 0!).
- **How It Works:** Add a `CrashPoint::kDuringBufferFlush` hook inside `FlushAllPagesLocked` after flushing half of the dirty pages (and also test injecting a torn half-written page!). Child commits Txn 1 and Txn 2, calls `FlushAllPages()` armed at `kDuringBufferFlush`, crashes midway! Parent recovers and verifies `RecoveryReport` (`skipped > 0` AND `replayed > 0`) and exact SQL + `BPlusTreeValidator` correctness!
- **Design Trade-offs:** Checking `skipped > 0 && replayed > 0` directly proves partial flush + LSN skip logic worked!
- **Common Mistakes:** Only testing "0 pages flushed" (`DAY-229`) without testing the mixed-LSN partial-flush case!
- **Understanding Checkpoint:** If Page 1 already flushed at `LSN = 12` while Page 2 is still at `LSN = 4` when crashing mid-`FlushAllPages()`, how does `RecoveryManager` handle WAL records `1..12`?

### Component Context
- **Why It Exists:** Proves partial page flush convergence and LSN filtering.
- **Module Relationships:** Tests `BufferPoolManager::FlushAllPages` + `RecoveryManager::Recover`.

### Files Expected to Change
- `src/buffer/buffer_pool_manager.cpp` (arm `kDuringBufferFlush`)
- `tests/recovery/crash_during_flush_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `CrashPoint::kDuringBufferFlush` cleanly in `FlushAllPagesLocked`.
2. Write `crash_during_flush_test.cpp` verifying mixed-LSN partial flush + torn page healing recovery.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Partial flush crash converges cleanly (`skipped > 0`, `replayed > 0`), passing all SQL and B+ Tree validator assertions.

### Definition of Done
Mid-flush crash tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-230: verify recovery after crash midway through DB page flushing`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-231` (Crash Test 5: crash DURING recovery itself).
````

---

## DAY-231 — Controlled Crash Test 5: Crash During Recovery (Recovery Idempotence)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-231
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Controlled Crash Test 5: Crash During Recovery (Recovery Idempotence)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-230 complete.

### Objective
Write crash-during-recovery tests (`tests/recovery/crash_during_recovery_test.cpp`) verifying that if `Database::Open` invokes `RecoveryManager::Recover` and crashes **halfway through replaying WAL records** (`CrashPoint::kDuringRecovery` after 2 replayed pages, BEFORE `FlushAll` or WAL reset!), a **second** `Database::Open` recovers smoothly from the partially recovered `.tinydb` + untouched `.wal` and reaches 100% identical final state!

### Concepts & Daily Learning
- **Concept:** Crash-safe, repeatable (idempotent) recovery ("recovering from a crash during recovery").
- **Why Database Engines Need It:** Power outages often flicker multiple times! If recovery truncated `.wal` *before* flushing recovered pages or wasn't idempotent on `PageHeader::lsn`, crashing during recovery would permanently destroy committed data!
- **How It Works:** Ensure `Database::Open` NEVER truncates `.wal` until `RecoveryManager` finishes AND `DiskManager::FlushAll()` succeeds (and actually keep `.wal` cleanly until next close or checkpoint!). In test: Child 1 commits multi-page Txn 1 + Txn 2 and crashes at `kAfterCommitWalFlush`. Child 2 opens DB armed with `CrashPoint::kDuringRecovery` (crashes midway through Redo!). Parent then opens DB cleanly with no crash hooks -> completes full recovery -> verifies exact rows and `BPlusTreeValidator`!
- **Design Trade-offs:** Never clearing `.wal` until `RecoveryManager` finishes AND syncs `.tinydb` makes repeated recovery 100% lossless across arbitrary consecutive crashes!
- **Common Mistakes:** Resetting/truncating `.wal` inside `RecoveryManager` before `DiskManager::FlushAll()` finishes!
- **Understanding Checkpoint:** Why MUST `.wal` remain preserved until *after* `RecoveryManager` has flushed and synced all recovered `.tinydb` pages to disk?

### Component Context
- **Why It Exists:** Proves TinyDB recovers reliably even across repeated crashes during recovery!
- **Module Relationships:** Tests `RecoveryManager` + `Database::Open` across multi-stage crashes.

### Files Expected to Change
- `src/recovery/recovery_manager.cpp` (`kDuringRecovery` hook)
- `tests/recovery/crash_during_recovery_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Wire `CrashPoint::kDuringRecovery` cleanly after applying N pages in `RecoveryManager::Recover`.
2. Implement `crash_during_recovery_test.cpp` exercising Child 1 (work + crash) -> Child 2 (recover + crash midway) -> Parent (final recovery + verify!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Database survives consecutive crashes (mid-commit -> mid-recovery) with zero data loss and 100% valid B+ Trees!

### Definition of Done
Crash-during-recovery tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-231: verify crash-during-recovery idempotence across multi-stage crashes`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-232` (Multi-transaction interleaved commit/rollback/crash test).
````

---

## DAY-232 — Verify Multi-Transaction Interleaved Sequences Under Crash Recovery

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-232
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Verify Multi-Transaction Interleaved Sequences Under Crash Recovery
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-231 complete.

### Objective
Write `tests/recovery/multi_txn_recovery_test.cpp` exercising realistic multi-transaction sequences (`Txn 1 COMMIT -> Txn 2 ROLLBACK -> Txn 3 COMMIT (evicting some dirty Txn 1 pages!) -> Txn 4 ROLLBACK -> Txn 5 COMMIT -> Txn 6 UNCOMMITTED CRASH`), verifying after recovery that ONLY `{Txn 1, Txn 3, Txn 5}` persist in exact causal order across `TableHeap` and `BPlusTree`s!

### Concepts & Daily Learning
- **Concept:** Causal LSN replay ordering across multiple sequential commits mixed with rollbacks and buffer-pool evictions.
- **Why Database Engines Need It:** Txn 1 modifies Row A (`lsn 2`), Txn 3 later overwrites Row A (`lsn 8`), Txn 5 deletes Row A (`lsn 14`). Replaying out of LSN order or mis-filtering Txn 2/4/6 would resurrect deleted rows!
- **How It Works:** Execute the 6-transaction scenario inside `fork()` child using a small buffer pool (`pool_size = 6`) so committed pages from Txn 1 actually evict while Txn 3 & Txn 5 continue running, then crash during Txn 6! Reopen in parent and verify exact final row values and `BPlusTreeValidator`!
- **Design Trade-offs:** Using a small pool (`6` frames) proves committed eviction + subsequent uncommitted rollback/crash compose cleanly!
- **Common Mistakes:** Confusing `before_image` dirty state when a page modified by committed Txn 1 gets modified by Txn 2 (`ROLLBACK`) and then by Txn 3 (`COMMIT`)!
- **Understanding Checkpoint:** Trace Page 2's `is_dirty` and `lsn` across: `Txn 1 COMMIT -> Txn 2 modify + ROLLBACK -> Txn 3 modify + COMMIT -> Crash`.

### Component Context
- **Why It Exists:** Validates complex multi-transaction WAL/buffer interactions.
- **Module Relationships:** Exercises `TransactionManager`, `BufferPoolManager`, `LogManager`, and `RecoveryManager`.

### Files Expected to Change
- `tests/recovery/multi_txn_recovery_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `multi_txn_recovery_test.cpp` covering the 6-transaction commit/rollback/evict/crash sequence.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Post-crash DB matches exact state of `{Txn 1, Txn 3, Txn 5}` and passes `BPlusTreeValidator` on all indexes.

### Definition of Done
Multi-txn crash recovery tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-232: verify multi-transaction commit/rollback/crash recovery sequences`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-233` (`BPlusTreeValidator` check across all crash scenarios).
````

---

## DAY-233 — Verify B+ Tree Structural Invariants Across All Crash Recovery Scenarios

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-233
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Verify B+ Tree Structural Invariants Across All Crash Recovery Scenarios
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-232 complete.

### Objective
Write `tests/recovery/btree_crash_invariant_test.cpp` running intense B+ Tree split/merge/root-shrink workloads across `COMMIT`s and `ROLLBACK`s, crashing at multiple points (mid-split uncommitted, post-root-split commit, post-root-shrink commit, mid-flush), and running `BPlusTreeValidator::Validate(&tree, &table, col)` on EVERY index after recovery!

### Concepts & Daily Learning
- **Concept:** Structural tree-table consistency after abrupt crashes across root height changes.
- **Why Database Engines Need It:** If `TableHeap` pages recovered to post-Txn-3 while `BPlusTree` pages or `.cat`'s `root_page_id` recovered to post-Txn-1 (or a leaf sibling `next_page_id` broke!), queries would return wrong answers or corrupt on the next `INSERT`!
- **How It Works:** Populate tables with 2 B+ Tree indexes, execute root-growing and root-shrinking transactions with crashes at each stage, recover, verify `BPlusTreeValidator` (tree invariants + table `RowID` equivalence!) AND continue running *new* inserts/deletes post-recovery!
- **Design Trade-offs:** Continuing to mutate the B+ Tree *after* crash recovery catches subtle leftover `parent_page_id` or `next_lsn_` bugs!
- **Common Mistakes:** Only running `SELECT` after recovery instead of running further `INSERT`/`DELETE`/`BPlusTreeValidator` mutations on the recovered tree!
- **Understanding Checkpoint:** Why is mutating a crash-recovered B+ Tree with additional splits and deletes an essential follow-up check after `RecoveryManager` finishes?

### Component Context
- **Why It Exists:** Proves structural integrity of indexed databases post-recovery.
- **Module Relationships:** Combines `RecoveryManager` + `BPlusTreeValidator` + `ExecutionEngine`.

### Files Expected to Change
- `tests/recovery/btree_crash_invariant_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `btree_crash_invariant_test.cpp` covering root-split, root-shrink, and post-recovery mutation workloads.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `BPlusTreeValidator` passes across all recovered trees AND post-recovery mutations persist cleanly.

### Definition of Done
B+ Tree crash-invariant tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-233: verify BPlusTree structural invariants and post-crash mutations`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-234` (WAL truncated tail vs mid-log corruption tests).
````

---

## DAY-234 — Test WAL Torn Tail Truncation vs Mid-Log Corruption Rejection During Recovery

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-234
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Test WAL Torn Tail Truncation vs Mid-Log Corruption Rejection During Recovery
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-233 complete.

### Objective
Write recovery corruption tests (`tests/recovery/wal_corruption_recovery_test.cpp`) verifying:
1. A WAL with Txn 1 committed followed by torn partial bytes of Txn 2 (`kPageImage` truncated mid-write at EOF) recovers Txn 1 cleanly, truncates the torn tail, and lets Txn 3 append cleanly afterward!
2. A WAL with a bit-flip inside Txn 1's middle record (or bad WAL magic header!) rejects recovery cleanly with `StatusCode::kCorruption` without overwriting `.tinydb`!

### Concepts & Daily Learning
- **Concept:** Byte-exact WAL torn-tail healing (`truncate` to `valid_end_offset`) vs strict rejection of mid-log tampering.
- **Why Database Engines Need It:** If `.wal` ends at offset `4180` because `write()` of Txn 2 tore at byte 40 of its `kPageImage`, leaving those 40 garbage tail bytes in `.wal` would break subsequent `LogManager::Append` writes at offset `4180`! Truncating `.wal` cleanly back to `valid_end_offset` (`4140`) after Analysis ensures subsequent WAL appends stay aligned and valid!
- **How It Works:** Verify `LogManager::Open` truncates `torn_tail_offset` back to `valid_end_offset` cleanly ONLY after `RecoveryManager::Analyze` confirms all prior records are valid (`kOk`), while any mid-file CRC error aborts `Database::Open` immediately before modifying `.tinydb`!
- **Design Trade-offs:** Truncating torn tails safely keeps `.wal` strictly clean for subsequent commits.
- **Common Mistakes:** Appending new WAL records AFTER unread torn tail bytes (creating an un-scannable gap inside `.wal`!).
- **Understanding Checkpoint:** Why MUST `LogManager` truncate a torn WAL tail back to `valid_end_offset` before appending new transactions after recovery?

### Component Context
- **Why It Exists:** Ensures `.wal` remains cleanly appendable after torn-write crashes while rejecting mid-log corruption.
- **Module Relationships:** Hardens `LogReader`, `LogManager`, and `RecoveryManager`.

### Files Expected to Change
- `tests/recovery/wal_corruption_recovery_test.cpp` (new)
- `src/recovery/log_manager.cpp`, `src/recovery/recovery_manager.cpp` (verify tail truncation order)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement torn-tail recovery + subsequent new commit + reopen test, plus mid-log bit-flip + bad WAL header rejection tests in `wal_corruption_recovery_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Torn tail heals and allows new durable commits; mid-log corruption aborts `Database::Open` cleanly without touching `.tinydb`.

### Definition of Done
WAL corruption & torn-tail tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-234: test WAL torn-tail healing vs mid-log corruption rejection`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-235` (CLI transaction UX & multi-page txn stress test).
````

---

## DAY-235 — Test Multi-Page Transactions and CLI `BEGIN` / `COMMIT` / `ROLLBACK` Integration

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-235
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Test Multi-Page Transactions and CLI `BEGIN` / `COMMIT` / `ROLLBACK` Integration
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-234 complete.

### Objective
Write CLI + multi-page transaction integration tests (`tests/integration/phase08_cli_txn_test.cpp`) verifying interactive/piped `BEGIN; ... COMMIT;`, `BEGIN; ... ROLLBACK;`, uncommitted transaction auto-rollback on `.exit`/EOF, and pool-exhaustion safety (`kResourceExhausted` when a huge uncommitted transaction exceeds `BufferPoolManager` capacity under No-Steal!).

### Concepts & Daily Learning
- **Concept:** Graceful handling of No-Steal buffer exhaustion when a single uncommitted transaction touches more pages than `pool_size` frames!
- **Why Database Engines Need It:** Because No-Steal forbids evicting uncommitted frames, a tiny pool (`pool_size = 4`) attempting an uncommitted `INSERT` across `10` distinct pages MUST fail gracefully (`kResourceExhausted`), allowing the caller to `ROLLBACK;` cleanly rather than corrupting disk or hanging!
- **How It Works:** Test `pool_size = 4` trying to dirty 6 pages inside `BEGIN`: verify `kResourceExhausted`, verify `ROLLBACK;` cleanly recovers all frames (`ActivePinCount() == 0`), and verify CLI `.exit` with an unclosed `BEGIN` discards uncommitted inserts!
- **Design Trade-offs:** Documenting the `pool_size` bound of No-Steal transactions honestly (solved by Steal+ARIES in v2) demonstrates crisp systems understanding!
- **Common Mistakes:** Leaving frames held as `uncommitted_hold_` if an error occurs mid-statement!
- **Understanding Checkpoint:** Why does a No-Steal transaction face a hard cap on how many distinct pages it can dirty simultaneously (`<= pool_size`), and how should TinyDB behave if that cap is hit?

### Component Context
- **Why It Exists:** Validates CLI transaction usability and No-Steal capacity error recovery.
- **Module Relationships:** Exercises `CliSession`, `ExecutionEngine`, `TransactionManager`, and `BufferPoolManager`.

### Files Expected to Change
- `tests/integration/phase08_cli_txn_test.cpp` (new)
- `src/cli/cli_session.cpp` (print helpful prompt/info on `BEGIN`/`COMMIT`/`ROLLBACK`!)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Ensure CLI outputs clean messages (`"Transaction begun."`, `"Transaction committed."`, `"Transaction rolled back."`) on TCL commands.
2. Implement `phase08_cli_txn_test.cpp` testing CLI TCL sessions, `.exit` auto-rollback, and `pool_size = 4` No-Steal exhaustion -> `ROLLBACK` recovery!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- CLI TCL flows, `.exit` auto-rollback, and No-Steal exhaustion recovery pass cleanly.

### Definition of Done
CLI transaction and No-Steal exhaustion tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-235: verify CLI transactions, auto-rollback on exit, and No-Steal limits`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-236` (ASan/UBSan audit of `transaction/` & `recovery/`).
````

---

## DAY-236 — Run ASan/UBSan and Memory-Leak Audit Across `transaction/` and `recovery/`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-236
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Run ASan/UBSan and Memory-Leak Audit Across `transaction/` and `recovery/`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-235 complete.

### Objective
Audit all files in `include/tinydb/transaction/`, `src/transaction/`, `include/tinydb/recovery/`, and `src/recovery/` for narrowing conversions, uninitialized buffers, `std::filesystem` error paths, dangling pointers across `Commit`/`Rollback`, and full ASan+UBSan+LSan cleanliness!

### Concepts & Daily Learning
- **Concept:** Memory-safety and error-handling audit across crash-critical subsystems.
- **Why Database Engines Need It:** Recovery and transaction rollback execute under rare error paths (`kIoError`, `kResourceExhausted`, partial logs); an untested leak or uninitialized variable in recovery paths causes catastrophic recovery failures!
- **How It Works:** Inspect every `src/transaction/*.cpp` and `src/recovery/*.cpp` file, verify `clang-format`, check `-Werror` warnings (`-Wconversion -Wsign-conversion -Wshadow`), and run all Phase 00–08 tests under ASan+UBSan and Release.
- **Design Trade-offs:** Fixing subtle signed/unsigned casts or error cleanup issues now keeps Phase 09 focused cleanly on full-system security testing.
- **Common Mistakes:** Leaving `std::filesystem::file_size` throwing unhandled `filesystem_error` exceptions instead of passing `std::error_code ec`!
- **Understanding Checkpoint:** Why should `std::filesystem` operations (`file_size`, `exists`, `rename`, `resize_file`) always use the `std::error_code` overload inside TinyDB's `Status`-returning storage/recovery code?

### Component Context
- **Why It Exists:** Hardens transaction & recovery modules under sanitizers.
- **Module Relationships:** Audits `src/transaction/` and `src/recovery/`.

### Files Expected to Change
- `include/tinydb/transaction/*.hpp`, `src/transaction/*.cpp`, `include/tinydb/recovery/*.hpp`, `src/recovery/*.cpp` (audit fixes)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit `std::filesystem` `error_code` usage, integer conversions, and `BeforeImage`/WAL buffer ownership.
2. Run Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format --dry-run --Werror`.

### Testing Instructions
- Full build matrix + `ctest --output-on-failure`.

### Acceptance Criteria
- Zero unhandled exceptions, zero sanitizer errors, zero compiler warnings.

### Definition of Done
Audit complete and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-236: audit transaction and recovery modules under ASan, UBSan, and -Werror`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-237` (Long-running transaction / multi-wave WAL stress test).
````

---

## DAY-237 — Add Multi-Wave Commit/Rollback/Recovery Endurance Stress Test

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-237
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Add Multi-Wave Commit/Rollback/Recovery Endurance Stress Test
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-236 complete.

### Objective
Write `tests/recovery/wal_endurance_stress_test.cpp` running 50+ sequential randomized transactions (random mix of `COMMIT`, `ROLLBACK`, `INSERT`, `UPDATE`, `DELETE` against an in-memory oracle `std::map<int64_t, Row>`) punctuated by 4 real `fork()/_Exit()` crashes at different points (`kAfterCommitWalFlush`, `kDuringBufferFlush`, uncommitted), verifying exact oracle convergence across every restart!

### Concepts & Daily Learning
- **Concept:** Stateful endurance testing combining randomized SQL + transactions + periodic process crashes!
- **Why Database Engines Need It:** Tests that `next_lsn_`, `next_txn_id_`, WAL append offsets, `BufferPoolManager` dirty tracking, and `BPlusTree`s stay healthy across *multiple* crash-and-recover lifecycles in sequence (Crash #1 -> Recover -> 15 txns -> Crash #2 -> Recover -> 15 txns)!
- **How It Works:** Seeded loop runs waves inside `fork()`ed children; parent tracks exact committed oracle across crashes (`oracle` updated only for committed txns!), reopens DB after each crash, verifies full `SELECT` == `oracle` + `BPlusTreeValidator`!
- **Design Trade-offs:** Multi-crash chained children catch bugs where Recovery #1 leaves a subtle mismatch that only fails on Recovery #2!
- **Common Mistakes:** Only crashing once on a fresh database instead of continuing transactional work and crashing *again* on the recovered database!
- **Understanding Checkpoint:** Why is running `Crash #1 -> Recover -> Continue Commits -> Crash #2 -> Recover` strictly stronger than testing a single crash?

### Component Context
- **Why It Exists:** High-confidence endurance proof for Phase 08.
- **Module Relationships:** Exercises entire Phase 01–08 stack continuously across crashes.

### Files Expected to Change
- `tests/recovery/wal_endurance_stress_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `wal_endurance_stress_test.cpp` with chained multi-crash waves verified against oracle and `BPlusTreeValidator`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-crash chained waves converge to exact committed oracle state with zero validator or sanitizer errors.

### Definition of Done
Endurance stress suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-237: add multi-crash chained transaction/WAL endurance test`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-238` (Refactor & polish transaction/recovery helpers).
````

---

## DAY-238 — Refactor and Polish Transaction & Recovery Code Clarity and Docstrings

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-238
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Refactor and Polish Transaction & Recovery Code Clarity and Docstrings
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-237 complete.

### Objective
Review `src/transaction/` and `src/recovery/` for function size, early returns, shared WAL serialization helpers, accurate docstrings explaining *why* (No-Steal ordering, LSN checks, torn-tail truncation), and `clang-format` compliance, re-running all crash tests afterward.

### Concepts & Daily Learning
- **Concept:** Systems code maintainability and interview-clarity review.
- **Why Database Engines Need It:** `TransactionManager::Commit`, `Rollback`, and `RecoveryManager::Recover` are prime interview discussion topics; crisp single-purpose helpers make every step immediately clear when reading the code!
- **How It Works:** Extract clean helpers where functions exceed ~50 lines, ensure zero duplicate serialization/checksum code, add concise docstrings, format with `clang-format`, and re-verify entire test suite.
- **Design Trade-offs:** Pure readability cleanup protected by the full crash test matrix.
- **Common Mistakes:** Accidentally reordering `LogManager::Flush()` vs `ReleaseUncommittedHold()` during refactoring!
- **Understanding Checkpoint:** Review `TransactionManager::Commit()` line-by-line and confirm the exact point of no return ("commit point").

### Component Context
- **Why It Exists:** Keeps transaction/WAL code clean and readable.
- **Module Relationships:** Refactors `src/transaction/` and `src/recovery/`.

### Files Expected to Change
- `include/tinydb/transaction/*.hpp`, `src/transaction/*.cpp`, `include/tinydb/recovery/*.hpp`, `src/recovery/*.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Refactor and polish `transaction/` and `recovery/`, format with `clang-format`.
2. Run full Debug + ASan/UBSan + Release test matrix.

### Testing Instructions
- Full `ctest --output-on-failure` across Debug, ASan+UBSan, Release + formatting check.

### Acceptance Criteria
- All Phase 00–08 tests pass across all configs with clean formatting and docstrings.

### Definition of Done
Refactored, formatted, and fully re-tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-238: refactor transaction and recovery helpers for clarity`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-239` (Finalize `docs/transaction-design.md`).
````

---

## DAY-239 — Finalize `docs/transaction-design.md` with Exact WAL Layout & Crash Matrix

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-239
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Finalize `docs/transaction-design.md` with Exact WAL Layout & Crash Matrix
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-238 complete.

### Objective
Audit and update `docs/transaction-design.md` against the finalized implementation (exact byte layouts of WAL header and all `LogRecordType`s, `Sequence` diagram of `BEGIN` -> `COMMIT` -> `Crash` -> `Recover`, No-Steal buffer-pool interaction, and comparison to full ARIES v2.0).

### Concepts & Daily Learning
- **Concept:** Authoritative durability specification and interview comparison (`No-Steal Redo-Only` vs `Steal/No-Force ARIES`).
- **Why Database Engines Need It:** Clearly documenting both *what* TinyDB v1 implements and *why* full ARIES adds CLRs + Analysis/Redo/Undo for concurrent stealing transactions ensures deep mastery of database recovery!
- **How It Works:** Cross-check every constant in `log_record.hpp` and every crash test scenario (`DAY-227`–`DAY-232`) against `docs/transaction-design.md`.
- **Design Trade-offs:** Accurate documentation serves as both onboarding guide and interview prep sheet.
- **Common Mistakes:** Leaving stale draft byte offsets from `DAY-205` if `kIndexRoot` (`DAY-219`) refined record layouts!
- **Understanding Checkpoint:** Explain how TinyDB's No-Steal Redo-Only design differs from PostgreSQL/ARIES on all four axes (`Steal/No-Steal`, `Force/No-Force`, `Concurrency`, `Undo`).

### Component Context
- **Why It Exists:** Completes technical documentation for Phase 08.
- **Module Relationships:** Documents `src/transaction/` and `src/recovery/`.

### Files Expected to Change
- `docs/transaction-design.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Sync `docs/transaction-design.md` with final code and include a Mermaid sequence diagram + crash recovery matrix table.
2. Verify build and tests stay green.

### Testing Instructions
- Run `ctest --test-dir build --output-on-failure`.

### Acceptance Criteria
- Every WAL offset, record type, and crash outcome in `docs/transaction-design.md` matches actual code and tests.

### Definition of Done
`docs/transaction-design.md` finalized, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-239: finalize transaction and WAL design documentation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-240` (Phase 08 sign-off).
````

---

## DAY-240 — Complete Phase 08 Acceptance Verification and Sign Off

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-240
- **Phase:** Phase 08 — Transactions and WAL
- **Session Title:** Complete Phase 08 Acceptance Verification and Sign Off
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-205 through DAY-239 complete.

### Objective
Execute the entire Phase 00–08 unit, integration, stress, and controlled crash recovery suites across Debug (`-Werror`), ASan+UBSan (`-Werror`), and Release (`-Werror`), verify `clang-format`, update `PROGRESS.md` (`186.0 / 220.0` hours completed), and sign off Phase 08!

### Concepts & Daily Learning
- **Concept:** Feature-complete database engine milestone gate prior to Phase 09 comprehensive security & adversarial QA!
- **Why Database Engines Need It:** All 8 core objectives of TinyDB v1 are now implemented end-to-end; verifying clean full-matrix runs sets up Phase 09's intensive adversarial hardening, fuzzing, differential testing vs SQLite3, and audit!
- **How It Works:** Run every CTest target across all 3 build configs, confirm zero warnings/leaks/failures, record exact results honestly in `PROGRESS.md`, and point `NEXT_SESSION.md` to `DAY-241`.
- **Design Trade-offs:** Never skip the Release (`-O3`) crash test check!
- **Common Mistakes:** Overlooking a crash test failure in Release `-O3` caused by uninitialized stack memory masked in Debug!
- **Understanding Checkpoint:** Verify from actual test output that ALL crash scenarios (before commit, after rollback, after commit pre-flush, mid-flush, during recovery) pass under BOTH ASan+UBSan AND Release `-O3`.

### Component Context
- **Why It Exists:** Completes Phase 08 (`28.0` hours).
- **Module Relationships:** Hands off the complete feature-implemented TinyDB v1 engine to Phase 09 (`Security and Testing`).

### Files Expected to Change
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Run full Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format --dry-run --Werror`.
2. Update `PROGRESS.md` (`186.0 / 220.0` hours, Phase 08 complete, Phase 09 active) and `NEXT_SESSION.md` -> `DAY-241`.

### Testing Instructions
- Full Debug, ASan+UBSan, Release `ctest --output-on-failure` + formatting check.

### Acceptance Criteria
- 100% of Phase 00–08 tests pass across all 3 build configs with zero warnings or sanitizer errors.

### Definition of Done
Phase 08 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-240: verify full test matrix and sign off Phase 08 Transactions & WAL`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 09 (`DAY-241` — Security threat model & attack surface audit).
````
