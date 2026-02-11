#include <benchmark/benchmark.h>
#include "lsm.h"
#include <random>
#include <string>
#include <vector>
#include <algorithm>

static std::vector<std::string> GenerateSequentialKeys(size_t count) {
    std::vector<std::string> keys;
    keys.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%010zu", i);
        keys.emplace_back(buf);
    }
    return keys;
}

class LSMFixture : public benchmark::Fixture {
protected:
    LSM<int> lsm{100000, 10};
    std::vector<std::string> keys;

    void SetUp(const ::benchmark::State&) override {
        keys = GenerateSequentialKeys(1'000'000);
        for (size_t i = 0; i < keys.size(); ++i) {
            lsm.Put(keys[i], static_cast<int>(i));
        }
    }
};

BENCHMARK_DEFINE_F(LSMFixture, BM_Put)(benchmark::State& state) {
    size_t n = state.range(0);
    auto insert_keys = GenerateSequentialKeys(n);
    std::shuffle(insert_keys.begin(), insert_keys.end(), std::mt19937{std::random_device{}()});

    for (auto _ : state) {
        for (const auto& k : insert_keys) {
            lsm.Put(k, 42);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK_REGISTER_F(LSMFixture, BM_Put)
    ->RangeMultiplier(10)
    ->Range(1000, 100000);

BENCHMARK_DEFINE_F(LSMFixture, BM_Get_Hit)(benchmark::State& state) {
    size_t stride = std::max<size_t>(1, keys.size() / 10000);

    for (auto _ : state) {
        for (size_t i = 0; i < keys.size(); i += stride) {
            auto opt = lsm.Get(keys[i]);
            benchmark::DoNotOptimize(opt);
        }
    }
    state.SetItemsProcessed(state.iterations() * (keys.size() / stride));
}
BENCHMARK_REGISTER_F(LSMFixture, BM_Get_Hit);

BENCHMARK_DEFINE_F(LSMFixture, BM_Get_Miss)(benchmark::State& state) {
    for (auto _ : state) {
        auto opt1 = lsm.Get("zzzzzzzzzzzzzzzz" + std::to_string(rand()));
        auto opt2 = lsm.Get("non-existent-key-999999" + std::to_string(rand()));
        benchmark::DoNotOptimize(opt1);
        benchmark::DoNotOptimize(opt2);
    }
     state.SetItemsProcessed(2 * state.iterations());
}
BENCHMARK_REGISTER_F(LSMFixture, BM_Get_Miss);

BENCHMARK_DEFINE_F(LSMFixture, BM_SCAN)(benchmark::State& state) {
    const auto keys = GenerateSequentialKeys(100);
    for (auto _ : state) {
        for (const auto& k : keys) {
            lsm.Get(k);
        }

        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * keys.size());
}
BENCHMARK_REGISTER_F(LSMFixture, BM_SCAN);

BENCHMARK_MAIN();