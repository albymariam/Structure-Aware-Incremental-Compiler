#include "login.h"
#include <iostream>

// Implementation of LoginSystem
bool LoginSystem::validateUser(const std::string& username, const std::string& password_hash) {
    // Comment: Check credentials against hash
    if (username.empty() || password_hash.empty()) {
        return false;
    }
    return username == "admin" && password_hash == "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8";
}

int LoginSystem::getUserAccessLevel(int userId) {
    if (userId <= 0) {
        return 0; // Guest
    }
    if (userId == 1) {
        return 100; // Super Admin
    }
    return 10; // Standard User
}
