#include <iostream>
#include "login.h"
#include "payment.h"
#include "database.h"

int main() {
    std::cout << "=== Structure-Aware C++ Target App Execution ===" << std::endl;
    
    LoginSystem login;
    bool isValid = login.validateUser("admin", "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8");
    std::cout << "Login Status: " << (isValid ? "SUCCESS" : "FAILED") << std::endl;

    PaymentProcessor payment;
    double total = payment.calculateTotal(99.99, 2, 0.08);
    std::cout << "Calculated Order Total: $" << total << std::endl;

    DatabaseManager db;
    if (db.connect("app_production_db")) {
        std::string record = db.fetchRecord(101);
        std::cout << "Retrieved: " << record << std::endl;
        db.disconnect();
    }

    return 0;
}
