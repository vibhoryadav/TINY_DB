# SQL Subset Specification (Planned — Phases 06 & 07)

> **Status:** Specification Blueprint. Lexer & Parser implemented in Phase 06 (`DAY-153`–`DAY-178`); Execution & CLI in Phase 07 (`DAY-179`–`DAY-204`); Transaction keywords (`BEGIN`, `COMMIT`, `ROLLBACK`) added in Phase 08 (`DAY-207`).

## Supported Statements (v1.0)

### DDL (Outside Transactions in v1.0)
- `CREATE TABLE <name> ( <col_name> <type>, ... );`
  - Supported types: `INT`, `BIGINT`, `BOOL`, `VARCHAR(n)` (`1 <= n <= 65535` bounded by page payload)
- `DROP TABLE <name>;`
- `CREATE INDEX <index_name> ON <table_name> (<col_name>);`

### DML
- `INSERT INTO <table_name> [(col1, ...)] VALUES (val1, ...), (...);`
- `SELECT * FROM <table_name> [WHERE <predicate>];`
- `SELECT <col1>, <col2>, ... FROM <table_name> [WHERE <predicate>];`
- `UPDATE <table_name> SET <col1> = <expr> [, ...] [WHERE <predicate>];`
- `DELETE FROM <table_name> [WHERE <predicate>];`

### Predicates (`WHERE`)
- Operators: `=`, `!=`, `<>`, `>`, `<`, `>=`, `<=`, `AND`, parentheses `(...)`
- Literals: signed 64-bit integers, single-quoted strings `'...'` (escaped quotes), booleans (`TRUE`/`FALSE`)

### Transaction Control (Phase 08)
- `BEGIN;`
- `COMMIT;`
- `ROLLBACK;`

## Explicitly Excluded in v1.0 (Deferred to v2.0)
`NULL`, `FLOAT`, `DECIMAL`, `FOREIGN KEY`, `JOIN`, subqueries, `OR`, `GROUP BY`, `ORDER BY`, window functions, and multi-column indexes. The parser must reject unsupported constructs with explicit line/column error diagnostics.
