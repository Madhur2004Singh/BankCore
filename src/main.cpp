#include "Bank.hpp"
#include "ThreadPool.hpp"

#include <iomanip>
#include <iostream>

static void printMoney(Bank::Cents cents) {
    std::cout << "Rs " << cents / 100 << '.'
              << std::setw(2) << std::setfill('0') << cents % 100
              << std::setfill(' ');
}

int main() {
    Bank bank;

    bank.createAccount(1001, 100000); // Rs 1,000.00
    bank.createAccount(1002, 25000);  // Rs 250.00

    auto printResult = [](const std::string& operation,
                          const Bank::TransactionResult& result) {
        std::cout << operation << ": "
                  << (result.success ? "SUCCESS - " : "FAILED - ")
                  << result.message << '\n';
    };

    printResult("Deposit Rs 50.00", bank.deposit(1001, 5000));
    printResult("Withdraw Rs 20.00", bank.withdraw(1001, 2000));
    printResult("Transfer Rs 100.00 from 1001 to 1002",
                bank.transfer(1001, 1002, 10000));

    std::cout << "\nFinal balances:\n";
    for (const auto& account : bank.snapshot()) {
        std::cout << "Account " << account.id << ": ";
        printMoney(account.balance);
        std::cout << '\n';
    }

    // Tiny thread-pool demonstration: four tasks deposit Rs 1.00 each.
    ThreadPool pool(4);
    std::vector<std::future<Bank::TransactionResult>> results;
    for (int i = 0; i < 4; ++i) {
        results.push_back(pool.submit([&bank]() {
            return bank.deposit(1001, 100);
        }));
    }
    for (auto& result : results) {
        if (!result.get().success) {
            std::cerr << "A demo deposit failed\n";
            return 1;
        }
    }

    Bank::Cents finalBalance = 0;
    bank.getBalance(1001, finalBalance);
    std::cout << "\nAccount 1001 after four concurrent Rs 1.00 deposits: ";
    printMoney(finalBalance);
    std::cout << '\n';

    return 0;
}
