# B+ Tree Design (Planned — Phase 05)

> **Status:** Specification Blueprint. Implemented incrementally in Phase 05 (`DAY-112`–`DAY-152`).

## Scope (v1.0)
- Single-column integer index (`int64_t` keys, widened safely from `INT` or directly from `BIGINT`).
- Leaf entries store `(key, RowID)` pairs.
- Entries are sorted lexicographically by `(key, RowID)` so duplicate keys are supported natively without overflow page chains.
- Integrated with `BufferPoolManager` via `PageGuard`s and persisted across restarts via `CatalogManager` (`root_page_id`).

## Node Page Layouts (4096 Bytes)
- **Common B+ Tree Header:** `PageHeader` (32B) + `node_type`, `size`, `max_size`, `parent_page_id`.
- **Leaf Node:** `next_page_id` sibling pointer + sorted `(int64_t key, RowID rid)` pairs.
- **Internal Node:** Sorted separator keys + `child_page_id` array (`size` child pointers, `size - 1` separator keys at indices `1..size-1`).

## Structural Validator Contract
A dedicated `BPlusTreeValidator` will verify after splits, merges, and randomized tests:
1. Every node obeys `min_size <= size <= max_size` (except root).
2. Keys within every internal and leaf node are strictly sorted under `(key, RowID)` / key order.
3. Internal separator keys correctly partition left and right child subtrees.
4. All leaf nodes reside at identical tree depth.
5. Leaf sibling `next_page_id` chain visits every leaf in ascending key order with no cycles.
6. Every `parent_page_id` matches the actual parent referencing that child.
