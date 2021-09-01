// intx: extended precision integer library.
// Copyright 2019-2020 Pawel Bylica.
// Licensed under the Apache License, Version 2.0.

#include <benchmark/benchmark.h>
#include <intx/int128.hpp>
#include <array>


template <typename T, unsigned ClzFn(T)>
static void clz(benchmark::State& state)
{
    constexpr int input_size = 1000;
    std::array<uint64_t, input_size> inputs{};
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        const auto s = i % 65;
        inputs[i] = s == 64 ? 0 : (uint64_t{1} << 63) >> s;
    }

    benchmark::ClobberMemory();
    while (state.KeepRunningBatch(inputs.size()))
    {
        for (auto& in : inputs)
            in = ClzFn(static_cast<T>(in));
    }
    benchmark::DoNotOptimize(inputs.data());
}
BENCHMARK_TEMPLATE(clz, uint32_t, intx::clz);
BENCHMARK_TEMPLATE(clz, uint32_t, intx::clz_generic);
BENCHMARK_TEMPLATE(clz, uint64_t, intx::clz);
BENCHMARK_TEMPLATE(clz, uint64_t, intx::clz_generic);
