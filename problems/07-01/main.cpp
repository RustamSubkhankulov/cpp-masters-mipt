#include <cmath>
#include <optional>
#include <utility>
#include <variant>

#include <gtest/gtest.h>

using TwoRoots = std::pair<double, double>;
using RootValue = std::variant<double, TwoRoots, std::monostate>;
using SolveResult = std::optional<RootValue>;

constexpr double epsilon() noexcept {
  return 1e-10;
}

constexpr double two() noexcept {
  return 2.0;
}

constexpr double four() noexcept {
  return 4.0;
}

bool isZero(double value) noexcept {
  return std::abs(value) < epsilon();
}

double normalizeZero(double value) noexcept {
  return isZero(value) ? 0.0 : value;
}

TwoRoots makeOrderedRoots(double first, double second) noexcept {
  first = normalizeZero(first);
  second = normalizeZero(second);
  if (second < first) {
    std::swap(first, second);
  }
  return {first, second};
}

SolveResult solve(double a, double b, double c) {
  if (isZero(a)) {
    if (isZero(b)) {
      if (isZero(c)) {
        return RootValue{std::monostate{}};
      }
      return std::nullopt;
    }

    return RootValue{normalizeZero(-c / b)};
  }

  const double discriminant = b * b - four() * a * c;
  if (discriminant > epsilon()) {
    const double sqrtDiscriminant = std::sqrt(discriminant);
    const double denominator = two() * a;
    const double firstRoot = (-b - sqrtDiscriminant) / denominator;
    const double secondRoot = (-b + sqrtDiscriminant) / denominator;
    return RootValue{makeOrderedRoots(firstRoot, secondRoot)};
  }

  if (discriminant < -epsilon()) {
    return std::nullopt;
  }

  return RootValue{normalizeZero(-b / (two() * a))};
}

namespace {

bool areClose(double left, double right) noexcept {
  return std::abs(left - right) < epsilon();
}

void expectNoRoots(const SolveResult& result) {
  EXPECT_FALSE(result.has_value());
}

void expectInfiniteRoots(const SolveResult& result) {
  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(std::holds_alternative<std::monostate>(*result));
}

void expectSingleRoot(const SolveResult& result, double expectedRoot) {
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<double>(*result));
  const double actualRoot = std::get<double>(*result);
  EXPECT_TRUE(areClose(actualRoot, expectedRoot));
}

void expectTwoRoots(
    const SolveResult& result,
    double expectedFirstRoot,
    double expectedSecondRoot) {
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<TwoRoots>(*result));
  const TwoRoots& actualRoots = std::get<TwoRoots>(*result);
  EXPECT_TRUE(areClose(actualRoots.first, expectedFirstRoot));
  EXPECT_TRUE(areClose(actualRoots.second, expectedSecondRoot));
}

TEST(SolveTest, ReturnsNoRootsForInconsistentDegenerateEquation) {
  expectNoRoots(solve(0.0, 0.0, 5.0));
}

TEST(SolveTest, ReturnsInfiniteRootsForIdenticallyZeroEquation) {
  expectInfiniteRoots(solve(0.0, 0.0, 0.0));
}

TEST(SolveTest, SolvesLinearEquation) {
  expectSingleRoot(solve(0.0, 2.0, -8.0), 4.0);
}

TEST(SolveTest, SolvesQuadraticEquationWithTwoDistinctRoots) {
  expectTwoRoots(solve(1.0, -3.0, 2.0), 1.0, 2.0);
}

TEST(SolveTest, ReturnsOrderedRootsForNegativeLeadingCoefficient) {
  expectTwoRoots(solve(-1.0, 3.0, -2.0), 1.0, 2.0);
}

TEST(SolveTest, SolvesQuadraticEquationWithOneRoot) {
  expectSingleRoot(solve(1.0, -2.0, 1.0), 1.0);
}

TEST(SolveTest, ReturnsNoRootsForNegativeDiscriminant) {
  expectNoRoots(solve(1.0, 0.0, 1.0));
}

TEST(SolveTest, TreatsSmallCoefficientsAsZero) {
  expectInfiniteRoots(solve(epsilon() / two(), epsilon() / two(), epsilon() / two()));
}

TEST(SolveTest, NormalizesNegativeZeroForLinearRoot) {
  const SolveResult result = solve(0.0, 1.0, 0.0);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<double>(*result));
  EXPECT_FALSE(std::signbit(std::get<double>(*result)));
}

TEST(SolveTest, NormalizesNegativeZeroForQuadraticDoubleRoot) {
  const SolveResult result = solve(1.0, 0.0, 0.0);
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<double>(*result));
  EXPECT_FALSE(std::signbit(std::get<double>(*result)));
}

TEST(DemoExampleTest, DemonstratesLinearCase) {
  expectSingleRoot(solve(0.0, -7.0, 21.0), 3.0);
}

TEST(DemoExampleTest, DemonstratesQuadraticCaseWithTwoRoots) {
  expectTwoRoots(solve(2.0, -5.0, 2.0), 0.5, 2.0);
}

TEST(DemoExampleTest, DemonstratesInfiniteSolutionsCase) {
  expectInfiniteRoots(solve(0.0, 0.0, 0.0));
}

}  // namespace

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
