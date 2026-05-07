#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <random>
#include <ranges>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

template <std::ranges::input_range Range, class OutputIterator, class Predicate,
          class Operation>
OutputIterator transform_if(Range&& input, OutputIterator output,
                            Predicate predicate, Operation operation) {
    using Value = std::ranges::range_value_t<Range>;

    std::vector<Value> filtered;
    std::ranges::copy_if(std::forward<Range>(input), std::back_inserter(filtered),
                         predicate);
    auto transform_result = std::ranges::transform(filtered, output, operation);
    return transform_result.out;
}

void validate_error_input(std::span<const double> expected,
                          std::span<const double> actual) {
    if (expected.empty()) {
        throw std::invalid_argument("empty input is not allowed");
    }
    if (expected.size() != actual.size()) {
        throw std::invalid_argument("input sizes must be equal");
    }
}

double mean_absolute_error(std::span<const double> expected,
                           std::span<const double> actual) {
    validate_error_input(expected, actual);

    constexpr double zero_sum = 0.0;
    const auto error_sum = std::transform_reduce(
        expected.begin(), expected.end(), actual.begin(), zero_sum, std::plus<>{},
        [](double expected_value, double actual_value) {
            return std::abs(expected_value - actual_value);
        });

    return error_sum / static_cast<double>(expected.size());
}

double mean_squared_error(std::span<const double> expected,
                          std::span<const double> actual) {
    validate_error_input(expected, actual);

    constexpr double zero_sum = 0.0;
    const auto error_sum = std::transform_reduce(
        expected.begin(), expected.end(), actual.begin(), zero_sum, std::plus<>{},
        [](double expected_value, double actual_value) {
            const double difference = expected_value - actual_value;
            return difference * difference;
        });

    return error_sum / static_cast<double>(expected.size());
}

// This solution does not fully satisfy the task requirements because it
// replaces the required standard C++23 view std::views::stride with a custom
// StrideView workaround. The workaround is needed for the current compiler /
// standard library, where std::views::stride is unavailable.
template <std::ranges::view View>
requires std::ranges::forward_range<View> && std::ranges::common_range<View>
class StrideView : public std::ranges::view_interface<StrideView<View>> {
public:
    using Difference = std::ranges::range_difference_t<View>;

    StrideView() = default;

    StrideView(View base, Difference step)
        : base_(std::move(base)), step_(step) {
        if (step_ < minimum_step) {
            throw std::invalid_argument("stride step must be positive");
        }
    }

    auto begin() {
        return Iterator(std::ranges::begin(base_), std::ranges::end(base_),
                        step_);
    }

    auto end() {
        return Iterator(std::ranges::end(base_), std::ranges::end(base_), step_);
    }

private:
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;
        using value_type = std::ranges::range_value_t<View>;
        using difference_type = std::ranges::range_difference_t<View>;

        Iterator() = default;

        Iterator(std::ranges::iterator_t<View> current,
                 std::ranges::iterator_t<View> end, difference_type step)
            : current_(current), end_(end), step_(step) {}

        decltype(auto) operator*() const {
            return *current_;
        }

        Iterator& operator++() {
            for (difference_type skipped = 0;
                 skipped < step_ && current_ != end_; ++skipped) {
                ++current_;
            }
            return *this;
        }

        Iterator operator++(int) {
            Iterator previous = *this;
            ++(*this);
            return previous;
        }

        friend bool operator==(const Iterator& left, const Iterator& right) {
            return left.current_ == right.current_;
        }

        friend bool operator!=(const Iterator& left, const Iterator& right) {
            return !(left == right);
        }

    private:
        std::ranges::iterator_t<View> current_{};
        std::ranges::iterator_t<View> end_{};
        difference_type step_ = minimum_step;
    };

    static constexpr Difference minimum_step = 1;

    View base_{};
    Difference step_ = minimum_step;
};

template <std::ranges::forward_range Range>
requires std::ranges::common_range<std::views::all_t<Range>>
auto make_stride_view(Range&& range,
                      std::ranges::range_difference_t<Range> step) {
    return StrideView<std::views::all_t<Range>>(std::views::all(range), step);
}

class Fibonacci : public std::ranges::view_interface<Fibonacci> {
public:
    explicit Fibonacci(std::size_t count) : count_(count) {}

    auto begin() const {
        return Iterator{};
    }

    auto end() const {
        return Iterator::make_end(count_);
    }

private:
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using iterator_concept = std::forward_iterator_tag;
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        Iterator() = default;

        value_type operator*() const {
            return current_;
        }

        Iterator& operator++() {
            const value_type next_current = next_;
            next_ += current_;
            current_ = next_current;
            ++index_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator previous = *this;
            ++(*this);
            return previous;
        }

        friend bool operator==(const Iterator& left, const Iterator& right) {
            return left.index_ == right.index_;
        }

        friend bool operator!=(const Iterator& left, const Iterator& right) {
            return !(left == right);
        }

    private:
        friend class Fibonacci;

        static Iterator make_end(std::size_t count) {
            Iterator iterator;
            iterator.index_ = count;
            return iterator;
        }

        value_type current_ = 0U;
        value_type next_ = 1U;
        std::size_t index_ = 0U;
    };

    std::size_t count_ = 0U;
};

static_assert(std::ranges::view<Fibonacci>);
static_assert(std::ranges::forward_range<Fibonacci>);

template <class Range>
auto to_vector(Range&& range) {
    using Reference = std::ranges::range_reference_t<Range>;
    using Value = std::remove_cvref_t<Reference>;

    std::vector<Value> result;
    for (auto&& value : range) {
        result.emplace_back(value);
    }
    return result;
}

