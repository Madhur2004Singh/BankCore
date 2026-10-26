#include "Bank.hpp"

#include <limits>
#include <mutex>

bool Bank::createAccount(int id, Cents openingBalance) {
    if (id <= 0 || openingBalance < 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(accountsMutex_);
    if (accounts_.contains(id)) {
        return false;
    }

    accounts_.emplace(id, std::make_shared<Account>(id, openingBalance));
    return true;
}

std::shared_ptr<Bank::Account> Bank::findAccount(int id) const {
    std::lock_guard<std::mutex> lock(accountsMutex_);
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
        return nullptr;
    }
    return it->second;
}

Bank::TransactionResult Bank::deposit(int id, Cents amount) {
    if (amount <= 0) {
        return {false, "Amount must be positive"};
    }

    auto account = findAccount(id);
    if (!account) {
        return {false, "Account not found"};
    }

    std::lock_guard<std::mutex> lock(account->mutex);
    if (account->balance > std::numeric_limits<Cents>::max() - amount) {
        return {false, "Balance overflow"};
    }

    account->balance += amount;
    return {true, "Deposit successful"};
}

Bank::TransactionResult Bank::withdraw(int id, Cents amount) {
    if (amount <= 0) {
        return {false, "Amount must be positive"};
    }

    auto account = findAccount(id);
    if (!account) {
        return {false, "Account not found"};
    }

    std::lock_guard<std::mutex> lock(account->mutex);
    if (account->balance < amount) {
        return {false, "Insufficient funds"};
    }

    account->balance -= amount;
    return {true, "Withdrawal successful"};
}

Bank::TransactionResult Bank::transfer(int fromId, int toId, Cents amount) {
    if (amount <= 0) {
        return {false, "Amount must be positive"};
    }
    if (fromId == toId) {
        return {false, "Source and destination must differ"};
    }

    // Account pointers are copied while holding the map mutex, then the map
    // mutex is released before account locks are acquired.
    auto from = findAccount(fromId);
    auto to = findAccount(toId);
    if (!from || !to) {
        return {false, "Source or destination account not found"};
    }

    // std::scoped_lock locks both mutexes without the usual lock-order
    // deadlock risk, even if another thread transfers in the opposite direction.
    std::scoped_lock<std::mutex, std::mutex> lock(from->mutex, to->mutex);

    if (from->balance < amount) {
        return {false, "Insufficient funds"};
    }
    if (to->balance > std::numeric_limits<Cents>::max() - amount) {
        return {false, "Destination balance overflow"};
    }

    // Both balances are protected at once: observers cannot see a half-transfer.
    from->balance -= amount;
    to->balance += amount;
    return {true, "Transfer successful"};
}

bool Bank::getBalance(int id, Cents& balance) const {
    auto account = findAccount(id);
    if (!account) {
        return false;
    }

    std::lock_guard<std::mutex> lock(account->mutex);
    balance = account->balance;
    return true;
}

std::vector<Bank::AccountSnapshot> Bank::snapshot() const {
    std::vector<std::shared_ptr<Account>> accounts;
    {
        std::lock_guard<std::mutex> lock(accountsMutex_);
        accounts.reserve(accounts_.size());
        for (const auto& [id, account] : accounts_) {
            (void)id;
            accounts.push_back(account);
        }
    }

    std::vector<AccountSnapshot> result;
    result.reserve(accounts.size());
    for (const auto& account : accounts) {
        std::lock_guard<std::mutex> lock(account->mutex);
        result.push_back({account->id, account->balance});
    }
    return result;
}
