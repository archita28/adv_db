// Constants.h
// Author: RepCRec Team
// Date: December 2024
// Purpose: Define system-wide constants for the distributed database
// No side effects - header only

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>
#include <set>

namespace RepCRec {

// System configuration
const int NUM_VARIABLES = 20;
const int NUM_SITES = 10;
const int INITIAL_VALUE_MULTIPLIER = 10;

// Variable distribution rules
inline bool isOddVariable(int varId) {
    return varId % 2 == 1;
}

inline bool isReplicatedVariable(int varId) {
    return varId % 2 == 0;
}

// Get the home site for odd-indexed variables
inline int getHomeSite(int varId) {
    return 1 + (varId % NUM_SITES);
}

// Get all sites that store a variable
inline std::set<int> getVariableSites(int varId) {
    std::set<int> sites;
    if (isOddVariable(varId)) {
        sites.insert(getHomeSite(varId));
    } else {
        for (int i = 1; i <= NUM_SITES; i++) {
            sites.insert(i);
        }
    }
    return sites;
}

// Transaction status
enum class TransactionStatus {
    ACTIVE,
    WAITING,
    COMMITTED,
    ABORTED
};

inline std::string statusToString(TransactionStatus status) {
    switch(status) {
        case TransactionStatus::ACTIVE: return "ACTIVE";
        case TransactionStatus::WAITING: return "WAITING";
        case TransactionStatus::COMMITTED: return "COMMITTED";
        case TransactionStatus::ABORTED: return "ABORTED";
    }
    return "UNKNOWN";
}

} // namespace RepCRec

#endif // CONSTANTS_H