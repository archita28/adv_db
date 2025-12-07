// DataManager.h
// Author: Aishwarya and Archita
// Date: December 2025
// Purpose: Manages data storage and versioning at a single site

#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include "Version.h"
#include "Constants.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace RepCRec {

class DataManager {
private:
    int siteId;
    bool isUp;
    int lastRecoveryTime;
    
    // Data storage: variableId -> list of versions (sorted by commitTimestamp)
    std::map<int, std::vector<Version>> dataStore;
    
    // Write buffers: transactionId -> (variableId -> value)
    std::map<std::string, std::map<int, int>> writeBuffer;
    
    // Replicated variable read gates (for post-recovery reads)
    std::map<int, bool> replicaReadEnabled;
    
public:
    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================
    
    /**
     * DataManager Constructor
     * Author: Aishwarya Anand
     * Input: 
     *   - siteId (int): Site identifier (1-10)
     * Output: None (constructor)
     * Description: Creates a DataManager for the specified site, sets initial 
     *              state to UP, and calls initialize() to populate variables 
     *              with their initial values.
     * Side Effects:
     *   - Sets siteId to the provided value
     *   - Sets isUp to true
     *   - Sets lastRecoveryTime to 0
     *   - Calls initialize() which populates dataStore with initial versions
     *   - Sets replicaReadEnabled[x] = true for all even-indexed variables
     */
    DataManager(int siteId);
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    
    /**
     * initialize - Populate site with initial variable values
     * Author: Archita Arora
     * Input: None
     * Output: None
     * Description: Determines which variables belong to this site based on 
     *              replication rules (odd variables at home site, even variables 
     *              at all sites) and creates initial versions with values 10×i.
     * Side Effects:
     *   - Populates dataStore with initial Version objects for each variable
     *   - Each version has value = 10×variableId, commitTimestamp = 0
     *   - Sets replicaReadEnabled[x] = true for all replicated (even) variables
     */
    void initialize();
    
    // ========================================================================
    // READ OPERATIONS
    // ========================================================================
    
    /**
     * readVariable - Read a variable at a specific snapshot time
     * Author: Aishwarya Anand
     * Input:
     *   - variableId (int): Variable to read (1-20)
     *   - snapshotTime (int): Timestamp for snapshot isolation
     * Output: shared_ptr<Version> - Version object or nullptr if unavailable
     * Description: Returns the latest version of the variable committed at or 
     *              before snapshotTime. For replicated variables, checks the 
     *              read gate (replicaReadEnabled) and returns nullptr if closed.
     * Side Effects: None (read-only operation)
     * Note: Returns nullptr if:
     *   - Variable doesn't exist at this site
     *   - Replicated variable has closed read gate (post-recovery)
     *   - No version committed before or at snapshotTime
     */
    std::shared_ptr<Version> readVariable(int variableId, int snapshotTime);
    
    // ========================================================================
    // WRITE OPERATIONS
    // ========================================================================
    
    /**
     * writeVariable - Buffer a write for a transaction
     * Author: Archita Arora
     * Input:
     *   - variableId (int): Variable to write (1-20)
     *   - value (int): Value to write
     *   - transactionId (string): Transaction performing the write
     * Output: None
     * Description: Stores the write in a temporary buffer without modifying the 
     *              committed dataStore. The write will be applied only if the 
     *              transaction commits successfully.
     * Side Effects:
     *   - Adds entry to writeBuffer[transactionId][variableId] = value
     *   - Does NOT modify dataStore (committed versions)
     */
    void writeVariable(int variableId, int value, const std::string& transactionId);
    
    /**
     * commitWrites - Persist buffered writes to committed storage
     * Author: Aishwarya Anand
     * Input:
     *   - transactionId (string): Transaction to commit
     *   - commitTimestamp (int): Commit time for new versions
     * Output: None
     * Description: Moves all buffered writes for the transaction from writeBuffer 
     *              to dataStore as new committed versions. Opens read gate for 
     *              replicated variables that receive writes.
     * Side Effects:
     *   - Creates new Version objects in dataStore with commitTimestamp
     *   - Sets replicaReadEnabled[x] = true for each replicated variable written
     *   - Removes transaction's entries from writeBuffer
     *   - Versions are appended to dataStore in chronological order
     */
    void commitWrites(const std::string& transactionId, int commitTimestamp);
    
