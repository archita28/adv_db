// DataManager.cpp
// Author: Aishwarya and Archita
// Date: December 2025
// Purpose: Implementation of DataManager methods

#include "DataManager.h"
#include <algorithm>
#include <iostream>

namespace RepCRec {

DataManager::DataManager(int id) 
    : siteId(id), isUp(true), lastRecoveryTime(0) {
    initialize();
}

void DataManager::initialize() {
    for (int varId = 1; varId <= NUM_VARIABLES; varId++) {
        std::set<int> sites = getVariableSites(varId);
        
        if (sites.find(siteId) != sites.end()) {
            int initialValue = varId * INITIAL_VALUE_MULTIPLIER;
            dataStore[varId].push_back(Version(initialValue));
            
            if (isReplicatedVariable(varId)) {
                replicaReadEnabled[varId] = true;
            }
        }
    }
}

std::shared_ptr<Version> DataManager::readVariable(int variableId, int snapshotTime) {
    if (dataStore.find(variableId) == dataStore.end()) {
        return nullptr;
    }
    
    if (isReplicatedVariable(variableId) && !replicaReadEnabled[variableId]) {
        return nullptr;  
    }
    
    auto& versions = dataStore[variableId];
    std::shared_ptr<Version> result = nullptr;
    
    for (auto it = versions.rbegin(); it != versions.rend(); ++it) {
        if (it->commitTimestamp <= snapshotTime) {
            result = std::make_shared<Version>(*it);
            break;
        }
    }
    
    return result;
}

void DataManager::writeVariable(int variableId, int value, const std::string& transactionId) {
    writeBuffer[transactionId][variableId] = value;
}

void DataManager::commitWrites(const std::string& transactionId, int commitTimestamp) {
    if (writeBuffer.find(transactionId) == writeBuffer.end()) {
        return;  
    }
    
    auto& writes = writeBuffer[transactionId];
    
    for (const auto& [variableId, value] : writes) {
        dataStore[variableId].push_back(Version(value, commitTimestamp, transactionId));
        
        if (isReplicatedVariable(variableId)) {
            replicaReadEnabled[variableId] = true;
        }
    }
    
    writeBuffer.erase(transactionId);
}

void DataManager::abortWrites(const std::string& transactionId) {
    writeBuffer.erase(transactionId);
}

void DataManager::onFailure() {
    isUp = false;
    writeBuffer.clear();
}

void DataManager::onRecovery(int currentTime) {
    isUp = true;
    lastRecoveryTime = currentTime;
    
    for (int varId = 1; varId <= NUM_VARIABLES; varId++) {
        if (isReplicatedVariable(varId) && 
            dataStore.find(varId) != dataStore.end()) {
            replicaReadEnabled[varId] = false;
        }
    }
}

std::map<int, int> DataManager::getCommittedState() const {
    std::map<int, int> state;
    
    for (const auto& [variableId, versions] : dataStore) {
        if (!versions.empty()) {
            state[variableId] = versions.back().value;
        }
    }
    
    return state;
}

bool DataManager::isReplicaReadable(int variableId) const {
    if (!isReplicatedVariable(variableId)) {
        return true;  
    }
    
    auto it = replicaReadEnabled.find(variableId);
    if (it == replicaReadEnabled.end()) {
        return false;  
    }
    
    return it->second;
}

} 