#include <array>
#include <cmath>
#include <cstddef>
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

constexpr std::size_t noRootsCount() noexcept {
  return 0U;
}

constexpr std::size_t singleRootCount() noexcept {
  return 1U;
}

constexpr std::size_t twoRootsCount() noexcept {
  return 2U;
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

enum class RootKind {
  kSingle,
  kTwo,
  kInfinite,
};

struct ExtractedRoots {
  RootKind kind;
  std::array<double, twoRootsCount()> roots;
  std::size_t count;
};

class Visitor {
 public:
  ExtractedRoots operator()(double root) const noexcept {
    return {RootKind::kSingle, {root, 0.0}, singleRootCount()};
  }

  ExtractedRoots operator()(const TwoRoots& roots) const noexcept {
    return {RootKind::kTwo, {roots.first, roots.second}, twoRootsCount()};
  }

  ExtractedRoots operator()(std::monostate) const noexcept {
    return {RootKind::kInfinite, {0.0, 0.0}, noRootsCount()};
  }
};

std::optional<ExtractedRoots> extractRoots(const SolveResult& result) {
  if (!result.has_value()) {
    return std::nullopt;
  }

  return std::visit(Visitor{}, *result);
}

namespace {

bool areClose(double left, double right) noexcept {
  return std::abs(left - right) < epsilon();
}

void expectNoRoots(const SolveResult& result) {
  EXPECT_FALSE(extractRoots(result).has_value());
}

void expectInfiniteRoots(const SolveResult& result) {
  const std::optional<ExtractedRoots> extractedRoots = extractRoots(result);
  ASSERT_TRUE(extractedRoots.has_value());
  EXPECT_EQ(extractedRoots->kind, RootKind::kInfinite);
  EXPECT_EQ(extractedRoots->count, noRootsCount());
}

void expectSingleRoot(const SolveResult& result, double expectedRoot) {
  const std::optional<ExtractedRoots> extractedRoots = extractRoots(result);
  ASSERT_TRUE(extractedRoots.has_value());
  EXPECT_EQ(extractedRoots->kind, RootKind::kSingle);
  EXPECT_EQ(extractedRoots->count, singleRootCount());
  EXPECT_TRUE(areClose(extractedRoots->roots.front(), expectedRoot));
}

void expectTwoRoots(const SolveResult& result, double expectedFirstRoot,
                    double expectedSecondRoot) {
  const std::optional<ExtractedRoots> extractedRoots = extractRoots(result);
  ASSERT_TRUE(extractedRoots.has_value());
  EXPECT_EQ(extractedRoots->kind, RootKind::kTwo);
  EXPECT_EQ(extractedRoots->count, twoRootsCount());
  EXPECT_TRUE(areClose(extractedRoots->roots[0], expectedFirstRoot));
  EXPECT_TRUE(areClose(extractedRoots->roots[1], expectedSecondRoot));
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
  expectInfiniteRoots(solve(epsilon() / two(), epsilon() / two(),
                            epsilon() / two()));
}

TEST(SolveTest, NormalizesNegativeZeroForLinearRoot) {
  const std::optional<ExtractedRoots> extractedRoots =
      extractRoots(solve(0.0, 1.0, 0.0));
  ASSERT_TRUE(extractedRoots.has_value());
  ASSERT_EQ(extractedRoots->kind, RootKind::kSingle);
  EXPECT_FALSE(std::signbit(extractedRoots->roots.front()));
}

TEST(SolveTest, NormalizesNegativeZeroForQuadraticDoubleRoot) {
  const std::optional<ExtractedRoots> extractedRoots =
      extractRoots(solve(1.0, 0.0, 0.0));
  ASSERT_TRUE(extractedRoots.has_value());
  ASSERT_EQ(extractedRoots->kind, RootKind::kSingle);
  EXPECT_FALSE(std::signbit(extractedRoots->roots.front()));
}

TEST(VisitorTest, ExtractsSingleRootFromVariant) {
  const RootValue rootValue = 7.0;
  const ExtractedRoots extractedRoots = std::visit(Visitor{}, rootValue);

  EXPECT_EQ(extractedRoots.kind, RootKind::kSingle);
  EXPECT_EQ(extractedRoots.count, singleRootCount());
  EXPECT_TRUE(areClose(extractedRoots.roots.front(), 7.0));
}

TEST(VisitorTest, ExtractsTwoRootsFromVariant) {
  const RootValue rootValue = TwoRoots{2.0, 5.0};
  const ExtractedRoots extractedRoots = std::visit(Visitor{}, rootValue);

  EXPECT_EQ(extractedRoots.kind, RootKind::kTwo);
  EXPECT_EQ(extractedRoots.count, twoRootsCount());
  EXPECT_TRUE(areClose(extractedRoots.roots[0], 2.0));
  EXPECT_TRUE(areClose(extractedRoots.roots[1], 5.0));
}

TEST(VisitorTest, ExtractsInfiniteRootsStateFromVariant) {
  const RootValue rootValue = std::monostate{};
  const ExtractedRoots extractedRoots = std::visit(Visitor{}, rootValue);

  EXPECT_EQ(extractedRoots.kind, RootKind::kInfinite);
  EXPECT_EQ(extractedRoots.count, noRootsCount());
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
