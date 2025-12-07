# RepCRec - Replicated Concurrency Control and Recovery

Distributed database with serializable snapshot isolation, replication, and failure recovery.

**Authors:** Aishwarya Anand & Archita Arora  
**Course:** Advanced Database Systems, NYU  
**December 2025**

---

## Quick Start

```bash
make
./repcrec < tests/test1.txt
```

Interactive:
```bash
./repcrec
begin(T1)
W(T1,x1,100)
end(T1)
```

---

## System Overview

**20 variables** (x1-x20) across **10 sites**:
- Odd variables: one site each (site = 1 + i mod 10)
- Even variables: replicated at all sites
- Initial values: xi = 10i

**Architecture:**
- TransactionManager: central coordinator, never fails
- DataManager: one per site, handles storage/versioning, can fail
- Transaction: metadata for SSI
- SiteState: tracks up/down status

---

## Commands

```
begin(T1)       - start transaction
R(T1,x4)        - read variable
W(T1,x6,42)     - write variable
end(T1)         - commit/abort
fail(3)         - site 3 fails
recover(3)      - site 3 recovers
dump()          - show all data
```

Lines starting with `//` are comments.

---

## How It Works

### Snapshot Isolation

Each transaction reads from a snapshot taken at its start time. Reads never block.

### Validation at Commit

Three checks:
1. **First-Committer-Wins**: If concurrent transactions write the same variable, first one wins
2. **RW-Cycle**: Abort if committing creates dangerous cycle (2+ consecutive RW edges)
3. **Failure Rule**: Abort if any written site failed before commit

### Available Copies

Writes go to all UP sites. Commits succeed even if some sites down. If a site fails after being written but before commit, transaction aborts.

### Recovery

When a site recovers:
- Unreplicated vars: readable immediately
- Replicated vars: blocked until next write (prevents stale reads)

---

## Testing

25 test files in `tests/` directory. Run with:

```bash
./repcrec < tests/test1.txt      # single test
./run_tests.sh                    # all tests
```

Notable tests:
- **test18.txt**: 5-transaction RW cycle (T5 aborts)
- **test22.txt**: WW conflict closes RW cycle
- **test25.txt**: transaction waits for recovery

See `test_suite.txt` for all tests with descriptions.

---

## Example

**test1.txt:**
```
begin(T1)
begin(T2)
W(T1,x1,101)
W(T2,x2,202)
W(T1,x2,102)
W(T2,x1,201)
end(T2)
end(T1)
```

Result: T2 commits, T1 aborts (first-committer-wins)

---

## Implementation Notes

**Version history:** Each variable keeps sorted list of (value, timestamp, writer)

**Write buffering:** Uncommitted writes stored per-transaction, applied at commit

**Failure tracking:** Sites record failure intervals for continuous-uptime checks

**Conflict graph:** RW edges tracked between transactions for cycle detection

---

## File Structure

```
repcrec/
├── main.cpp
├── TransactionManager.h
├── TransactionManager.cpp
├── DataManager.h
├── DataManager.cpp
├── Parser.h
├── Parser.cpp
├── Transaction.h
├── SiteState.h
├── Constants.h
├── Version.h
├── WaitInfo.h
├── ReadInfo.h
├── WriteInfo.h
├── Makefile
├── tests/           # 25 test files (test1.txt - test25.txt)
├── test_suite.txt   # all tests with comments
├── split_tests.py   # script to generate test files
├── run_tests.sh     # test runner
└── README.md
```

---

## Build

```bash
make              # compile
make clean        # remove artifacts
make test         # run test1
```

**Prerequisites:** C++17 compiler (g++ or clang++)

---

## ReproZip Package

This project is packaged with ReproZip for reproducibility.

**If you have the .rpz file:**

```bash
pip install reprounzip
reprounzip directory setup repcrec.rpz repcrec-run
reprounzip directory run repcrec-run
```

**To create the package:**

```bash
pip install reprozip
reprozip trace ./repcrec < tests/test1.txt
reprozip pack repcrec.rpz
```

The package includes the executable, dependencies, and test data.

---

## Design Choices

- Single TM simplifies coordination (spec allows this)
- In-memory only (no disk persistence)
- Integer timestamps (simpler than vector clocks)
- BFS for cycle detection

