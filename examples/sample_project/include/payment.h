#ifndef PAYMENT_H
#define PAYMENT_H

class PaymentProcessor {
public:
    double calculateTotal(double price, int quantity, double taxRate);
    bool processTransaction(int accountId, double amount);
};

#endif // PAYMENT_H
