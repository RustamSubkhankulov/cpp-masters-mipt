#include <compare>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

class IPv4 final {
public:
  constexpr IPv4(std::uint8_t a, std::uint8_t b, std::uint8_t c,
                 std::uint8_t d) noexcept
    : octets_{a, b, c, d} {}

  constexpr IPv4() = default;

  explicit constexpr IPv4(std::uint32_t value) noexcept
    : octets_{static_cast<std::uint8_t>((value >> 24u) & 0xFFu),
              static_cast<std::uint8_t>((value >> 16u) & 0xFFu),
              static_cast<std::uint8_t>((value >> 8u) & 0xFFu),
              static_cast<std::uint8_t>(value & 0xFFu)} {}

  [[nodiscard]] constexpr std::uint8_t octet(std::size_t i) const noexcept {
    return octets_[i];
  }

  [[nodiscard]] constexpr std::uint32_t to_uint32() const noexcept {
    return pack(octets_[0], octets_[1], octets_[2], octets_[3]);
  }

  [[nodiscard]] constexpr operator std::uint32_t() const noexcept {
    return to_uint32();
  }

  [[nodiscard]] std::string to_string() const {
    std::ostringstream os;
    os << *this;
    return os.str();
  }

  [[nodiscard]] operator std::string() const {
    return to_string();
  }

  IPv4& operator++() noexcept {
    std::uint32_t v = to_uint32();
    v += 1u;
    *this = IPv4{v};
    return *this;
  }

  IPv4& operator--() noexcept {
    std::uint32_t v = to_uint32();
    v -= 1u;
    *this = IPv4{v};
    return *this;
  }

  IPv4 operator++(int) noexcept {
    IPv4 tmp{*this};
    ++(*this);
    return tmp;
  }

  IPv4 operator--(int) noexcept {
    IPv4 tmp{*this};
    --(*this);
    return tmp;
  }

  friend bool operator==(const IPv4& lhs, const IPv4& rhs) noexcept {
    return lhs.to_uint32() == rhs.to_uint32();
  }

  friend std::strong_ordering operator<=>(const IPv4& lhs,
                                          const IPv4& rhs) noexcept {
    return lhs.to_uint32() <=> rhs.to_uint32();
  }

  friend std::ostream& operator<<(std::ostream& os, const IPv4& ip) {
    return os << static_cast<unsigned>(ip.octets_[0]) << '.'
              << static_cast<unsigned>(ip.octets_[1]) << '.'
              << static_cast<unsigned>(ip.octets_[2]) << '.'
              << static_cast<unsigned>(ip.octets_[3]);
  }

  friend std::istream& operator>>(std::istream& is, IPv4& ip) {
    int a = 0, b = 0, c = 0, d = 0;
    char dot1 = 0, dot2 = 0, dot3 = 0;

    std::istream::sentry s{is};
    if (!s) {
      return is;
    }

    auto good = (is >> a >> dot1 >> b >> dot2 >> c >> dot3 >> d) &&
                dot1 == '.' && dot2 == '.' && dot3 == '.' &&
                (0 <= a && a <= 255) && (0 <= b && b <= 255) &&
                (0 <= c && c <= 255) && (0 <= d && d <= 255);

    if (good) {
      ip.octets_[0] = static_cast<std::uint8_t>(a);
      ip.octets_[1] = static_cast<std::uint8_t>(b);
      ip.octets_[2] = static_cast<std::uint8_t>(c);
      ip.octets_[3] = static_cast<std::uint8_t>(d);
    } else {
      is.setstate(std::ios::failbit);
    }
    return is;
  }

private:
  std::uint8_t octets_[4]{};

  static constexpr std::uint32_t pack(std::uint8_t a, std::uint8_t b,
                                      std::uint8_t c, std::uint8_t d) noexcept {
    return (static_cast<std::uint32_t>(a) << 24u) |
           (static_cast<std::uint32_t>(b) << 16u) |
           (static_cast<std::uint32_t>(c) << 8u) |
           (static_cast<std::uint32_t>(d));
  }
};

