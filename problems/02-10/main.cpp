#include <algorithm>
#include <cassert>
#include <cstddef>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace {

void insertion(std::vector<int>& a, std::size_t left, std::size_t right) {
  for (auto i = left + 1uz; i <= right; ++i) {
    for (auto j = i; j > left; --j) {
      if (a[j - 1uz] > a[j]) {
        std::swap(a[j], a[j - 1uz]);
      }
    }
  }
}

int median_of_three(std::vector<int>& a, std::size_t left, std::size_t right) {
  std::size_t mid = std::midpoint(left, right);

  if (a[right] < a[left])
    std::swap(a[left], a[right]);

  if (a[mid] < a[left])
    std::swap(a[mid], a[left]);

  if (a[right] < a[mid])
    std::swap(a[right], a[mid]);

  return a[mid];
}

std::size_t hoare_partition(std::vector<int>& a, std::size_t left,
                            std::size_t right) {
  int pivot = median_of_three(a, left, right);

  std::size_t i = left + 1uz;
  std::size_t j = right - 1uz;

  while (true) {
    while (a[i] < pivot) {
      i += 1uz;
    }
    while (a[j] > pivot) {
      j -= 1uz;
    }

    if (i >= j) {
      break;
    }

    std::swap(a[i], a[j]);
    i += 1uz;
    j -= 1uz;
  }

  return j;
}

void quick_sort_impl(std::vector<int>& a, std::size_t left, std::size_t right) {
  if (right - left + 1uz <= 16uz) {
    insertion(a, left, right);
    return;
  }

  std::size_t p = hoare_partition(a, left, right);

  quick_sort_impl(a, left, p);
  quick_sort_impl(a, p + 1uz, right);
}

std::vector<int> make_random_vector(std::size_t n) {
  thread_local std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<int> dist(std::numeric_limits<int>::min(),
                                          std::numeric_limits<int>::max());

  std::vector<int> v;
  v.reserve(n);

  for (std::size_t i = 0; i < n; ++i) {
    v.push_back(dist(rng));
  }
  return v;
}

} // namespace

void quick_sort(std::vector<int>& a) {
  if (a.size() <= 1uz) {
    return;
  }
  quick_sort_impl(a, 0, a.size() - 1uz);
}

/*
 * Quick Sort using Hoare partition and median-of-three pivot selection.
 * Insertion sort is used as a cutoff for small subarrays to reduce overhead.
 * The median-of-three pivot helps avoid bad pivots (e.g., first/last element in
 * sorted data), producing more balanced partitions and improving average
 * performance.
 *
 * Average case: O(n log n) — each partition scans all n elements, recursion
 * depth ≈ log n. Worst case: O(n^2) — occurs when partitions are very
 * unbalanced despite pivot choice. Space complexity: O(log n) average
 * (recursion stack), O(n) worst case.
 */

int main() {
  auto size = 1'000uz;
  std::vector<int> v(size, 0);

  for (auto i = 0uz; i < size; ++i) {
    v[i] = static_cast<int>(size - i);
  }

  quick_sort(v);
  assert(std::ranges::is_sorted(v));

  auto rv = make_random_vector(1'000uz);

  quick_sort(rv);
  assert(std::ranges::is_sorted(rv));
}
