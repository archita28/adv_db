# RepCRec Quick Start Guide

## Setup Instructions

### 1. Create Project Directory

```bash
mkdir repcrec
cd repcrec
```

### 2. Create All Source Files

You need to create the following files (copy content from the artifacts):

**Header Files:**
- `Constants.h`
- `Version.h`
- `WaitInfo.h`
- `ReadInfo.h`
- `WriteInfo.h`
- `Transaction.h`
- `SiteState.h`
- `DataManager.h`
- `TransactionManager.h`
- `Parser.h`

**Implementation Files:**
- `DataManager.cpp`
- `TransactionManager.cpp` (use the "Complete" version)
- `Parser.cpp`
- `main.cpp`

**Build Files:**
- `Makefile`

**Documentation:**
- `README.md`

### 3. Build the Project

```bash
make
```

This should produce an executable called `repcrec`.

### 4. Run Tests

```bash
# Run with input from file
./repcrec < test1.txt

# Run with input from stdin
./repcrec
# Then type commands manually, one per line
# Press Ctrl+D (Unix/Mac) or Ctrl+Z (Windows) to end input
```

## Sample Test Cases

### Test 1: First-Committer-Wins

Create `test1.txt`:
```
begin(T1)
begin(T2)
W(T1, x1, 101)
W(T2, x2, 202)
W(T1, x2, 102)
W(T2, x1, 201)
end(T2)
end(T1)
dump()
```

Expected: T2 commits, T1 aborts

### Test 2: Snapshot Isolation

Create `test2.txt`:
```
begin(T1)
begin(T2)
W(T1, x1, 101)
R(T2, x2)
W(T1, x2, 102)
R(T2, x1)
end(T1)
end(T2)
dump()
```

Expected: Both commit, T2 reads initial values

### Test 3: Site Failure

Create `test3.txt`:
```
begin(T1)
begin(T2)
R(T1, x3)
fail(2)
W(T2, x8, 88)
R(T2, x3)
W(T1, x5, 91)
end(T2)
recover(2)
end(T1)
dump()
```

Expected: Both commit, site 2 recovers

## Common Commands

| Command | Description | Example |
|---------|-------------|---------|
| `begin(T)` | Start transaction T | `begin(T1)` |
| `R(T, xi)` | T reads variable xi | `R(T1, x4)` |
| `W(T, xi, v)` | T writes value v to xi | `W(T1, x2, 100)` |
| `end(T)` | Commit/abort T | `end(T1)` |
| `fail(s)` | Site s fails | `fail(3)` |
| `recover(s)` | Site s recovers | `recover(3)` |
| `dump()` | Show all committed data | `dump()` |

## Understanding Output

### Read Output
```
x4: 40
```
Transaction read x4 with value 40

### Write Output
```
W(T1, x2, 100) -> sites: 1 2 3 4 5 6 7 8 9 10
```
T1 wrote to x2 (replicated) on all available sites

### Commit/Abort Output
```
T1 commits
T2 aborts (First-committer-wins)
```

### Dump Output
```
=== DUMP ===
site 1 - x2: 20, x4: 40, x6: 60, x8: 80, x10: 100, x12: 120, x14: 140, x16: 160, x18: 180, x20: 200
site 2 - x1: 10, x2: 20, x4: 40, ...
...
============
```

## Debugging Tips

1. **Transaction not found**: Make sure you've called `begin(T)` first
2. **No version found**: The variable might not exist at the queried site
3. **Transaction waits**: Site is down, will retry when it recovers
4. **Abort (Site failure)**: Transaction wrote to a site that later failed
5. **Abort (First-committer-wins)**: Another transaction committed first
6. **Abort (RW-cycle)**: Dangerous cycle detected in conflict graph

## Troubleshooting Compilation

If you get compilation errors:

```bash
# Check C++ version
g++ --version
# Should be g++ 7.0 or later for C++17 support

# Clean and rebuild
make clean
make

# Compile with more warnings
make CXXFLAGS="-std=c++17 -Wall -Wextra -O2 -g"
```

## Next Steps

1. Test with all 25 provided test cases
2. Create your own custom test scenarios
3. Verify output matches expected results
4. Use a debugger (gdb) if needed:
   ```bash
   g++ -std=c++17 -g -o repcrec main.cpp DataManager.cpp Parser.cpp
   gdb ./repcrec
   ```

## Project Structure

```
repcrec/
├── Constants.h         # System constants
├── Version.h          # Version data structure
├── WaitInfo.h         # Wait tracking
├── ReadInfo.h         # Read tracking
├── WriteInfo.h        # Write tracking
├── Transaction.h      # Transaction class
├── SiteState.h        # Site status
├── DataManager.h      # Data manager (header)
├── DataManager.cpp    # Data manager (impl)
├── TransactionManager.h  # TM (header)
├── TransactionManager.cpp # TM (impl) 
├── Parser.h           # Parser (header)
├── Parser.cpp         # Parser (impl)
├── main.cpp           # Entry point
├── Makefile           # Build config
├── README.md          # Full documentation
└── test*.txt          # Test cases
```

## Key Algorithms Implemented

✅ **Serializable Snapshot Isolation (SSI)**
- Snapshot reads at transaction start time
- First-Committer-Wins validation
- RW-cycle detection with 2+ RW edges

✅ **Available Copies Replication**
- Writes to all available sites
- Abort on site failure after access

✅ **Failure Recovery**
- Replicated variables blocked until post-recovery commit
- Site continuous-up checking for valid snapshots

## Contact & Support

For issues or questions about this implementation, refer to:
- Full documentation in `README.md`
- Code comments in each file
- Original project specification