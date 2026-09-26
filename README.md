# CustomDB

CustomDB is a small C++17 embedded key-value database built around an ordered index, append-only write-ahead log, snapshot persistence, and crash recovery.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
./build/Release/customdb.exe data.db
```

Commands:

```text
PUT name devendra
GET name
BEGIN
PUT balance 1000
COMMIT
DELETE name
ROLLBACK
COMPACT
CREATE TABLE users ( id INT , name TEXT )
INSERT INTO users VALUES ( 1 , Devendra )
SELECT * FROM users
UPDATE users SET row = Changed WHERE id = 0
DELETE FROM users WHERE id = 0
EXIT
```

The database stores the logical snapshot in the selected file, the page file beside it with a `.pages` suffix, and the WAL beside it with a `.wal` suffix. A transaction is durable only after its commit record has been written and durably synchronized. During startup, only transactions with a commit record are replayed.

## Architecture

- `storage/page.*`: fixed-size 4096-byte pages with checksums.
- `storage/pager.*`: page-aligned file I/O and allocation.
- `storage/buffer_pool.*`: pinned page frames with LRU eviction.
- `index/btree.*`: ordered key/value index abstraction.
- `wal/wal.*`: length-aware append-only WAL records.
- `transaction/transaction.*`: explicit begin, commit, and rollback states.
- `api/database.*`: public API, snapshot replacement, recovery, and compaction.
- `query/query.*`: command lexer and stateful query executor.
- `mvcc/version.*`: version chains and snapshot reads.
- `index/secondary_index.*`: value-to-primary-key secondary index.
- `transaction/lock_manager.*`: per-key transaction locking.

The page-backed B-tree node format and full relational SQL planner remain future extensions; the current implementation provides the storage primitives and ordered index needed for them.
