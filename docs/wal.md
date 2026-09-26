# Write-Ahead Logging

The WAL record types are `BEGIN`, `PUT`, `DELETE`, `COMMIT`, and `ROLLBACK`. The commit marker is the durability boundary. Recovery considers a transaction committed only when its commit marker exists, so a process crash before that marker leaves no logical mutation behind.
