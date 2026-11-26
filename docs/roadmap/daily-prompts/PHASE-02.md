# Phase 02 — Records and Table Heap (`DAY-050` – `DAY-075`)

**Phase Budget:** 20.0 hours (24 × 45 min + 2 × 60 min = 26 sessions)
**Goal:** Implement TinyDB's relational type system (`INT`, `BIGINT`, `BOOL`, `VARCHAR(n)`), schema & tuple explicit serialization, 4096-byte slotted pages with compaction, `RowID`, and multi-page `TableHeap` with sequential scans.

---

## DAY-050 — Define `DataType` Enum (`INT`, `BIGINT`, `BOOL`, `VARCHAR`) and Type Helpers

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-050
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Define `DataType` Enum (`INT`, `BIGINT`, `BOOL`, `VARCHAR`) and Type Helpers
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** Phase 01 complete (`DAY-049`).

### Objective
Define `enum class DataType : std::uint8_t` (`kInt`, `kBigInt`, `kBool`, `kVarchar`) and type metadata helpers (`DecodeDataType`, `DataTypeName`, `IsFixedLength`, `FixedByteSize`) in `include/tinydb/storage/data_type.hpp` and `src/storage/data_type.cpp`.

### Concepts & Daily Learning
- **Concept:** Logical SQL types vs physical storage encodings (`INT` = 4B `int32_t`, `BIGINT` = 8B `int64_t`, `BOOL` = 1B, `VARCHAR(n)` = variable-length bytes).
- **Why Database Engines Need It:** Decoupling `DataType` metadata from C++ host types guarantees deterministic disk widths and catches unsupported raw type tags on catalog/tuple deserialization.
- **How It Works:** `FixedByteSize` returns `4`, `8`, `1` for fixed types and `std::nullopt` for `kVarchar`; `DecodeDataType(uint8_t)` rejects unknown tags cleanly.
- **Design Trade-offs:** Excluding `NULL`, `FLOAT`, and `DECIMAL` in v1 keeps tuple layouts simple and comparison strictly total (no SQL tri-valued logic or NaN traps).
- **Common Mistakes:** Using `sizeof(bool)` or `sizeof(int)` directly without verifying 1-byte / 4-byte widths.
- **Understanding Checkpoint:** Why is `kVarchar` variable-length at runtime even though its schema specifies a maximum length `n` (`VARCHAR(n)`)?

### Component Context
- **Why It Exists:** Canonical type descriptor shared by `Value`, `Column`, `Schema`, `Tuple`, `Catalog`, and `Parser`.
- **Module Relationships:** Used across Phases 02, 03, 06, and 07.

### Files Expected to Change
- `include/tinydb/storage/data_type.hpp` (new)
- `src/storage/data_type.cpp` (new)
- `tests/unit/storage/data_type_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `DataType`, `DecodeDataType`, `DataTypeName`, `IsFixedLength`, and `FixedByteSize`.
2. Add unit tests checking all 4 types and rejecting raw invalid type bytes (`0`, `5..255`).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All 4 types map deterministically; invalid raw type bytes fail with `StatusCode::kCorruption`.

### Definition of Done
Implemented, tested, formatted, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-050: define DataType enum and storage type helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-051` (`Value` representation).
````

---

## DAY-051 — Implement `Value` Tagged Variant Representation and Constructors

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-051
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Value` Tagged Variant Representation and Constructors
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-050 complete.

### Objective
Implement `tinydb::Value` (`include/tinydb/storage/value.hpp`, `src/storage/value.cpp`) wrapping `std::variant<std::int32_t, std::int64_t, bool, std::string>` with explicit factories (`MakeInt`, `MakeBigInt`, `MakeBool`, `MakeVarchar`), `Type()`, typed getters (`AsInt()`, etc.), and `ToString()`.

### Concepts & Daily Learning
- **Concept:** Type-safe discriminated unions (`std::variant`) in C++20 for dynamic runtime column values.
- **Why Database Engines Need It:** A tuple holds heterogeneous column types determined at runtime by a table's `Schema`, not at compile time; `std::variant` provides safe runtime polymorphism without heap-allocating every `INT`!
- **How It Works:** `Value` stores `DataType` + `std::variant` (or derives `DataType` from `variant::index()`), exposing safe `Result`/`std::optional` typed accessors and `ToString()` for CLI printing.
- **Design Trade-offs:** Using `std::variant` inline avoids per-integer `new`/`virtual` overhead (unlike classic `Object*` inheritance hierarchies) while keeping `Value` copyable and movable.
- **Common Mistakes:** Confusing `bool` and `int32_t` in implicit C++ constructors (e.g., `Value(true)` calling `int32_t` if `bool` isn't distinct—explicit factory methods `MakeInt`/`MakeBool` completely prevent implicit C++ coercion traps!).
- **Understanding Checkpoint:** Why are explicit factories `Value::MakeInt(1)` and `Value::MakeBool(true)` safer in C++ than overloaded constructors `Value(int32_t)` and `Value(bool)`?

### Component Context
- **Why It Exists:** Represents individual cell values inside `Tuple`s, `InsertStmt`s, and `FilterExecutor`.
- **Module Relationships:** Built on `DataType`; composed into `Tuple` (`DAY-055`).

### Files Expected to Change
- `include/tinydb/storage/value.hpp` (new)
- `src/storage/value.cpp` (new)
- `tests/unit/storage/value_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Value` with explicit factories (`MakeInt`, `MakeBigInt`, `MakeBool`, `MakeVarchar`), `Type()`, `AsInt()`, `AsBigInt()`, `AsBool()`, `AsVarchar()`, and `ToString()`.
2. Write unit tests verifying type tags, getters, and type-mismatch rejection (`MakeInt(42).AsBool()` fails cleanly).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero implicit `int <-> bool` ambiguity; wrong-type getter returns error `Result`.

### Definition of Done
`Value` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-051: implement type-safe Value variant representation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-052` (`Value` validation & comparisons).
````

---

## DAY-052 — Implement `Value` Type Validation, Bounds Checks, and Comparison Operators

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-052
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Value` Type Validation, Bounds Checks, and Comparison Operators
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-051 complete.

### Objective
Add `Value::Compare(const Value& lhs, const Value& rhs)` (`<`, `<=`, `==`, `!=`, `>`, `>=` returning `Result<int>` / `Result<bool>`), safe integer widening/coercion rules (`INT` <-> `BIGINT` comparison without overflow!), and `ValidateAgainst(type, max_len)` checking `VARCHAR(n)` bounds.

### Concepts & Daily Learning
- **Concept:** Safe cross-width integer comparison (`int32_t` vs `int64_t`), signed comparison pitfalls, and `VARCHAR(n)` truncation refusal vs silent truncation.
- **Why Database Engines Need It:** SQL `WHERE id = 5` parses `5` as a 64-bit integer literal while `id` may be `INT` (`int32_t`); if you cast `5000000000` (`BIGINT`) down to `int32_t`, it wraps and falsely matches!
- **How It Works:** Compare `INT` and `BIGINT` by widening `int32_t` to `int64_t` (lossless!); reject comparing `INT` with `VARCHAR` or `BOOL` with `StatusCode::kInvalidArgument`; enforce `str.size() <= max_len` (never silently truncate!).
- **Design Trade-offs:** Strict refusal on `VARCHAR` overflow aligns with PostgreSQL / modern SQL strict mode rather than silent MySQL truncation.
- **Common Mistakes:** Narrowing `int64_t` to `int32_t` during comparison or assignment without range check (`INT32_MIN <= val <= INT32_MAX`).
- **Understanding Checkpoint:** How do you safely convert or compare a SQL `BIGINT` literal against an `INT` column without UB or wraparound?

### Component Context
- **Why It Exists:** Guarantees safe comparisons and assignments for both `Tuple`s and Phase 07's `FilterExecutor`/`InsertExecutor`.
- **Module Relationships:** Used by `Tuple` (`DAY-058`), `FilterExecutor`, and `IndexExecutor`.

### Files Expected to Change
- `include/tinydb/storage/value.hpp`
- `src/storage/value.cpp`
- `tests/unit/storage/value_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Value::Compare(const Value&, const Value&)` (handling `INT`/`BIGINT` promotion cleanly, rejecting incompatible types), `TryCastTo(DataType target_type, uint32_t max_len)`, and equality operators.
2. Test boundary cases: `INT32_MIN/MAX`, `INT64_MIN/MAX`, `INT` vs out-of-range `BIGINT` (`3000000000LL` rejected by `TryCastTo(kInt)`!), `VARCHAR(5)` accepting length `5` and rejecting length `6`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Zero signed overflow or narrowing bugs; incompatible type comparisons fail with clear `Status`.

### Definition of Done
Validation and comparisons tested under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-052: implement Value comparisons, safe narrowing checks, and VARCHAR bounds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-053` (`Column` metadata).
````

---

