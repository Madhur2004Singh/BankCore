#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Bank {
public:
    using Cents = std::int64_t;

    struct TransactionResult {
        bool success;
        std::string message;
    };

    struct AccountSnapshot {
        int id;
        Cents balance;
    };

    bool createAccount(int id, Cents openingBalance = 0);
    TransactionResult deposit(int id, Cents amount);
    TransactionResult withdraw(int id, Cents amount);
    TransactionResult transfer(int fromId, int toId, Cents amount);
    bool getBalance(int id, Cents& balance) const;
    std::vector<AccountSnapshot> snapshot() const;

private:
    struct Account {
        explicit Account(int accountId, Cents initialBalance)
            : id(accountId), balance(initialBalance) {}

        int id;
        Cents balance;                 // Protected by mutex.
        mutable std::mutex mutex;      // Protects this account's balance.
    };

    std::shared_ptr<Account> findAccount(int id) const;

    mutable std::mutex accountsMutex_; // Protects the map itself.
    std::unordered_map<int, std::shared_ptr<Account>> accounts_;
};
