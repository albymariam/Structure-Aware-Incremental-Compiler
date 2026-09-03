#ifndef LOGIN_H
#define LOGIN_H

#include <string>

class LoginSystem {
public:
    bool validateUser(const std::string& username, const std::string& password_hash);
    int getUserAccessLevel(int userId);
};

#endif // LOGIN_H
