#include <bit>
#include <limits>
#include <optional>

#include <gtest/gtest.h>

namespace {

struct Platform final {
  static constexpr int bits_per_byte = 8;
};

struct FloatFormat final {
  static constexpr int exponent_bits = 8;
  static constexpr int fraction_bits = 23;
  static constexpr int exponent_bias = 127;
  static constexpr int min_normal_exponent = 1 - exponent_bias;
  static constexpr int min_subnormal_exponent =
    min_normal_exponent - fraction_bits;
  static constexpr unsigned int unsigned_bit_count =
    static_cast<unsigned int>(sizeof(unsigned int) * Platform::bits_per_byte);
  static constexpr unsigned int exponent_all_ones = (1U << exponent_bits) - 1U;
  static constexpr unsigned int fraction_mask = (1U << fraction_bits) - 1U;
  static constexpr unsigned int sign_mask = 1U << (unsigned_bit_count - 1U);
};

static_assert(sizeof(int) == 4U);
static_assert(sizeof(unsigned int) == 4U);
static_assert(sizeof(float) == 4U);
static_assert(std::numeric_limits<int>::digits == 31);
static_assert(std::numeric_limits<unsigned int>::digits == 32);
static_assert(std::numeric_limits<float>::is_iec559);

[[nodiscard]] unsigned int FloatToUnsignedBits(float value) {
  return std::bit_cast<unsigned int>(value);
}

[[nodiscard]] std::optional<int>
FindMostSignificantBitIndex(unsigned int value) {
  if (value == 0U) {
    return std::nullopt;
  }

  int bit_index = -1;
  while (value != 0U) {
    value >>= 1U;
    ++bit_index;
  }
  return bit_index;
}

[[nodiscard]] std::optional<int> BinaryLogarithmFloor(int value) {
  if (value <= 0) {
    return std::nullopt;
  }

  return FindMostSignificantBitIndex(static_cast<unsigned int>(value));
}

[[nodiscard]] std::optional<int> BinaryLogarithmFloor(float value) {
  const unsigned int bits = FloatToUnsignedBits(value);

  if ((bits & FloatFormat::sign_mask) != 0U) {
    return std::nullopt;
  }

  const unsigned int biased_exponent =
    (bits >> FloatFormat::fraction_bits) & FloatFormat::exponent_all_ones;
  const unsigned int fraction = bits & FloatFormat::fraction_mask;

  if (biased_exponent == FloatFormat::exponent_all_ones) {
    return std::nullopt;
  }

  if (biased_exponent == 0U) {
    if (fraction == 0U) {
      return std::nullopt;
    }

    const std::optional<int> fraction_msb =
      FindMostSignificantBitIndex(fraction);
    if (!fraction_msb.has_value()) {
      return std::nullopt;
    }

    return fraction_msb.value() + FloatFormat::min_subnormal_exponent;
  }

  return static_cast<int>(biased_exponent) - FloatFormat::exponent_bias;
}

void ExpectOptionalValue(const std::optional<int>& actual, int expected) {
  ASSERT_TRUE(actual.has_value());
  EXPECT_EQ(actual.value(), expected);
}

} // namespace

TEST(IntBinaryLogarithmFloorTest, HandlesPositiveIntegers) {
  struct TestCase {
    int input;
    int expected;
  };

  const TestCase test_cases[] = {
    {1, 0},   {2, 1},    {3, 1},     {4, 2},
    {7, 2},   {8, 3},    {37, 5},    {255, 7},
    {256, 8}, {1023, 9}, {1024, 10}, {std::numeric_limits<int>::max(), 30},
  };

  for (const TestCase& test_case : test_cases) {
    ExpectOptionalValue(BinaryLogarithmFloor(test_case.input),
                        test_case.expected);
  }
}

TEST(IntBinaryLogarithmFloorTest, RejectsNonPositiveIntegers) {
  EXPECT_FALSE(BinaryLogarithmFloor(0).has_value());
  EXPECT_FALSE(BinaryLogarithmFloor(-1).has_value());
  EXPECT_FALSE(
    BinaryLogarithmFloor(std::numeric_limits<int>::min()).has_value());
}

TEST(FloatBinaryLogarithmFloorTest, HandlesNormalizedValues) {
  struct TestCase {
    float input;
    int expected;
  };

  const TestCase test_cases[] = {
    {1.0f, 0},
    {1.5f, 0},
    {1.75f, 0},
    {2.0f, 1},
    {3.5f, 1},
    {4.0f, 2},
    {37.625f, 5},
    {0.5f, -1},
    {0.75f, -1},
    {0.25f, -2},
    {std::numeric_limits<float>::min(), -126},
    {std::numeric_limits<float>::max(), 127},
  };

  for (const TestCase& test_case : test_cases) {
    ExpectOptionalValue(BinaryLogarithmFloor(test_case.input),
                        test_case.expected);
  }
}

TEST(FloatBinaryLogarithmFloorTest, HandlesSubnormalValues) {
  const float smallest_subnormal = std::numeric_limits<float>::denorm_min();
  const float second_subnormal = std::bit_cast<float>(0x00000002U);
  const float exact_power_minus_127 = std::bit_cast<float>(0x00400000U);
  const float largest_subnormal = std::bit_cast<float>(0x007FFFFFU);

  ExpectOptionalValue(BinaryLogarithmFloor(smallest_subnormal), -149);
  ExpectOptionalValue(BinaryLogarithmFloor(second_subnormal), -148);
  ExpectOptionalValue(BinaryLogarithmFloor(exact_power_minus_127), -127);
  ExpectOptionalValue(BinaryLogarithmFloor(largest_subnormal), -127);
}

TEST(FloatBinaryLogarithmFloorTest, RejectsInvalidFloatInputs) {
  const float positive_infinity = std::numeric_limits<float>::infinity();
  const float quiet_nan = std::numeric_limits<float>::quiet_NaN();
  const float negative_zero = std::bit_cast<float>(FloatFormat::sign_mask);

  EXPECT_FALSE(BinaryLogarithmFloor(0.0f).has_value());
  EXPECT_FALSE(BinaryLogarithmFloor(negative_zero).has_value());
  EXPECT_FALSE(BinaryLogarithmFloor(-1.0f).has_value());
  EXPECT_FALSE(BinaryLogarithmFloor(positive_infinity).has_value());
  EXPECT_FALSE(BinaryLogarithmFloor(quiet_nan).has_value());
}

TEST(DemoExamples, ShowsTypicalUsageExamples) {
  ExpectOptionalValue(BinaryLogarithmFloor(37), 5);
  ExpectOptionalValue(BinaryLogarithmFloor(37.625f), 5);
  ExpectOptionalValue(
    BinaryLogarithmFloor(std::numeric_limits<float>::denorm_min()), -149);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
