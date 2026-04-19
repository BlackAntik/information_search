# Run unit tests

## TF-IDF

```bash
g++ -std=c++17 -Wall -Wextra tf_idf.cpp test_tf_idf.cpp -o tests -I/opt/homebrew/Cellar/googletest/1.17.0/include -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main && ./tests
```

## Vector Space Model

```bash
g++ -std=c++17 -Wall -Wextra vector-space-model.cpp test_vector-space-model.cpp -o tests_vsm -I/opt/homebrew/Cellar/googletest/1.17.0/include -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main && ./tests_vsm
```
