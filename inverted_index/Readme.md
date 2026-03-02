# Run unit tests

```bash
g++ -std=c++17 -Wall -Wextra inverted_index.cpp ../common/roaring/roaring.c test_inverted_index.cpp -o tests -I/opt/homebrew/Cellar/googletest/1.17.0/include -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main && ./tests
```