TEST(RangesAlgorithms, ReplaceFillUniqueRotateAndSample) {
    std::vector<int> numbers{1, 2, 2, 3, 2};

    constexpr int old_value = 2;
    constexpr int new_value = 7;
    std::ranges::replace(numbers, old_value, new_value);
    EXPECT_EQ(numbers, (std::vector<int>{1, 7, 7, 3, 7}));

    constexpr int fill_value = 5;
    std::ranges::fill(numbers, fill_value);
    EXPECT_EQ(numbers, (std::vector<int>{5, 5, 5, 5, 5}));

    std::vector<int> repeated{1, 1, 2, 2, 2, 3, 3, 4};
    const auto unique_tail = std::ranges::unique(repeated);
    repeated.erase(unique_tail.begin(), unique_tail.end());
    EXPECT_EQ(repeated, (std::vector<int>{1, 2, 3, 4}));

    std::vector<int> rotated{1, 2, 3, 4, 5};
    constexpr std::size_t rotation_offset = 2U;
    (void)std::ranges::rotate(rotated, rotated.begin() + rotation_offset);
    EXPECT_EQ(rotated, (std::vector<int>{3, 4, 5, 1, 2}));

    const std::vector<int> source{1, 2, 3, 4, 5, 6, 7, 8};
    constexpr int sample_count = 3;
    constexpr std::mt19937::result_type sample_seed = 17U;

    std::mt19937 generator(sample_seed);
    std::vector<int> sampled;
    (void)std::ranges::sample(source, std::back_inserter(sampled), sample_count,
                              generator);

    EXPECT_EQ(sampled.size(), static_cast<std::size_t>(sample_count));

    std::vector<int> sorted_sample = sampled;
    std::ranges::sort(sorted_sample);
    EXPECT_EQ(std::ranges::adjacent_find(sorted_sample), sorted_sample.end());

    for (int value : sampled) {
        EXPECT_NE(std::ranges::find(source, value), source.end());
    }
}

TEST(TransformIf, CopiesMatchingValuesAndTransformsThem) {
    const std::vector<int> values{1, 2, 3, 4, 5, 6};
    std::vector<int> result;

    const auto is_even = [](int value) {
        return value % 2 == 0;
    };
    const auto square = [](int value) {
        return value * value;
    };

    (void)transform_if(values, std::back_inserter(result), is_even, square);

    EXPECT_EQ(result, (std::vector<int>{4, 16, 36}));
}

TEST(ErrorMetrics, ComputesMaeAndMse) {
    const std::vector<double> expected{3.0, -0.5, 2.0, 7.0};
    const std::vector<double> actual{2.5, 0.0, 2.0, 8.0};

    EXPECT_DOUBLE_EQ(mean_absolute_error(expected, actual), 0.5);
    EXPECT_DOUBLE_EQ(mean_squared_error(expected, actual), 0.375);
}

TEST(ErrorMetrics, RejectsInvalidInput) {
    const std::vector<double> empty;
    const std::vector<double> one_value{1.0};
    const std::vector<double> two_values{1.0, 2.0};

    EXPECT_THROW((void)mean_absolute_error(empty, empty), std::invalid_argument);
    EXPECT_THROW((void)mean_squared_error(one_value, two_values),
                 std::invalid_argument);
}

TEST(RangesViews, FilterDropJoinZipAndStride) {
    const std::vector<int> numbers{1, 2, 3, 4, 5, 6};

    auto even_numbers = numbers | std::views::filter([](int value) {
                            return value % 2 == 0;
                        });
    EXPECT_EQ(to_vector(even_numbers), (std::vector<int>{2, 4, 6}));

    constexpr std::size_t dropped_prefix_size = 3U;
    auto suffix = numbers | std::views::drop(dropped_prefix_size);
    EXPECT_EQ(to_vector(suffix), (std::vector<int>{4, 5, 6}));

    const std::vector<std::vector<int>> nested{{1, 2}, {3}, {4, 5}};
    auto flattened = nested | std::views::join;
    EXPECT_EQ(to_vector(flattened), (std::vector<int>{1, 2, 3, 4, 5}));

    const std::vector<int> left{1, 2, 3};
    const std::vector<int> right{10, 20};

    std::vector<int> zipped_sums;
    for (auto entry : std::views::zip(left, right)) {
        zipped_sums.push_back(std::get<0>(entry) + std::get<1>(entry));
    }
    EXPECT_EQ(zipped_sums, (std::vector<int>{11, 22}));

    constexpr std::ptrdiff_t stride_step = 2;
    auto strided = make_stride_view(numbers, stride_step);
    EXPECT_EQ(to_vector(strided), (std::vector<int>{1, 3, 5}));
}

TEST(FibonacciView, ProvidesFiniteFibonacciRange) {
    constexpr std::size_t fibonacci_count = 10U;
    const Fibonacci fibonacci(fibonacci_count);

    const auto values = to_vector(fibonacci);

    const std::vector<std::uint64_t> expected{0U, 1U, 1U, 2U, 3U,
                                              5U, 8U, 13U, 21U, 34U};
    EXPECT_EQ(values, expected);
}

TEST(FibonacciView, WorksWithStandardViews) {
    constexpr std::size_t fibonacci_count = 8U;
    constexpr std::size_t dropped_count = 2U;
    constexpr std::uint64_t threshold = 3U;

    auto values = Fibonacci(fibonacci_count) |
                  std::views::drop(dropped_count) |
                  std::views::filter([](std::uint64_t value) {
                      return value > threshold;
                  });

    EXPECT_EQ(to_vector(values), (std::vector<std::uint64_t>{5U, 8U, 13U}));
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
