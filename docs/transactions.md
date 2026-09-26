# Transactions

Transactions are `Active`, then either `Committed` or `RolledBack`. A commit writes mutation records followed by a commit marker and flushes the WAL before changing the in-memory index. Rollback writes a rollback marker and applies no mutations.

This provides atomic recovery semantics for the current single-process engine. Isolation and MVCC are intentionally reserved for a later transaction-manager iteration.
