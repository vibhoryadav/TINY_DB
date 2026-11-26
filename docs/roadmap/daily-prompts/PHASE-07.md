# Phase 07 — Query Engine and CLI (`DAY-179` – `DAY-204`)

**Phase Budget:** 20.0 hours (24 × 45 min + 2 × 60 min = 26 sessions)
**Goal:** Build TinyDB's pull-based Volcano query execution engine (`SeqScan`, `IndexScan`, `Filter`, `Projection`, `Insert`, `Update`, `Delete`), rule-based index selection planner, type validation, tabular output formatter, and interactive `tinydb` CLI with dot-commands (`.exit`, `.quit`, `.help`, `.tables`, `.schema`).

---

## DAY-179 — Design Pull-Based Volcano Executor Interface and `ExecutionContext`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-179
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Design Pull-Based Volcano Executor Interface and `ExecutionContext`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 06 complete (`DAY-178`).

### Objective
Define `AbstractExecutor` (`Init()`, `Next()`, `OutputSchema()`), `ExecutionItem` (`Tuple` + optional `RowID`), and `ExecutionContext` (wiring `CatalogManager&`, `BufferPoolManager&`, and optional Phase 08 `Transaction*` / `TransactionContext*`!) in `include/tinydb/execution/executor.hpp` and `include/tinydb/execution/execution_context.hpp`.

### Concepts & Daily Learning
- **Concept:** Pull-based Volcano iterator model (`Open`/`Init` -> repeated `Next` -> close) and pipelined streaming.
- **Why Database Engines Need It:** Pulling one tuple at a time lets `Projection -> Filter -> SeqScan` execute queries over 1,000,000 rows in `O(1)` working memory instead of materializing intermediate tables!
- **How It Works:** `AbstractExecutor::Init()` resets cursor state and initializes child executors; `Next()` returns `Result<std::optional<ExecutionItem>>` (`std::nullopt` signals EOF cleanly, non-ok `Status` propagates errors cleanly!). `ExecutionContext` carries engine dependencies centrally.
- **Design Trade-offs:** Returning `Result<std::optional<ExecutionItem>>` instead of `(Tuple*, RID*, bool return)` keeps error reporting explicit and memory ownership crystal clear!
- **Common Mistakes:** Returning `StatusCode::kNotFound` for normal end-of-scan instead of `std::nullopt` (confusing EOF with missing-table errors!); forgetting to call `child_->Init()` inside `Init()`.
- **Understanding Checkpoint:** Contrast pull-based Volcano execution vs push-based/materialized execution when executing `SELECT name FROM users WHERE id > 10`.

### Component Context
- **Why It Exists:** Uniform interface shared by every query operator in TinyDB.
- **Module Relationships:** Implemented by `DAY-182`–`DAY-188`; driven by `ExecutionEngine` (`DAY-191`).

### Files Expected to Change
- `include/tinydb/execution/executor.hpp` (new)
- `include/tinydb/execution/execution_context.hpp` (new)
- `tests/unit/execution/executor_interface_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `ExecutionContext`, `ExecutionItem`, and `AbstractExecutor`.
2. Write a minimal mock-executor test confirming `Init -> Next -> EOF (nullopt)` pipeline composition works cleanly.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Interface compiles cleanly with `-Werror` and mock pipeline streams tuples with zero leaks.

### Definition of Done
Executor interfaces created and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-179: define Volcano AbstractExecutor and ExecutionContext interfaces`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-180` (`QueryResult` & `ResultSet` structures).
````

---

## DAY-180 — Implement `QueryResult` and `ResultSet` Structures

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-180
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `QueryResult` and `ResultSet` Structures
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-179 complete.

### Objective
Implement `QueryResult` (`include/tinydb/execution/query_result.hpp`, `src/execution/query_result.cpp`) encapsulating DDL messages (`"Table 'users' created."`), DML affected row counts (`inserted/updated/deleted`), `SELECT` result schemas + rows, and `Status` error details cleanly for both programmatic callers and CLI display.

### Concepts & Daily Learning
- **Concept:** Uniform engine result envelope (`QueryKind`: `kDdl`, `kRows`, `kMutationCount`) bridging `ExecutionEngine` to tests and CLI.
- **Why Database Engines Need It:** `CREATE TABLE`, `INSERT`, and `SELECT` produce fundamentally different outputs; returning a single structured `QueryResult` lets tests assert exact values/types while the CLI formats clean tables or status lines!
- **How It Works:** `QueryResult` holds `Status status`, `QueryResultKind kind`, optional `Schema schema`, `std::vector<Tuple> rows`, `std::size_t affected_rows`, and `std::string message`.
- **Design Trade-offs:** Returning `QueryResult` from `ExecutionEngine::ExecuteSql` keeps programmatic SQL assertions crisp and decouples engine logic from `std::cout` formatting!
- **Common Mistakes:** Printing directly to `std::cout` inside executors/engine instead of returning structured `QueryResult`s!
- **Understanding Checkpoint:** Why should `ExecutionEngine` NEVER write directly to `std::cout`?

### Component Context
- **Why It Exists:** Clean structured return contract for `ExecutionEngine`.
- **Module Relationships:** Produced by `ExecutionEngine` (`DAY-191`), rendered by `TableFormatter` (`DAY-196`).

### Files Expected to Change
- `include/tinydb/execution/query_result.hpp` (new)
- `src/execution/query_result.cpp` (new)
- `tests/unit/execution/query_result_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `QueryResult` with factories (`DdlOk`, `MutationOk`, `RowsOk`, `Error`).
2. Add unit tests verifying all result kinds and accessors.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `QueryResult` constructs cleanly, stores rows/messages accurately, and passes unit tests.

### Definition of Done
`QueryResult` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-180: implement QueryResult and ResultSet structures`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-181` (Expression evaluation engine).
````

---

## DAY-181 — Implement SQL Expression Evaluation and Type-Checking Engine

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-181
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement SQL Expression Evaluation and Type-Checking Engine
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-052 (`Value::Compare`), DAY-180 complete.

### Objective
Implement `ExprEvaluator` (`include/tinydb/execution/expr_evaluator.hpp`, `src/execution/expr_evaluator.cpp`) evaluating `AST Expression` nodes against `(const Schema&, const Tuple&)` (resolving `ColumnRefExpr` case-insensitively, validating type compatibility for `=`, `!=`, `<`, `<=`, `>`, `>=`, enforcing `BOOL` results for `AND`, and returning crisp errors on unknown columns or mismatched types like `age = 'abc'`).

### Concepts & Daily Learning
- **Concept:** AST interpretation (`Evaluate(expr, schema, tuple)`), semantic binding of column names, and strict runtime type rules.
- **Why Database Engines Need It:** `Parser` checks grammar (`WHERE age = 'abc'` is grammatically valid SQL!), while semantic evaluation knows `age` is `INT` and `'abc'` is `VARCHAR`—rejecting incompatible comparisons cleanly instead of coercing `'abc'` to `0` like loose SQL dialects!
- **How It Works:** Recursively evaluate `Expression`:
  - `LiteralExpr` -> return its `Value`.
  - `ColumnRefExpr` -> lookup column index in `schema`, return `tuple.GetValue(idx)` (`kNotFound` / `kInvalidArgument` if missing!).
  - `BinaryOpExpr`: if `kAnd`, evaluate `lhs` (require `kBool`), short-circuit or evaluate `rhs` (Wait: evaluate BOTH or type-check `rhs`? Actually in SQL `FALSE AND (1 = 'bad')` shouldn't skip type-checking `'bad'`—let's provide a separate `ValidatePredicateSchema(expr, schema)` pass before execution AND short-circuit cleanly at runtime!). For `=`, `!=`, `<`, `<=`, `>`, `>=`: call `Value::Compare(lhs_val, rhs_val)` and return `Value::MakeBool(...)`!
- **Design Trade-offs:** Separating `ValidateExprTypes(expr, schema)` from `Evaluate(expr, schema, tuple)` catches `WHERE missing_col = 1` or `WHERE age = 'hello'` even on an **empty table** with 0 rows!!
- **Common Mistakes:** Only checking column names/types per tuple—causing `SELECT * FROM empty_table WHERE nonexistent = 1;` to silently return `0 rows` instead of an error!!
- **Understanding Checkpoint:** Why MUST semantic validation of `WHERE nonexistent_col = 1` run in `Init()` / `Planner` *before* pulling tuples from an empty table?

### Component Context
- **Why It Exists:** Evaluates predicates and values cleanly across `Filter`, `Insert`, `Update`, and `Planner`.
- **Module Relationships:** Used by `FilterExecutor`, `InsertExecutor`, `UpdateExecutor`, and `Planner`.

