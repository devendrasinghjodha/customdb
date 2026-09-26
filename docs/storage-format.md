# Storage Format

Pages are exactly 4096 bytes. The first bytes hold a page ID, page type, and checksum; payload begins at byte 32. The checksum is an FNV-1a-style 32-bit checksum over the page with its checksum field treated as zero.

The logical snapshot is a line-oriented format using quoted strings:

```text
PUT "key" "value"
```

The WAL uses tab-separated records with explicit key and value lengths so tabs and spaces in values remain recoverable. The WAL is an implementation detail and should be treated as append-only.
