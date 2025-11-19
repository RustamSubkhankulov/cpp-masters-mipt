#include <compare>
#include <istream>
#include <numeric>
#include <ostream>

// Google Test
#include <gtest/gtest.h>

class Rational {
public:
  /* explicit */ Rational(int num = 0, int den = 1)
    : m_num(num)
    , m_den(den) {
    reduce();
  }

  explicit operator double() const {
    return 1. * m_num / m_den;
  }

  auto& operator+=(Rational const& other) {
    auto lcm = std::lcm(m_den, other.m_den);
    m_num = m_num * (lcm / m_den) + other.m_num * (lcm / other.m_den);
    m_den = lcm;
    reduce();
    return *this;
  }

  auto& operator-=(Rational const& other) {
    return *this += Rational(other.m_num * -1, other.m_den);
  }

  auto& operator*=(Rational const& other) {
    m_num *= other.m_num;
    m_den *= other.m_den;
    reduce();
    return *this;
  }

  auto& operator/=(Rational const& other) {
    return *this *= Rational(other.m_den, other.m_num);
  }

  auto operator++(int) {
    auto x = *this;
    *this += 1;
    return x;
  }
  auto operator--(int) {
    auto x = *this;
    *this -= 1;
    return x;
  }

  auto& operator++() {
    *this += 1;
    return *this;
  }
  auto& operator--() {
    *this -= 1;
    return *this;
  }

  friend auto operator+(Rational lhs, Rational const& rhs) {
    return lhs += rhs;
  }
  friend auto operator-(Rational lhs, Rational const& rhs) {
    return lhs -= rhs;
  }
  friend auto operator*(Rational lhs, Rational const& rhs) {
    return lhs *= rhs;
  }
  friend auto operator/(Rational lhs, Rational const& rhs) {
    return lhs /= rhs;
  }

  // Equality: class keeps canonical form (denominator > 0; reduced by gcd), so
  // field-wise compare is valid.
  friend bool operator==(Rational const& lhs, Rational const& rhs) {
    return lhs.m_num == rhs.m_num && lhs.m_den == rhs.m_den;
  }

  // Strong three-way comparison using cross multiplication with wider
  // intermediate.
  friend std::strong_ordering operator<=>(Rational const& lhs,
                                          Rational const& rhs) {
    long long left =
      static_cast<long long>(lhs.m_num) * static_cast<long long>(rhs.m_den);
    long long right =
      static_cast<long long>(rhs.m_num) * static_cast<long long>(lhs.m_den);

    return left <=> right;
  }

  friend auto& operator>>(std::istream& stream, Rational& rational) {
    return (stream >> rational.m_num).ignore() >> rational.m_den;
  }

  friend auto& operator<<(std::ostream& stream, Rational const& rational) {
    return stream << rational.m_num << '/' << rational.m_den;
  }

private:
  void reduce() {
    if (m_den < 0) {
      m_num = -m_num;
      m_den = -m_den;
    }

    auto gcd = std::gcd(m_num, m_den);
    m_num /= gcd;
    m_den /= gcd;
  }

  int m_num = 0, m_den = 1;
};

static_assert(std::three_way_comparable<Rational, std::strong_ordering>);

namespace {

TEST(RationalCompare, ReflexiveEquality) {
  Rational a(3, -6);

  EXPECT_TRUE(a == a);
  EXPECT_FALSE(a != a);
  EXPECT_EQ((a <=> a), std::strong_ordering::equal);
}

TEST(RationalCompare, Reduction) {
  Rational a(1, 2);
  Rational b(2, 4);

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_EQ((a <=> b), std::strong_ordering::equal);
}

TEST(RationalCompare, SignNormalization) {
  Rational a(1, -2);
  Rational b(-2, 4);

  EXPECT_TRUE(a == b);
  EXPECT_EQ((a <=> b), std::strong_ordering::equal);
}

TEST(RationalCompare, Generic) {
  Rational a(1, 3);
  Rational b(2, 5);

  EXPECT_LT(a, b);
  EXPECT_LE(a, b);
  EXPECT_GT(b, a);
  EXPECT_GE(b, a);
  EXPECT_NE((a <=> b), std::strong_ordering::equal);
  EXPECT_EQ((a <=> b), std::strong_ordering::less);
  EXPECT_EQ((b <=> a), std::strong_ordering::greater);
}

TEST(RationalCompare, Antisymmetry) {
  Rational a(5, 7);
  Rational b(8, 7);

  EXPECT_FALSE(b <= a);
  EXPECT_TRUE(b > a);
}

TEST(RationalCompare, Transitivity) {
  Rational a(1, 4);
  Rational b(1, 3);
  Rational c(1, 2);

  EXPECT_LT(a, b);
  EXPECT_LT(b, c);
  EXPECT_LT(a, c);
}

TEST(RationalCompare, MixedSigns) {
  Rational neg(-7, 3);
  Rational pos(2, 9);
  EXPECT_LT(neg, pos);
  EXPECT_GT(pos, neg);
  EXPECT_NE((neg <=> pos), std::strong_ordering::equal);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
