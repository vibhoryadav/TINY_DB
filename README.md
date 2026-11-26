# TinyDB

> An educational, disk-oriented relational database management system written from scratch in C++20.

TinyDB is built incrementally over a structured 220-hour engineering roadmap (`DAY-001` through `DAY-284`). Every subsystem is designed for clarity, strict bounds safety, explicit binary serialization, and verifiable correctness so every line of code can be understood, explained, and defended in technical interviews.

---

## Project Specifications (v1.0 Target)

| Component | Specification |
|---|---|
| Language | C++20 |
| Build System | CMake (>= 3.20) |
| Storage Model | Disk-oriented, fixed 4096-byte pages |
| Record Layout | Slotted pages + explicit tuple serialization (`INT`, `BIGINT`, `BOOL`, `VARCHAR(n)`) |
| Catalog | Separate versioned metadata file with atomic rename persistence |
| Memory Management | Buffer Pool Manager with pin counts, dirty tracking, LRU eviction, RAII guards |
| Indexing | Disk-backed persistent B+ Tree (`int64_t` keys -> `RowID`) + structural validator |
| Query Language | SQL subset (`CREATE/DROP TABLE`, `CREATE INDEX`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`, `WHERE`) |
| Execution Engine | Pull-based Volcano iterator model + interactive CLI (`tinydb`) |
| Transactions & Recovery | Single active transaction, no-steal redo-only simplified WAL recovery |
| Testing | GoogleTest + ASan + UBSan + differential testing against SQLite + crash testing |

---

## Repository Layout

```text
TINY_DB/
├── src/
│   ├── core/          # Shared types, status/result, endianness, CRC32, file I/O
│   ├── storage/       # DiskManager, Page format, SlottedPage, Tuple, TableHeap
│   ├── buffer/        # BufferPoolManager, LRUReplacer, PageGuard
│   ├── index/         # Disk-backed B+ Tree & invariant validator
│   ├── catalog/       # CatalogManager & atomic metadata persistence
│   ├── parser/        # SQL Lexer, Recursive-Descent Parser, AST
│   ├── execution/     # Pull-based Volcano executors & query planner
│   ├── transaction/   # TransactionManager & rollback undo records
│   ├── recovery/      # WAL LogManager & redo-only RecoveryManager
│   └── cli/           # Interactive REPL & dot-commands
├── include/tinydb/    # Public project headers
├── tests/
│   ├── unit/          # Component-level GoogleTest suites
│   ├── integration/   # End-to-end SQL + storage + index tests
│   ├── stress/        # Randomized fuzz, B+ Tree & differential SQLite tests
│   └── recovery/      # Controlled crash & WAL recovery tests
├── benchmarks/        # Reproducible throughput and latency benchmarks
├── examples/          # Sample SQL scripts and demo sessions
└── docs/              # Architecture, format specs, design decisions & daily roadmap
```

---

## Quick Start (Bootstrap)

### Prerequisites

- C++20 compiler (Apple Clang / Clang 15+ / GCC 12+)
- CMake 3.20+
- GoogleTest (installed via Homebrew/system package, or fetched automatically by CMake when online)
- `clang-format` (optional, for formatting checks)

### Build and Test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Build with Sanitizers (ASan + UBSan)

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DTINYDB_ENABLE_SANITIZERS=ON
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

### Run Minimal CLI

```bash
./build/tinydb --version
```

---

## Development Workflow & Roadmap

All active development follows the phased session plan under [`docs/roadmap/`](docs/roadmap/README.md):

- **Current Progress:** [`docs/roadmap/PROGRESS.md`](docs/roadmap/PROGRESS.md)
- **Next Session Pointer:** [`docs/roadmap/NEXT_SESSION.md`](docs/roadmap/NEXT_SESSION.md)
- **Master Session Index:** [`docs/roadmap/SESSION-INDEX.md`](docs/roadmap/SESSION-INDEX.md)
- **Daily Prompts:** [`docs/roadmap/daily-prompts/`](docs/roadmap/daily-prompts/)

### Branch Strategy

- `main`: Stable, tested, release-ready snapshots only.
- `develop`: Active daily session development (`day-XXX: <summary>`).

---

## Documentation

- [Architecture Overview](docs/architecture.md)
- [Storage Format Specification](docs/storage-format.md)
- [Buffer Pool Design](docs/buffer-pool.md)
- [B+ Tree Design](docs/btree-design.md)
- [SQL Subset Specification](docs/sql-spec.md)
- [Transactions and WAL Design](docs/transaction-design.md)
- [Architectural Design Decisions](docs/design-decisions.md)

---

## License

MIT License — see [LICENSE](LICENSE).
