#include "Bank.hpp"
#include "ThreadPool.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

struct WorkItem {
    int accountId;
    bool deposit;
    Bank::Cents amount;
};

static void runBenchmark(std::size_t threadCount, int operationCount) {
    Bank bank;
    constexpr int accountCount = 100;
    for (int id = 1; id <= accountCount; ++id) {
        bank.createAccount(id, 1'000'000'000);
    }

    std::vector<WorkItem> work;
    work.reserve(static_cast<std::size_t>(operationCount));
    for (int i = 0; i < operationCount; ++i) {
        work.push_back({(i % accountCount) + 1, (i % 2 == 0), 1});
    }

    const auto start = Clock::now();

    if (threadCount == 1) {
        for (const auto& item : work) {
            if (item.deposit) {
                (void)bank.deposit(item.accountId, item.amount);
            } else {
                (void)bank.withdraw(item.accountId, item.amount);
            }
        }
    } else {
        ThreadPool pool(threadCount);
        std::vector<std::future<Bank::TransactionResult>> futures;
        futures.reserve(work.size());

        for (const auto& item : work) {
            futures.push_back(pool.submit([&bank, item]() {
                if (item.deposit) {
                    return bank.deposit(item.accountId, item.amount);
                }
                return bank.withdraw(item.accountId, item.amount);
            }));
        }

        for (auto& future : futures) {
            (void)future.get();
        }
    }

    const auto end = Clock::now();
    const double seconds = std::chrono::duration<double>(end - start).count();
    const double tps = static_cast<double>(operationCount) / seconds;

    std::cout << std::left << std::setw(12) << threadCount
              << std::right << std::setw(16) << std::fixed << std::setprecision(2)
              << seconds << std::setw(20) << std::setprecision(0) << tps << '\n';
}

int main(int argc, char** argv) {
    int operations = 200000;
    if (argc >= 2) {
        try {
            operations = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Usage: " << argv[0] << " [operation-count]\n";
            return 2;
        }
    }
    if (operations <= 0) {
        std::cerr << "Operation count must be positive.\n";
        return 2;
    }

    std::cout << "BankCore Lite microbenchmark\n";
    std::cout << "Operations per run: " << operations << '\n';
    std::cout << std::left << std::setw(12) << "Threads"
              << std::right << std::setw(16) << "Seconds"
              << std::setw(20) << "Operations/sec" << '\n';

    runBenchmark(1, operations);
    runBenchmark(2, operations);
    runBenchmark(4, operations);
    runBenchmark(8, operations);
    return 0;
}
