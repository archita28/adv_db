// TransactionManager.h
// Author: Aishwarya and Archita
// Date: December 2025
// Purpose: Central coordinator for all transactions and sites
// Manages SSI, Available Copies, and failure/recovery

#ifndef TRANSACTIONMANAGER_H
#define TRANSACTIONMANAGER_H

#include "Transaction.h"
#include "DataManager.h"
#include "SiteState.h"
#include "Constants.h"
#include <map>
#include <vector>
#include <memory>
#include <set>
#include <string>

namespace RepCRec {

class TransactionManager {
private:
    int currentTimestamp;
    std::map<std::string, std::shared_ptr<Transaction>> transactions;
    std::vector<std::shared_ptr<Transaction>> committedTransactions;
    std::map<int, std::shared_ptr<DataManager>> dataManagers;  // siteId -> DataManager
    std::map<int, SiteState> siteStates;  // siteId -> SiteState
    
    // Track commit history per variable for FCW
    std::map<int, std::vector<std::pair<std::string, int>>> variableCommitHistory;
    // variableId -> list of (transactionId, commitTime)
    
public:
    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================
    
    /**
     * TransactionManager Constructor
     * Author: Aishwarya Anand
     * Input: None (constructor)
     * Output: None
     * Description: Initializes the TransactionManager with timestamp set to 0,
     *              creates 10 DataManager instances (one per site), and 
     *              initializes SiteState for each site.
     * Side Effects:
     *   - Sets currentTimestamp to 0
     *   - Creates and stores 10 DataManager objects in dataManagers map
     *   - Creates and stores 10 SiteState objects in siteStates map
     *   - Each DataManager is initialized with its site variables and initial values
     */
    TransactionManager();
    
    // ========================================================================
    // CORE TRANSACTION OPERATIONS
    // ========================================================================
    
    /**
     * begin - Start a new transaction
     * Author: Aishwarya Anand
     * Input: 
     *   - transactionId (string): Unique identifier for the transaction (e.g., "T1")
     * Output: None (prints to stdout)
     * Description: Creates a new transaction with the given ID, assigns it the 
     *              current timestamp as its start time, and adds it to the active 
     *              transactions map.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Creates new Transaction object with current timestamp as startTime
     *   - Adds transaction to transactions map
     *   - Prints: "Transaction T1 begins at time X"
     */
    void begin(const std::string& transactionId);
    
    /**
     * read - Read a variable under snapshot isolation
     * Author: Archita Arora
     * Input:
     *   - transactionId (string): ID of the transaction performing the read
     *   - variableId (int): Variable to read (1-20)
     * Output: Prints variable value to stdout (e.g., "x4: 40")
     * Description: Reads a variable under snapshot isolation. First checks 
     *              read-your-own-write in writeSet, then routes to readFromHomeSite() 
     *              for odd variables or readReplicated() for even variables.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - May update transaction's readSet with read information
     *   - May update transaction's criticalReadSites (for unreplicated reads)
     *   - May update transaction's firstAccessTimePerSite
     *   - May set transaction to WAITING state if site is down
     *   - Prints read value or waiting message
     */
    void read(const std::string& transactionId, int variableId);
    
    /**
     * write - Buffer a write operation
     * Author: Archita Arora
     * Input:
     *   - transactionId (string): ID of the transaction performing the write
     *   - variableId (int): Variable to write (1-20)
     *   - value (int): Value to write
     * Output: Prints sites written to stdout
     * Description: Buffers a write to all currently UP sites that store the variable. 
     *              Does not modify committed data until transaction commits.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Adds write to transaction's writeSet
     *   - Sends write to writeBuffer of each UP DataManager storing the variable
     *   - Updates transaction's writeSites set
     *   - Updates transaction's firstAccessTimePerSite for each written site
     *   - Prints: "W(T1, x2, 50) -> sites: 1 2 3 ..."
     */
    void write(const std::string& transactionId, int variableId, int value);
    
