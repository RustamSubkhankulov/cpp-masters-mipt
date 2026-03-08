#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

template <typename T>
void quick_sort(std::vector<T>& values, std::size_t insertion_sort_cutoff);

namespace {

constexpr std::size_t kBenchmarkContainerSize = 10000;
constexpr std::size_t kMinInsertionSortCutoff = 2;
constexpr std::size_t kMaxInsertionSortCutoff = 64;
constexpr std::size_t kInsertionSortCutoffStep = 2;

template <typename T>
void insertion(std::vector<T>& values, std::size_t left, std::size_t right) {
  for (std::size_t i = left + 1; i <= right; ++i) {
    for (std::size_t j = i; j > left; --j) {
      if (values[j - 1] > values[j]) {
        std::swap(values[j], values[j - 1]);
      }
    }
  }
}

template <typename T>
T median_of_three(std::vector<T>& values, std::size_t left, std::size_t right) {
  const std::size_t middle = std::midpoint(left, right);

  if (values[right] < values[left]) {
    std::swap(values[left], values[right]);
  }
  if (values[middle] < values[left]) {
    std::swap(values[middle], values[left]);
  }
  if (values[right] < values[middle]) {
    std::swap(values[right], values[middle]);
  }

  return values[middle];
}

template <typename T>
std::size_t hoare_partition(std::vector<T>& values, std::size_t left,
                            std::size_t right) {
  const T pivot = median_of_three(values, left, right);

  std::size_t i = left + 1;
  std::size_t j = right - 1;

  while (true) {
    while (values[i] < pivot) {
      ++i;
    }
    while (values[j] > pivot) {
      --j;
    }

    if (i >= j) {
      break;
    }

    std::swap(values[i], values[j]);
    ++i;
    --j;
  }

  return j;
}

template <typename T>
void quick_sort_impl(std::vector<T>& values, std::size_t left,
                     std::size_t right, std::size_t insertion_sort_cutoff) {
  if (right - left + 1 <= insertion_sort_cutoff) {
    insertion(values, left, right);
    return;
  }

  const std::size_t pivot_index = hoare_partition(values, left, right);

  quick_sort_impl(values, left, pivot_index, insertion_sort_cutoff);
  quick_sort_impl(values, pivot_index + 1, right, insertion_sort_cutoff);
}

std::vector<double> make_reverse_sorted_vector(std::size_t size) {
  std::vector<double> values(size);
  for (std::size_t i = 0; i < size; ++i) {
    values[i] = static_cast<double>(size - i);
  }
  return values;
}

void benchmark_quick_sort_reverse_sorted(benchmark::State& state) {
  const std::size_t insertion_sort_cutoff =
    static_cast<std::size_t>(state.range(0));
  const std::vector<double> source =
    make_reverse_sorted_vector(kBenchmarkContainerSize);

  std::vector<double> values;
  values.reserve(source.size());

  for (auto _ : state) {
    state.PauseTiming();
    values = source;
    state.ResumeTiming();

    quick_sort(values, insertion_sort_cutoff);
    benchmark::DoNotOptimize(values.data());
    benchmark::ClobberMemory();

    state.PauseTiming();
    if (!std::ranges::is_sorted(values)) {
      state.SkipWithError("quick_sort produced unsorted output");
      break;
    }
    state.ResumeTiming();
  }

  state.SetItemsProcessed(
    static_cast<int64_t>(state.iterations() * source.size()));
  state.SetBytesProcessed(static_cast<int64_t>(
    state.iterations() * source.size() * sizeof(source.front())));
}

} // namespace

template <typename T>
void quick_sort(std::vector<T>& values, std::size_t insertion_sort_cutoff) {
  if (values.size() <= 1) {
    return;
  }

  quick_sort_impl(values, 0, values.size() - 1, insertion_sort_cutoff);
}

BENCHMARK(benchmark_quick_sort_reverse_sorted)
  ->DenseRange(static_cast<int>(kMinInsertionSortCutoff),
               static_cast<int>(kMaxInsertionSortCutoff),
               static_cast<int>(kInsertionSortCutoffStep));

BENCHMARK_MAIN();
