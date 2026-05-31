# Run unit tests

```bash
g++ -std=c++17 -Wall -Wextra test_kd_tree.cpp -o tests \
    -I/opt/homebrew/Cellar/googletest/1.17.0/include \
    -L/opt/homebrew/Cellar/googletest/1.17.0/lib \
    -lgtest -lgtest_main && ./tests
```

# Run benchmark

```bash
g++ -std=c++17 -O2 -Wall -Wextra benchmark_kd_tree.cpp -o benchmark && ./benchmark
```
