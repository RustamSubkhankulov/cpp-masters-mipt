#include <algorithm>
#include <array>
#include <deque>
#include <iterator>
#include <random>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

constexpr std::ptrdiff_t kInsertionSortCutoff = 16;

template <std::random_access_iterator RandomIt>
void insertion_sort(RandomIt first, RandomIt last) {
  for (auto it = std::next(first); it != last; ++it) {
    for (auto current = it; current != first; --current) {
      auto previous = std::prev(current);
      if (*previous > *current) {
        std::iter_swap(previous, current);
      }
    }
  }
}

template <std::random_access_iterator RandomIt>
std::iter_value_t<RandomIt> median_of_three(RandomIt first, RandomIt last) {
  const auto size = std::distance(first, last);

  auto middle = first;
  std::advance(middle, (size - 1) / 2);

  auto right = std::prev(last);

  if (*right < *first) {
    std::iter_swap(first, right);
  }

  if (*middle < *first) {
    std::iter_swap(middle, first);
  }

  if (*right < *middle) {
    std::iter_swap(right, middle);
  }

  return *middle;
}

template <std::random_access_iterator RandomIt>
RandomIt hoare_partition(RandomIt first, RandomIt last) {
  const auto pivot = median_of_three(first, last);

  auto left = std::next(first);
  auto right = std::prev(last, 2);

  while (true) {
    while (*left < pivot) {
      ++left;
    }

    while (*right > pivot) {
      --right;
    }

    if (left >= right) {
      break;
    }

    std::iter_swap(left, right);
    ++left;
    --right;
  }

  return right;
}

template <std::random_access_iterator RandomIt>
void quick_sort_impl(RandomIt first, RandomIt last) {
  if (std::distance(first, last) <= kInsertionSortCutoff) {
    insertion_sort(first, last);
    return;
  }

  const auto partition_point = hoare_partition(first, last);

  quick_sort_impl(first, std::next(partition_point));
  quick_sort_impl(std::next(partition_point), last);
}

std::vector<int> make_random_vector(std::size_t size) {
  std::mt19937 generator(123456789u);
  std::uniform_int_distribution<int> distribution(-1000000, 1000000);

  std::vector<int> values;
  values.reserve(size);

  for (std::size_t index = 0; index < size; ++index) {
    values.push_back(distribution(generator));
  }

  return values;
}

struct Item {
  int key;
  int payload;

  auto operator<=>(const Item&) const = default;
};

} // namespace

template <std::random_access_iterator RandomIt>
void quick_sort(RandomIt first, RandomIt last) {
  if (std::distance(first, last) <= 1) {
    return;
  }

  quick_sort_impl(first, last);
}

TEST(QuickSortTest, SortsEmptyRange) {
  std::vector<int> values;
  quick_sort(values.begin(), values.end());
  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsSingleElementRange) {
  std::vector<int> values{42};
  quick_sort(values.begin(), values.end());
  EXPECT_TRUE(std::ranges::is_sorted(values));
  EXPECT_EQ(values[0], 42);
}

TEST(QuickSortTest, SortsReverseOrderedVector) {
  std::vector<int> values;
  for (int value = 1000; value >= 1; --value) {
    values.push_back(value);
  }

  quick_sort(values.begin(), values.end());

  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsVectorWithDuplicates) {
  std::vector<int> values{5, 1, 3, 3, 2, 5, 4, 1, 0, 0, 2, 4, 4, 3};

  quick_sort(values.begin(), values.end());

  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsDeque) {
  std::deque<int> values{9, 7, 5, 3, 1, 2, 4, 6, 8, 0};

  quick_sort(values.begin(), values.end());

  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsStrings) {
  std::vector<std::string> values{"pear",   "apple",  "orange",
                                  "banana", "banana", "kiwi"};

  quick_sort(values.begin(), values.end());

  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsCustomType) {
  std::vector<Item> values{{3, 30}, {1, 10}, {2, 20}, {1, 5}, {3, 25}, {2, 15}};

  quick_sort(values.begin(), values.end());

  EXPECT_TRUE(std::ranges::is_sorted(values));
}

TEST(QuickSortTest, SortsOnlyHalfOpenSubrange) {
  std::array<int, 8> values{100, 7, 5, 3, 1, 9, 200, 300};

  quick_sort(std::next(values.begin()), std::prev(values.end(), 2));

  EXPECT_EQ(values[0], 100);
  EXPECT_EQ(values[6], 200);
  EXPECT_EQ(values[7], 300);
  EXPECT_TRUE(std::ranges::is_sorted(std::next(values.begin()),
                                     std::prev(values.end(), 2)));
}

TEST(QuickSortTest, SortsRandomVectorLikeReferenceSort) {
  std::vector<int> values = make_random_vector(1000);
  std::vector<int> expected = values;

  std::sort(expected.begin(), expected.end());
  quick_sort(values.begin(), values.end());

  EXPECT_EQ(values, expected);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
