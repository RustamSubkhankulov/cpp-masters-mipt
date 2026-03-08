#include <compare>
#include <exception>
#include <ios>
#include <istream>
#include <iostream>
#include <limits>
#include <new>
#include <numeric>
#include <optional>
#include <ostream>
#include <sstream>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

template <class Derived>
struct addable {
  friend auto operator+(Derived lhs, Derived const& rhs) {
    lhs += rhs;
    return lhs;
  }
};

template <class Derived>
struct subtractable {
  friend auto operator-(Derived lhs, Derived const& rhs) {
    lhs -= rhs;
    return lhs;
  }
};

template <class Derived>
struct multipliable {
  friend auto operator*(Derived lhs, Derived const& rhs) {
    lhs *= rhs;
    return lhs;
  }
};

template <class Derived>
struct dividable {
  friend auto operator/(Derived lhs, Derived const& rhs) {
    lhs /= rhs;
    return lhs;
  }
};

template <class Derived>
struct incrementable {
  friend auto& operator++(Derived& value) {
    value += Derived::unit_value();
    return value;
  }

  friend auto operator++(Derived& value, int) {
    Derived copy(value);
    ++value;
    return copy;
  }
};

template <class Derived>
struct decrementable {
  friend auto& operator--(Derived& value) {
    value -= Derived::unit_value();
    return value;
  }

  friend auto operator--(Derived& value, int) {
    Derived copy(value);
    --value;
    return copy;
  }
};

class Exception final : public std::exception {
public:
  explicit Exception(char const* message) noexcept : message_(message) {
  }

  auto what() const noexcept -> char const* override {
    return message_;
  }

private:
  char const* message_;
};

template <class T>
class Rational : public addable<Rational<T>>,
                 public subtractable<Rational<T>>,
                 public multipliable<Rational<T>>,
                 public dividable<Rational<T>>,
                 public incrementable<Rational<T>>,
                 public decrementable<Rational<T>> {
public:
  using value_type = T;

  Rational(T numerator = T{}, T denominator = T{1})
      : numerator_(numerator), denominator_(denominator) {
    if (denominator_ == zero_value()) {
      throw Exception("zero denominator");
    }
    reduce();
  }

  explicit operator double() const {
    return static_cast<double>(numerator_) / static_cast<double>(denominator_);
  }

  auto& operator+=(Rational const& other) {
    T const common_denominator = std::lcm(denominator_, other.denominator_);
    numerator_ = numerator_ * (common_denominator / denominator_) +
                 other.numerator_ * (common_denominator / other.denominator_);
    denominator_ = common_denominator;
    reduce();
    return *this;
  }

  auto& operator-=(Rational const& other) {
    return *this += Rational(-other.numerator_, other.denominator_);
  }

  auto& operator*=(Rational const& other) {
    numerator_ *= other.numerator_;
    denominator_ *= other.denominator_;
    reduce();
    return *this;
  }

  auto& operator/=(Rational const& other) {
    if (other.numerator_ == zero_value()) {
      throw std::domain_error("division by zero rational");
    }
    return *this *= Rational(other.denominator_, other.numerator_);
  }

  static auto unit_value() -> Rational {
    return Rational(T{1});
  }

  friend bool operator==(Rational const& lhs, Rational const& rhs) {
    return lhs.numerator_ == rhs.numerator_ &&
           lhs.denominator_ == rhs.denominator_;
  }

  friend auto operator<=>(Rational const& lhs, Rational const& rhs) {
    return lhs.numerator_ * rhs.denominator_ <=>
           rhs.numerator_ * lhs.denominator_;
  }

  friend auto& operator>>(std::istream& stream, Rational& rational) {
    T numerator{};
    T denominator{};
    char separator = '\0';

    if (!(stream >> numerator >> separator >> denominator) || separator != '/' ||
        denominator == zero_value()) {
      stream.setstate(std::ios::failbit);
      return stream;
    }

    rational = Rational(numerator, denominator);
    return stream;
  }

  friend auto& operator<<(std::ostream& stream, Rational const& rational) {
    return stream << rational.numerator_ << '/' << rational.denominator_;
  }

private:
  static constexpr auto zero_value() -> T {
    return T{};
  }

  void reduce() {
    if (denominator_ < zero_value()) {
      numerator_ = -numerator_;
      denominator_ = -denominator_;
    }

    T const divisor = std::gcd(numerator_, denominator_);
    numerator_ /= divisor;
    denominator_ /= divisor;
  }

  T numerator_ = T{};
  T denominator_ = T{1};
};