### Files Expected to Change
- `include/tinydb/execution/expr_evaluator.hpp` (new)
- `src/execution/expr_evaluator.cpp` (new)
- `tests/unit/execution/expr_evaluator_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ExprEvaluator::ValidatePredicate(expr, schema)` and `Evaluate(expr, schema, tuple)` + `EvaluatePredicate(expr, schema, tuple) -> Result<bool>`.
2. Test: valid comparisons (`INT`/`BIGINT` cross-width, `VARCHAR`, `BOOL`), `AND` logic, unknown column error, `INT vs VARCHAR` mismatch error, and non-boolean `WHERE` predicate error (`WHERE 42`!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Semantic validation catches type/column errors even with zero tuples; runtime evaluation handles all v1 comparisons accurately.

### Definition of Done
Expression evaluator implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-181: implement SQL ExprEvaluator and static schema type validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-182` (`SequentialScanExecutor`).
````

---

## DAY-182 — Implement `SequentialScanExecutor`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-182
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `SequentialScanExecutor`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-181 complete.

### Objective
Implement `SequentialScanExecutor` (`include/tinydb/execution/seq_scan_executor.hpp`, `src/execution/seq_scan_executor.cpp`) wrapping `TableHeap::ScanIterator`, resetting cleanly on `Init()` (so re-scanning works!) and yielding `(Tuple, RowID)` pairs lazily on each `Next()` without pinning more than 1 page at a time.

### Concepts & Daily Learning
- **Concept:** Table scan operator wrapping storage iterators in the Volcano interface.
- **Why Database Engines Need It:** Base leaf operator for queries without usable indexes (`SELECT * FROM users`, `DELETE`, `UPDATE`), and fallback when `IndexScan` isn't applicable.
- **How It Works:** Hold `TableMetadata` + `std::optional<TableHeap::ScanIterator>`. `Init()` resets `iterator_ = table_heap->Begin()`. `Next()` pulls `iterator_->Next()` returning `ExecutionItem{tuple, rid}` or `std::nullopt`.
- **Design Trade-offs:** Returning `RowID` alongside `Tuple` lets parent `DeleteExecutor` / `UpdateExecutor` mutate the exact physical slot without re-searching!
- **Common Mistakes:** Holding the scan page guard open after `Next()` returns `std::nullopt` (exhausted scan must release its final page guard immediately!).
- **Understanding Checkpoint:** Why must `SequentialScanExecutor::Init()` recreate `TableHeap::ScanIterator` cleanly instead of only initializing in the constructor?

### Component Context
- **Why It Exists:** Fundamental full-table access path.
- **Module Relationships:** Leaf operator under `FilterExecutor`, `ProjectionExecutor`, `UpdateExecutor`, `DeleteExecutor`.

### Files Expected to Change
- `include/tinydb/execution/seq_scan_executor.hpp` (new)
- `src/execution/seq_scan_executor.cpp` (new)
- `tests/unit/execution/seq_scan_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `SequentialScanExecutor`.
2. Add tests scanning empty table, multi-page table with deleted gaps, and calling `Init()` twice to re-scan cleanly with `ActivePinCount() == 0` afterward.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Yields exact live `(Tuple, RowID)` sequence, supports `Init()` reset, and leaks 0 buffer pins.

### Definition of Done
`SequentialScanExecutor` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-182: implement SequentialScanExecutor over TableHeap`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-183` (`FilterExecutor`).
````

---

## DAY-183 — Implement `FilterExecutor`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-183
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `FilterExecutor`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-182 complete.

### Objective
Implement `FilterExecutor` (`include/tinydb/execution/filter_executor.hpp`, `src/execution/filter_executor.cpp`) taking a child `unique_ptr<AbstractExecutor>` and `const Expression*` predicate, validating predicate types against `child_->OutputSchema()` inside `Init()`, and pulling tuples from `child_->Next()` until `EvaluatePredicate` returns `true` or child hits EOF!

### Concepts & Daily Learning
- **Concept:** Pull-based pipelined selection (`σ_p(R)` in relational algebra).
- **Why Database Engines Need It:** Evaluates `WHERE` predicates lazily row-by-row while preserving the original `RowID` so `UPDATE` and `DELETE` work seamlessly on filtered rows!
- **How It Works:** `Init()`: call `child_->Init()`, then `ExprEvaluator::ValidatePredicate(*predicate_, child_->OutputSchema())`! `Next()`: loop pulling `child_->Next()`; if `std::nullopt` return `std::nullopt`; evaluate predicate; if `true`, return the exact `ExecutionItem` (preserving `RowID`!).
- **Design Trade-offs:** Validating the predicate in `Init()` catches semantic `WHERE` errors even when `child_` yields 0 rows!
- **Common Mistakes:** Stripping `RowID` when forwarding the surviving `ExecutionItem` from `child_` (breaking `UPDATE` / `DELETE`!).
- **Understanding Checkpoint:** Why MUST `FilterExecutor::Next()` preserve `item.rid` when passing a matching tuple up to its parent executor?

### Component Context
- **Why It Exists:** Executes SQL `WHERE` filtering for `SELECT`, `UPDATE`, and `DELETE`.
- **Module Relationships:** Wraps `SequentialScanExecutor` or `IndexScanExecutor`; feeds `ProjectionExecutor`, `UpdateExecutor`, or `DeleteExecutor`.

### Files Expected to Change
- `include/tinydb/execution/filter_executor.hpp` (new)
- `src/execution/filter_executor.cpp` (new)
- `tests/unit/execution/filter_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `FilterExecutor`.
2. Add tests verifying single and compound `AND` predicates, empty match, preserving `RowID`, and invalid column/type rejection on an empty table!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Filters rows lazily, preserves `RowID`, and catches invalid `WHERE` types on empty tables.

### Definition of Done
`FilterExecutor` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-183: implement FilterExecutor with Init-time predicate validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-184` (`ProjectionExecutor`).
````

---

## DAY-184 — Implement `ProjectionExecutor`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-184
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `ProjectionExecutor`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-058 (`Tuple::Project`), DAY-183 complete.

### Objective
Implement `ProjectionExecutor` (`include/tinydb/execution/projection_executor.hpp`, `src/execution/projection_executor.cpp`) mapping requested output column names to input column indices and output `Schema` (handling `SELECT b, a` reordered columns and duplicate projected columns cleanly!), pulling child tuples, and projecting via `Tuple::Project`.

### Concepts & Daily Learning
- **Concept:** Relational projection (`π_{c1, c2}(R)`), output schema derivation, and handling reordered or repeated columns (`SELECT name, id, name`).
- **Why Database Engines Need It:** Users often select a subset or reordered list of columns (`SELECT email, id FROM users`); `ProjectionExecutor` computes the output `Schema` once and transforms every pulled tuple to match it!
- **How It Works:** Given `child_schema` and requested `column_names`: resolve each name case-insensitively (`kNotFound` / `kInvalidArgument` if missing), construct output `Schema` (Note: remember `Schema::Create` rejects duplicate column names in a *table*! Think carefully: what if someone runs `SELECT id, id FROM users`? Either build a lightweight `ResultSchema` allowing duplicate output column labels OR use `Schema::Create` + clear policy! Supporting arbitrary output `Column` list cleanly in `QueryResult` / `ProjectionExecutor` handles even `SELECT id, id` without crashing!).
- **Design Trade-offs:** Resolving column indices once at planning/construction time keeps per-tuple projection `O(out_cols)`.
- **Common Mistakes:** Assuming output column order always matches table schema order!
- **Understanding Checkpoint:** What breaks if `ProjectionExecutor` assumes requested columns are always in ascending table column index order (`0, 1, 2`)?

### Component Context
- **Why It Exists:** Implements `SELECT col1, col2` column selection and reordering.
- **Module Relationships:** Sits at top of `SELECT` pipelines above `FilterExecutor` or Scan executors.

### Files Expected to Change
- `include/tinydb/execution/projection_executor.hpp` (new)
- `src/execution/projection_executor.cpp` (new)
- `tests/unit/execution/projection_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ProjectionExecutor`.
2. Write unit tests verifying subset projection, reordered projection (`SELECT name, id`), unknown column rejection, and clean pass-through on `Filter -> SeqScan`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Projected output `Schema` and tuple column values match requested column order accurately; unknown columns fail cleanly.

### Definition of Done
`ProjectionExecutor` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-184: implement ProjectionExecutor with column validation and reordering`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-185` (`InsertExecutor` + index updates).
````

---

## DAY-185 — Implement `InsertExecutor` (Validation, `TableHeap` Insert, and Multi-Index Maintenance)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-185
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `InsertExecutor` (Validation, `TableHeap` Insert, and Multi-Index Maintenance)
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-184 complete.

### Objective
Implement `InsertExecutor` (`include/tinydb/execution/insert_executor.hpp`, `src/execution/insert_executor.cpp`) mapping optional column lists `(col2, col1)` to table schema order, validating ALL rows in `INSERT INTO ... VALUES (...), (...)` BEFORE mutating storage (all-or-nothing validation!), inserting into `TableHeap`, updating every attached `BPlusTree` index (and recording rollback undo hooks cleanly for Phase 08!).

### Concepts & Daily Learning
- **Concept:** Atomic statement-level pre-validation, column-list permutation mapping, and multi-index synchronization.
- **Why Database Engines Need It:** In `INSERT INTO t VALUES (1, 'ok'), (2, 'too_long_varchar')`, validating row #2 BEFORE inserting row #1 ensures a failed multi-row `INSERT` doesn't leave half the rows inserted! And forgetting to insert `(key, rid)` into every table index leaves queries returning stale/missing rows!
- **How It Works:** In `Init()` (or constructor): verify target column names (no duplicates, all columns covered if v1 has no `NULL` defaults—if column omitted when v1 forbids `NULL`, reject clearly with `"Missing value for column 'X' (NULL not supported in v1)"`!), evaluate & cast literals (`TryCastTo`) for ALL rows into `std::vector<Tuple>` FIRST! Then insert each `Tuple` into `TableHeap`, get `rid`, and insert `(ExtractIndexKey(tuple, idx.col_index), rid)` into every `BPlusTree` attached to the table!
- **Design Trade-offs:** Pre-validating all rows in `VALUES` before writing row 0 prevents partial-statement corruption even before Phase 08 transactions arrive!
- **Common Mistakes:** Ignoring `INSERT INTO t (b, a) VALUES ('bob', 1)` column ordering (`b` at slot 1, `a` at slot 0); forgetting to reject duplicate column names like `INSERT INTO t (a, a) VALUES (1, 2)`; failing to update ALL indexes when a table has 2+ indexes!
- **Understanding Checkpoint:** Why must `InsertExecutor` reject `INSERT INTO users (id) VALUES (1)` when `users` has `(id INT, name VARCHAR(20))` in TinyDB v1?

### Component Context
- **Why It Exists:** Executes SQL `INSERT INTO` while keeping `TableHeap` and all `BPlusTree` indexes synchronized.
- **Module Relationships:** Calls `TableHeap::InsertTuple` and `BPlusTree::Insert`.

### Files Expected to Change
- `include/tinydb/execution/insert_executor.hpp` (new)
- `src/execution/insert_executor.cpp` (new)
- `tests/unit/execution/insert_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `InsertExecutor` with pre-validation, reordered column mapping, and multi-index updates.
2. Add tests verifying: single & multi-row inserts, reordered `(name, id)`, missing column rejection, `VARCHAR` overflow on row #2 leaves row #1 un-inserted, and all table indexes match `TableHeap` via `BPlusTreeValidator`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Valid inserts populate `TableHeap` and all B+ Trees; invalid row #2 leaves row #1 untouched.

### Definition of Done
`InsertExecutor` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-185: implement InsertExecutor with pre-validation and multi-index sync`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-186` (`DeleteExecutor`).
````

---

## DAY-186 — Implement `DeleteExecutor` (Safe Target Collection and Multi-Index Removal)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-186
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `DeleteExecutor` (Safe Target Collection and Multi-Index Removal)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-185 complete.

### Objective
Implement `DeleteExecutor` (`include/tinydb/execution/delete_executor.hpp`, `src/execution/delete_executor.cpp`) pulling matching `(Tuple, RowID)` targets from its child executor (`SeqScan`/`Filter`/`IndexScan`), removing `(key, rid)` from every attached `BPlusTree` index, and deleting `rid` from `TableHeap` without iterator invalidation ("Halloween / scan-mutation hazard")!

### Concepts & Daily Learning
- **Concept:** Preventing scan-during-mutation hazards when deleting via an `IndexScan` or `TableIterator`!
- **Why Database Engines Need It:** Remember in `BPlusTree::Iterator` (`DAY-128`), mutating the B+ Tree (`Remove`) while an `IndexScan` iterator holds a `ReadPageGuard` on a leaf could trigger a leaf merge or page pin deadlock/invalidation! Materializing target `(RowID, Tuple)`s (or `RowID`s + index keys) from `child_` FIRST—letting `child_` exhaust and drop all its `ReadPageGuard`s—and THEN deleting cleanly guarantees zero iterator-mutation bugs!
- **How It Works:** In `Init()` / `Execute`: materialize `targets` from `child_->Next()`, drop/close child scan so `ActivePinCount() == 0`!, then for each `(tuple, rid)` in `targets`: remove `(key, rid)` from each index, then call `table_heap->DeleteTuple(rid)`. Track and return deleted row count!
- **Design Trade-offs:** Collecting `(RowID, index_keys)` before mutating eliminates BOTH `IndexScan` leaf-merge invalidation AND re-visiting migrated tuples, at modest memory cost proportional to deleted rows!
- **Common Mistakes:** Calling `BPlusTree::Remove` while `IndexScanExecutor` still pins the very same leaf page (pin-count deadlock or corrupt leaf iterator!).
- **Understanding Checkpoint:** Why would deleting directly inside a loop pulling from `IndexScanExecutor` without finishing the child scan first risk corrupting or deadlocking `BPlusTree`?

### Component Context
- **Why It Exists:** Executes `DELETE FROM ... [WHERE ...]` safely across heap and indexes.
- **Module Relationships:** Coordinates child scan, `TableHeap::DeleteTuple`, and `BPlusTree::Remove`.

### Files Expected to Change
- `include/tinydb/execution/delete_executor.hpp` (new)
- `src/execution/delete_executor.cpp` (new)
- `tests/unit/execution/delete_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DeleteExecutor` with child-drain-before-mutate safety.
2. Add tests deleting via BOTH `SeqScan + Filter` AND `IndexScan` (when `WHERE indexed_col = ...`!), verifying deleted count, `TableHeap` removal, and `BPlusTreeValidator::Validate(&tree, &table, col)` passing afterward!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Deleting via `SeqScan` or `IndexScan` leaves 0 stale `RowID`s in any index and `ActivePinCount() == 0`.

### Definition of Done
`DeleteExecutor` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-186: implement DeleteExecutor with scan-drain safety and index cleanup`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-187` (`UpdateExecutor`).
````

---

## DAY-187 — Implement `UpdateExecutor` (Type Validation, Halloween Protection, and Index Sync)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-187
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `UpdateExecutor` (Type Validation, Halloween Protection, and Index Sync)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-186 complete.

### Objective
Implement `UpdateExecutor` (`include/tinydb/execution/update_executor.hpp`, `src/execution/update_executor.cpp`) validating `SET` column names and value types/lengths in `Init()`, materializing target rows first (preventing the classic **Halloween Problem** where updating an indexed column moves a row forward into an unread portion of the scan and updates it twice!), updating `TableHeap`, and updating attached B+ Tree indexes whenever EITHER the indexed key changed OR `new_rid != old_rid`!

### Concepts & Daily Learning
- **Concept:** The Halloween Problem in databases and keeping indexes consistent across in-place vs relocated (`new_rid != old_rid`) updates.
- **Why Database Engines Need It:** Classic bug: `UPDATE emp SET salary = salary + 1000` scanning an index on `salary` sees the updated row again later in the index scan and adds `1000` forever! Materializing matching `(Tuple, RowID)` targets before mutating solves the Halloween Problem cleanly! Also, if a `VARCHAR` update forces `TableHeap` cross-page migration (`new_rid != old_rid`), EVEN indexes on untouched columns MUST update `old_rid -> new_rid`!
- **How It Works:** In `Init()`: validate target columns (reject duplicates like `SET a = 1, a = 2` and invalid types/lengths up front!). Materialize matching targets from `child_`. Pre-build & validate ALL `new_tuple`s before mutating row #1! Then for each target: call `new_rid = table_heap->UpdateTuple(old_rid, new_tuple)`. For each attached index: if `new_rid != old_rid || new_key != old_key`, `tree->Remove(old_key, old_rid)` and `tree->Insert(new_key, new_rid)`.
- **Design Trade-offs:** Checking `new_rid != old_rid || new_key != old_key` skips unnecessary B+ Tree work when non-indexed columns update in-place while handling migrations 100% accurately!
- **Common Mistakes:** Only updating an index if the indexed column itself changed—forgetting that updating a `VARCHAR` column can move the tuple to a new page (`new_rid != old_rid`)!
- **Understanding Checkpoint:** Explain why updating a *non-indexed* `VARCHAR` column can still require updating a B+ Tree index on `id`, and what the **Halloween Problem** is.

### Component Context
- **Why It Exists:** Safe SQL `UPDATE` execution across heap and B+ Tree indexes.
- **Module Relationships:** Coordinates `TableHeap::UpdateTuple` and `BPlusTree::Remove`/`Insert`.

### Files Expected to Change
- `include/tinydb/execution/update_executor.hpp` (new)
- `src/execution/update_executor.cpp` (new)
- `tests/unit/execution/update_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `UpdateExecutor` with `Init` pre-validation, Halloween-safe drain, and index sync.
2. Add tests covering: indexed column update (`IndexScan`), `VARCHAR` growth causing `new_rid != old_rid` with an untouched `INT` index verifying index still resolves `new_rid`!, invalid value error leaving all rows unchanged, and `BPlusTreeValidator::Validate(&tree, &table, col)`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both key changes and cross-page `RowID` migrations keep all B+ Trees 100% synchronized; invalid `SET` values fail atomically before mutating row 0.

### Definition of Done
`UpdateExecutor` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-187: implement UpdateExecutor with Halloween protection and index sync`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-188` (`IndexScanExecutor`).
````

---

## DAY-188 — Implement `IndexScanExecutor` (Point and Range Lookups via `BPlusTree`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-188
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `IndexScanExecutor` (Point and Range Lookups via `BPlusTree`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-187 complete.

### Objective
Implement `IndexScanExecutor` (`include/tinydb/execution/index_scan_executor.hpp`, `src/execution/index_scan_executor.cpp`) wrapping `BPlusTree` range bounds (`IndexRange { min_key, min_inclusive, max_key, max_inclusive }`), iterating `(key, RowID)` entries via `BPlusTree::Iterator` (or point `GetValue`), and fetching full `Tuple`s from `TableHeap::GetTuple(rid)`!

### Concepts & Daily Learning
- **Concept:** Index-backed scan operator (`IndexScan` + RID table lookup) supporting `=`, `<`, `<=`, `>`, `>=`, and `[lo, hi]` intersection!
- **Why Database Engines Need It:** `SELECT * FROM users WHERE id = 42000` reads `~2–3` B+ Tree pages + `1` table heap page instead of scanning 500 heap pages sequentially!
- **How It Works:** `IndexScanPlan` passes `TableHeap&`, `BPlusTree&`, and `IndexRange`. `Init()` opens `BPlusTree::Iterator` at `min_key` (or `Begin()` if unbounded below). `Next()` pulls next `(key, rid)` within `max_key` bound, calls `table_heap_.GetTuple(rid)`, returns `ExecutionItem{tuple, rid}` (dropping iterator immediately on EOF!).
- **Design Trade-offs:** Notice if a `WHERE` clause has BOTH `id = 5` AND `name = 'alice'`, `IndexScanExecutor` finds all `id = 5` rows via B+ Tree, and a parent `FilterExecutor` verifies `name = 'alice'` (or even full `WHERE` predicate safely!).
- **Common Mistakes:** Returning a stale/out-of-range key when `min_inclusive == false` (`>` vs `>=`); leaving `BPlusTree::Iterator` alive pinning a leaf page after hitting `max_key`!
- **Understanding Checkpoint:** Why can `IndexScanExecutor` (`id > 10`) safely sit underneath a `FilterExecutor` (`id > 10 AND name = 'alice'`) when the planner uses the index for a multi-predicate `WHERE` clause?

### Component Context
- **Why It Exists:** High-speed indexed access path in Volcano pipelines.
- **Module Relationships:** Selected by `Planner` (`DAY-190`) instead of `SequentialScanExecutor`.

### Files Expected to Change
- `include/tinydb/execution/index_scan_executor.hpp` (new)
- `src/execution/index_scan_executor.cpp` (new)
- `tests/unit/execution/index_scan_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `IndexScanExecutor` handling `=`, `<`, `<=`, `>`, `>=`, and `[lo, hi]` ranges (`min_key == max_key && both inclusive` = point range!).
2. Add tests verifying exact equivalence against `SeqScan + Filter` over duplicate keys, empty ranges, exclusive bounds (`>`, `<`), and `INT32_MIN/MAX` + `INT64_MIN/MAX` limits with `ActivePinCount() == 0`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `IndexScanExecutor` returns identical multiset of rows as `SeqScan + Filter`, triggers fewer `DiskStats::page_reads`, and drops all pins on EOF.

### Definition of Done
`IndexScanExecutor` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-188: implement IndexScanExecutor for point and range B+ Tree lookups`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-189` (DDL execution & index backfill).
````

---

## DAY-189 — Implement DDL Execution (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX` + Backfill)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-189
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement DDL Execution (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX` + Backfill)
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-188 complete.

### Objective
Implement DDL execution handlers (`include/tinydb/execution/ddl_executor.hpp`, `src/execution/ddl_executor.cpp`) bridging `CreateTableStmt`, `DropTableStmt`, and `CreateIndexStmt` onto `CatalogManager` + `BPlusTree`—critically **backfilling** all existing rows from `TableHeap` into the newly created `BPlusTree` when `CREATE INDEX` runs on a populated table!

### Concepts & Daily Learning
- **Concept:** Online/offline index creation via full heap scan backfill and atomic rollback if backfill fails.
- **Why Database Engines Need It:** Users frequently run `CREATE TABLE ...; INSERT ...; CREATE INDEX ...;`—if `CREATE INDEX` created an empty B+ Tree without scanning existing heap rows, queries using the index would silently miss every previously inserted row!
- **How It Works:**
  - `ExecuteCreateTable`: convert `ColumnDef`s to `Column` + `Schema::Create`, call `CatalogManager::CreateTable`.
  - `ExecuteDropTable`: call `CatalogManager::DropTable` (cascading index + heap page reclamation).
  - `ExecuteCreateIndex`: verify table + column (`INT`/`BIGINT` only!), register index in `CatalogManager`, scan existing `TableHeap` rows and insert `(col_value, rid)` into the new `BPlusTree`! (If backfill fails mid-way, clean up and `DropIndex` cleanly!).
- **Design Trade-offs:** Scanning `TableHeap` once during `CREATE INDEX` keeps existing data and index 100% consistent.
- **Common Mistakes:** Registering `IndexMetadata` in `CatalogManager` and returning success without scanning existing rows in `TableHeap`!
- **Understanding Checkpoint:** What happens to existing rows if `CREATE INDEX idx ON users (id)` runs AFTER 500 rows were inserted, and how do we guarantee rollback if backfill hits an error?

### Component Context
- **Why It Exists:** Completes SQL DDL execution (`CREATE TABLE`, `DROP TABLE`, `CREATE INDEX`).
- **Module Relationships:** Invoked by `ExecutionEngine` (`DAY-191`).

### Files Expected to Change
- `include/tinydb/execution/ddl_executor.hpp` (new)
- `src/execution/ddl_executor.cpp` (new)
- `tests/unit/execution/ddl_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement DDL execution functions in `ddl_executor.cpp`.
2. Add tests verifying: `CREATE TABLE`, duplicate table error, `DROP TABLE`, `CREATE INDEX` on populated table (300 rows) populates B+ Tree verified by `BPlusTreeValidator(&tree, &table, col)`, and `CREATE INDEX` on `VARCHAR` column rejects cleanly with `kUnsupported`!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `CREATE INDEX` backfills 100% of existing rows cleanly; invalid DDL fails cleanly without half-created catalog entries.

### Definition of Done
DDL executors implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-189: implement DDL execution and CREATE INDEX backfill`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-190` (Rule-based query planner & index selection).
````

---

## DAY-190 — Implement Rule-Based `Planner` (Index Selection vs Sequential-Scan Fallback)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-190
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement Rule-Based `Planner` (Index Selection vs Sequential-Scan Fallback)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-189 complete.

### Objective
Implement `tinydb::Planner` (`include/tinydb/execution/planner.hpp`, `src/execution/planner.cpp`) assembling Volcano executor pipelines for `SelectStmt`, `UpdateStmt`, `DeleteStmt`, and `InsertStmt`—inspecting `WHERE` conjunctive (`AND`) predicates to extract `IndexRange` bounds when an `INT`/`BIGINT` column has a B+ Tree index (`=`, `<`, `<=`, `>`, `>=`—never `!=`!), selecting `IndexScanExecutor` (plus residual `FilterExecutor`!), and falling back to `SequentialScanExecutor` otherwise!

### Concepts & Daily Learning
- **Concept:** Rule-based predicate pushdown / access-path selection (`IndexScan` vs `SeqScan`), conjunctive normal extraction, and residual filtering.
- **Why Database Engines Need It:** Even without a cost-based optimizer (deferred to v2), picking an index for `WHERE id = 42` or `WHERE id >= 10 AND id < 50 AND active = TRUE` transforms `O(N)` table scans into `O(log N)` lookups!
- **How It Works:** Flatten top-level `AND` nodes in `WHERE`. For each comparison `col <op> int_literal` (handling swapped literal `<op> col` like `10 < id` -> `id > 10` too!!), check if `CatalogManager` has an index on `col` (`!=` cannot use single range scan so skip it!). Watch integer bounds: if column is `INT` (`int32_t`) and literal is `5000000000` (`> INT32_MAX`), handle carefully (e.g., `id = 5000000000` can match 0 `int32_t` rows, or `id < 5000000000` matches all `int32_t` rows—let's clamp/widen cleanly because `BPlusTree` keys are `int64_t`!). Keep full `FilterExecutor` above `IndexScanExecutor` whenever residual predicates exist (safe & exact!).
- **Design Trade-offs:** Always keeping residual `FilterExecutor` for multi-term `WHERE` guarantees exact SQL semantics while letting `IndexScanExecutor` prune disk I/O!
- **Common Mistakes:** Using `IndexScan` on `WHERE id != 5` (which isn't a single contiguous B+ Tree range!); mishandling `10 < id` vs `id < 10`; forgetting to disable `IndexScan` when `lo > hi` (contradiction like `id > 20 AND id < 10` -> empty range returns 0 rows cleanly!).
- **Understanding Checkpoint:** Why can `WHERE id >= 10 AND name = 'bob'` use an index on `id` combined with a `FilterExecutor`, whereas `WHERE id != 10` falls back to `SequentialScanExecutor`?

### Component Context
- **Why It Exists:** Translates parsed ASTs into optimized Volcano execution plans.
- **Module Relationships:** Driven by `ExecutionEngine` (`DAY-191`).

### Files Expected to Change
- `include/tinydb/execution/planner.hpp` (new)
- `src/execution/planner.cpp` (new)
- `tests/unit/execution/planner_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Planner::PlanSelect`, `PlanUpdate`, `PlanDelete`, `PlanInsert`, and `ExtractIndexCandidate` (including `PlanSummary` metadata so tests can directly assert whether `IndexScan` or `SeqScan` was chosen!).
2. Add tests verifying: `IndexScan` selected for `=`, `<`, `<=`, `>`, `>=`, `[lo, hi]`, and `10 <= id`; `SeqScan` selected when unindexed or `!=`; `Filter` preserved for `AND` residual terms!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `Planner` selects `IndexScan` accurately for indexable predicates and falls back cleanly to `SeqScan` otherwise.

### Definition of Done
`Planner` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-190: implement rule-based Planner with B+ Tree index selection`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-191` (`ExecutionEngine` orchestrator).
````

---

## DAY-191 — Implement `ExecutionEngine` Orchestrator (`ExecuteSql` and Statement Pipeline)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-191
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement `ExecutionEngine` Orchestrator (`ExecuteSql` and Statement Pipeline)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-190 complete.

### Objective
Create `tinydb::DatabaseEngine` / `ExecutionEngine` (`include/tinydb/execution/execution_engine.hpp`, `src/execution/execution_engine.cpp`) owning/coordinating `DiskManager`, `BufferPoolManager`, `CatalogManager`, `Parser`, and `Planner` behind a clean top-level API: `Open(path)` / `Create(path)` / `ExecuteSql(sql) -> std::vector<QueryResult>` / `Close()`.

### Concepts & Daily Learning
- **Concept:** Engine facade coordinating startup lifecycle (`.tinydb` + `.cat`), SQL parsing, planning, pull execution, and clean shutdown.
- **Why Database Engines Need It:** Tests and the CLI shouldn't manually wire `DiskManager`, `BufferPoolManager`, `CatalogManager`, `Parser`, and `Planner` in 20 different places; a cohesive `Database` / `ExecutionEngine` facade manages construction/destruction order safely!
- **How It Works:** `Database::OpenOrCreate(base_path, pool_size)`: manages `<base>.tinydb` and `<base>.cat` consistently, wires `BufferPoolPageProvider`, loads or initializes catalog + open `BPlusTree` handles, and runs `ExecuteStatement(const Statement&)` + `ExecuteSql(std::string_view)`.
- **Design Trade-offs:** Having `Database` own `DiskManager` -> `BufferPoolManager` -> `CatalogManager` in exact dependency order guarantees safe reverse destruction (`~Catalog` -> `~BPM` -> `~Disk`)!
- **Common Mistakes:** Declaring `DiskManager` *after* `BufferPoolManager` in class member order (destroying `DiskManager` FIRST during `~Database()`!).
- **Understanding Checkpoint:** In C++, class member destructors execute in the **reverse** order of declaration in the class body—what exact order MUST `disk_manager_`, `buffer_pool_`, and `catalog_` be declared inside `Database`?

### Component Context
- **Why It Exists:** Single clean entry point for running SQL end-to-end against persisted storage.
- **Module Relationships:** Consumed directly by `tinydb` CLI (`DAY-197`) and integration tests.

### Files Expected to Change
- `include/tinydb/execution/execution_engine.hpp` (new)
- `src/execution/execution_engine.cpp` (new)
- `tests/unit/execution/execution_engine_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Database` / `ExecutionEngine` (`Create`, `Open`, `OpenOrCreate`, `Close`, `ExecuteSql`, `ExecuteStatement`, `Catalog()`, `BufferPool()`, `Disk()`).
2. Add tests verifying full SQL workflow (`CREATE TABLE`, `INSERT`, `SELECT`, `CREATE INDEX`, `UPDATE`, `DELETE`, `DROP TABLE`, `Close` -> `Open`!).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Member destruction order is verified safe; SQL scripts run end-to-end and persist cleanly across `Close -> Open`.

### Definition of Done
`ExecutionEngine` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-191: implement Database/ExecutionEngine facade with safe lifecycle`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-192` (Runtime type validation & semantic error tests).
````

---

## DAY-192 — Test Type Validation and Runtime Semantic Error Reporting

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-192
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Test Type Validation and Runtime Semantic Error Reporting
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-191 complete.

### Objective
Write comprehensive negative semantic tests (`tests/unit/execution/semantic_error_test.cpp`) verifying clear, non-crashing error messages for: missing tables, duplicate tables/indexes, unknown columns in `SELECT`/`WHERE`/`INSERT`/`UPDATE`, `INSERT` arity mismatches, `INT32` out-of-range inserts, `VARCHAR(n)` overflows, type mismatches (`INT` <- `'str'`, `BOOL` <- `1`), and invalid `WHERE` comparisons even on empty tables!

### Concepts & Daily Learning
- **Concept:** Semantic validation boundaries (syntax errors vs catalog/type/range errors).
- **Why Database Engines Need It:** Garbage data must never sneak into `TableHeap` via unchecked casts, and helpful diagnostics (`"Value 3000000000 out of range for INT column 'age'"`) make debugging SQL scripts effortless.
- **How It Works:** Exercise every semantic failure mode against both empty and populated tables through `ExecuteSql`, verifying clear `StatusCode` + error text and verifying existing table rows remain completely unmodified on failed statements!
- **Design Trade-offs:** Ensuring failed `INSERT`/`UPDATE` leaves table + index state untouched prevents partial corruption even before multi-statement transactions.
- **Common Mistakes:** Allowing `INSERT INTO t VALUES (1, 2)` when column 2 is `BOOL` (silently converting `2` to `true` instead of rejecting non-boolean!).
- **Understanding Checkpoint:** Why should `INSERT INTO t (flag_bool) VALUES (2)` fail with a type error rather than silently converting `2` to `true`?

### Component Context
- **Why It Exists:** Proves the query engine rejects every malformed/mismatched SQL operation cleanly.
- **Module Relationships:** Validates `ExecutionEngine`, `Planner`, `ExprEvaluator`, and DML executors.

### Files Expected to Change
- `tests/unit/execution/semantic_error_test.cpp` (new)
- `src/execution/*.cpp` (fix any semantic validation gap found)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all semantic error cases in `semantic_error_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Every semantic error case returns clear diagnostic `Status` and leaves pre-existing rows unchanged.

### Definition of Done
Semantic error suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-192: add semantic type and bounds validation error tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-193` (Executor unit test sweep).
````

---

## DAY-193 — Expand Isolated Unit Tests Across All 7 Volcano Executors

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-193
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Expand Isolated Unit Tests Across All 7 Volcano Executors
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-192 complete.

### Objective
Audit and expand unit test coverage across all 7 executors (`SeqScan`, `IndexScan`, `Filter`, `Projection`, `Insert`, `Update`, `Delete`) for edge cases: multiple indexes on one table during `Insert`/`Update`/`Delete`, updating a non-indexed `VARCHAR` column across page boundaries while an index exists, `SELECT *` vs projected subsets, and `ActivePinCount() == 0` after every executor pipeline!

### Concepts & Daily Learning
- **Concept:** Compositional executor pipeline testing and multi-index synchronization invariants.
- **Why Database Engines Need It:** A table with 2 indexes (`idx_id` on `id` and `idx_score` on `score`) exercises multi-index updates where one key changes, one stays constant, and `RowID` may migrate simultaneously!
- **How It Works:** Construct tables with 2 secondary indexes, run mixed `Insert`/`Update`/`Delete`, and verify BOTH indexes via `BPlusTreeValidator::Validate(&tree, &table, col)` after every query!
- **Design Trade-offs:** Checking `ActivePinCount() == 0` in test fixtures catches subtle guard leaks in `IndexScan`/`SeqScan` immediately.
- **Common Mistakes:** Only testing tables with `0` or `1` index, missing bugs in multi-index loops!
- **Understanding Checkpoint:** When a table has indexes on BOTH `a` and `b`, and an `UPDATE` changes `a` (plus grows `c` forcing cross-page relocation `old_rid -> new_rid`), what must happen to BOTH `idx_a` AND `idx_b`?

### Component Context
- **Why It Exists:** Ensures every individual executor passes strict edge-case checks.
- **Module Relationships:** Hardens all `src/execution/*_executor.cpp` files.

### Files Expected to Change
- `tests/unit/execution/multi_index_executor_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `multi_index_executor_test.cpp` testing 2+ indexes across `Insert`, `Update` (key change + cross-page RID change!), and `Delete`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-index CRUD passes `BPlusTreeValidator` on every index and leaks 0 buffer pins.

### Definition of Done
Executor edge tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-193: add multi-index executor unit tests and pin leak assertions`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-194` (End-to-end IndexScan vs SeqScan equivalence integration tests).
````

---

## DAY-194 — Write End-to-End Query Engine Integration Tests (`IndexScan` == `SeqScan`)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-194
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Write End-to-End Query Engine Integration Tests (`IndexScan` == `SeqScan`)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-193 complete.

### Objective
Write `tests/integration/phase07_query_equivalence_test.cpp` running 100+ randomized/scripted SQL `SELECT`, `UPDATE`, and `DELETE` queries on two identical tables—one WITH B+ Tree indexes (`IndexScan` path) and one WITHOUT indexes (`SeqScan` path)—asserting identical sorted result multisets AND verifying `IndexScan` performs drastically fewer `DiskStats::page_reads`!

### Concepts & Daily Learning
- **Concept:** Differential self-testing (`Indexed Execution == Unindexed Sequential Execution`).
- **Why Database Engines Need It:** Adding an index MUST NEVER change query answers—only make them faster! Comparing `IndexScan` results against `SeqScan` on identical data proves planner predicate pushdown and B+ Tree synchronization are 100% semantically faithful!
- **How It Works:** Execute identical SQL DDL+DML streams on `users_seq` (no index) and `users_idx` (with `CREATE INDEX` both before AND after inserts!), run point & range `SELECT`s (`=`, `<`, `<=`, `>`, `>=`, `AND`), assert identical sorted row sets, and assert `Planner` actually picked `IndexScan` for `users_idx`!
- **Design Trade-offs:** Sorting result rows by `(all columns)` in test verification handles legal unordered relational output while validating exact row multisets!
- **Common Mistakes:** Assuming SQL `SELECT` without `ORDER BY` guarantees identical row output order between `SeqScan` (heap order) and `IndexScan` (key order)!
- **Understanding Checkpoint:** Why is sorting the returned rows in the test harness mandatory when verifying `SeqScan` equivalence against `IndexScan`?

### Component Context
- **Why It Exists:** Proves index optimization never alters query correctness.
- **Module Relationships:** Tests `Parser` + `Planner` + `ExecutionEngine` + `BPlusTree` + `TableHeap`.

### Files Expected to Change
- `tests/integration/phase07_query_equivalence_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `phase07_query_equivalence_test.cpp` with `SeqScan` vs `IndexScan` equivalence checks across CRUD + cold restarts + `DiskStats` comparisons.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `IndexScan` and `SeqScan` return identical row multisets across all test queries while `IndexScan` uses strictly fewer page reads on large tables.

### Definition of Done
Equivalence integration suite passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-194: verify SeqScan vs IndexScan SQL equivalence and I/O reduction`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-195` (Multi-statement SQL batch & error isolation semantics).
````

---

## DAY-195 — Implement Multi-Statement SQL Batch Execution and Error Isolation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-195
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement Multi-Statement SQL Batch Execution and Error Isolation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-194 complete.

### Objective
Refine `ExecutionEngine::ExecuteSql(std::string_view sql)` multi-statement batch behavior (`stmt1; stmt2; stmt3;`): parse entire batch first (or stop cleanly on the first failing statement, reporting exact failing statement index + partial prior results cleanly!), and verify behavior across mixed DDL/DML batches.

### Concepts & Daily Learning
- **Concept:** Multi-statement script execution semantics (fail-fast on error vs continue-on-error) and parse-before-execute vs statement-by-statement.
- **Why Database Engines Need It:** If statement #2 in a 5-statement script is a syntax error, rejecting the script at parse time before executing statement #1 avoids half-applied schema changes; if statement #3 fails semantically (`INSERT` wrong type), stopping immediately prevents statement #4 from running on unexpected state!
- **How It Works:** `ExecuteSql` calls `Parser::ParseAll(sql)` first (failing cleanly before execution on syntax errors), then loops executing each `Statement` in order; on runtime error, append the failing `QueryResult` and stop executing subsequent statements (returning results collected up to & including the error!).
- **Design Trade-offs:** Returning `std::vector<QueryResult>` (where `.back().status` holds the error if execution stopped early) preserves exact results of statements `0..k-1` AND the precise failure reason for statement `k`!
- **Common Mistakes:** Continuing past a failed `CREATE TABLE` into subsequent `INSERT INTO that_table` statements (producing cascading noise errors!).
- **Understanding Checkpoint:** Why is failing fast on the first runtime error inside a multi-statement script cleaner than blindly running remaining statements?

### Component Context
- **Why It Exists:** Predictable batch script execution in engine & CLI.
- **Module Relationships:** Refines `ExecutionEngine::ExecuteSql`.

### Files Expected to Change
- `include/tinydb/execution/execution_engine.hpp`
- `src/execution/execution_engine.cpp`
- `tests/unit/execution/multi_stmt_exec_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Refine `ExecuteSql` batch handling and verify empty/comment-only scripts return empty result vector cleanly.
2. Add tests for: multi-statement success batch, syntax error on stmt #3 preventing stmt #1 execution, and semantic error on stmt #3 halting before stmt #4!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Syntax error prevents batch start; runtime error halts cleanly at failing statement without executing later statements.

### Definition of Done
Batch execution semantics tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-195: refine multi-statement ExecuteSql batch and fail-fast semantics`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-196` (Tabular output formatter).
````

---

## DAY-196 — Implement Readable Tabular Output Formatter

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-196
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement Readable Tabular Output Formatter
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-195 complete.

### Objective
Implement `ResultFormatter` (`include/tinydb/cli/formatter.hpp`, `src/cli/formatter.cpp`) rendering `QueryResult` into crisp aligned ASCII tables (`+----+-------+`, header row, padded columns, row count footer `(N rows)` / `(1 row)`), mutation counts (`"3 rows affected."`), DDL status lines, and clean error lines (`"Error [Code]: ..."`).

### Concepts & Daily Learning
- **Concept:** Dynamic column-width alignment, escaping/displaying control characters safely in terminal tables, and zero-row result formatting.
- **Why Database Engines Need It:** Readable tabular formatting lets developers inspect schemas, test queries interactively, and spot misaligned values immediately.
- **How It Works:** Compute `width[j] = max(header[j].size(), max_row_val[j].size())` across all rows; render border `+---+---+`, header `| id | name |`, border `+---+---+`, data rows, bottom border, and `(N rows)` footer. Handle empty result sets cleanly (either printing headers + `(0 rows)` or `Empty set (0 rows)`—printing headers + `(0 rows)` shows exact projected columns clearly!).
- **Design Trade-offs:** Formatting to `std::string` (or `std::ostream&`) keeps formatting 100% unit-testable without capturing `stdout`!
- **Common Mistakes:** Right-aligning strings/left-aligning unpredictably or miscomputing column widths when `rows` is empty (`width[j]` must initialize from `column_name.size()`, not `0`!).
- **Understanding Checkpoint:** Why should `ResultFormatter::Format` accept `std::ostream&` or return `std::string` instead of hardcoding `std::cout`?

### Component Context
- **Why It Exists:** Presentation layer for `tinydb` CLI and schema output.
- **Module Relationships:** Renders `QueryResult` (`DAY-180`), used by CLI REPL (`DAY-197`–`DAY-199`).

### Files Expected to Change
- `include/tinydb/cli/formatter.hpp` (new)
- `src/cli/formatter.cpp` (new)
- `tests/unit/cli/formatter_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ResultFormatter::Format(const QueryResult&)` and `FormatSchemaTable(...)` for `.schema`.
2. Add unit tests checking exact ASCII alignment on 0 rows, 1 row, multi-row mixed widths, DDL/DML messages, and error results!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Exact expected ASCII borders and column widths match across empty, single, and multi-row result sets.

### Definition of Done
Formatter implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-196: implement aligned ASCII tabular ResultFormatter`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-197` (CLI REPL skeleton & multi-line buffering).
````

---

## DAY-197 — Implement Interactive CLI REPL Skeleton and Multi-Line SQL Buffering

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-197
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement Interactive CLI REPL Skeleton and Multi-Line SQL Buffering
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-196 complete.

### Objective
Implement `tinydb::CliSession` (`include/tinydb/cli/cli_session.hpp`, `src/cli/cli_session.cpp`) and update `src/cli/main.cpp` to open/create a database path (`tinydb <db_path>` or `--version` / `--help`), run a stream-driven REPL (`std::istream& in`, `std::ostream& out`, `std::ostream& err`), print prompts (`tinydb> ` / `   ...> `), handle multi-line SQL up to `;`, handle dot-commands on empty buffer, and flush/close DB cleanly on EOF!

### Concepts & Daily Learning
- **Concept:** Stream-injected REPL (`std::istream&`, `std::ostream&`), multi-line statement buffering (respecting `;` inside string literals `'a;b'`!!), and clean EOF shutdown.
- **Why Database Engines Need It:** If a user types `INSERT INTO t VALUES ('hello;world');` across two lines, naively splitting on `;` with `str.find(';')` breaks the string literal! A tiny quote-aware semicolon detector knows when a SQL buffer is complete! And accepting `std::istream&`/`std::ostream&` lets GoogleTest test the real REPL in-memory via `std::istringstream`/`std::ostringstream`!
- **How It Works:** Read line-by-line. If buffer is empty and trimmed line starts with `.`, dispatch dot-command immediately! Otherwise append line to `sql_buffer_`; check quote-and-comment-aware `HasTerminatingSemicolon(sql_buffer_)`; when complete, invoke `db_.ExecuteSql(sql_buffer_)`, print results, clear buffer. On EOF (`!std::getline`), execute any trailing non-empty `sql_buffer_` (or report incomplete buffer cleanly) and call `db_.Close()`!
- **Design Trade-offs:** Keeping `CliSession` stream-based (`istream&`/`ostream&`) makes CLI tests 100x faster and cleaner than spawning subprocesses for every case (while subprocess tests in `DAY-200` verify `main.cpp` CLI args too!).
- **Common Mistakes:** Splitting `'foo;bar'` on `;` inside quotes; ignoring `-- comment with ;` inside multi-line input!
- **Understanding Checkpoint:** Why must the CLI REPL's semicolon completion detector track single-quoted strings `'...'` and `--` comments before concluding a statement is ready to execute?

### Component Context
- **Why It Exists:** Interactive user shell for TinyDB.
- **Module Relationships:** Connects `main.cpp` -> `CliSession` -> `ExecutionEngine` -> `ResultFormatter`.

### Files Expected to Change
- `include/tinydb/cli/cli_session.hpp` (new)
- `src/cli/cli_session.cpp` (new)
- `src/cli/main.cpp`
- `tests/unit/cli/cli_repl_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `CliSession` and wire `src/cli/main.cpp` (`tinydb <db_prefix>`, `--version`, `--help`).
2. Add tests verifying multi-line queries, `'a;b'` string literals not splitting early, `-- comment ;` handling, and clean DB flush on EOF!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- REPL executes single- and multi-line SQL cleanly, handles semicolons inside `'...'` and `--`, and shuts down DB cleanly on EOF.

### Definition of Done
CLI REPL skeleton tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-197: implement quote-aware CLI REPL skeleton and main entry point`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-198` (`.exit`, `.quit`, `.help` commands).
````

---

## DAY-198 — Implement CLI Meta-Commands: `.exit`, `.quit`, and `.help`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-198
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement CLI Meta-Commands: `.exit`, `.quit`, and `.help`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-197 complete.

### Objective
Implement dot-command parsing and execution in `CliSession` for `.exit`, `.quit`, `.help`, plus clean unknown dot-command error diagnostics (`"Unknown meta-command '.foo'. Type '.help' for usage."`), ensuring `.exit`/`.quit` always flushes & closes `Database` cleanly!

### Concepts & Daily Learning
- **Concept:** Separating interactive shell meta-commands (`.help`, `.quit`) from SQL grammar (`Parser`).
- **Why Database Engines Need It:** Like SQLite (`sqlite3`) and PostgreSQL (`psql`), keeping `.commands` in the CLI layer keeps the SQL `Lexer`/`Parser` clean and standard-focused while providing ergonomic tooling.
- **How It Works:** Trim leading/trailing whitespace on dot-commands (and tolerate optional trailing spaces/args rejection cleanly!). `.exit` and `.quit` set `should_exit_ = true` breaking the REPL loop cleanly so `db_.Close()` runs. `.help` prints all supported commands (`.exit`, `.quit`, `.help`, `.tables`, `.schema [table]`) and supported SQL summary.
- **Design Trade-offs:** Tolerating leading spaces before `.help` is user-friendly; rejecting `.exit extra_arg` or ignoring whitespace safely keeps behavior crisp.
- **Common Mistakes:** Calling `std::exit(0)` directly inside `.exit` (skipping C++ destructors, `BufferPoolManager::Shutdown()`, and `DiskManager::Close()`, causing data loss & ASan leak reports!!).
- **Understanding Checkpoint:** Why would calling `std::exit(0)` inside the `.exit` handler break durability AND cause destructor bugs?

### Component Context
- **Why It Exists:** Essential CLI control and help commands.
- **Module Relationships:** Inside `CliSession` (`src/cli/cli_session.cpp`).

### Files Expected to Change
- `src/cli/cli_session.cpp`
- `tests/unit/cli/cli_repl_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `.exit`, `.quit`, `.help`, and unknown dot-command handling in `CliSession`.
2. Add tests verifying `.help` output lists all 5 dot-commands, `.exit` and `.quit` exit immediately without executing buffered subsequent lines and flush DB cleanly, and `.unknown` emits helpful error without terminating session!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `.exit`/`.quit` terminates cleanly via loop break (zero `std::exit`), `.help` prints accurately, unknown `.cmd` reports clean error.

### Definition of Done
Meta-commands tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-198: implement .exit, .quit, and .help CLI meta-commands`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-199` (`.tables` and `.schema` introspection commands).
````

---

## DAY-199 — Implement CLI Introspection Commands: `.tables` and `.schema [table]`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-199
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Implement CLI Introspection Commands: `.tables` and `.schema [table]`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-198 complete.

### Objective
Implement `.tables` (listing all tables alphabetically with column count / page info cleanly) and `.schema` / `.schema <table_name>` (rendering `CREATE TABLE` DDL + attached `CREATE INDEX` statements from `CatalogManager`!) inside `CliSession`.

### Concepts & Daily Learning
- **Concept:** Catalog introspection and reverse-generating DDL from `TableMetadata` + `IndexMetadata`.
- **Why Database Engines Need It:** Users interacting with a persisted database after restart need `.tables` and `.schema` to discover existing table names, column types, `VARCHAR(n)` limits, and existing indexes!
- **How It Works:**
  - `.tables`: query `db_.Catalog().ListTables()` sorted alphabetically; print `"(no tables)"` or formatted table list.
  - `.schema`: if no argument, print schema for all tables (sorted); if `<table>` argument given, look up `GetTable(table)` and `GetTableIndexes(meta.table_id)`—render `CREATE TABLE name (col1 TYPE, ...);` plus `CREATE INDEX idx ON table (col);`! Report clear error if table doesn't exist!
- **Design Trade-offs:** Formatting `.schema` output as valid copy-pasteable TinyDB SQL (`CREATE TABLE ...;`) is immediately intuitive and matches SQLite's `.schema` convention!
- **Common Mistakes:** Crashing on `.schema` when extra whitespace separates `.schema   users`, or failing to report an error on `.schema missing_table`!
- **Understanding Checkpoint:** Why is formatting `.schema` output as valid `CREATE TABLE ...;` + `CREATE INDEX ...;` DDL statements better than a custom ad-hoc text dump?

### Component Context
- **Why It Exists:** Completes all 5 required CLI meta-commands (`.exit`, `.quit`, `.help`, `.tables`, `.schema`).
- **Module Relationships:** Reads `CatalogManager` metadata cleanly from `CliSession`.

### Files Expected to Change
- `src/cli/cli_session.cpp`
- `tests/unit/cli/cli_repl_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `.tables` and `.schema [table]` in `CliSession`.
2. Write unit tests verifying: empty DB `.tables`/`.schema`, populated DB with 2 tables + 1 index, `.schema users` (case-insensitive!), `.schema missing` error, and extra arguments error (`".tables foo"`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `.tables` and `.schema` output accurate DDL/listings and handle missing/empty cases cleanly.

### Definition of Done
Introspection commands tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-199: implement .tables and .schema CLI introspection commands`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-200` (Subprocess CLI automated integration tests).
````

---

## DAY-200 — Write Real CLI Subprocess Piped Stdin/Stdout Automated Integration Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-200
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Write Real CLI Subprocess Piped Stdin/Stdout Automated Integration Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-199 complete.

### Objective
Write automated CLI tests (`tests/integration/phase07_cli_subprocess_test.cpp`) verifying non-interactive piped invocation of the actual compiled `tinydb` binary (`--version`, `--help`, piping SQL + dot-commands into `./tinydb <db_path>` in Session 1, reopening in Session 2, verifying exit codes, stdout tables, and stderr errors!).

### Concepts & Daily Learning
- **Concept:** Black-box CLI binary testing (`main()` argument parsing, exit codes, interactive vs piped prompt cleanliness, stdout vs stderr separation).
- **Why Database Engines Need It:** Testing `CliSession` in-process skips `main(argc, argv)`, exit codes, and `isatty` / non-interactive terminal behavior! Black-box subprocess tests verify the real executable users actually run.
- **How It Works:** Detect TTY vs piped stdin cleanly in `main.cpp` / `CliSession` (only printing interactive `tinydb> ` prompt when `isatty(STDIN_FILENO)` or optional `--quiet` flag, OR verifying cleanly!). Actually suppressing interactive prompts when piped (just like `sqlite3`!) makes piped scripts produce super clean output—document this cleanly! Invoke `./tinydb` from `phase07_cli_subprocess_test.cpp` (passing `$<TARGET_FILE:tinydb>` via CMake `target_compile_definitions`!) using POSIX `pipe`/`fork`/`waitpid` (or safe `popen` helper on fixed temp paths without shell injection!).
- **Design Trade-offs:** Using `fork` + `execvp` + `pipe` in tests avoids `/bin/sh` shell-escaping bugs and directly captures exact `stdout`, `stderr`, and exit status!
- **Common Mistakes:** Hardcoding `./build/tinydb` path in tests (breaks when user builds in `build-asan/` or `Release/`! Always inject `$<TARGET_FILE:tinydb>` from CMake!).
- **Understanding Checkpoint:** Why is passing `$<TARGET_FILE:tinydb>` via CMake compile definitions AND using `execvp` instead of `system()` safer for CLI integration tests?

### Component Context
- **Why It Exists:** End-to-end verification of the `tinydb` executable binary.
- **Module Relationships:** Exercises `src/cli/main.cpp` and entire stack.

### Files Expected to Change
- `src/cli/main.cpp`, `include/tinydb/cli/cli_session.hpp`, `src/cli/cli_session.cpp` (interactive prompt TTY check)
- `tests/integration/phase07_cli_subprocess_test.cpp` (new)
- `CMakeLists.txt` (pass `TINYDB_CLI_PATH="$<TARGET_FILE:tinydb>"` safely)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Ensure `tinydb` suppresses `tinydb> ` prompt on non-TTY piped stdin cleanly (or via config option tested in unit tests!).
2. Implement `phase07_cli_subprocess_test.cpp` running real `tinydb` process across two invocations (`Session 1: CREATE + INSERT + CREATE INDEX + .exit` -> `Session 2: .tables + .schema + SELECT + UPDATE + DELETE + .quit`) and invalid arg / corrupt DB path checks!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Real `tinydb` binary passes multi-session piped persistence tests under both Debug and ASan/UBSan with clean exit codes.

### Definition of Done
Subprocess CLI tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-200: add real tinydb binary subprocess CLI integration tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-201` (Multi-session persistence & recovery-free restart tests).
````

---

## DAY-201 — Test End-to-End SQL Persistence Across Multi-Session Restarts

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-201
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Test End-to-End SQL Persistence Across Multi-Session Restarts
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-200 complete.

### Objective
Write `tests/integration/phase07_sql_persistence_test.cpp` executing a realistic 5-restart SQL lifecycle (creating 3 tables + 2 indexes, bulk inserting 500 rows, restarting, updating 100 rows including cross-page `VARCHAR` growth, restarting, deleting 150 rows + `DROP TABLE`, restarting, creating new table reusing reclaimed pages, and verifying all remaining `SELECT`s and `BPlusTreeValidator`!).

### Concepts & Daily Learning
- **Concept:** Full-stack cross-restart invariance (`CLI/SQL -> Catalog -> TableHeap -> BPlusTree -> BufferPool -> DiskManager`).
- **Why Database Engines Need It:** Tests subtle multi-session interactions like `CREATE TABLE` reusing freed heap+index pages from a `DROP TABLE` executed in an earlier session!
- **How It Works:** Destroy `Database` completely between all 5 stages, reopen cold from disk path, assert `SELECT` results, `.tables`/`.schema` state, `BPlusTreeValidator`, and `VerifyFreeListIntegrity()`.
- **Design Trade-offs:** Keeping buffer pool tiny (`pool_size = 8`) in one of the test stages forces heavy eviction simultaneously across tables and indexes!
- **Common Mistakes:** Only testing single-restart scenarios (`create -> restart -> select`), missing bugs on the 2nd or 3rd session (`drop -> restart -> recreate -> restart`)!
- **Understanding Checkpoint:** Trace what happens across 3 restarts when Session 1 creates & drops `temp_tbl`, and Session 2 creates `users` with `CREATE INDEX`—which disk pages get reused?

### Component Context
- **Why It Exists:** Proves full SQL + storage persistence across arbitrary restarts.
- **Module Relationships:** Exercises Phases 01–07 together.

### Files Expected to Change
- `tests/integration/phase07_sql_persistence_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement 5-stage SQL restart integration test in `phase07_sql_persistence_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 5 restart stages preserve exact SQL rows, indexes, and freelist invariants.

### Definition of Done
Multi-restart SQL persistence tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-201: add multi-restart SQL DDL/DML/index persistence integration tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-202` (SQL boundary & edge-case tests).
````

---

## DAY-202 — Test Query Engine Edge Cases (Empty Tables, 0-Row Mutations, Special Strings, Extremes)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-202
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Test Query Engine Edge Cases (Empty Tables, 0-Row Mutations, Special Strings, Extremes)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-201 complete.

### Objective
Write edge-case tests (`tests/unit/execution/execution_edge_cases_test.cpp`) verifying: `SELECT`/`UPDATE`/`DELETE` on empty tables, `UPDATE`/`DELETE` matching 0 rows, `CREATE INDEX` on an empty table followed by inserts, `VARCHAR` containing quotes/pipes/`\n`/`\t`/empty `''`, `INT32_MIN/MAX`, `INT64_MIN/MAX`, and max-size single-page `VARCHAR` rows!

### Concepts & Daily Learning
- **Concept:** Edge-case completeness across data extremes and degenerate empty states.
- **Why Database Engines Need It:** Empty tables + `CREATE INDEX` (where `root_page_id == kInvalidPageId` initially!) or strings containing `|` / `\n` inside `ResultFormatter` often trip naive implementations!
- **How It Works:** Test creating an index on an empty table -> saving/reopening -> inserting rows -> querying via index! Test `ResultFormatter` on strings with `'a|b\nc'` (sanitizing control newlines or formatting safely!). Test `WHERE id = -2147483648` and `-9223372036854775808`.
- **Design Trade-offs:** Sanitizing/escaping newlines inside `ResultFormatter` keeps ASCII tables visually intact without crashing or corrupting alignment.
- **Common Mistakes:** `IndexScan` or `CREATE INDEX` assuming `TableHeap` has at least 1 row or `BPlusTree` has a non-invalid `root_page_id`!
- **Understanding Checkpoint:** Trace `CREATE INDEX idx ON empty_tbl (id);` -> restart DB -> `INSERT INTO empty_tbl VALUES (1);` -> `SELECT * FROM empty_tbl WHERE id = 1;`! Why must `CatalogManager` load an empty index (`root_page_id == kInvalidPageId`) cleanly?

### Component Context
- **Why It Exists:** Eliminates empty-state and data-extreme surprises before Phase 08.
- **Module Relationships:** Hardens `ExecutionEngine`, `BPlusTree`, `ResultFormatter`, and `CatalogManager`.

### Files Expected to Change
- `tests/unit/execution/execution_edge_cases_test.cpp` (new)
- `src/cli/formatter.cpp`, `src/execution/*.cpp` (fix any edge bug uncovered)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement all empty-state, extreme-integer, and special-string tests in `execution_edge_cases_test.cpp`.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All edge cases pass cleanly without crashes, formatting corruption, or pin leaks.

### Definition of Done
Edge-case tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-202: verify empty-table, extreme-int, and special-string SQL edge cases`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-203` (ASan/UBSan audit of query engine & CLI).
````

---

## DAY-203 — Run ASan/UBSan Audit Across Query Engine and CLI

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-203
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Run ASan/UBSan Audit Across Query Engine and CLI
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-202 complete.

### Objective
Audit `include/tinydb/execution/*`, `src/execution/*`, `include/tinydb/cli/*`, and `src/cli/*` for dangling references, exception/error memory safety, `-Wconversion`/`-Wshadow` warnings under `-Werror`, `clang-format` compliance, and full sanitizer testing (including running the `tinydb` binary itself built with ASan+UBSan via the subprocess test!).

### Concepts & Daily Learning
- **Concept:** Full-pipeline sanitizer auditing across compiled CLI binaries + unit/integration suites.
- **Why Database Engines Need It:** `ExecutionEngine` wires together raw pointers/references (`TableHeap*`, `BPlusTree*`, `const Expression*`, `const Schema&`); verifying zero lifetime hazards across `Init`/`Next`/error paths guarantees Phase 08 transactions start from a leak-free baseline!
- **How It Works:** Inspect ownership lifetimes in `Planner` (do `FilterExecutor` or `ProjectionExecutor` hold `const Expression*` safely while `Statement` outlives execution?), verify `SelectStmt` / `Statement` lifetimes vs `Planner`, fix any questionable borrowing, and run ASan+UBSan+LSan across the entire suite!
- **Design Trade-offs:** Either keeping `Statement` alive during execution OR cloning/sharing predicate pointers cleanly eliminates dangling `const Expression*` hazards!
- **Common Mistakes:** Returning a `std::unique_ptr<AbstractExecutor>` from `Planner` that holds a raw `const Expression*` pointing into a temporary `Statement` destroyed before `Next()` runs!
- **Understanding Checkpoint:** Check `Planner::PlanSelect(const SelectStmt&)`—how do you guarantee `FilterExecutor`'s `Expression` pointer NEVER outlives the `SelectStmt` AST node?

### Component Context
- **Why It Exists:** Lifetime and memory-safety gate before adding transactions & WAL.
- **Module Relationships:** Audits `src/execution/` and `src/cli/`.

### Files Expected to Change
- `include/tinydb/execution/*.hpp`, `src/execution/*.cpp`, `src/cli/*.cpp` (any lifetime/warning cleanups)
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Audit AST `Expression*` / `Schema*` / `TableHeap*` lifetimes in `Planner` and all executors.
2. Run Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format --dry-run --Werror`.

### Testing Instructions
- Full matrix build & `ctest --output-on-failure` (including ASan-instrumented `tinydb` subprocess tests).

### Acceptance Criteria
- Zero dangling pointer risks, zero leaks, zero compiler warnings across all builds.

### Definition of Done
Audit complete and verified, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-203: audit Query Engine and CLI lifetimes under ASan and UBSan`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-204` (Phase 07 sign-off).
````

---

## DAY-204 — Add Example SQL Walkthrough and Sign Off Phase 07

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-204
- **Phase:** Phase 07 — Query Engine and CLI
- **Session Title:** Add Example SQL Walkthrough and Sign Off Phase 07
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-179 through DAY-203 complete.

### Objective
Create a runnable sample SQL script (`examples/basic_tour.sql`) + `examples/README.md` update, verify `./build/tinydb` runs the example cleanly end-to-end, update `docs/architecture.md` if needed, and sign off Phase 07 (`158.0 / 220.0` hours)!

### Concepts & Daily Learning
- **Concept:** Milestone acceptance verification (TinyDB now works as an end-to-end persistent relational SQL database with B+ Tree indexes!).
- **Why Database Engines Need It:** Reaching a working interactive SQL + persistent B+ Tree database BEFORE adding transactions/WAL validates the entire query and storage stack cleanly before tackling durability and crash recovery.
- **How It Works:** Write `examples/basic_tour.sql` exercising `CREATE TABLE`, `INSERT`, `CREATE INDEX`, `SELECT`, `UPDATE`, `DELETE`, `.tables`, `.schema`, verify it executes cleanly against `tinydb`, and verify full CTest matrix.
- **Design Trade-offs:** Keeping Phase 07 non-transactional until Phase 08 cleanly isolates query bugs from WAL/recovery bugs.
- **Common Mistakes:** Proceeding to Phase 08 while any `IndexScan`, `Update`, or `CLI` test is flaky or failing.
- **Understanding Checkpoint:** Trace an interactive `SELECT name FROM users WHERE id = 5;` in `tinydb` from `std::getline` all the way down to `DiskManager::ReadPage` and back to ASCII table output.

### Component Context
- **Why It Exists:** Completes Phase 07 (`20.0` hours).
- **Module Relationships:** Hands off a stable SQL + storage engine to Phase 08 (`Transactions & WAL`).

### Files Expected to Change
- `examples/basic_tour.sql` (new)
- `examples/README.md`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Create `examples/basic_tour.sql` and test executing it with `./build/tinydb` on a temporary scratch path (then remove scratch DB!).
2. Run full Debug/ASan+UBSan/Release test suites and formatting check.
3. Update `PROGRESS.md` (`158.0 / 220.0` hours, Phase 07 complete, Phase 08 active) and `NEXT_SESSION.md` -> `DAY-205`.

### Testing Instructions
- Execute `examples/basic_tour.sql` cleanly; run full `ctest` matrix + `clang-format --dry-run --Werror`.

### Acceptance Criteria
- `examples/basic_tour.sql` runs cleanly; 100% of Phase 00–07 tests pass across Debug, ASan+UBSan, and Release.

### Definition of Done
Phase 07 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-204: add basic SQL walkthrough example and sign off Phase 07`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 08 (`DAY-205` — Transactions & No-Steal WAL architecture).
````
