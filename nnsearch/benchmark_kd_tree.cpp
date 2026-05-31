#include "kd-tree.h"
#include <iostream>
#include <random>
#include <utility>
#include <vector>

template <std::size_t N>
void RunBenchmark(std::mt19937& rng) {
    std::uniform_real_distribution<double> dist(-1e6, 1e6);

    std::vector<Point<N>> points(100000);
    for (auto& p : points) {
        for (auto& coord : p) {
            coord = dist(rng);
        }
    }

    KDTree<N> tree(std::move(points));

    double total_visited = 0;
    for (int i = 0; i < 1000; ++i) {
        Point<N> query;
        for (auto& coord : query) {
            coord = dist(rng);
        }
        auto [closest, cnt] = tree.GetClosest(query);
        total_visited += cnt;
    }

    std::cout << "dim=" << N << " avg_visited=" << total_visited / 1000.0 << "\n";
}

template <std::size_t... Dims>
void RunAll(std::mt19937& rng, std::index_sequence<Dims...>) {
    (RunBenchmark<Dims>(rng), ...);
}

int main() {
    std::mt19937 rng(42);
    RunAll(rng, std::index_sequence<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 20, 30, 40, 50>{});
    return 0;
}
