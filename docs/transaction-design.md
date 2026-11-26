# Transactions and WAL Design (Planned — Phase 08)

> **Status:** Specification Blueprint. Implemented incrementally in Phase 08 (`DAY-205`–`DAY-240`).

## v1.0 Transaction Model
- **Concurrency:** Single active transaction at a time (`TransactionManager` enforces mutual exclusion).
- **Buffering Policy:** **No-Steal** — uncommitted dirty pages stay pinned/retained in memory and are never written to the database file before `COMMIT`.
- **Runtime Rollback:** Before-images / write-records stored in memory allow `ROLLBACK` to undo table and index changes cleanly without touching disk recovery logs.
- **Recovery Policy:** **Redo-Only** — because no uncommitted pages ever reach disk, startup recovery never needs disk-based UNDO or CLRs.

## Commit Sequence
1. Append WAL `PAGE_IMAGE` after-image records for all pages modified by the transaction, updating each page's `PageHeader::lsn`.
2. Append WAL `COMMIT` record for `txn_id`.
3. Call `LogManager::Flush()` (`fsync`) so the WAL is durable on disk **before** returning commit success.
4. Release no-steal hold so modified pages can flush to `.tinydb` lazily or at checkpoint/shutdown.

## Startup Recovery (`RecoveryManager`)
1. **Analysis:** Scan `.wal` sequentially from LSN 1, verifying CRC32 checksums on each record; stop gracefully at a torn tail and track `committed_txns`.
2. **Redo:** Replay `PAGE_IMAGE` records belonging to `committed_txns` in LSN order if `disk_page.lsn < record.lsn` (idempotent redo).
3. Flush recovered pages to `.tinydb`.