using IntRational = Rational<int>;

static_assert(std::three_way_comparable<IntRational, std::strong_ordering>);

namespace {

void PrintException(char const* label, std::exception const& error) {
  std::cerr << label << ": " << error.what() << '\n';
}

void DemonstrateBadAlloc() {
  try {
    // std::bad_alloc is used to report failure to obtain dynamic memory.
    throw std::bad_alloc{};
  } catch (std::exception const& error) {
    PrintException("std::bad_alloc", error);
  }
}

void DemonstrateBadVariantAccess() {
  try {
    // std::bad_variant_access is thrown when std::get requests
    // an alternative that is not currently stored in the variant.
    std::variant<int, double> value(3.5);
    (void)std::get<int>(value);
  } catch (std::exception const& error) {
    PrintException("std::bad_variant_access", error);
  }
}

void DemonstrateBadOptionalAccess() {
  try {
    // std::bad_optional_access is thrown when value() is called
    // for an optional object that does not contain a value.
    std::optional<int> value;
    (void)value.value();
  } catch (std::exception const& error) {
    PrintException("std::bad_optional_access", error);
  }
}

void DemonstrateVectorLengthError() {
  try {
    // std::vector::reserve throws std::length_error when the requested
    // capacity exceeds the maximum size supported by the allocator.
    std::vector<int> values;
    values.reserve(values.max_size() + 1U);
  } catch (std::exception const& error) {
    PrintException("std::length_error", error);
  }
}

void DemonstrateVectorOutOfRange() {
  try {
    // std::vector::at throws std::out_of_range when the requested index
    // is outside the valid range [0, size()).
    std::vector<int> values{1, 2, 3};
    (void)values.at(values.size());
  } catch (std::exception const& error) {
    PrintException("std::out_of_range", error);
  }
}

TEST(ExceptionClass, StoresMessage) {
  Exception const error("custom error");
  EXPECT_STREQ(error.what(), "custom error");
}

TEST(RationalConstruction, ReducesAndNormalizesSign) {
  EXPECT_EQ(IntRational(2, 4), IntRational(1, 2));
  EXPECT_EQ(IntRational(3, -9), IntRational(-1, 3));
  EXPECT_EQ(IntRational(-3, -9), IntRational(1, 3));
}

TEST(RationalConstruction, RejectsZeroDenominator) {
  EXPECT_THROW((void)IntRational(1, 0), Exception);
}

TEST(RationalConstruction, ReportsZeroDenominatorMessage) {
  try {
    (void)IntRational(1, 0);
    FAIL() << "Exception was not thrown";
  } catch (Exception const& error) {
    EXPECT_STREQ(error.what(), "zero denominator");
  } catch (...) {
    FAIL() << "Unexpected exception type";
  }
}

TEST(RationalArithmetic, UsesMixedInBinaryOperators) {
  EXPECT_EQ(IntRational(1, 6) + IntRational(1, 3), IntRational(1, 2));
  EXPECT_EQ(IntRational(5, 6) - IntRational(1, 2), IntRational(1, 3));
  EXPECT_EQ(IntRational(2, 3) * IntRational(9, 10), IntRational(3, 5));
  EXPECT_EQ(IntRational(3, 5) / IntRational(9, 10), IntRational(2, 3));
}

TEST(RationalArithmetic, SupportsCompoundAssignments) {
  IntRational value(1, 4);

  value += IntRational(1, 2);
  EXPECT_EQ(value, IntRational(3, 4));

  value -= IntRational(1, 8);
  EXPECT_EQ(value, IntRational(5, 8));

  value *= IntRational(4, 5);
  EXPECT_EQ(value, IntRational(1, 2));

  value /= IntRational(3, 2);
  EXPECT_EQ(value, IntRational(1, 3));
}

TEST(RationalArithmetic, RejectsDivisionByZeroRational) {
  IntRational value(1, 2);
  EXPECT_THROW(value /= IntRational(0, 7), std::domain_error);
  EXPECT_THROW((void)(value / IntRational(0, 7)), std::domain_error);
}

TEST(RationalIncrementDecrement, SupportsPrefixAndPostfixViaMixins) {
  IntRational value(3, 2);

  EXPECT_EQ(++value, IntRational(5, 2));
  EXPECT_EQ(value++, IntRational(5, 2));
  EXPECT_EQ(value, IntRational(7, 2));

  EXPECT_EQ(--value, IntRational(5, 2));
  EXPECT_EQ(value--, IntRational(5, 2));
  EXPECT_EQ(value, IntRational(3, 2));
}

TEST(RationalCompare, PreservesTotalOrdering) {
  IntRational const first(1, 4);
  IntRational const second(1, 3);
  IntRational const third(1, 2);

  EXPECT_TRUE(first < second);
  EXPECT_TRUE(second < third);
  EXPECT_TRUE(first < third);
  EXPECT_TRUE(third > second);
  EXPECT_TRUE(first <= first);
  EXPECT_TRUE(third >= second);
  EXPECT_EQ((first <=> IntRational(2, 8)), std::strong_ordering::equal);
}

TEST(RationalConversion, ConvertsToDouble) {
  double const value = static_cast<double>(IntRational(1, 8));
  EXPECT_DOUBLE_EQ(value, 0.125);
}

TEST(RationalStreamIO, WritesCanonicalForm) {
  std::ostringstream output;
  output << IntRational(-6, -8);
  EXPECT_EQ(output.str(), "3/4");
}

TEST(RationalStreamIO, ReadsValidInput) {
  std::istringstream input("-10/20");
  IntRational value;

  input >> value;

  EXPECT_TRUE(input.good() || input.eof());
  EXPECT_EQ(value, IntRational(-1, 2));
}

TEST(RationalStreamIO, RejectsInvalidInput) {
  std::istringstream input("3:4");
  IntRational value(5, 7);

  input >> value;

  EXPECT_TRUE(input.fail());
  EXPECT_EQ(value, IntRational(5, 7));
}

TEST(RationalStreamIO, RejectsZeroDenominatorInInput) {
  std::istringstream input("3/0");
  IntRational value(5, 7);

  input >> value;

  EXPECT_TRUE(input.fail());
  EXPECT_EQ(value, IntRational(5, 7));
}

TEST(RationalDemo, ArithmeticScenario) {
  IntRational total(1, 2);
  total += IntRational(1, 3);
  total *= IntRational(9, 10);
  total -= IntRational(1, 4);

  EXPECT_EQ(total, IntRational(1, 2));
}

TEST(RationalDemo, MixedSyntaxScenario) {
  IntRational value = IntRational(1, 2) + 1 - IntRational(1, 4);
  EXPECT_EQ(value, IntRational(5, 4));
}

}  // namespace

int main(int argc, char** argv) {
  try {
    DemonstrateBadAlloc();
    DemonstrateBadVariantAccess();
    DemonstrateBadOptionalAccess();
    DemonstrateVectorLengthError();
    DemonstrateVectorOutOfRange();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
  } catch (std::exception const& error) {
    std::cerr << "std::exception: " << error.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "unknown exception\n";
    return 1;
  }
}