## DAY-053 — Implement `Column` Metadata Class (Name, Type, Max Length, Validation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-053
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Column` Metadata Class (Name, Type, Max Length, Validation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-015 (`identifier.hpp`), DAY-052 complete.

### Objective
Implement `tinydb::Column` (`include/tinydb/storage/column.hpp`, `src/storage/column.cpp`) validating column names, type constraints (`VARCHAR(n)` requires `1 <= n <= config::kMaxVarcharLength`), and fixed/max serialized byte sizes.

### Concepts & Daily Learning
- **Concept:** Column descriptors (`name`, `DataType`, `max_length`), enforcing max length bounds relative to the 4096-byte page ceiling.
- **Why Database Engines Need It:** Because TinyDB v1 stores each tuple inside a single 4096-byte slotted page (no TOAST/overflow pages in v1), `VARCHAR(100000)` must be rejected at `CREATE TABLE` time!
- **How It Works:** `Column::Create(name, type, max_length)` validates `IsValidIdentifier(name)`, enforces `max_length == fixed_size` for fixed types and `1 <= max_length <= kMaxVarcharLength` for `kVarchar`.
- **Design Trade-offs:** Returning `Result<Column>` from `Column::Create` enforces valid column objects by construction (making invalid `Column`s impossible to build!).
- **Common Mistakes:** Allowing `VARCHAR(0)` or unvalidated empty column names.
- **Understanding Checkpoint:** Why must `Column::Create` cap `VARCHAR(n)` max length below `4096` bytes in TinyDB v1?

### Component Context
- **Why It Exists:** Building block of `Schema` and `CatalogManager` metadata.
- **Module Relationships:** Composed inside `Schema` (`DAY-054`) and persisted by `CatalogManager` (Phase 03).

### Files Expected to Change
- `include/tinydb/storage/column.hpp` (new)
- `src/storage/column.cpp` (new)
- `tests/unit/storage/column_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Column` with `Create(...)`, accessors (`Name()`, `Type()`, `MaxLength()`, `IsInlined()`, `ValidateValue(const Value&)`), and `operator==`.
2. Add unit tests for all 4 types, invalid names, `VARCHAR(0)`, and `VARCHAR(> kMaxVarcharLength)`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Invalid column specs fail via `Result<Column>`; valid columns report exact size/type metadata.

### Definition of Done
`Column` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-053: implement validated Column metadata class`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-054` (`Schema` class).
````

---

## DAY-054 — Implement `Schema` Class (Column Lookup, Duplicate Checks, Page-Fit Validation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-054
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Schema` Class (Column Lookup, Duplicate Checks, Page-Fit Validation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-053 complete.

### Objective
Implement `tinydb::Schema` (`include/tinydb/storage/schema.hpp`, `src/storage/schema.cpp`) managing ordered `Column` lists, case-normalized duplicate-column rejection, O(1)/O(N) column index lookup (`GetColumnIndex`), max tuple byte estimation, and minimum-tuple page-fit check.

### Concepts & Daily Learning
- **Concept:** Table schemas, column name uniqueness, physical row width bounds, and schema projection.
- **Why Database Engines Need It:** Creating a table with two columns named `id` or 2,000 `BIGINT` columns that cannot fit into a 4096-byte page must fail at schema creation rather than failing mysteriously at `INSERT` time.
- **How It Works:** `Schema::Create(std::vector<Column>)` checks non-empty columns (`<= config::kMaxColumnsPerTable`, e.g., 64), rejects duplicate normalized column names, verifies minimum tuple serialized size fits within max slotted-page payload (`~4040` bytes), and indexes `name -> col_idx`.
- **Design Trade-offs:** Checking *minimum* serialized size (or *worst-case* fixed + varlen max? Checking fixed columns fit is essential, and documenting what happens if a row's actual `VARCHAR` sum exceeds a single page's max payload!).
- **Common Mistakes:** Allowing zero-column schemas or duplicate column names (`id`, `ID`)!
- **Understanding Checkpoint:** Why does `Schema::Create` check duplicate column names case-insensitively (`id` vs `ID`)?

### Component Context
- **Why It Exists:** Authoritative descriptor for tuple layout, catalog tables, and query outputs.
- **Module Relationships:** Used by `Tuple` (`DAY-055`), `TableHeap`, `CatalogManager`, `Planner`, and `ProjectionExecutor`.

### Files Expected to Change
- `include/tinydb/storage/schema.hpp` (new)
- `src/storage/schema.cpp` (new)
- `tests/unit/storage/schema_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Schema::Create(std::vector<Column>)` and `Schema::SubSchema(std::span<const std::size_t> col_indices)`.
2. Implement `ColumnCount()`, `GetColumn(idx)`, `GetColumnIndex(name)`, `ValidateValues(std::span<const Value>)`, and `MinSerializedSize()`.
3. Test valid schemas, duplicate column rejection, empty schema rejection, `GetColumnIndex` hits/misses, and oversized fixed-column schema rejection.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Duplicate names (`id` and `ID`) and impossible schemas (`> max payload`) are rejected cleanly.

### Definition of Done
`Schema` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-054: implement Schema with duplicate checks and column lookup`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-055` (`Tuple` representation).
````

---

## DAY-055 — Implement `Tuple` Representation and Schema-Checked Construction

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-055
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Tuple` Representation and Schema-Checked Construction
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-054 complete.

### Objective
Implement `tinydb::Tuple` (`include/tinydb/storage/tuple.hpp`, `src/storage/tuple.cpp`) representing a row of `Value`s validated against a `Schema`, with explicit `Tuple::Create(schema, values)`, `GetValue(idx)`, `SetValue(idx, val)`, and `ColumnCount()`.

### Concepts & Daily Learning
- **Concept:** Typed row records vs raw byte blobs, and construction-time schema validation.
- **Why Database Engines Need It:** Separating structured `Tuple` (`std::vector<Value>`) from its compact binary wire/disk encoding (`DAY-056`) makes expression evaluation clean while keeping disk layout tight.
- **How It Works:** `Tuple::Create(const Schema& schema, std::vector<Value> values)` checks `values.size() == schema.ColumnCount()` and checks each value's type and `VARCHAR(n)` max length against `schema.GetColumn(i)`.
- **Design Trade-offs:** Storing `std::vector<Value>` inside `Tuple` in memory is slightly heavier than raw serialized bytes, while greatly simplifying `ProjectionExecutor`, `FilterExecutor`, and `UpdateExecutor` in Phase 07!
- **Common Mistakes:** Allowing `Tuple` construction with wrong column count or `VARCHAR` exceeding column limit.
- **Understanding Checkpoint:** Why does validating `Tuple` values against `Schema` at `Tuple::Create` time eliminate redundant type checks in lower storage layers?

### Component Context
- **Why It Exists:** In-memory row abstraction passed between `TableHeap`, `Executor`s, and `TupleCodec`.
- **Module Relationships:** Serialized/deserialized via `DAY-056`–`DAY-057`, stored in `SlottedPage` via `TableHeap`.

### Files Expected to Change
- `include/tinydb/storage/tuple.hpp` (new)
- `src/storage/tuple.cpp` (new)
- `tests/unit/storage/tuple_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Tuple::Create(const Schema&, std::vector<Value>)`, `Values()`, `GetValue(idx)`, `Size()`, `operator==`.
2. Add tests checking valid construction, wrong column count rejection, wrong column type rejection, and `VARCHAR(n)` overflow rejection.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Schema mismatches fail cleanly with `StatusCode::kInvalidArgument`.

### Definition of Done
`Tuple` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-055: implement schema-validated Tuple representation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-056` (`Tuple` explicit serialization).
````

---

## DAY-056 — Implement Explicit `Tuple` Serialization Format

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-056
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement Explicit `Tuple` Serialization Format
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-008 (`endian.hpp`), DAY-055 complete.

### Objective
Design and implement explicit binary `TupleCodec::Serialize(const Schema&, const Tuple&)` returning `Result<ByteBuffer>` (or writing to `std::vector<std::byte>`), with overflow-checked payload sizing (`<= kMaxTupleBytes`).

### Concepts & Daily Learning
- **Concept:** Compact binary row encoding (`col_count` or schema-driven fixed fields + length-prefixed `VARCHAR`s) and overflow-safe size calculation.
- **Why Database Engines Need It:** Saving `INT` as ASCII `"1234567"` takes 7+ bytes and delimiter parsing; binary little-endian encoding takes 4 bytes fixed, zero parsing overhead, and exact byte lengths.
- **How It Works:** Encode `uint16_t col_count` (or schema-implied; storing `uint16_t col_count` enables quick structural sanity checks on decode!), followed sequentially by each field in schema order: `INT` (4B `i32`), `BIGINT` (8B `i64`), `BOOL` (1B `0/1`), `VARCHAR` (`uint16_t len` + raw bytes). Compute total serialized size beforehand with `uint32_t` overflow checks (`<= kMaxTupleBytes`).
- **Design Trade-offs:** Storing fixed columns sequentially + inline `uint16_t` length-prefixed `VARCHAR`s (vs separate variable-length offset table at front of tuple): sequential layout is crystal clear and requires zero random-access offset patching in v1, while `O(num_cols)` decode fits TinyDB's `<= 64` columns comfortably.
- **Common Mistakes:** Letting `total_size` exceed `uint16_t` max slotted-page payload without returning an explicit `kInvalidArgument` / `kOutOfRange` error.
- **Understanding Checkpoint:** Given `(INT, BOOL, VARCHAR(20))` holding `(42, true, "alice")`, trace the exact byte-by-byte serialized representation and its total byte length.

### Component Context
- **Why It Exists:** Converts structured `Tuple`s into compact byte payloads stored in `SlottedPage`.
- **Module Relationships:** Paired with `TupleCodec::Deserialize` (`DAY-057`) and `SlottedPage` (`DAY-063`).

### Files Expected to Change
- `include/tinydb/storage/tuple_codec.hpp` (new)
- `src/storage/tuple_codec.cpp` (new)
- `docs/storage-format.md` (document exact Tuple binary format)
- `tests/unit/storage/tuple_codec_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TupleCodec::SerializedSize(const Schema&, const Tuple&)` and `TupleCodec::Serialize(...)`.
2. Enforce `Tuple` matches `Schema` and `SerializedSize <= config::kMaxTupleBytes` (leaving room for `PageHeader + SlottedPageHeader + 1 Slot`).
3. Verify exact byte output for sample tuples including empty `""` `VARCHAR`, binary-like strings with embedded `\0`, and negative `INT`/`BIGINT`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Exact serialized byte sequences match `docs/storage-format.md`; oversized multi-`VARCHAR` tuples exceeding 1 page fail cleanly before writing.

### Definition of Done
Serializer implemented, documented, tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-056: implement explicit Tuple binary serialization`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-057` (`Tuple` deserialization & boundary validation).
````

---

## DAY-057 — Implement Explicit `Tuple` Deserialization with Overrun and Truncation Validation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-057
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement Explicit `Tuple` Deserialization with Overrun and Truncation Validation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-056 complete.

### Objective
Implement `Result<Tuple> TupleCodec::Deserialize(const Schema&, std::span<const std::byte>)` validating exact `col_count`, non-truncated reads, valid `BOOL` bytes (`0` or `1` only!), `VARCHAR` length `<= column.MaxLength()`, and exact end-of-buffer consumption (no trailing garbage bytes!).

### Concepts & Daily Learning
- **Concept:** Defensive deserialization of variable-length records against crafted/corrupt length prefixes.
- **Why Database Engines Need It:** A corrupted `VARCHAR` length prefix (`0xFFFF`) without bounds validation causes an out-of-bounds read or gigantic `std::string` allocation DoS!
- **How It Works:** Read `col_count`, check `col_count == schema.ColumnCount()`, decode each column via `BinaryReader`, reject `bool_byte > 1`, reject `varchar_len > col.MaxLength()` or `varchar_len > reader.Remaining()`, and verify `reader.Remaining() == 0` at the end.
- **Design Trade-offs:** Checking `reader.Remaining() == 0` rejects both truncated AND bloated corrupt payloads.
- **Common Mistakes:** Allocating `std::string(len, '\0')` from an untrusted `len` BEFORE checking `len <= reader.Remaining()`!
- **Understanding Checkpoint:** Why must `TupleCodec::Deserialize` check `len <= reader.Remaining()` AND `len <= col.MaxLength()` BEFORE resizing a string buffer?

### Component Context
- **Why It Exists:** Safe disk-to-memory decoder for table rows.
- **Module Relationships:** Used by `TableHeap::GetTuple` and `TableIterator`.

### Files Expected to Change
- `include/tinydb/storage/tuple_codec.hpp`
- `src/storage/tuple_codec.cpp`
- `tests/unit/storage/tuple_codec_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TupleCodec::Deserialize`.
2. Write comprehensive tests: full roundtrips (`INT`, `BIGINT`, `BOOL`, `VARCHAR`), truncated at every possible byte index `[0..size-1]`, trailing extra byte, `bool_byte = 2`, forged `varchar_len = 65535`, and wrong `col_count`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Truncating at ANY byte position `[0..size-1]` fails with `StatusCode::kCorruption` without crashes or huge allocations.

### Definition of Done
Deserializer hardened and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-057: implement defensive Tuple deserialization with bounds validation`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-058` (`Tuple` comparison & projection helpers).
````

---

## DAY-058 — Implement `Tuple` Comparison, Projection Helpers, and Roundtrip Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-058
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `Tuple` Comparison, Projection Helpers, and Roundtrip Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-057 complete.

### Objective
Add `Tuple::Project(const Schema& full_schema, const std::vector<std::size_t>& col_indices, const Schema& out_schema)` and lexicographical `Tuple::Compare` / `TupleCodec` roundtrip stress tests across multi-column mixed schemas.

### Concepts & Daily Learning
- **Concept:** Logical projection (`SELECT a, c`) and column-index lookup efficiency.
- **Why Database Engines Need It:** `ProjectionExecutor` (Phase 07) strips unused columns cleanly using precomputed column indices, and tests need deterministic tuple equality/ordering checks.
- **How It Works:** `Project` validates indices `< full_schema.ColumnCount()`, extracts values in index order, and returns `Tuple::Create(out_schema, projected_values)`.
- **Design Trade-offs:** Precomputing `col_indices` once in `Init()` rather than resolving column names per tuple keeps row iteration `O(out_cols)`.
- **Common Mistakes:** Passing mismatched `col_indices` and `out_schema` without validating them.
- **Understanding Checkpoint:** Why is resolving column names to integer offsets once in `Init()` crucial during a 100,000-row scan?

### Component Context
- **Why It Exists:** Completes the logical `Tuple` toolkit before slotted-page physical storage.
- **Module Relationships:** Reused directly by `ProjectionExecutor` and `IndexScanExecutor`.

### Files Expected to Change
- `include/tinydb/storage/tuple.hpp`
- `src/storage/tuple.cpp`
- `tests/unit/storage/tuple_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Tuple::Project` and `Tuple::Compare` (with `Result<int>`).
2. Add roundtrip and projection unit tests including boundary strings (`kMaxVarcharLength`), negative integers, and invalid index rejections.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Projected tuples validate cleanly against projected schemas; invalid indices fail cleanly.

### Definition of Done
Tuple helpers and tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-058: add Tuple projection, comparison, and roundtrip tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-059` (`RowID` structure).
````

---

## DAY-059 — Implement `RowID` (`PageId`, `SlotId`), Sentinels, Serialization, and Hashing

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-059
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `RowID` (`PageId`, `SlotId`), Sentinels, Serialization, and Hashing
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-007 (`types.hpp`), DAY-008 (`endian.hpp`), DAY-058 complete.

### Objective
Implement `tinydb::RowID` (`include/tinydb/storage/row_id.hpp`, `src/storage/row_id.cpp`) containing `page_id` (`PageId`, 4B) and `slot_id` (`SlotId`, 2B) — 6 bytes serialized explicitly — plus `kInvalidRowId`, `IsValid()`, total ordering (`<=>` for B+ Tree composite `(key, RowID)`!), `std::hash<RowID>`, and `ToString()`.

### Concepts & Daily Learning
- **Concept:** Physical tuple addresses (`RID` = `(page_id, slot_id)`), C++20 spaceship operator `<=>`, and 6-byte vs 8-byte serialized representation.
- **Why Database Engines Need It:** B+ Tree leaf nodes map keys to `RowID`s rather than duplicating entire tuples; keeping `RowID` compact (6 bytes serialized) maximizes B+ Tree leaf capacity!
- **How It Works:** `RowID` has `PageId page_id = kInvalidPageId`, `SlotId slot_id = kInvalidSlotId`. `IsValid()` checks `IsDataPageId(page_id) && slot_id != kInvalidSlotId`. `Serialize` encodes 4B `page_id` + 2B `slot_id` (`kSerializedSize = 6`).
- **Design Trade-offs:** Keeping `RowID` 6 bytes on disk saves 25% space inside B+ Tree leaf entries compared to 8 bytes!
- **Common Mistakes:** Using `sizeof(RowID)` (which is 8 bytes in RAM due to struct padding!) instead of `RowID::kSerializedSize == 6` on disk!
- **Understanding Checkpoint:** Why is `sizeof(tinydb::RowID)` in memory `8` bytes on 64-bit/32-bit aligned architectures even though its serialized size is `6` bytes?

### Component Context
- **Why It Exists:** Universal physical address of every tuple across `TableHeap`, `BPlusTree`, and `DeleteExecutor`.
- **Module Relationships:** Returned by `TableHeap::InsertTuple`, indexed in `BPlusTree` (`Phase 05`).

### Files Expected to Change
- `include/tinydb/storage/row_id.hpp` (new)
- `src/storage/row_id.cpp` (new)
- `tests/unit/storage/row_id_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `RowID` with `<=>`, `IsValid()`, `Serialize`, `Deserialize`, `ToString()`, and `std::hash<tinydb::RowID>`.
2. Write unit tests verifying exact 6-byte serialization, `sizeof` vs `kSerializedSize` distinction, ordering, hashing in `std::unordered_set`, and invalid sentinel handling.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `RowID` serializes to exactly 6 bytes and deserializes with full validation; `<=>` provides strict weak ordering.

### Definition of Done
`RowID` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-059: implement RowID with 6-byte serialization and ordering`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-060` (Slotted page layout design).
````

---

## DAY-060 — Design the 4096-Byte Slotted Page Layout and Slot Directory Specification

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-060
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Design the 4096-Byte Slotted Page Layout and Slot Directory Specification
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-025 (`PageHeader`), DAY-059 complete.

### Objective
Design the exact byte layout of `SlottedPage` inside a 4096-byte `PageType::kTableHeap` page (`include/tinydb/storage/slotted_page.hpp`, `docs/storage-format.md`): `[0..31]` `PageHeader`, `[32..39]` `SlottedPageHeader` (`slot_count`, `free_space_start`, `free_space_end`, `deleted_count`), `[40..free_space_start)` `Slot` array growing forward, `[free_space_end..4096)` tuple payloads growing backward.

### Concepts & Daily Learning
- **Concept:** Slotted-page architecture (PAX/NSM row-store layout) separating logical `slot_id` indirection from physical byte offsets inside the page.
- **Why Database Engines Need It:** Variable-length `VARCHAR` tuples cannot live at fixed offsets; if deleting or updating a record shifted subsequent records without a slot directory, every B+ Tree `RowID` pointing to that page would break!
- **How It Works:** A `Slot` (6 bytes: `uint16_t offset`, `uint16_t length`, `uint8_t flags`, `uint8_t reserved`—or 5/6 bytes) maps `slot_id -> byte_offset` inside `[40..4096)`. Compacting the page moves tuple payloads toward `4096` and updates `Slot.offset` in place so `slot_id` NEVER changes!
- **Design Trade-offs:** 6-byte aligned slots vs 5-byte unaligned slots: 6 bytes (`offset`, `length`, `flags`, `reserved`) keeps serialization simple and explicit while leaving `4056` payload+slot bytes!
- **Common Mistakes:** Letting `free_space_start` (growing up) cross `free_space_end` (growing down); forgetting that inserting a new tuple requires BOTH `tuple_size` AND `sizeof_serialized_slot` (`6` bytes) unless reusing a deleted slot!
- **Understanding Checkpoint:** Why can `SlottedPage::Compact()` defragment fragmented deleted gaps inside a page without invalidating any existing `RowID` stored in a B+ Tree?

### Component Context
- **Why It Exists:** Core intra-page variable-length record manager for TinyDB tables.
- **Module Relationships:** Wraps a `Page`; driven by `TableHeap`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp` (new — layout constants, structs, API declaration)
- `docs/storage-format.md` (Section 4 exact SlottedPage byte map)
- `tests/unit/storage/slotted_page_layout_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Define `SlottedPageHeader` (`kSerializedSize = 8`: `u16 slot_count`, `u16 free_space_start`, `u16 free_space_end`, `u16 deleted_count`) and `SlotEntry` (`kSerializedSize = 6`: `u16 offset`, `u16 length`, `u8 flags` (`kLive`, `kDeleted`), `u8 reserved`).
2. Compute exact `kMaxSingleTupleBytes = 4096 - 32 - 8 - 6 = 4050` bytes and align `config::kMaxTupleBytes` if needed.
3. Update `docs/storage-format.md` and write layout constant tests.

### Testing Instructions
- Build and run `ctest --output-on-failure`.

### Acceptance Criteria
- Exact byte constants (`32 + 8 + 6 + kMaxSingleTupleBytes == 4096`) compile and pass layout assertions.

### Definition of Done
Layout finalized and documented, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-060: define 4KB SlottedPage header and slot directory layout`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-061` (`SlottedPage` header & slot serialization).
````

---

## DAY-061 — Implement `SlottedPage` Initialization, Header, and `SlotEntry` Serialization

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-061
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage` Initialization, Header, and `SlotEntry` Serialization
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-060 complete.

### Objective
Implement `SlottedPage` view wrapper (`src/storage/slotted_page.cpp`) with `Init(Page&, PageId)`, `ReadHeader()`, `WriteHeader()`, `ReadSlot(SlotId)`, `WriteSlot(SlotId, SlotEntry)`, and structural invariant verification (`ValidateStructure()`).

### Concepts & Daily Learning
- **Concept:** Overlay view wrapper vs owning class (`SlottedPage` borrows `Page&`), explicit slot-directory read/write without struct-overlay UB.
- **Why Database Engines Need It:** Rather than casting `reinterpret_cast<SlottedPage*>(page.Data())` (which violates strict aliasing and alignment rules!), explicit `ReadSlot`/`WriteSlot` via `endian.hpp` keeps everything portable and UBSan-clean.
- **How It Works:** `Init` writes `PageHeader(page_id, PageType::kTableHeap)`, writes `SlottedPageHeader{0, 40, 4096, 0}` at `[32..39]`, and zeroes `[40..4095]`. `ReadSlot(slot_id)` checks `slot_id < slot_count` and reads 6 bytes at `40 + slot_id * 6`.
- **Design Trade-offs:** Reading 8/6 header/slot bytes via `endian.hpp` is negligible CPU cost (~nanoseconds) compared to disk/memory latencies and prevents 100% of alignment/endian UB.
- **Common Mistakes:** Confusing `free_space_start == 40` (beginning of slots) with `0`!
- **Understanding Checkpoint:** Why is `SlottedPage::ValidateStructure()` valuable when loading a `TABLE_HEAP` page from disk?

### Component Context
- **Why It Exists:** Safe read/write primitives for slotted-page metadata.
- **Module Relationships:** Used by `DAY-062`–`DAY-068` (`Insert`, `Get`, `Delete`, `Update`, `Compact`).

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp` (new)
- `tests/unit/storage/slotted_page_header_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `Init`, `ReadSlottedHeader`, `WriteSlottedHeader`, `ReadSlot`, `WriteSlot`, and `ValidateStructure` (checking `40 <= free_space_start <= free_space_end <= 4096`, `free_space_start == 40 + slot_count * 6`, and every live slot lies within `[free_space_end, 4096)` without overlap!).
2. Write unit tests verifying `Init`, slot roundtrip, and `ValidateStructure` catching corrupted `free_space_start`/`free_space_end` or overlapping slots.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- `Init` yields `free_space_start == 40`, `free_space_end == 4096`; corrupted invariants fail `ValidateStructure`.

### Definition of Done
Header/slot codec and validator passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-061: implement SlottedPage header, slot serialization, and structure validator`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-062` (Free-space tracking & overflow-safe checks).
````

---

## DAY-062 — Implement `SlottedPage` Free-Space Tracking and Overflow-Safe Space Checks

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-062
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage` Free-Space Tracking and Overflow-Safe Space Checks
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-061 complete.

### Objective
Implement `ContiguousFreeSpace()`, `ReclaimableFreeSpace()` (total dead space + contiguous space), and `HasSpaceForInsert(std::size_t tuple_bytes)` distinguishing whether a deleted slot can be reused (`tuple_bytes`) vs appending a new slot (`tuple_bytes + 6`).

### Concepts & Daily Learning
- **Concept:** Contiguous free space (`free_space_end - free_space_start`) vs fragmented reclaimable space (`contiguous + sum(deleted_slot_lengths)`).
- **Why Database Engines Need It:** A page might have 1,500 bytes of freed tuple gaps scattered across deleted records while contiguous space is only 200 bytes; tracking BOTH tells `InsertTuple` when `Compact()` will avoid allocating a brand-new page!
- **How It Works:**
  - `ContiguousFreeSpace` = `free_space_end - free_space_start` (checked `free_space_end >= free_space_start`).
  - `TotalAvailableSpace` = `ContiguousFreeSpace + deleted_bytes_total`.
  - Check `tuple_bytes <= kMaxSingleTupleBytes` BEFORE addition to avoid overflow!
- **Design Trade-offs:** Checking whether a deleted `SlotEntry` can be reused (`deleted_count > 0`) saves 6 slot-directory bytes per insert when slots were previously deleted.
- **Common Mistakes:** Checking `ContiguousFreeSpace() >= tuple_bytes` without reserving the 6-byte `SlotEntry` when no deleted slot is available (corrupting the first payload byte!).
- **Understanding Checkpoint:** If `free_space_start == 100`, `free_space_end == 200`, and there are no deleted slots, why must a `98`-byte tuple be rejected?

### Component Context
- **Why It Exists:** Accurate capacity gate preventing page buffer corruption.
- **Module Relationships:** Queried by `InsertTuple` (`DAY-063`), `UpdateTuple` (`DAY-066`), and `TableHeap`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_space_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `ContiguousFreeSpace()`, `ReclaimableFreeSpace()`, and `CanFitTuple(std::size_t bytes, bool has_reusable_slot)` using overflow-safe comparisons.
2. Add boundary tests: empty page max fit (`4050` bytes), `4051` rejection, zero-byte edge case, and tight boundary (`space == bytes + 5` fails, `space == bytes + 6` passes).

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Exact 1-byte boundaries pass (`4050` fits on empty page, `4051` rejected; `space == tuple + 5` rejected without reusable slot).

### Definition of Done
Free-space helpers and boundary tests passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-062: implement overflow-safe SlottedPage free-space accounting`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-063` (`SlottedPage::InsertTuple`).
````

---

## DAY-063 — Implement `SlottedPage::InsertTuple` (Slot Reuse, Append, and Boundary Protection)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-063
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage::InsertTuple` (Slot Reuse, Append, and Boundary Protection)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-062 complete.

### Objective
Implement `Result<SlotId> SlottedPage::InsertTuple(Page& page, std::span<const std::byte> tuple_bytes)` allocating payload space from `free_space_end` downward, assigning either a new `SlotId` or reusing a deleted slot cleanly (or triggering compaction when fragmented space suffices!).

### Concepts & Daily Learning
- **Concept:** Backward payload allocation, slot-directory growth, and transparent intra-page compaction on insert when fragmented dead space exists.
- **Why Database Engines Need It:** Inserting until a page fills, deleting half the rows, and inserting again must reclaim dead space without leaking holes forever.
- **How It Works:** Check `tuple_bytes.size() > 0 && <= kMaxSingleTupleBytes`. Find first deleted slot if any (Wait: careful! In a slotted page, wait—can reusing a deleted `slot_id` affect a sequential scan cursor or old stale `RowID`? In Phase 02 without MVCC, reusing a deleted slot inside the SAME page after `MarkDelete` is fine as long as deleted B+ Tree entries were removed first! Even simpler: track `deleted_count` and scan slots or append cleanly—let's document slot-reuse semantics clearly!). If contiguous space is insufficient while total reclaimable space suffices, call `Compact()` (stubbed until `DAY-067` or implemented cleanly once `DAY-067` arrives), otherwise return `StatusCode::kResourceExhausted`.
- **Design Trade-offs:** Returning `StatusCode::kResourceExhausted` cleanly tells `TableHeap` to advance to the next page or allocate a new page.
- **Common Mistakes:** Copying tuple bytes BEFORE updating `free_space_end` and checking `new_end >= new_start`!
- **Understanding Checkpoint:** Walk through exact pointer/offset updates (`free_space_start`, `free_space_end`, `SlotEntry.offset`, `SlotEntry.length`) when inserting a 100-byte tuple into a fresh page.

### Component Context
- **Why It Exists:** Places serialized tuples physically onto a 4096-byte page.
- **Module Relationships:** Called by `TableHeap::InsertTuple`; paired with `GetTuple` (`DAY-064`).

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_ops_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `SlottedPage::InsertTuple` (appending slots cleanly when no compaction needed; returning `kResourceExhausted` when page is full).
2. Add unit tests verifying sequential inserts (`slot_id` `0, 1, 2...`), exact full-page pack, full-page rejection without mutating header, and `ValidateStructure()` passing after every insert!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Failed inserts leave the page byte-for-byte unchanged (`ValidateStructure()` passes).

### Definition of Done
`InsertTuple` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-063: implement SlottedPage::InsertTuple with bounds protection`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-064` (`SlottedPage::GetTuple`).
````

---

## DAY-064 — Implement `SlottedPage::GetTuple` (Bounds Check, Tombstone Rejection, Safe Copy)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-064
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage::GetTuple` (Bounds Check, Tombstone Rejection, Safe Copy)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-063 complete.

### Objective
Implement `Result<ByteBuffer> SlottedPage::GetTuple(const Page& page, SlotId slot_id)` and zero-copy `Result<std::span<const std::byte>> SlottedPage::GetTupleView(const Page& page, SlotId slot_id)` verifying `slot_id < slot_count`, live slot status (not deleted/tombstoned), and `[offset, offset + length) <= 4096`.

### Concepts & Daily Learning
- **Concept:** Validating indirect offsets from slot tables before returning byte slices, and tombstone semantics.
- **Why Database Engines Need It:** A corrupt slot entry (`offset = 4090, length = 50`) would read past the 4096-byte page boundary into adjacent heap memory unless explicitly bounds-checked!
- **How It Works:** Read `SlottedPageHeader`, verify `slot_id < slot_count`, read `SlotEntry`, if `flags == kDeleted` return `StatusCode::kNotFound`, verify `offset >= free_space_end` and `offset + length <= 4096` (using overflow-safe subtraction `length <= 4096 - offset`), and return span/copy.
- **Design Trade-offs:** Providing `GetTupleView` (zero-copy `span` valid while `Page` stays pinned) alongside `GetTuple` (`ByteBuffer` owned copy) gives both speed and lifetime flexibility.
- **Common Mistakes:** Checking `offset + length <= 4096` in `uint16_t` (`offset=60000, length=6000` wraps around 65536!).
- **Understanding Checkpoint:** Why is checking `length <= 4096U - offset` strictly safer than `offset + length <= 4096U`?

### Component Context
- **Why It Exists:** Reads tuple bytes safely out of `SlottedPage`s.
- **Module Relationships:** Used by `TableHeap::GetTuple`, `IndexScanExecutor`, and `TableIterator`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_ops_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `GetTuple` and `GetTupleView` plus `SlotCount(const Page&)`.
2. Add tests verifying exact byte retrieval across multiple inserts, out-of-range `slot_id`, and forged corrupt `SlotEntry` (`offset + length > 4096`) rejection.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Inserted tuples read back byte-for-byte identical; invalid slots and corrupt slot offsets return explicit errors.

### Definition of Done
`GetTuple` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-064: implement SlottedPage::GetTuple with tombstone and range checks`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-065` (`SlottedPage::MarkDelete`).
````

---

## DAY-065 — Implement `SlottedPage::MarkDelete` and Physical Tombstone Semantics

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-065
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage::MarkDelete` and Physical Tombstone Semantics
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-064 complete.

### Objective
Implement `Status SlottedPage::DeleteTuple(Page& page, SlotId slot_id)` (marking `SlotEntry::flags = kDeleted`, zeroing deleted payload bytes or keeping length for accounting cleanly, incrementing `deleted_count`, and ensuring deleted tuples return `kNotFound` on `GetTuple` and cannot be double-deleted).

### Concepts & Daily Learning
- **Concept:** Tombstone deletion inside slotted pages vs immediate slot-array erasure.
- **Why Database Engines Need It:** If deleting slot `1` shifted slots `[2, 3, 4]` down to `[1, 2, 3]`, every B+ Tree index entry storing `RowID(page, 2)` would suddenly point to the wrong tuple!
- **How It Works:** Keep `slot_count` unchanged while flagging `slot.flags = kDeleted`, zeroing the dead tuple's payload bytes (for privacy/determinism, keeping `slot.length` or setting `length = 0`—WAIT: keeping `length` or wiping payload bytes `[offset..offset+length)` while setting `length = 0` after reclaiming at compaction! Setting `length` to dead length vs `0`? Wait: if `Compact()` simply scans all `kLive` slots and repacks them from `4096` downward, then ANY dead bytes between `free_space_end` and `4096` are automatically reclaimed by `Compact()` even if we set `slot.length = 0` and `slot.offset = 0` upon delete! That's super clean because `free_space_end` still marks the payload floor and `Compact()` ignores `kDeleted` slots completely!).
- **Design Trade-offs:** Zeroing deleted payload bytes + setting `flags = kDeleted` preserves `slot_id` stability (`RowID`s of neighbors unchanged) while wiping deleted data immediately.
- **Common Mistakes:** Decrementing `slot_count` when deleting a middle slot; failing to guard against deleting an already-deleted slot (`deleted_count` corruption!).
- **Understanding Checkpoint:** Why must deleting slot `0` in a 3-slot page leave slots `1` and `2` at indices `1` and `2` instead of shifting the slot array?

### Component Context
- **Why It Exists:** Safe record removal preserving neighbor `RowID` stability.
- **Module Relationships:** Driven by `TableHeap::DeleteTuple` and `DeleteExecutor`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_ops_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `SlottedPage::DeleteTuple(Page&, SlotId)` and `IsSlotLive(const Page&, SlotId)`.
2. Test: insert 3 tuples -> delete slot `1` -> `GetTuple(1)` returns `kNotFound`, `GetTuple(0)` and `GetTuple(2)` still return exact original tuples! Double-delete on slot `1` returns `kNotFound`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Neighbor `slot_id`s stay valid; double-delete fails cleanly; `ValidateStructure()` passes.

### Definition of Done
Delete semantics tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-065: implement SlottedPage::DeleteTuple preserving neighbor SlotIds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-066` (`SlottedPage::UpdateTuple`).
````

---

## DAY-066 — Implement `SlottedPage::UpdateTuple` (In-Place Fit vs Relocated Update)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-066
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage::UpdateTuple` (In-Place Fit vs Relocated Update)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-065 complete.

### Objective
Implement `Status SlottedPage::UpdateTuple(Page& page, SlotId slot_id, std::span<const std::byte> new_bytes)` updating a live slot in place when `new_bytes.size() <= old_length` (preserving `slot_id` AND zeroing leftover trailing bytes!), or allocating fresh intra-page space when `new_bytes.size() > old_length` (returning `kResourceExhausted` if the page cannot fit the larger tuple even after freeing the old payload).

### Concepts & Daily Learning
- **Concept:** In-place update (`new_len <= old_len`) vs intra-page relocation (`new_len > old_len`) vs cross-page relocation (handled by `TableHeap` in `DAY-073`).
- **Why Database Engines Need It:** `UPDATE users SET age = 26` keeps exact same size (`in-place` = O(1), zero fragmentation!); changing a `VARCHAR` from `"Al"` to `"Alexander"` grows the tuple beyond its old space.
- **How It Works:** Verify slot is live and `new_bytes.size() <= kMaxSingleTupleBytes`.
  - Case 1 (`new_len <= old_len`): overwrite at `slot.offset`, zero `[offset + new_len, offset + old_len)`, update `slot.length = new_len`.
  - Case 2 (`new_len > old_len`): check if `ContiguousFreeSpace() >= new_len` (no new slot needed!). If yes, zero old bytes, copy `new_bytes` at `free_space_end - new_len`, update `slot.offset` and `slot.length`—`slot_id` STILL stays intact! If not, return `kResourceExhausted` (or compact first on `DAY-067`!).
- **Design Trade-offs:** Leaving old bytes dead until compaction preserves `slot_id` (`RowID`) without moving neighboring tuples, saving index updates when the expanded tuple still fits on the same page!
- **Common Mistakes:** Leaving stale tail bytes in `[offset + new_len, offset + old_len)` when shrinking an entry; updating `slot.length` before checking space.
- **Understanding Checkpoint:** Why can an in-place OR same-page relocated `UpdateTuple` keep `RowID(page_id, slot_id)` completely unchanged, avoiding B+ Tree updates when non-indexed columns change?

### Component Context
- **Why It Exists:** Enables efficient `UPDATE` inside a single slotted page.
- **Module Relationships:** Used by `TableHeap::UpdateTuple`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_ops_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `SlottedPage::UpdateTuple`.
2. Test: equal-size update, shrinking update, growing update (relocating inside same page while `slot_id` stays constant!), update on deleted slot (`kNotFound`), and oversized update returning `kResourceExhausted` without corrupting old tuple!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Failed update (`kResourceExhausted`) leaves original tuple 100% intact.

### Definition of Done
`UpdateTuple` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-066: implement SlottedPage::UpdateTuple for in-place and intra-page growth`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-067` (Page compaction).
````

---

## DAY-067 — Implement `SlottedPage::Compact` (Defragmenting Dead Space While Preserving `SlotId`s)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-067
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage::Compact` (Defragmenting Dead Space While Preserving `SlotId`s)
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-066 complete.

### Objective
Implement `Status SlottedPage::Compact(Page& page)` defragmenting all dead gaps left by deletions and shrunk/relocated updates so live tuple payloads pack tightly against byte `4096` with `ContiguousFreeSpace()` restored—while preserving EVERY live `SlotId`—and wire `Compact()` cleanly into `InsertTuple` / `UpdateTuple`!

### Concepts & Daily Learning
- **Concept:** Physical intra-page garbage collection / defragmentation with invariant `SlotId` preservation.
- **Why Database Engines Need It:** Without compaction, repeated updates/deletes turn a 4096-byte page into Swiss cheese where `ContiguousFreeSpace()` approaches `0` even though live tuples occupy only 500 bytes!
- **How It Works:** Copy live tuples (or build a fresh 4064B scratch payload buffer preserving `PageHeader`), reset write cursor `end = 4096`, iterate `slot_id = 0 .. slot_count - 1`: for each `kLive` slot, copy its `length` bytes downward to `[end - length, end)`, update `slot.offset = end - length`, `end -= length`. Zero `[free_space_start, end)`, update `free_space_end = end`. Also support optional slot reuse in `InsertTuple` AFTER `DeleteTuple` and `Compact`!
- **Design Trade-offs:** Using a 4KB local scratch buffer during `Compact()` is trivial, bug-free (no overlapping `memmove` ordering hazards!), and runs infrequently only when a fragmented insert/update needs space.
- **Common Mistakes:** Overwriting an adjacent live tuple during in-place compaction because tuples weren't sorted by physical offset! (A 4KB scratch payload completely eliminates overlap bugs!)
- **Understanding Checkpoint:** Why is using a temporary 4096-byte buffer during `Compact()` immune to overlapping-copy bugs regardless of what order live slots are physically laid out in the page?

### Component Context
- **Why It Exists:** Reclaims dead intra-page fragments without changing external `RowID`s.
- **Module Relationships:** Called automatically on fragmented `InsertTuple`/`UpdateTuple` inside `SlottedPage`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_compact_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `SlottedPage::Compact(Page& page)`.
2. Integrate `Compact` fallback into `InsertTuple` and `UpdateTuple` when `ContiguousFreeSpace()` is too small but reclaiming fragmented dead bytes makes the operation fit!
3. Test: fill page -> delete alternating slots (checkerboard fragmentation!) -> insert a large tuple that previously wouldn't fit contiguously -> verify compaction happens, large tuple succeeds, AND every surviving tuple at its original `SlotId` still matches!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Checkerboard fragmentation reclaims 100% of dead payload bytes while keeping all live `SlotId`s and data intact.

### Definition of Done
Compaction implemented, integrated, and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-067: implement SlottedPage::Compact defragmentation preserving SlotIds`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-068` (`SlottedPage` fuzz & edge-case tests).
````

---

## DAY-068 — Implement `SlottedPage` Randomized Fuzz and Edge-Case Tests

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-068
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `SlottedPage` Randomized Fuzz and Edge-Case Tests
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-067 complete.

### Objective
Write deterministic seeded randomized property/oracle tests (`tests/unit/storage/slotted_page_fuzz_test.cpp`) executing 2,000+ random `Insert`, `Get`, `Update` (smaller & larger), `Delete`, and `Compact` operations on a `SlottedPage`, verifying `ValidateStructure()` and golden `std::map<SlotId, std::vector<std::byte>>` equivalence after every single step.

### Concepts & Daily Learning
- **Concept:** Stateful property-based fuzz testing against a pure reference model.
- **Why Database Engines Need It:** `Compact` + `Update` + `Delete` + slot-reuse interactions create corner cases (like updating the very last live tuple after 20 deletes) that manual tests often miss.
- **How It Works:** Seeded PRNG chooses variable tuple sizes (`1` to `600` bytes) and mutations; any `kResourceExhausted` must keep state identical to the golden map; `ValidateStructure()` runs after every mutating op.
- **Design Trade-offs:** Testing `SlottedPage` exhaustively in isolation makes `TableHeap` bugs 10x easier to isolate later.
- **Common Mistakes:** Only testing fixed-size tuples (missing variable-size fragmentation bugs).
- **Understanding Checkpoint:** What invariants must `SlottedPage::ValidateStructure()` check after every single operation in the fuzz loop?

### Component Context
- **Why It Exists:** Hardens `SlottedPage` before building multi-page `TableHeap`.
- **Module Relationships:** Verifies `SlottedPage`.

### Files Expected to Change
- `tests/unit/storage/slotted_page_fuzz_test.cpp` (new)
- `src/storage/slotted_page.cpp` (fix any edge case uncovered)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Write `slotted_page_fuzz_test.cpp` with variable-length payloads, edge sizes (`1`, `4050`), and corrupt-slot rejection tests.
2. Run under Debug + ASan/UBSan.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- 2,000+ randomized ops across multiple seeds pass with zero structure violations or oracle mismatches.

### Definition of Done
Fuzz tests passing under ASan/UBSan, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-068: add oracle-verified randomized SlottedPage fuzz tests`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-069` (`TablePage` doubly-linked page chaining).
````

---

## DAY-069 — Implement `TablePage` Chaining (`prev_page_id`, `next_page_id`) via `PageHeader`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-069
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `TablePage` Chaining (`prev_page_id`, `next_page_id`) via `PageHeader`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-068 complete.

### Objective
Add doubly-linked `TablePage` chain navigation helpers (`GetNextPageId`, `SetNextPageId`, `GetPrevPageId`, `SetPrevPageId`) on `SlottedPage` (using `PageHeader::next_page_id` and `PageHeader::prev_page_id`) and verify multi-page linked integrity.

### Concepts & Daily Learning
- **Concept:** Doubly-linked page heaps connecting multiple `TABLE_HEAP` pages belonging to a single table.
- **Why Database Engines Need It:** Multiple tables share the same `.tinydb` file; a table's pages are not guaranteed to be contiguous `PageId`s (`1, 2, 3`), because inserts across tables interleave allocations (`Table A: 1, 3, 5; Table B: 2, 4`).
- **How It Works:** Every `TABLE_HEAP` page stores `prev_page_id` and `next_page_id` inside its `PageHeader`. A table's metadata stores `first_page_id` (and optionally `last_page_id`), allowing sequential scan to follow `next_page_id` until `kInvalidPageId`.
- **Design Trade-offs:** Intrusive linked list inside `PageHeader` requires zero separate extent directory in v1 while supporting arbitrary interleaved multi-table allocations.
- **Common Mistakes:** Overwriting `PageHeader::next_page_id` without re-serializing `PageHeader` to the `Page` buffer!
- **Understanding Checkpoint:** Why cannot a table's pages simply be scanned by incrementing `page_id + 1` in a multi-table database file?

### Component Context
- **Why It Exists:** Links individual 4KB `SlottedPage`s into a single logical `TableHeap`.
- **Module Relationships:** Used directly by `TableHeap` and `TableIterator`.

### Files Expected to Change
- `include/tinydb/storage/slotted_page.hpp`
- `src/storage/slotted_page.cpp`
- `tests/unit/storage/slotted_page_chain_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `GetNextPageId`, `SetNextPageId`, `GetPrevPageId`, `SetPrevPageId` ensuring `PageType::kTableHeap` is verified and `prev`/`next` pointers update `PageHeader` cleanly without touching `SlottedPageHeader` or tuple bytes!
2. Write tests chaining 4 pages, updating links, and verifying tuple payloads stay untouched.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Linking/unlinking pages preserves all `SlottedPage` tuples and passes `ValidateStructure()`.

### Definition of Done
Page chaining helpers implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-069: add TablePage prev/next page chaining helpers`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-070` (`TableHeap` design & creation).
````

---

## DAY-070 — Design and Implement `TableHeap` Skeleton and First-Page Creation

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-070
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Design and Implement `TableHeap` Skeleton and First-Page Creation
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-041 (`DiskManager::AllocatePage`), DAY-069 complete.

### Objective
Design `tinydb::TableHeap` (`include/tinydb/storage/table_heap.hpp`, `src/storage/table_heap.cpp`) abstracting page access behind a lightweight `PageStore` interface (implemented immediately via `DiskManager` now, and seamlessly swapped to `BufferPoolManager` in Phase 04!) and implement `TableHeap::Create` / `TableHeap::Open`.

### Concepts & Daily Learning
- **Concept:** Dependency inversion (`IPageAccess` / `StorageBackend` adapter) so `TableHeap` doesn't hardcode raw disk writes forever and migrates cleanly to `BufferPoolManager` in Phase 04!
- **Why Database Engines Need It:** In Phase 02, `BufferPoolManager` doesn't exist yet; defining a clean narrow interface (`ReadPage`/`WritePage`/`AllocatePage`/`DeallocatePage` or `Fetch`/`Unpin`) lets `TableHeap` work durably on `DiskManager` today AND plug straight into `BufferPoolManager` on `DAY-103` with minimal churn!
- **How It Works:** `TableHeap::Create(backend, schema)` allocates Page `first_page_id`, initializes it as a `SlottedPage`, writes it back, and records `first_page_id` (to be persisted by `CatalogManager` in Phase 03). `TableHeap::Open(backend, schema, first_page_id)` validates `first_page_id` and Page 1's `kTableHeap` header.
- **Design Trade-offs:** A small virtual/adapter interface (`PageProvider`) adds tiny vtable indirection while cleanly decoupling `TableHeap` from direct `DiskManager` calls.
- **Common Mistakes:** Letting `TableHeap::Create` succeed without persisting the empty `SlottedPage` header to disk (failing on reopen!).
- **Understanding Checkpoint:** Why is decoupling `TableHeap` from raw `DiskManager` via a clean page-provider interface helpful when `BufferPoolManager` arrives in Phase 04?

### Component Context
- **Why It Exists:** Represents one relational table's multi-page physical heap on disk.
- **Module Relationships:** Managed by `CatalogManager` (Phase 03), wrapped with `BufferPoolManager` (Phase 04), queried by `ExecutionEngine` (Phase 07).

### Files Expected to Change
- `include/tinydb/storage/page_provider.hpp` (new — clean interface + `DiskPageProvider` adapter)
- `include/tinydb/storage/table_heap.hpp` (new)
- `src/storage/table_heap.cpp` (new)
- `tests/unit/storage/table_heap_create_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `IPageProvider` and `DiskPageProvider` (forwarding safely to `DiskManager`).
2. Implement `TableHeap::Create` and `TableHeap::Open` + `DropAllPages()` stub/helper for `DROP TABLE`.
3. Write unit tests verifying `Create`, closing `DiskManager`, reopening `DiskManager`, and `TableHeap::Open(first_page_id)`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Newly created `TableHeap` persists cleanly and reopens from `first_page_id` across `DiskManager` restarts.

### Definition of Done
`TableHeap` skeleton and creation tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-070: design TableHeap with IPageProvider abstraction and Create/Open`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-071` (`TableHeap::InsertTuple`).
````

---

## DAY-071 — Implement `TableHeap::InsertTuple` (Multi-Page Traversal and New Page Allocation)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-071
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `TableHeap::InsertTuple` (Multi-Page Traversal and New Page Allocation)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-056 (`TupleCodec`), DAY-070 complete.

### Objective
Implement `Result<RowID> TableHeap::InsertTuple(const Tuple& tuple)` serializing the tuple against the table's `Schema`, scanning existing `TablePage`s (with bounded cycle detection!) for space, or allocating and linking a new `TABLE_HEAP` page when existing pages are full.

### Concepts & Daily Learning
- **Concept:** Multi-page free-space search, tail-page allocation, and doubly-linked list pointer updates on disk.
- **Why Database Engines Need It:** A table with 10,000 rows spans dozens or hundreds of 4KB pages; `InsertTuple` must seamlessly grow the table chain without losing page links!
- **How It Works:** Serialize `tuple` via `TupleCodec::Serialize`. Walk from `first_page_id_` (or track `hint_last_page_id_` in memory to avoid O(N_pages) walks on bulk inserts!): try `SlottedPage::InsertTuple`. If all pages return `kResourceExhausted`, allocate `new_page_id`, `SlottedPage::Init` with `prev_page_id = tail_id`, update `tail.SetNextPageId(new_page_id)` and persist both pages, then insert into `new_page_id` and return `RowID(new_page_id, slot_id)`.
- **Design Trade-offs:** Maintaining an in-memory `last_page_id_` hint makes sequential bulk inserts `O(1)` page reads instead of `O(P)` while still supporting scanning earlier pages or falling back cleanly on restart!
- **Common Mistakes:** Updating `tail.next_page_id` in memory without writing `tail` back to disk! Failing to check for cycles in `next_page_id` on corrupted pages.
- **Understanding Checkpoint:** Why is writing BOTH the newly allocated page AND updating `prev_tail.next_page_id` required to keep the `TableHeap` chain intact after restart?

### Component Context
- **Why It Exists:** Persists arbitrary numbers of tuples across multiple 4KB pages.
- **Module Relationships:** Called by `InsertExecutor` (`Phase 07`); pairs with `GetTuple` (`DAY-072`).

### Files Expected to Change
- `include/tinydb/storage/table_heap.hpp`
- `src/storage/table_heap.cpp`
- `tests/unit/storage/table_heap_ops_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TableHeap::InsertTuple(const Tuple&)`.
2. Write unit tests inserting 200+ mixed `INT`/`BIGINT`/`BOOL`/`VARCHAR` tuples spanning 5+ pages, verifying `RowID`s across multiple pages and schema-mismatch rejection.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Multi-page inserts allocate and link pages cleanly; invalid tuples fail before modifying any page.

### Definition of Done
`TableHeap::InsertTuple` tested and passing, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-071: implement multi-page TableHeap::InsertTuple`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-072` (`TableHeap::GetTuple` & `DeleteTuple`).
````

---

## DAY-072 — Implement `TableHeap::GetTuple` and `TableHeap::DeleteTuple`

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-072
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `TableHeap::GetTuple` and `TableHeap::DeleteTuple`
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-071 complete.

### Objective
Implement `Result<Tuple> TableHeap::GetTuple(RowID rid)` and `Status TableHeap::DeleteTuple(RowID rid)` validating `rid.IsValid()`, reading the target page (verifying `PageType::kTableHeap`), invoking `SlottedPage::GetTuple` / `DeleteTuple`, and decoding via `TupleCodec::Deserialize`.

### Concepts & Daily Learning
- **Concept:** Direct point lookup (`O(1)` page read by `RowID`), cross-type page validation, and persistent deletion.
- **Why Database Engines Need It:** Once a B+ Tree returns a `RowID(page_id, slot_id)`, `IndexScanExecutor` fetches the exact row in 1 page read via `TableHeap::GetTuple(rid)`.
- **How It Works:** Reject `!rid.IsValid()`, read `rid.page_id`, verify `PageType::kTableHeap`, get/delete slot `rid.slot_id`, write page on delete, deserialize `Tuple` against `schema_`.
- **Design Trade-offs:** Returning `StatusCode::kNotFound` for deleted slots lets callers cleanly handle stale/deleted `RowID`s.
- **Common Mistakes:** Not checking `PageType::kTableHeap` when reading `rid.page_id` (what if an index had a corrupt `RowID` pointing to a B+ Tree page?!).
- **Understanding Checkpoint:** What protects `TableHeap::GetTuple` if a bug passes a `RowID` whose `page_id` actually belongs to a `BPlusTree` node or another table?

### Component Context
- **Why It Exists:** Completes point read and delete on `TableHeap`.
- **Module Relationships:** Used by `IndexScanExecutor` and `DeleteExecutor` in Phase 07.

### Files Expected to Change
- `include/tinydb/storage/table_heap.hpp`
- `src/storage/table_heap.cpp`
- `tests/unit/storage/table_heap_ops_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TableHeap::GetTuple` and `TableHeap::DeleteTuple` (and `Drop()` deallocating all chained pages!).
2. Test: insert 150 tuples across multiple pages -> read random `RowID`s -> delete subset -> verify deleted `RowID`s return `kNotFound` -> verify remaining `RowID`s survive `DiskManager` restart!

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- All live `RowID`s survive restart; deleted and invalid `RowID`s fail cleanly with `kNotFound`/`kInvalidArgument`.

### Definition of Done
`GetTuple` and `DeleteTuple` tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-072: implement TableHeap::GetTuple, DeleteTuple, and page drop`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-073` (`TableHeap::UpdateTuple`).
````

---

## DAY-073 — Implement `TableHeap::UpdateTuple` (In-Place vs Relocated Cross-Page Update)

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-073
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `TableHeap::UpdateTuple` (In-Place vs Relocated Cross-Page Update)
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-072 complete.

### Objective
Implement `Result<RowID> TableHeap::UpdateTuple(RowID old_rid, const Tuple& new_tuple)` returning either the SAME `old_rid` (when `SlottedPage::UpdateTuple` succeeds on the current page) OR allocating a `new_rid` via `InsertTuple` + deleting `old_rid` when a growing tuple no longer fits on `old_rid.page_id`!

### Concepts & Daily Learning
- **Concept:** Physical `RowID` stability vs row migration when a variable-length row outgrows its original page.
- **Why Database Engines Need It:** If `UPDATE users SET bio = '<3000 chars>'` runs on a full 4KB page, the updated row MUST move to another page—and returning the resulting `RowID` tells `UpdateExecutor` (Phase 07) whether indexes pointing to `old_rid` must be updated to `new_rid`!
- **How It Works:** Validate `new_tuple` against `schema_` and serialize first (fail before mutating!). Try `SlottedPage::UpdateTuple` on `old_rid.page_id`. If `ok()`, write page and return `old_rid`. If `kResourceExhausted`, call `InsertTuple(new_tuple)` (creating `new_rid`), then `DeleteTuple(old_rid)` and return `new_rid`!
- **Design Trade-offs:** Returning the new `RowID` (instead of forwarding pointers inside the old page) avoids an extra disk hop on every future read, at the cost of updating secondary B+ Tree entries when `new_rid != old_rid`.
- **Common Mistakes:** Deleting `old_rid` BEFORE `InsertTuple(new_tuple)` succeeds (if `InsertTuple` fails, the original tuple would be lost!); forgetting to check schema validity before starting mutation.
- **Understanding Checkpoint:** Why must `TableHeap::UpdateTuple` serialize `new_tuple` and insert onto the new page BEFORE deleting `old_rid` during a cross-page relocation?

### Component Context
- **Why It Exists:** Completes all CRUD primitives on `TableHeap`.
- **Module Relationships:** Called by `UpdateExecutor` (Phase 07), which inspects `new_rid != old_rid` to keep B+ Tree indexes synchronized!

### Files Expected to Change
- `include/tinydb/storage/table_heap.hpp`
- `src/storage/table_heap.cpp`
- `tests/unit/storage/table_heap_ops_test.cpp`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TableHeap::UpdateTuple(RowID, const Tuple&)`.
2. Test: in-place update keeps `new_rid == old_rid`; growing `VARCHAR` on a packed page migrates cleanly (`new_rid != old_rid`), `GetTuple(old_rid)` returns `kNotFound`, and `GetTuple(new_rid)` returns `new_tuple`; invalid tuple fails without altering `old_rid`.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Both same-page and cross-page updates preserve data integrity and fail atomically on invalid input.

### Definition of Done
`UpdateTuple` implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-073: implement TableHeap::UpdateTuple with cross-page migration`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-074` (`TableIterator` sequential scan).
````

---

## DAY-074 — Implement `TableIterator` and Multi-Page Sequential Scan

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-074
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Implement `TableIterator` and Multi-Page Sequential Scan
- **Estimated Duration:** 45 minutes (5m review/plan | 28m implementation | 8m testing/debugging | 4m docs & Git)
- **Prerequisites:** DAY-073 complete.

### Objective
Implement `TableHeap::ScanIterator` (`Begin()` / `Next()` returning `Result<std::optional<std::pair<RowID, Tuple>>>`) traversing every linked page in order, skipping deleted tombstones and empty pages, and validating against page-chain cycles.

### Concepts & Daily Learning
- **Concept:** Cursor-based sequential scan iterator skipping tombstones across chained pages.
- **Why Database Engines Need It:** `SequentialScanExecutor` (Phase 07), `CREATE INDEX` backfill, and `SELECT * FROM table` must stream rows page-by-page without loading the whole table into RAM at once!
- **How It Works:** Iterator holds `current_page_id`, `next_slot_id`, cached current `Page` (until `BufferPoolManager` manages frames in Phase 04), and visited page count/cycle guard (`<= total_pages` or visited set). On `Next()`, advance `slot_id` skipping `kDeleted` slots; when `slot_id == slot_count`, follow `next_page_id` until a live slot is found or `kInvalidPageId`.
- **Design Trade-offs:** Returning `(RowID, Tuple)` pairs lets `UpdateExecutor`, `DeleteExecutor`, and `CREATE INDEX` backfill know every scanned tuple's exact `RowID`!
- **Common Mistakes:** Stopping the scan prematurely when encountering an empty page in the middle of the chain (what if ALL rows on Page 2 were deleted, while Page 3 still has rows?!); infinite loop on a corrupt `next_page_id` cycle.
- **Understanding Checkpoint:** Why must `TableIterator::Next()` use a loop across pages when an intermediate page has `0` live tuples?

### Component Context
- **Why It Exists:** Streaming sequential table reader.
- **Module Relationships:** Wrapped directly by `SequentialScanExecutor` (`Phase 07`).

### Files Expected to Change
- `include/tinydb/storage/table_heap.hpp`
- `src/storage/table_heap.cpp`
- `tests/unit/storage/table_scan_test.cpp` (new)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `TableHeap::ScanIterator` (`Begin()` and `Next()`).
2. Write unit tests covering: empty table (`Next()` yields `std::nullopt` immediately), 5-page scan, middle page completely deleted (`Page 2` empty, `Page 1` & `Page 3` scanned cleanly!), and exact `(RowID, Tuple)` set verification.

### Testing Instructions
- Build and run `ctest --output-on-failure` under Debug + ASan/UBSan.

### Acceptance Criteria
- Scans skip tombstones and empty pages cleanly, terminate at `kInvalidPageId`, and return exact `(RowID, Tuple)` pairs.

### Definition of Done
Sequential scan iterator implemented and tested, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-074: implement TableHeap sequential ScanIterator skipping tombstones`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks `DAY-075` (Phase 02 integration & sign-off).
````

---

## DAY-075 — Complete Phase 02 Persistence, Compaction & ASan/UBSan Integration Suite and Sign Off

````markdown
Execute ONLY this assigned TinyDB working session. Do not automatically proceed to the next day. Inspect the repository and current progress before implementation. Implement the assigned scope, run relevant tests, update project documentation and complete the Git workflow.

### Session Metadata
- **Day:** DAY-075
- **Phase:** Phase 02 — Records and Table Heap
- **Session Title:** Complete Phase 02 Persistence, Compaction & ASan/UBSan Integration Suite and Sign Off
- **Estimated Duration:** 60 minutes (5m review/plan | 40m implementation | 10m testing/debugging | 5m docs & Git)
- **Prerequisites:** DAY-050 through DAY-074 complete.

### Objective
Build `tests/integration/phase02_table_heap_test.cpp` exercising multiple independent `TableHeap`s sharing one `DiskManager` file with mixed `INT`/`BIGINT`/`BOOL`/`VARCHAR` schemas, heavy inserts, deletes, updates (both in-place and cross-page), compaction, restarts, and `Drop()`, update `docs/storage-format.md`, and sign off Phase 02 (`58.0 / 220.0` hours).

### Concepts & Daily Learning
- **Concept:** Multi-table storage interleaving inside a single `.tinydb` file and end-to-end phase verification.
- **Why Database Engines Need It:** Real databases store dozens of tables interleaved on pages of the same file; verifying two independent `TableHeap`s don't clobber each other's page chains or free lists proves storage isolation.
- **How It Works:** Create `Table A` and `Table B` in one `DiskManager`, interleave 500 inserts/deletes/updates across both, reopen `DiskManager` cold, scan both tables fully against `std::map` oracles, then drop `Table A` and confirm `Table B` remains 100% intact while `FreePageCount()` increased!
- **Design Trade-offs:** Verifying `Drop()` reclaims pages back to `DiskManager` ensures zero space leaks across table lifecycles.
- **Common Mistakes:** Assuming `TableHeap` owns the `DiskManager` file or assuming sequential page IDs (`1, 2, 3`) for a single table.
- **Understanding Checkpoint:** Trace what happens to `DiskManager::FreePageCount()` and `Table B`'s page chain when `Table A` (interleaved across pages `1, 3, 5`) is dropped.

### Component Context
- **Why It Exists:** Completes Phase 02 (`20.0` hours).
- **Module Relationships:** Delivers a verified record/table-heap engine ready for `CatalogManager` (`Phase 03`) and `BufferPoolManager` (`Phase 04`).

### Files Expected to Change
- `tests/integration/phase02_table_heap_test.cpp` (new)
- `docs/storage-format.md` (verify Section 4 matches final `TupleCodec` & `SlottedPage` code)
- `CMakeLists.txt`
- `docs/roadmap/PROGRESS.md`, `docs/roadmap/NEXT_SESSION.md`, `docs/roadmap/SESSION-INDEX.md`

### Implementation Instructions
1. Implement `phase02_table_heap_test.cpp` testing interleaved multi-table CRUD, cold restart, and `Drop()`.
2. Sync `docs/storage-format.md` with exact implemented constants.
3. Run Debug, Release, ASan+UBSan suites + `clang-format` check.
4. Update `PROGRESS.md` (`58.0` hours completed, Phase 02 complete) and `NEXT_SESSION.md` -> `DAY-076`.

### Testing Instructions
- Full Debug (`-Werror`), ASan+UBSan (`-Werror`), Release (`-Werror`), and `clang-format` check.

### Acceptance Criteria
- All Phase 00–02 tests pass cleanly across Debug, Release, and ASan+UBSan.

### Definition of Done
Phase 02 signed off, roadmap updated, committed and pushed to `origin/develop`.

### Git Commit Instructions
- Commit on `develop`: `day-075: complete Phase 02 TableHeap integration suite and sign off`
- Push and verify `origin/develop`.

### Next Session Dependency
Unlocks Phase 03 (`DAY-076` — Catalog metadata structures).
````
