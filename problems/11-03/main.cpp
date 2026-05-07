#include <algorithm>
#include <array>
#include <concepts>
#include <deque>
#include <functional>
#include <iterator>
#include <random>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

constexpr std::ptrdiff_t kInsertionSortCutoff = 16;

template <typename Compare, typename T, typename U>
bool compare_values(Compare compare, const T& left, const U& right) {
  return std::invoke(compare, left, right);
}

template <std::random_access_iterator RandomIt, typename Compare>
void insertion_sort(RandomIt first, RandomIt last, Compare compare) {
  if (first == last) {
    return;
  }

  for (auto it = std::next(first); it != last; ++it) {
    auto current = it;
    while (current != first) {
      auto previous = std::prev(current);
      if (!compare_values(compare, *current, *previous)) {
        break;
      }

      std::iter_swap(previous, current);
      current = previous;
    }
  }
}

template <std::random_access_iterator RandomIt, typename Compare>
std::iter_value_t<RandomIt> median_of_three(RandomIt first, RandomIt last,
                                            Compare compare) {
  const auto size = std::distance(first, last);

  auto middle = first;
  std::advance(middle, size / 2);

  auto right = std::prev(last);

  if (compare_values(compare, *middle, *first)) {
    std::iter_swap(middle, first);
  }

  if (compare_values(compare, *right, *first)) {
    std::iter_swap(right, first);
  }

  if (compare_values(compare, *right, *middle)) {
    std::iter_swap(right, middle);
  }

  return *middle;
}

template <std::random_access_iterator RandomIt, typename Compare>
RandomIt hoare_partition(RandomIt first, RandomIt last, Compare compare) {
  const auto pivot = median_of_three(first, last, compare);

  auto left = first;
  auto right = std::prev(last);

  while (true) {
    while (compare_values(compare, *left, pivot)) {
      ++left;
    }

    while (compare_values(compare, pivot, *right)) {
      --right;
    }

    if (left >= right) {
      return right;
    }

    std::iter_swap(left, right);
    ++left;
    --right;
  }
}

template <std::random_access_iterator RandomIt, typename Compare>
void quick_sort_impl(RandomIt first, RandomIt last, Compare compare) {
  const auto size = std::distance(first, last);
  if (size <= 1) {
    return;
  }

  if (size <= kInsertionSortCutoff) {
    insertion_sort(first, last, compare);
    return;
  }

  const auto partition_point = hoare_partition(first, last, compare);

  quick_sort_impl(first, std::next(partition_point), compare);
  quick_sort_impl(std::next(partition_point), last, compare);
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

bool descending_compare(int left, int right) {
  return left > right;
}

struct Item {
  int key;
  int payload;

  auto operator<=>(const Item&) const = default;
};

} // namespace

template <std::random_access_iterator RandomIt,
          typename Compare = std::less<std::iter_value_t<RandomIt>>>
  requires std::strict_weak_order<Compare, std::iter_value_t<RandomIt>,
                                  std::iter_value_t<RandomIt>>
void quick_sort(RandomIt first, RandomIt last, Compare compare = Compare()) {
  quick_sort_impl(first, last, compare);
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

TEST(QuickSortComparatorTest, SortsWithFreeFunctionComparator) {
  std::vector<int> values{4, 1, 7, 3, 9, 2, 8, 6, 5, 0};

  quick_sort(values.begin(), values.end(), descending_compare);

  EXPECT_TRUE(std::ranges::is_sorted(values, std::greater<>()));
}

TEST(QuickSortComparatorTest, SortsWithStdLessComparator) {
  std::vector<int> values{9, 4, 7, 1, 3, 8, 2, 6, 5, 0};

  quick_sort(values.begin(), values.end(), std::less<int>());

  EXPECT_TRUE(std::ranges::is_sorted(values, std::less<int>()));
}

TEST(QuickSortComparatorTest, SortsWithLambdaComparator) {
  std::vector<Item> values{{3, 30}, {1, 99}, {2, 10},
                           {1, 20}, {3, 15}, {2, 25}};

  const auto compare = [](const Item& left, const Item& right) {
    if (left.key != right.key) {
      return left.key < right.key;
    }

    return left.payload > right.payload;
  };

  quick_sort(values.begin(), values.end(), compare);

  EXPECT_TRUE(std::ranges::is_sorted(values, compare));
  EXPECT_EQ(values[0].key, 1);
  EXPECT_EQ(values[0].payload, 99);
  EXPECT_EQ(values[1].key, 1);
  EXPECT_EQ(values[1].payload, 20);
  EXPECT_EQ(values[2].key, 2);
  EXPECT_EQ(values[2].payload, 25);
  EXPECT_EQ(values[3].key, 2);
  EXPECT_EQ(values[3].payload, 10);
  EXPECT_EQ(values[4].key, 3);
  EXPECT_EQ(values[4].payload, 30);
  EXPECT_EQ(values[5].key, 3);
  EXPECT_EQ(values[5].payload, 15);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
