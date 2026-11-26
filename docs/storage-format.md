# Storage Format Specification (Planned — Phases 01 & 02)

> **Status:** Specification Blueprint. Implemented incrementally in Phase 01 (`DAY-019`–`DAY-049`) and Phase 02 (`DAY-050`–`DAY-075`).

## 1. Byte Order & Integrity Rules
- **Byte Order:** Little-endian for all multi-byte integers (`uint16_t`, `uint32_t`, `uint64_t`, `int32_t`, `int64_t`).
- **Page Size:** Fixed `4096` bytes (`kPageSize = 4096`).
- **Serialization Rule:** Every field is explicitly encoded/decoded via `core/endian.hpp`. No raw struct casts.
- **Checksums:** CRC32 stored in page and file headers, computed with the checksum field zeroed during calculation.

---

## 2. Database File Layout (`.tinydb`)
- **Page 0 (File Header Page):**
  - `magic[8]`: `"TINYDB\0\1"`
  - `format_version` (`uint32_t`): `1`
  - `page_size` (`uint32_t`): `4096`
  - `page_count` (`uint32_t`): Total allocated pages (including page 0)
  - `first_free_page_id` (`uint32_t`): Head of free-page linked list (`INVALID_PAGE_ID` if empty)
  - `free_page_count` (`uint32_t`): Number of pages on free list
  - `header_crc32` (`uint32_t`): Integrity checksum over header fields
- **Pages 1..N:** Fixed `4096`-byte typed pages (`FREE`, `TABLE_HEAP`, `BTREE_INTERNAL`, `BTREE_LEAF`).

---

## 3. Common 4096-Byte Page Header (32 Bytes)
Every page (`1..N`) begins with a fixed common header:

| Offset | Size | Field | Description |
|---:|---:|---|---|
| 0 | 4 | `page_id` | Self-identifying page number |
| 4 | 1 | `page_type` | `FREE`, `TABLE_HEAP`, `BTREE_INTERNAL`, `BTREE_LEAF`, etc. |
| 5 | 1 | `flags` | Page-level bitflags |
| 6 | 2 | `reserved` | Zero-initialized reserved bytes |
| 8 | 8 | `lsn` | Log Sequence Number of last WAL modification (`0` if none) |
| 16 | 4 | `checksum` | CRC32 of the 4096-byte page (with this field treated as `0`) |
| 20 | 4 | `free_space_offset` | Payload offset metadata |
| 24 | 8 | `next_page_id` / `aux` | `uint32_t next_page_id` + `uint32_t prev_page_id` or auxiliary metadata |

*(Exact byte offsets will be finalized and unit-tested in `DAY-019`–`DAY-028`.)*

---

## 4. Slotted Page & Record Layout (Phase 02)
Within a `TABLE_HEAP` page (`4096` bytes):
- Bytes `[0..31]`: Common `PageHeader`
- Bytes `[32..47]`: `SlottedPageHeader` (`slot_count`, `free_space_start`, `free_space_end`, `deleted_count`)
- Growing Forward (`48 ->`): Fixed-size `Slot` entries `(uint16_t offset, uint16_t length, uint8_t flags)`
- Growing Backward (`<- 4096`): Serialized `Tuple` payloads
- Compaction defragments live payloads toward byte `4096` without renumbering active `slot_id`s so existing `RowID(page_id, slot_id)` references remain valid.

### Supported Data Types (v1.0)
- `INT`: 4-byte signed integer (`int32_t`)
- `BIGINT`: 8-byte signed integer (`int64_t`)
- `BOOL`: 1-byte (`0` or `1`; invalid byte values rejected on decode)
- `VARCHAR(n)`: `uint16_t` length prefix (`<= n`) followed by UTF-8/raw bytes
