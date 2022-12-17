#include <benchmark/benchmark.h>

#include <cstdlib>
#include <vector>
#include <stable_vector.hpp>
template <typename T>
static void populate(T& v, size_t max)
{
    for (size_t i = 0; i != max; ++i)
    {
      v.push_back(i);
    }
}

template <typename T>
static size_t measure_populate(benchmark::State& state)
{
    size_t count = 0;
    auto max = state.range();
    for (auto&& _ : state)
    {
        T t;
        state.ResumeTiming();
        populate(t, max);
        state.PauseTiming();
        ++count;
    }
    return count;
}

template <typename T>
static size_t measure_destroy(benchmark::State& state)
{
    size_t count = 0;
    auto max = state.range();
    for (auto&& _ : state)
    {
        state.PauseTiming();
        T t;
        populate(t, max);
        state.ResumeTiming();
        ++count;
    }
    return count;
}

template <typename T>
static std::size_t  measure_pop_back(benchmark::State& state)
{
    size_t sum = 0;
    auto max = state.range();
    for (auto&& _ : state)
    {
        state.PauseTiming();
        T t;
        populate(t, max);
        state.ResumeTiming();
        while (!t.empty())
        {
            sum += t.back();
            t.pop_back();
        }
    }
    return sum;
}

template <typename T>
static size_t iterate_forward(const T&  t, benchmark::State& state)
{
    size_t sum  = 0;
    for (auto&& _ : state)
    {
        for (auto&& v : t)
        {
            sum += v;
        }
    }
    return sum;
}

template <typename T>
static size_t iterate_backward(const T&  t, benchmark::State& state)
{
    size_t sum  = 0;
    for (auto&& _ : state)
    {
        auto const end = t.rend();
        for (auto i = t.rbegin(); i != end; ++i)
        {
            sum += *i;
        }
    }
    return sum;
}

static void populate_std_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_populate<std::vector<size_t>>(state));
}

static void populate_stable_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_populate<stable_vector<size_t>>(state));
}

static void destroy_std_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_destroy<std::vector<size_t>>(state));
}

static void destroy_stable_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_destroy<stable_vector<size_t>>(state));
}

static void pop_back_std_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_pop_back<std::vector<size_t>>(state));
}

static void pop_back_stable_vector(benchmark::State& state)
{
    benchmark::DoNotOptimize(measure_pop_back<stable_vector<size_t>>(state));
}

static void iterate_forward_std_vector(benchmark::State& state)
{
    std::vector<size_t> v;
    populate(v, state.range());
    benchmark::DoNotOptimize(iterate_forward(v, state));
}

static void iterate_forward_stable_vector(benchmark::State& state)
{
    stable_vector<size_t> v;
    populate(v, state.range());
    benchmark::DoNotOptimize(iterate_forward(v, state));
}

static void iterate_backward_std_vector(benchmark::State& state)
{
    std::vector<size_t> v;
    populate(v, state.range());
    benchmark::DoNotOptimize(iterate_backward(v, state));
}

static void iterate_backward_stable_vector(benchmark::State& state)
{
    stable_vector<size_t> v;
    populate(v, state.range());
    benchmark::DoNotOptimize(iterate_backward(v, state));
}

BENCHMARK(populate_std_vector)->Range(2,65536);
BENCHMARK(populate_stable_vector)->Range(2,65536);
BENCHMARK(destroy_std_vector)->Range(2,65536);
BENCHMARK(destroy_stable_vector)->Range(2,65536);
BENCHMARK(pop_back_std_vector)->Range(2,65536);
BENCHMARK(pop_back_stable_vector)->Range(2,65536);

BENCHMARK(iterate_forward_std_vector)->Range(2,65536);
BENCHMARK(iterate_forward_stable_vector)->Range(2,65536);
BENCHMARK(iterate_backward_std_vector)->Range(2,65536);
BENCHMARK(iterate_backward_stable_vector)->Range(2,65536);
