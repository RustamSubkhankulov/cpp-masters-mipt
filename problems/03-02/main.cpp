#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include <gtest/gtest.h>

class Triangle final {
public:
  Triangle() = delete;
  explicit Triangle(double a, double b, double c)
    : a_(a)
    , b_(b)
    , c_(c) {
    validate_();
  }

  [[nodiscard]] double perimeter() const noexcept {
    return a_ + b_ + c_;
  }

  [[nodiscard]] double area() const noexcept {
    const double p = perimeter();
    const double s = p / 2.;
    const double radicand = std::max(0., (s) * (s - a_) * (s - b_) * (s - c_));
    return std::sqrt(radicand);
  }

  [[nodiscard]] double getA() const noexcept {
    return a_;
  }
  [[nodiscard]] double getB() const noexcept {
    return b_;
  }
  [[nodiscard]] double getC() const noexcept {
    return c_;
  }

private:
  double a_;
  double b_;
  double c_;

  void validate_() const {
    if (!(std::isfinite(a_) && std::isfinite(b_) && std::isfinite(c_))) {
      throw std::invalid_argument("Triangle sides must be finite numbers.");
    }
    if (a_ <= 0. || b_ <= 0. || c_ <= 0.) {
      throw std::invalid_argument("Triangle sides must be positive.");
    }
    if (!(a_ + b_ > c_ && a_ + c_ > b_ && b_ + c_ > a_)) {
      throw std::invalid_argument("Triangle inequality violated.");
    }
  }
};

class Square final {
public:
  Square() = delete;
  explicit Square(double side)
    : side_(side) {
    validate_();
  }

  [[nodiscard]] double perimeter() const noexcept {
    return 4. * side_;
  }

  [[nodiscard]] double area() const noexcept {
    return side_ * side_;
  }

  [[nodiscard]] double getSide() const noexcept {
    return side_;
  }

private:
  double side_;

  void validate_() const {
    if (!std::isfinite(side_)) {
      throw std::invalid_argument("Square side must be a finite number.");
    }
    if (side_ <= 0.) {
      throw std::invalid_argument("Square side must be positive.");
    }
  }
};

class Circle final {
public:
  Circle() = delete;
  explicit Circle(double radius)
    : radius_(radius) {
    validate_();
  }

  [[nodiscard]] double perimeter() const noexcept {
    return 2. * std::numbers::pi_v<double> * radius_;
  }

  [[nodiscard]] double area() const noexcept {
    return std::numbers::pi_v<double> * radius_ * radius_;
  }

  [[nodiscard]] double getRadius() const noexcept {
    return radius_;
  }

private:
  double radius_;

  void validate_() const {
    if (!std::isfinite(radius_)) {
      throw std::invalid_argument("Circle radius must be a finite number.");
    }
    if (radius_ <= 0.) {
      throw std::invalid_argument("Circle radius must be positive.");
    }
  }
};

namespace {

TEST(Triangle, Basic) {
  const Triangle t(2.5, 3.5, 3.);
  EXPECT_DOUBLE_EQ(t.getA(), 2.5);
  EXPECT_DOUBLE_EQ(t.getB(), 3.5);
  EXPECT_DOUBLE_EQ(t.getC(), 3.);
  EXPECT_DOUBLE_EQ(t.perimeter(), 9.);
  const double s = 4.5;
  const double expected = std::sqrt(s * (s - 2.5) * (s - 3.5) * (s - 3.));
  EXPECT_NEAR(t.area(), expected, 1e-12);
}

TEST(Triangle, Validity) {
  EXPECT_THROW(Triangle(0., 1., 1.), std::invalid_argument);
  EXPECT_THROW(Triangle(-1., 2., 2.), std::invalid_argument);

  EXPECT_THROW(Triangle(1., 2., 3.5), std::invalid_argument);
  EXPECT_THROW(Triangle(10., 1., 1.), std::invalid_argument);

  EXPECT_THROW(Triangle(std::numeric_limits<double>::infinity(), 2., 3.5), std::invalid_argument);
  EXPECT_THROW(Triangle(1., std::numeric_limits<double>::infinity(), 3.5), std::invalid_argument);
  EXPECT_THROW(Triangle(1., 2., std::numeric_limits<double>::infinity()), std::invalid_argument);

  EXPECT_THROW(Triangle(std::numeric_limits<double>::quiet_NaN(), 2., 3.5), std::invalid_argument);
  EXPECT_THROW(Triangle(1., std::numeric_limits<double>::quiet_NaN(), 3.5), std::invalid_argument);
  EXPECT_THROW(Triangle(1., 2., std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
}

TEST(Square, Basic) {
  const Square s(2.);
  EXPECT_DOUBLE_EQ(s.getSide(), 2.);
  EXPECT_DOUBLE_EQ(s.perimeter(), 8.);
  EXPECT_DOUBLE_EQ(s.area(), 4.);
}

TEST(Square, Validity) {
  EXPECT_THROW(Square(0.), std::invalid_argument);
  EXPECT_THROW(Square(-5.), std::invalid_argument);
  EXPECT_THROW(Square{std::numeric_limits<double>::infinity()},
               std::invalid_argument);
  EXPECT_THROW(Square{std::numeric_limits<double>::quiet_NaN()},
               std::invalid_argument);
}

TEST(Circle, Basic) {
  const double r = 2.25;
  const Circle c(r);
  EXPECT_DOUBLE_EQ(c.getRadius(), r);
  EXPECT_DOUBLE_EQ(c.perimeter(), 2. * std::numbers::pi_v<double> * r);
  EXPECT_DOUBLE_EQ(c.area(), std::numbers::pi_v<double> * r * r);
}

TEST(CircleTest, Validity) {
  EXPECT_THROW(Circle(0.), std::invalid_argument);
  EXPECT_THROW(Circle(-0.1), std::invalid_argument);
  EXPECT_THROW(Circle{std::numeric_limits<double>::infinity()},
               std::invalid_argument);
  EXPECT_THROW(Circle{std::numeric_limits<double>::quiet_NaN()},
               std::invalid_argument);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
