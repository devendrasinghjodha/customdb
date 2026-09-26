# Architecture

The current execution path is:

```text
CLI -> Database -> Transaction -> WAL -> BTree -> snapshot
                         |
                         +-> recovery replays committed records

Pager and BufferPool provide the fixed-page storage boundary for the next index iteration.
```

A `PUT` or `DELETE` outside an explicit transaction creates a short transaction automatically. Explicit transactions collect changes in memory, append all mutation records, append `COMMIT`, flush the WAL, and only then apply the mutations and replace the snapshot.

A startup recovery pass reads the snapshot first, scans the WAL for committed transaction IDs, and replays only mutation records belonging to those IDs. An active transaction without a commit record is therefore discarded.
