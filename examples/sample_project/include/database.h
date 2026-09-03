#ifndef DATABASE_H
#define DATABASE_H

#include <string>

class DatabaseManager {
public:
    bool connect(const std::string& dbName);
    std::string fetchRecord(int recordId);
    void disconnect();
};

#endif // DATABASE_H