namespace {

TEST(IPv4, Uint32Roundabout) {
  IPv4 ip{192, 168, 1, 1};

  EXPECT_EQ(ip.to_uint32(), 0xC0A80101u);
  EXPECT_EQ(static_cast<std::uint32_t>(ip), 0xC0A80101u);

  IPv4 back{0xC0A80101u};

  EXPECT_EQ(back.octet(0), 192);
  EXPECT_EQ(back.octet(1), 168);
  EXPECT_EQ(back.octet(2), 1);
  EXPECT_EQ(back.octet(3), 1);

  EXPECT_EQ(back.to_string(), "192.168.1.1");
  EXPECT_EQ(static_cast<std::string>(back), "192.168.1.1");
}

TEST(IPv4, Output) {
  IPv4 ip{10, 0, 0, 42};
  std::ostringstream os;
  os << ip;
  EXPECT_EQ(os.str(), "10.0.0.42");
}

TEST(IPv4, InputValid) {
  std::istringstream is{"0.0.0.0 255.255.255.255 1.2.3.4\n"};
  IPv4 a{}, b{}, c{};
  is >> a >> b >> c;

  EXPECT_FALSE(is.fail());

  EXPECT_EQ(a.to_string(), "0.0.0.0");
  EXPECT_EQ(b.to_string(), "255.255.255.255");
  EXPECT_EQ(c.to_string(), "1.2.3.4");
}

TEST(IPv4, InputValidWithWhitespaces) {
  std::istringstream is{"   12   . 34\t.\n56 .78  "};
  IPv4 ip{};
  is >> ip;

  EXPECT_FALSE(is.fail());
  EXPECT_EQ(ip.to_string(), "12.34.56.78");
}

TEST(IPv4, InputInvalidOutOfRange) {
  IPv4 ip{1, 1, 1, 1};
  std::istringstream is{"256.0.0.1"};
  is >> ip;

  EXPECT_TRUE(is.fail());
  EXPECT_EQ(ip.to_string(), "1.1.1.1");
}

TEST(IPv4, InputInvalidNegative) {
  IPv4 ip{2, 2, 2, 2};
  std::istringstream is{"-1.0.0.0"};
  is >> ip;

  EXPECT_TRUE(is.fail());
  EXPECT_EQ(ip.to_string(), "2.2.2.2");
}

TEST(IPv4, InputInvalidSeparator) {
  IPv4 ip{3, 3, 3, 3};
  std::istringstream is{"1,2,3,4"};
  is >> ip;

  EXPECT_TRUE(is.fail());
  EXPECT_EQ(ip.to_string(), "3.3.3.3");
}

TEST(IPv4, PrefixIncr) {
  IPv4 ip{1, 2, 3, 4};
  IPv4& ref = ++ip;

  EXPECT_EQ(&ref, &ip);
  EXPECT_EQ(ip.to_string(), "1.2.3.5");
}

TEST(IPv4, PostfixIncr) {
  IPv4 ip{1, 2, 3, 4};
  IPv4 old = ip++;

  EXPECT_EQ(old.to_string(), "1.2.3.4");
  EXPECT_EQ(ip.to_string(), "1.2.3.5");
}

TEST(IPv4, IncrCarry) {
  IPv4 ip{1, 2, 3, 255};
  ++ip;

  EXPECT_EQ(ip.to_string(), "1.2.4.0");
}

TEST(IPv4, IncrWrap) {
  IPv4 ip{255, 255, 255, 255};
  ++ip;

  EXPECT_EQ(ip.to_string(), "0.0.0.0");
}

TEST(IPv4, PrefixDecr) {
  IPv4 ip{1, 0, 0, 0};
  --ip;

  EXPECT_EQ(ip.to_string(), "0.255.255.255");
}

TEST(IPv4, PostfixDecr) {
  IPv4 ip{1, 2, 3, 0};
  IPv4 old = ip--;

  EXPECT_EQ(old.to_string(), "1.2.3.0");
  EXPECT_EQ(ip.to_string(), "1.2.2.255");
}

TEST(IPv4, DecrWrap) {
  IPv4 ip{0, 0, 0, 0};
  --ip;

  EXPECT_EQ(ip.to_string(), "255.255.255.255");
}

TEST(IPv4, EqualityAndOrdering) {
  IPv4 a{10, 0, 0, 1};
  IPv4 b{10, 0, 0, 1};
  IPv4 c{10, 0, 0, 2};
  IPv4 d{9, 255, 255, 255};

  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);
  EXPECT_TRUE(a < c);
  EXPECT_TRUE(c > a);
  EXPECT_TRUE(d < a);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(c >= a);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