    /**
     * end - End transaction with validation
     * Author: Aishwarya Anand
     * Input:
     *   - transactionId (string): ID of the transaction to end
     * Output: Prints commit or abort message to stdout
     * Description: Performs validation checks (failure rule, first-committer-wins, 
     *              RW-cycle) and either commits or aborts the transaction based on 
     *              validation results.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Calls validation methods that may modify conflict graphs
     *   - On commit: updates all written DataManagers, adds to committedTransactions, 
     *     updates variableCommitHistory
     *   - On abort: discards buffered writes at all DataManagers
     *   - Removes transaction from active transactions map
     *   - Prints: "T1 commits" or "T1 aborts (reason)"
     */
    void end(const std::string& transactionId);
    
    // ========================================================================
    // SITE MANAGEMENT
    // ========================================================================
    
    /**
     * fail - Simulate site failure
     * Author: Archita Arora
     * Input:
     *   - siteId (int): ID of the site to fail (1-10)
     * Output: Prints failure message to stdout
     * Description: Simulates site failure by marking the site as down, recording 
     *              the failure time, and clearing uncommitted writes at that site.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Marks site as down in siteStates
     *   - Records failure interval with failTime in siteStates[siteId].failureHistory
     *   - Calls DataManager.onFailure() which clears writeBuffer
     *   - Committed data at the site remains intact
     *   - Prints: "Site X fails"
     */
    void fail(int siteId);
    
    /**
     * recover - Bring a failed site back online
     * Author: Archita Arora
     * Input:
     *   - siteId (int): ID of the site to recover (1-10)
     * Output: Prints recovery message to stdout
     * Description: Brings a failed site back online, closes the failure interval, 
     *              enables the read gate mechanism for replicated variables, and 
     *              retries waiting transactions.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Marks site as up in siteStates
     *   - Records recovery time in siteStates[siteId].failureHistory
     *   - Sets lastRecoveryTime in siteStates
     *   - Calls DataManager.onRecovery() which sets replicaReadEnabled[x] = false 
     *     for replicated variables
     *   - Calls retryWaitingTransactions() which may resume blocked transactions
     *   - Prints: "Site X recovers" and possibly "Retry: T1" messages
     */
    void recover(int siteId);
    
    // ========================================================================
    // QUERY OPERATIONS
    // ========================================================================
    
    /**
     * dump - Print database state
     * Author: Aishwarya Anand
     * Input: None
     * Output: Prints committed state of all variables at all sites to stdout
     * Description: Prints the current committed values of all variables at each site 
     *              in ascending order by variable ID. Includes down sites showing 
     *              their last committed state.
     * Side Effects:
     *   - Increments currentTimestamp by 1
     *   - Prints formatted output:
     *       === DUMP ===
     *       site 1 - x2: 20, x4: 40, ...
     *       site 2 - x1: 10, x2: 20, ...
     *       ...
     *       ============
     */
    void dump();
    
private:
    // ========================================================================
    // READ OPERATIONS (PRIVATE HELPERS)
    // ========================================================================
    
    /**
     * readFromHomeSite - Read unreplicated variable from home site
     * Author: Aishwarya Anand
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction performing the read
     *   - variableId (int): Unreplicated variable to read
     * Output: None (prints to stdout)
     * Description: Reads an unreplicated (odd-indexed) variable from its unique 
     *              home site. If the site is down, puts the transaction in WAITING state.
     * Side Effects:
     *   - If site is down: sets transaction to WAITING state
     *   - If site is up: 
     *       * Adds read to transaction's readSet
     *       * Adds site to transaction's criticalReadSites
     *       * Records firstAccessTimePerSite
     *       * Prints read value
     */
    void readFromHomeSite(std::shared_ptr<Transaction> txn, int variableId);
    
    /**
     * readReplicated - Read replicated variable from any valid site
     * Author: Archita Arora
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction performing the read
     *   - variableId (int): Replicated variable to read
     * Output: None (prints to stdout)
     * Description: Reads a replicated (even-indexed) variable from any site with 
     *              a valid snapshot. Computes valid sites, waits if needed, or 
     *              aborts if no valid snapshot exists.
     * Side Effects:
     *   - Calls computeValidSnapshotSites() to find valid sites
     *   - If no valid sites but potential sites exist: sets transaction to WAITING
     *   - If no potential sites exist: aborts transaction
     *   - If valid site found:
     *       * Adds read to transaction's readSet
     *       * Records firstAccessTimePerSite
     *       * Prints read value
     */
    void readReplicated(std::shared_ptr<Transaction> txn, int variableId);
    
