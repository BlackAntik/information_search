# Run unit tests
```bash
g++ -std=c++17 -Wall -Wextra tests.cpp -o tests \
    -I/opt/homebrew/Cellar/googletest/1.17.0/include \
    -L/opt/homebrew/Cellar/googletest/1.17.0/lib \
    -lgtest -lgtest_main && ./tests
```

# Run benchmark

```bash
g++ -std=c++17 -O2 -DNDEBUG \
    lsm_benchmark.cpp \
    -I/opt/homebrew/include \
    -L/opt/homebrew/lib \
    -lbenchmark -pthread -o lsm_bench
```

```bash
./lsm_bench --benchmark_min_warmup_time=2
```