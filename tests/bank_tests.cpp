#include "Bank.hpp"
#include "ThreadPool.hpp"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main() {
    try {
        Bank bank;
        require(bank.createAccount(1, 1000), "create account should succeed");
        require(!bank.createAccount(1, 0), "duplicate account should fail");
        require(!bank.createAccount(2, -1), "negative opening balance should fail");
        require(bank.createAccount(2, 0), "second account should succeed");

        require(bank.deposit(1, 500).success, "deposit should succeed");
        require(bank.withdraw(1, 200).success, "withdrawal should succeed");
        require(!bank.withdraw(1, 5000).success, "overdraft should fail");
        require(!bank.deposit(1, 0).success, "zero deposit should fail");
        require(bank.transfer(1, 2, 300).success, "transfer should succeed");

        Bank::Cents balance1 = 0;
        Bank::Cents balance2 = 0;
        require(bank.getBalance(1, balance1), "account 1 should exist");
        require(bank.getBalance(2, balance2), "account 2 should exist");
        require(balance1 == 1000, "account 1 balance should be 1000 cents");
        require(balance2 == 300, "account 2 balance should be 300 cents");
        require(!bank.transfer(1, 1, 1).success, "self transfer should fail");

        // Exactly ten of these 100 withdrawals can succeed.
        Bank concurrentBank;
        require(concurrentBank.createAccount(99, 1000), "create concurrent account");
        ThreadPool pool(8);
        std::atomic<int> successfulWithdrawals{0};
        std::vector<std::future<void>> jobs;
        jobs.reserve(100);

        for (int i = 0; i < 100; ++i) {
            jobs.push_back(pool.submit([&concurrentBank, &successfulWithdrawals]() {
                if (concurrentBank.withdraw(99, 100).success) {
                    successfulWithdrawals.fetch_add(1, std::memory_order_relaxed);
                }
            }));
        }
        for (auto& job : jobs) {
            job.get();
        }

        Bank::Cents finalBalance = -1;
        require(concurrentBank.getBalance(99, finalBalance), "account 99 should exist");
        require(successfulWithdrawals.load() == 10,
                "exactly ten withdrawals should succeed");
        require(finalBalance == 0, "balance must never become negative");

        // Concurrent transfers in opposite directions should not deadlock.
        Bank transferBank;
        require(transferBank.createAccount(10, 100000), "create account 10");
        require(transferBank.createAccount(20, 100000), "create account 20");
        {
            ThreadPool transferPool(8);
            std::vector<std::future<void>> transferJobs;
            for (int i = 0; i < 1000; ++i) {
                transferJobs.push_back(transferPool.submit([&transferBank, i]() {
                    if (i % 2 == 0) {
                        (void)transferBank.transfer(10, 20, 1);
                    } else {
                        (void)transferBank.transfer(20, 10, 1);
                    }
                }));
            }
            for (auto& job : transferJobs) {
                job.get();
            }
        }

        Bank::Cents b10 = 0;
        Bank::Cents b20 = 0;
        transferBank.getBalance(10, b10);
        transferBank.getBalance(20, b20);
        require(b10 + b20 == 200000, "transfers must preserve total funds");

        std::cout << "All BankCore Lite tests passed.\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "TEST FAILURE: " << ex.what() << '\n';
        return 1;
    }
}
