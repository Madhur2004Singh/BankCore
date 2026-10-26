# BankCore Lite

A compact C++20 systems project demonstrating thread-safe banking operations,
a worker thread pool, concurrency tests, and a basic throughput benchmark.

## Build on Ubuntu

```bash
sudo apt update
sudo apt install -y build-essential cmake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

```bash
./build/bank_demo
ctest --test-dir build --output-on-failure
./build/bank_benchmark 200000
```

For a smaller benchmark run, use `./build/bank_benchmark 20000`.

## Design notes

- Money is stored as integer cents/paise, not floating-point values.
- `accountsMutex_` protects the account map.
- Each account has its own mutex, so unrelated accounts can be updated concurrently.
- Transfers lock both accounts together using `std::scoped_lock`, avoiding the
  common opposite-transfer lock-order deadlock.
- The thread pool uses a condition variable to sleep workers when no tasks are
  available.
- Tests cover basic validation, concurrent withdrawals, and opposite-direction
  transfers.

## Limitations

This is an educational in-memory simulator, not production banking software.
It has no authentication, durable storage, audit log, network API, or database.
The benchmark includes task-queue and future overhead and is not a universal
measure of C++ performance. Results depend on CPU, compiler, build mode, and
workload; report measured results with those conditions.

## Suggested experiments

1. Compare `Debug` and `Release` builds.
2. Run 20,000, 200,000, and 1,000,000 operations.
3. Compare 1, 2, 4, and 8 workers.
4. Build with ThreadSanitizer in a separate build directory and run tests.
