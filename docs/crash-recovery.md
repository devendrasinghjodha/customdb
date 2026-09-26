# Crash Recovery

On open, CustomDB loads the last snapshot and scans the WAL. It collects committed transaction IDs first, then replays only `PUT` and `DELETE` records from those transactions. Incomplete transactions are ignored. The recovered state is written as a new temporary snapshot and atomically renamed into place.

The current implementation is designed for a single process. A future production version should add platform-specific durable file synchronization, checksummed WAL blocks, file locking, and page-level redo/undo.
