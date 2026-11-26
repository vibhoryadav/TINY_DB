# TinyDB 220-Hour Development Roadmap

This directory contains the complete session-by-session engineering roadmap for building TinyDB v1.0.0 from scratch across **284 working sessions** totaling **220.0 focused hours**.

---

## Session Cadence & Rules

- **Standard Session:** 45 minutes (`5m` review/plan, `28m` implementation, `8m` test/debug, `4m` docs & Git).
- **Deep/Integration Session:** 60 minutes maximum (`5m` review/plan, `40m` implementation, `10m` test/debug, `5m` docs & Git).
- **Hard Rule:** Never exceed 60 minutes in a single session, never proceed automatically to the next day, and never commit unfinished work directly to `main`.

---

## Phase Allocation Summary

| Phase | Area | Budget Hours | Sessions | Day Range | Prompt File |
|---|---|---:|---:|---|---|
| 00 | Prerequisites and architecture | 14.0 | 18 | `DAY-001` – `DAY-018` | [PHASE-00.md](daily-prompts/PHASE-00.md) |
| 01 | Disk Manager and page format | 24.0 | 31 | `DAY-019` – `DAY-049` | [PHASE-01.md](daily-prompts/PHASE-01.md) |
| 02 | Records and Table Heap | 20.0 | 26 | `DAY-050` – `DAY-075` | [PHASE-02.md](daily-prompts/PHASE-02.md) |
| 03 | Catalog management | 8.0 | 10 | `DAY-076` – `DAY-085` | [PHASE-03.md](daily-prompts/PHASE-03.md) |
| 04 | Buffer Pool | 20.0 | 26 | `DAY-086` – `DAY-111` | [PHASE-04.md](daily-prompts/PHASE-04.md) |
| 05 | B+ Tree | 32.0 | 41 | `DAY-112` – `DAY-152` | [PHASE-05.md](daily-prompts/PHASE-05.md) |
| 06 | SQL Lexer and Parser | 20.0 | 26 | `DAY-153` – `DAY-178` | [PHASE-06.md](daily-prompts/PHASE-06.md) |
| 07 | Query Engine and CLI | 20.0 | 26 | `DAY-179` – `DAY-204` | [PHASE-07.md](daily-prompts/PHASE-07.md) |
| 08 | Transactions and WAL | 28.0 | 36 | `DAY-205` – `DAY-240` | [PHASE-08.md](daily-prompts/PHASE-08.md) |
| 09 | Security and testing | 24.0 | 31 | `DAY-241` – `DAY-271` | [PHASE-09.md](daily-prompts/PHASE-09.md) |
| 10 | Documentation and release | 10.0 | 13 | `DAY-272` – `DAY-284` | [PHASE-10.md](daily-prompts/PHASE-10.md) |
| **Total** | | **220.0** | **284** | **`DAY-001` – `DAY-284`** | |

---

## Tracking Files

- [`PROGRESS.md`](PROGRESS.md) — Current hours, phase, build/test status, design decisions, and blockers.
- [`NEXT_SESSION.md`](NEXT_SESSION.md) — Exact next day number, title, prerequisites, and prompt location.
- [`SESSION-INDEX.md`](SESSION-INDEX.md) — Complete 284-session index with duration, prerequisites, and status.
