# RepCRec - Replicated Concurrency Control and Recovery

A distributed database implementation featuring:
- **Serializable Snapshot Isolation (SSI)** for concurrency control
- **Available Copies** replication algorithm
- **Site failure and recovery** handling

## Authors
RepCRec Team - December 2024

## System Overview

### Components

1. **TransactionManager** - Central coordinator managing all transactions and sites
2. **DataManager** - Manages data storage and versioning at each site (10 sites total)
3. **Transaction** - Represents a single transaction with SSI metadata
4. **SiteState** - Tracks site availability and failure history
5. **Parser** - Parses and executes input commands

### Data Model

- **20 variables**: x1, x2, ..., x20
- **10 sites**: site 1, site 2, ..., site 10
- **Replication**:
  - Odd-indexed variables (x1, x3, ..., x19): stored at one site each
    - Site = 1 + (variable_index mod 10)
    - Example: x1 at site 2, x3 at site 4
  - Even-indexed variables (x2, x4, ..., x20): replicated at all 10 sites
- **Initial values**: xi = 10 * i

## Building the Project

```bash
# Compile
make

# Clean build artifacts
make clean

# Run with a test file
./repcrec < test1.txt
```

## Input Format

Commands (one per line):

- `begin(T1)` - Start transaction T1
- `R(T1, x4)` - Transaction T1 reads variable x4
- `W(T1, x6, 42)` - Transaction T1 writes value 42 to x6
- `end(T1)` - Attempt to commit T1 (may abort)
- `fail(3)` - Site 3 fails
- `recover(3)` - Site 3 recovers
- `dump()` - Print committed state of all sites

Comments: Lines starting with `//` or `#` are ignored

## Key Algorithms

### Serializable Snapshot Isolation (SSI)

1. **Snapshot Reads**: Each transaction sees a consistent snapshot as of its start time
2. **First-Committer-Wins (FCW)**: Detects write-write conflicts
3. **RW-Cycle Detection**: Prevents dangerous structures with consecutive read-write edges

### Available Copies Algorithm

- Writes go to all currently available sites
- Commits succeed even if some copies are down
- **Abort rule**: If a transaction writes to site s and s fails before the transaction commits, the transaction must abort

### Recovery Semantics

- **Non-replicated variables**: Immediately readable after recovery
- **Replicated variables**: Cannot be read until a committed write occurs post-recovery
  - Prevents reading stale data
  - Ensures snapshot consistency

### Site Availability Tracking

For replicated variable reads, the system checks if a site:
1. Has the required version
2. Was **continuously up** from that version's commit time to the transaction's start time

This prevents reading from sites that may have missed intervening updates.

## Implementation Highlights

### Critical Methods

**TransactionManager:**
- `computeValidSnapshotSites()` - Finds sites valid for snapshot reads
- `wasSiteUpContinuously()` - Checks if site was up during time interval
- `violatesFailureRule()` - Checks abort-on-failure condition
- `violatesFirstCommitterWins()` - Detects write-write conflicts
- `violatesReadWriteCycle()` - Detects RW cycles (including WW edges closing cycles)

**DataManager:**
- `readVariable()` - Returns version visible at snapshot time
- `writeVariable()` - Buffers uncommitted writes
- `commitWrites()` - Applies buffered writes with commit timestamp
- `onRecovery()` - Disables replicated reads until next commit

### Data Structures

- **Version history**: Sorted list of committed versions per variable
- **Write buffers**: Per-transaction uncommitted writes
- **Failure history**: Time intervals when each site was down
- **Conflict graph**: RW edges between transactions for cycle detection

## Testing

The implementation handles 25+ test cases covering:
- Snapshot isolation and multiversion reads
- First-committer-wins conflicts
- RW-cycle detection (including cycles closed by WW edges)
- Site failures during transaction execution
- Recovery and replicated variable read gates
- Waiting/blocking when sites are unavailable

### Example Test

```
begin(T1)
begin(T2)
W(T1, x1, 101)
W(T2, x2, 202)
W(T1, x2, 102)
W(T2, x1, 201)
end(T2)  // T2 commits
end(T1)  // T1 aborts (first-committer-wins)
dump()
```

Expected output:
- T2 commits with x1=201, x2=202
- T1 aborts

## File Structure

```
repcrec/
├── Constants.h         # System constants and utilities
├── Version.h          # Version data structure
├── WaitInfo.h         # Transaction wait information
├── ReadInfo.h         # Read operation tracking
├── WriteInfo.h        # Write operation tracking
├── Transaction.h      # Transaction class
├── SiteState.h        # Site status tracking
├── DataManager.h      # Data manager header
├── DataManager.cpp    # Data manager implementation
├── TransactionManager.h  # Transaction manager header
├── Parser.h           # Command parser header
├── Parser.cpp         # Command parser implementation
├── main.cpp           # Entry point
├── Makefile           # Build configuration
└── README.md          # This file
```

## Design Decisions

1. **No distributed TM**: Single transaction manager simplifies implementation per spec
2. **In-memory storage**: All data kept in memory (no persistence beyond recovery)
3. **Timestamp-based versioning**: Simple integer timestamps for global ordering
4. **BFS cycle detection**: Efficient graph traversal for RW-cycle checking
5. **Failure intervals**: Track precise failure/recovery times for continuous-up checks

## Known Limitations

- Single transaction manager (not truly distributed)
- No network simulation or delays
- No disk persistence (committed data lost on full system failure)
- Simplified deadlock handling (via waiting/blocking)

## References

- Serializable Snapshot Isolation: Fekete et al.
- Available Copies: Based on distributed database literature
- Project specification: NYU Advanced Database Systems course