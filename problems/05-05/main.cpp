#include <compare>
#include <ios>
#include <istream>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>

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
    : numerator_(numerator)
    , denominator_(denominator) {
    if (denominator_ == zero_value()) {
      throw std::invalid_argument("zero denominator");
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

    if (!(stream >> numerator >> separator >> denominator) ||
        separator != '/' || denominator == zero_value()) {
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

TEST(RationalConstruction, ReducesAndNormalizesSign) {
  EXPECT_EQ(IntRational(2, 4), IntRational(1, 2));
  EXPECT_EQ(IntRational(3, -9), IntRational(-1, 3));
  EXPECT_EQ(IntRational(-3, -9), IntRational(1, 3));
}

TEST(RationalConstruction, RejectsZeroDenominator) {
  EXPECT_THROW((void)IntRational(1, 0), std::invalid_argument);
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
  IntRational const a(1, 4);
  IntRational const b(1, 3);
  IntRational const c(1, 2);

  EXPECT_TRUE(a < b);
  EXPECT_TRUE(b < c);
  EXPECT_TRUE(a < c);
  EXPECT_TRUE(c > b);
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(c >= b);
  EXPECT_EQ((a <=> IntRational(2, 8)), std::strong_ordering::equal);
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

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
