#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>

#include <boost/iterator/iterator_facade.hpp>
#include <gtest/gtest.h>

class Iterator
    : public boost::iterator_facade<Iterator, const int, boost::forward_traversal_tag, int> {
 public:
  Iterator() noexcept : older_(kSecondValue), newer_(kFirstValue) {}

 private:
  friend class boost::iterator_core_access;

  static constexpr int kFirstValue = 0;
  static constexpr int kSecondValue = 1;

  // The reversed initial pair lets the iterator yield 0 first and still reach
  // the largest Fibonacci value that fits into int.
  int older_;
  int newer_;

  static int CheckedAdd(const int left, const int right) {
    if (left > std::numeric_limits<int>::max() - right) {
      throw std::overflow_error("Fibonacci value does not fit into int");
    }
    return left + right;
  }

  void increment() {
    const int next = CheckedAdd(older_, newer_);
    older_ = newer_;
    newer_ = next;
  }

  int dereference() const noexcept {
    return newer_;
  }

  bool equal(const Iterator& other) const noexcept {
    return older_ == other.older_ && newer_ == other.newer_;
  }
};

std::vector<int> ComputeFibonacciWithLoop(const std::size_t count) {
  std::vector<int> values;
  values.reserve(count);

  Iterator iterator;
  for (std::size_t index = 0; index < count; ++index) {
    values.push_back(*iterator);
    if (index + 1 != count) {
      ++iterator;
    }
  }

  return values;
}

std::vector<int> ComputeFibonacciWithGenerateN(const std::size_t count) {
  std::vector<int> values;
  values.reserve(count);

  Iterator iterator;
  std::size_t index = 0;
  std::generate_n(std::back_inserter(values), static_cast<std::ptrdiff_t>(count), [&]() {
    const int value = *iterator;
    ++index;
    if (index != count) {
      ++iterator;
    }
    return value;
  });

  return values;
}

TEST(FibonacciIteratorTest, DefaultIteratorPointsToZero) {
  Iterator iterator;
  EXPECT_EQ(*iterator, 0);
}

TEST(FibonacciIteratorTest, PrefixIncrementBuildsCorrectPrefix) {
  Iterator iterator;

  EXPECT_EQ(*iterator, 0);

  ++iterator;
  EXPECT_EQ(*iterator, 1);

  ++iterator;
  EXPECT_EQ(*iterator, 1);

  ++iterator;
  EXPECT_EQ(*iterator, 2);

  ++iterator;
  EXPECT_EQ(*iterator, 3);
}

TEST(FibonacciIteratorTest, PostfixIncrementReturnsPreviousState) {
  Iterator iterator;
  const Iterator previous = iterator++;

  EXPECT_EQ(*previous, 0);
  EXPECT_EQ(*iterator, 1);
}

TEST(FibonacciIteratorTest, EqualityDependsOnState) {
  Iterator left;
  Iterator right;

  EXPECT_EQ(left, right);

  ++left;
  EXPECT_NE(left, right);

  ++right;
  EXPECT_EQ(left, right);
}

TEST(FibonacciAlgorithmTest, LoopBasedAlgorithmBuildsExpectedPrefix) {
  constexpr std::array<int, 8> kExpected = {0, 1, 1, 2, 3, 5, 8, 13};
  const std::vector<int> expected(kExpected.begin(), kExpected.end());

  EXPECT_EQ(ComputeFibonacciWithLoop(kExpected.size()), expected);
}

TEST(FibonacciAlgorithmTest, GenerateNBasedAlgorithmBuildsExpectedPrefix) {
  constexpr std::array<int, 10> kExpected = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34};
  const std::vector<int> expected(kExpected.begin(), kExpected.end());

  EXPECT_EQ(ComputeFibonacciWithGenerateN(kExpected.size()), expected);
}

TEST(FibonacciAlgorithmTest, BothAlgorithmsProduceIdenticalSequences) {
  constexpr std::size_t kCount = 20;
  EXPECT_EQ(ComputeFibonacciWithLoop(kCount), ComputeFibonacciWithGenerateN(kCount));
}

TEST(FibonacciAlgorithmTest, AlgorithmsHandleEmptySequence) {
  EXPECT_TRUE(ComputeFibonacciWithLoop(0).empty());
  EXPECT_TRUE(ComputeFibonacciWithGenerateN(0).empty());
}

TEST(FibonacciAlgorithmTest, AlgorithmsSupportLargestSafePrefixForInt) {
  constexpr std::size_t kLargestSafeCount = 47;
  constexpr int kLargestFibonacciInInt = 1836311903;

  const std::vector<int> loop_values = ComputeFibonacciWithLoop(kLargestSafeCount);
  const std::vector<int> algorithm_values = ComputeFibonacciWithGenerateN(kLargestSafeCount);

  ASSERT_FALSE(loop_values.empty());
  ASSERT_FALSE(algorithm_values.empty());

  EXPECT_EQ(loop_values.back(), kLargestFibonacciInInt);
  EXPECT_EQ(algorithm_values.back(), kLargestFibonacciInInt);
  EXPECT_EQ(loop_values, algorithm_values);
}

TEST(FibonacciAlgorithmTest, AlgorithmsThrowOnOverflowInsteadOfTriggeringUndefinedBehavior) {
  constexpr std::size_t kOverflowCount = 48;

  EXPECT_THROW(ComputeFibonacciWithLoop(kOverflowCount), std::overflow_error);
  EXPECT_THROW(ComputeFibonacciWithGenerateN(kOverflowCount), std::overflow_error);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
