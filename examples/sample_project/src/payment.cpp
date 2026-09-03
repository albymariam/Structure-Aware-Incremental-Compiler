#include "payment.h"
#include <iostream>

double PaymentProcessor::calculateTotal(double price, int quantity, double taxRate) {
    // Total calculation logic
    double subtotal = price * quantity;
    double taxAmount = subtotal * taxRate;
    return subtotal + taxAmount;
}

bool PaymentProcessor::processTransaction(int accountId, double amount) {
    if (accountId <= 0 || amount <= 0.0) {
        return false;
    }
    std::cout << "[PaymentProcessor] Charged $" << amount << " to account " << accountId << std::endl;
    return true;
}
