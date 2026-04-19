# Run unit tests

```bash
g++ -std=c++17 -Wall -Wextra positional_index.cpp test_positional_index.cpp -o tests -I/opt/homebrew/Cellar/googletest/1.17.0/include -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main && ./tests
```
