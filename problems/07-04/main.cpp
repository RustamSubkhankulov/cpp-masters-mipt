#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

template <typename T>
void quick_sort(std::vector<T>& values);

namespace {

constexpr std::size_t kInsertionSortCutoff = 16;
constexpr std::size_t kLargeTestSize = 1000;
constexpr std::uint32_t kRandomSeed = 123456789U;

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
T median_of_three(std::vector<T>& values, std::size_t left,
                  std::size_t right) {
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
                     std::size_t right) {
  if (right - left + 1 <= kInsertionSortCutoff) {
    insertion(values, left, right);
    return;
  }

  const std::size_t pivot_index = hoare_partition(values, left, right);

  quick_sort_impl(values, left, pivot_index);
  quick_sort_impl(values, pivot_index + 1, right);
}

std::vector<int> make_descending_int_vector(std::size_t size) {
  std::vector<int> values(size);
  for (std::size_t i = 0; i < size; ++i) {
    values[i] = static_cast<int>(size - i);
  }
  return values;
}

std::vector<int> make_random_int_vector(std::size_t size) {
  std::mt19937 generator(kRandomSeed);
  std::uniform_int_distribution<int> distribution(
      std::numeric_limits<int>::min(), std::numeric_limits<int>::max());

  std::vector<int> values;
  values.reserve(size);

  for (std::size_t i = 0; i < size; ++i) {
    values.push_back(distribution(generator));
  }

  return values;
}

struct Box {
  int key;

  auto operator<=>(const Box& other) const noexcept = default;
};

template <typename T>
void expect_quick_sort_matches_std_sort(const std::vector<T>& input) {
  std::vector<T> actual = input;
  std::vector<T> expected = input;

  quick_sort(actual);
  std::ranges::sort(expected);

  EXPECT_EQ(actual, expected);
  EXPECT_TRUE(std::ranges::is_sorted(actual));
}

}  // namespace

template <typename T>
void quick_sort(std::vector<T>& values) {
  if (values.size() <= 1) {
    return;
  }

  quick_sort_impl(values, 0, values.size() - 1);
}

TEST(QuickSortTest, SortsEmptyVector) {
  expect_quick_sort_matches_std_sort(std::vector<int>{});
}

TEST(QuickSortTest, SortsSingleElementVector) {
  expect_quick_sort_matches_std_sort(std::vector<int>{42});
}

TEST(QuickSortTest, SortsVectorAtInsertionCutoff) {
  expect_quick_sort_matches_std_sort(
      make_descending_int_vector(kInsertionSortCutoff));
}

TEST(QuickSortTest, SortsVectorAboveInsertionCutoff) {
  expect_quick_sort_matches_std_sort(
      make_descending_int_vector(kInsertionSortCutoff + 1));
}

TEST(QuickSortTest, SortsVectorWithRepeatedValues) {
  expect_quick_sort_matches_std_sort(
      std::vector<int>{7, -3, 7, 0, -3, 4, 4, 4, 1, 7, -3});
}

TEST(QuickSortTest, SortsLargeRandomVector) {
  expect_quick_sort_matches_std_sort(make_random_int_vector(kLargeTestSize));
}

TEST(QuickSortTest, SortsUserDefinedType) {
  expect_quick_sort_matches_std_sort(
      std::vector<Box>{{5}, {-1}, {8}, {8}, {0}, {-7}, {3}, {3}});
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
