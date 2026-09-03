#include "database.h"
#include <iostream>

bool DatabaseManager::connect(const std::string& dbName) {
    if (dbName.empty()) {
        return false;
    }
    std::cout << "[DatabaseManager] Connected to database: " << dbName << std::endl;
    return true;
}

std::string DatabaseManager::fetchRecord(int recordId) {
    if (recordId <= 0) {
        return "NULL_RECORD";
    }
    return "Record_Data_For_ID_" + std::to_string(recordId);
}

void DatabaseManager::disconnect() {
    std::cout << "[DatabaseManager] Disconnected safely." << std::endl;
}