    // ========================================================================
    // VALIDATION AT COMMIT TIME
    // ========================================================================
    
    /**
     * violatesFailureRule - Check Available Copies failure rule
     * Author: Aishwarya Anand
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction to validate
     * Output: Boolean (true if rule violated, false otherwise)
     * Description: Checks if any site written to or read from (for unreplicated 
     *              variables) failed after the transaction accessed it but before 
     *              commit time.
     * Side Effects: None (read-only validation)
     */
    bool violatesFailureRule(std::shared_ptr<Transaction> txn);
    
    /**
     * violatesFirstCommitterWins - Check write-write conflicts
     * Author: Archita Arora
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction to validate
     * Output: Boolean (true if conflict detected, false otherwise)
     * Description: Checks if any variable in the transaction's writeSet was 
     *              committed by another concurrent transaction (one that committed 
     *              after this transaction started but before it commits).
     * Side Effects: None (read-only validation)
     */
    bool violatesFirstCommitterWins(std::shared_ptr<Transaction> txn);
    
    /**
     * violatesReadWriteCycle - Check for dangerous RW cycles
     * Author: Aishwarya Anand
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction to validate
     * Output: Boolean (true if dangerous cycle detected, false otherwise)
     * Description: Detects dangerous structures (cycles with 2+ consecutive RW edges) 
     *              by first creating RW edges for this commit, then checking for 
     *              cycles using DFS and BFS.
     * Side Effects:
     *   - Calls createRWEdgesForCommit() which modifies incomingReadWriteConflicts 
     *     and outgoingReadWriteConflicts of transaction objects
     *   - Updates conflict graph edges for both active and committed transactions
     */
    bool violatesReadWriteCycle(std::shared_ptr<Transaction> txn);
    
    // ========================================================================
    // SITE AVAILABILITY CHECKING
    // ========================================================================
    
    /**
     * computeValidSnapshotSites - Find sites with valid snapshots
     * Author: Archita Arora
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction requesting the read
     *   - variableId (int): Variable to read
     * Output: Set of site IDs that can serve valid snapshots
     * Description: Determines which sites have a valid snapshot for the transaction 
     *              by checking if they: (1) are currently UP, (2) have a committed 
     *              version from before transaction start, (3) were up continuously 
     *              from that commit to transaction start.
     * Side Effects: None (read-only computation)
     */
    std::set<int> computeValidSnapshotSites(std::shared_ptr<Transaction> txn, int variableId);
    
    /**
     * wasSiteUpContinuously - Check continuous site availability
     * Author: Aishwarya Anand
     * Input:
     *   - siteId (int): Site to check
     *   - fromTime (int): Start of time range
     *   - toTime (int): End of time range
     * Output: Boolean (true if site was up continuously, false otherwise)
     * Description: Checks if the site had no failures during the specified time 
     *              range by examining the failureHistory.
     * Side Effects: None (delegates to SiteState.wasUpContinuously())
     */
    bool wasSiteUpContinuously(int siteId, int fromTime, int toTime);
    
    // ========================================================================
    // CYCLE DETECTION HELPERS
    // ========================================================================
    
    /**
     * createRWEdgesForCommit - Create read-write anti-dependency edges
     * Author: Archita Arora
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction that is committing
     * Output: None
     * Description: Creates read-write anti-dependency edges from all transactions 
     *              that read variables this transaction is writing. Checks both 
     *              active and committed transactions.
     * Side Effects:
     *   - Updates outgoingReadWriteConflicts of reading transactions
     *   - Updates incomingReadWriteConflicts of the committing transaction
     *   - Modifies conflict graph for cycle detection
     */
    void createRWEdgesForCommit(std::shared_ptr<Transaction> txn);
    