    /**
     * abortWrites - Discard buffered writes for a transaction
     * Author: Archita Arora
     * Input:
     *   - transactionId (string): Transaction to abort
     * Output: None
     * Description: Removes all buffered writes for the transaction without 
     *              modifying committed data.
     * Side Effects:
     *   - Removes transaction's entries from writeBuffer
     *   - Does NOT modify dataStore (no changes to committed versions)
     */
    void abortWrites(const std::string& transactionId);
    
    // ========================================================================
    // FAILURE AND RECOVERY
    // ========================================================================
    
    /**
     * onFailure - Handle site failure
     * Author: Aishwarya Anand
     * Input: None
     * Output: None
     * Description: Marks the site as down and discards all uncommitted writes. 
     *              Committed data remains intact for when the site recovers.
     * Side Effects:
     *   - Sets isUp = false
     *   - Clears entire writeBuffer (all uncommitted writes are lost)
     *   - dataStore (committed versions) remains unchanged
     */
    void onFailure();
    
    /**
     * onRecovery - Handle site recovery
     * Author: Archita Arora
     * Input:
     *   - currentTime (int): Recovery timestamp
     * Output: None
     * Description: Marks the site as up and implements the read gate mechanism 
     *              for replicated variables to prevent stale reads. Unreplicated 
     *              variables are immediately readable.
     * Side Effects:
     *   - Sets isUp = true
     *   - Sets lastRecoveryTime = currentTime
     *   - For all replicated (even-indexed) variables at this site:
     *       Sets replicaReadEnabled[x] = false (closes read gate)
     *   - Unreplicated variables remain immediately readable
     *   - Read gates will be opened when new writes commit
     */
    void onRecovery(int currentTime);
    
    // ========================================================================
    // QUERY OPERATIONS
    // ========================================================================
    
    /**
     * getCommittedState - Get current committed values for dump
     * Author: Aishwarya Anand
     * Input: None
     * Output: Map<int, int> - variableId → latest committed value
     * Description: Returns the most recent committed value of each variable 
     *              stored at this site. Used by the dump() command.
     * Side Effects: None (read-only operation)
     */
    std::map<int, int> getCommittedState() const;
    
    /**
     * isReplicaReadable - Check if replicated variable is readable
     * Author: Archita Arora
     * Input:
     *   - variableId (int): Variable to check
     * Output: Boolean - true if readable, false if read gate is closed
     * Description: Checks if a replicated variable's read gate is open. 
     *              Unreplicated variables always return true. Used to enforce 
     *              post-recovery read restrictions.
     * Side Effects: None (read-only operation)
     */
    bool isReplicaReadable(int variableId) const;
    
    // ========================================================================
    // GETTERS
    // ========================================================================
    
    /**
     * getIsUp - Check if site is currently up
     * Author: Aishwarya Anand
     * Input: None
     * Output: Boolean - true if site is up, false if down
     * Description: Returns the current operational status of the site.
     * Side Effects: None (read-only inline getter)
     */
    bool getIsUp() const { return isUp; }
    
    /**
     * getSiteId - Get site identifier
     * Author: Archita Arora
     * Input: None
     * Output: int - Site ID (1-10)
     * Description: Returns the site identifier for this DataManager.
     * Side Effects: None (read-only inline getter)
     */
    int getSiteId() const { return siteId; }
    
    /**
     * getDataStore - Get direct access to version history
     * Author: Aishwarya Anand
     * Input: None
     * Output: Reference to map<int, vector<Version>> - dataStore
     * Description: Provides direct access to the dataStore for snapshot 
     *              validation in TransactionManager. Used by 
     *              computeValidSnapshotSites() to check version timestamps 
     *              and continuous site availability.
     * Side Effects: None (returns reference, but used read-only by caller)
     * Note: This is a convenience method to avoid copying large version histories
     */
    std::map<int, std::vector<Version>>& getDataStore() { return dataStore; }
};

} // namespace RepCRec

#endif // DATAMANAGER_H