    /**
     * hasRWCyclePath - DFS-based cycle detection
     * Author: Aishwarya Anand
     * Input:
     *   - fromTxnId (string): Starting transaction for DFS
     *   - targetTxnId (string): Target transaction (cycle detection)
     *   - edgeCount (int): Number of edges traversed so far
     *   - visited (set<string>&): Set of visited transaction IDs
     * Output: Boolean (true if cycle with ≥2 edges found, false otherwise)
     * Description: Performs depth-first search to detect cycles in the RW conflict 
     *              graph, ensuring the cycle contains at least 2 RW edges.
     * Side Effects:
     *   - Modifies visited set during traversal
     *   - Traverses both active transactions and committedTransactions
     */
    bool hasRWCyclePath(const std::string& fromTxnId, const std::string& targetTxnId, 
                        int edgeCount, std::set<std::string>& visited);
    
    /**
     * hasPathViaRW - BFS-based path finding
     * Author: Archita Arora
     * Input:
     *   - fromTxnId (string): Source transaction
     *   - toTxnId (string): Destination transaction
     * Output: Boolean (true if path exists, false otherwise)
     * Description: Uses breadth-first search to check if there exists any path 
     *              through RW edges from source to destination transaction.
     * Side Effects: Creates temporary visited set and queue for BFS traversal
     */
    bool hasPathViaRW(const std::string& fromTxnId, const std::string& toTxnId);
    
    // ========================================================================
    // COMMIT/ABORT OPERATIONS
    // ========================================================================
    
    /**
     * commit - Commit a transaction
     * Author: Aishwarya Anand
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction to commit
     * Output: None (prints to stdout)
     * Description: Commits the transaction by persisting all buffered writes to 
     *              DataManagers, updating commit history, and moving transaction 
     *              to committedTransactions list.
     * Side Effects:
     *   - Sets transaction commitTime and status to COMMITTED
     *   - Calls DataManager.commitWrites() at all UP sites in writeSites
     *   - Updates variableCommitHistory for all written variables
     *   - Moves transaction from transactions to committedTransactions
     *   - Removes transaction from active transactions map
     *   - Prints: "T1 commits"
     */
    void commit(std::shared_ptr<Transaction> txn);
    
    /**
     * abort - Abort a transaction
     * Author: Archita Arora
     * Input:
     *   - txn (shared_ptr<Transaction>): Transaction to abort
     *   - reason (string): Reason for abort (e.g., "Site failure", "RW-cycle")
     * Output: None (prints to stdout)
     * Description: Aborts the transaction by discarding all buffered writes at 
     *              DataManagers and removing the transaction from active transactions.
     * Side Effects:
     *   - Sets transaction status to ABORTED
     *   - Calls DataManager.abortWrites() at all sites in writeSites
     *   - Removes transaction from active transactions map
     *   - Does NOT add to committedTransactions
     *   - Prints: "T1 aborts (reason)"
     */
    void abort(std::shared_ptr<Transaction> txn, const std::string& reason);
    
    // ========================================================================
    // RECOVERY HELPERS
    // ========================================================================
    
    /**
     * retryWaitingTransactions - Resume transactions after site recovery
     * Author: Aishwarya Anand
     * Input:
     *   - recoveredSiteId (int): ID of the site that just recovered
     * Output: None (may print retry messages)
     * Description: Scans all WAITING transactions to see if they were waiting for 
     *              the recovered site. If so, attempts to resume their read operation.
     * Side Effects:
     *   - Identifies waiting transactions that can now proceed
     *   - Calls resumeFromWaiting() on eligible transactions
     *   - Calls readFromHomeSite() or readReplicated() to retry the read
     *   - May print: "Retry: T1"
     *   - May complete reads and update transaction state
     */
    void retryWaitingTransactions(int recoveredSiteId);
    
    // ========================================================================
    // OUTPUT HELPERS
    // ========================================================================
    
    /**
     * printTransactionOutput - Format transaction outcome messages
     * Author: Archita Arora
     * Input:
     *   - transactionId (string): Transaction ID
     *   - committed (bool): Whether transaction committed or aborted
     *   - reason (string): Reason if aborted (optional, default="")
     * Output: Prints to stdout
     * Description: Helper function to format and print transaction outcome messages.
     * Side Effects: Prints commit or abort message with optional reason
     * Note: This method is declared but appears to be unused in the current 
     *       implementation. The commit() and abort() methods handle printing directly.
     */
    void printTransactionOutput(const std::string& transactionId, bool committed, 
                               const std::string& reason = "");
};

} // namespace RepCRec

#endif // TRANSACTIONMANAGER_H