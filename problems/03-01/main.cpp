#include <algorithm>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

class Rectangle final {
private:
  int x1_;
  int y1_;
  int x2_;
  int y2_;

public:
  Rectangle(int x1, int y1, int x2, int y2)
    : x1_(x1)
    , y1_(y1)
    , x2_(x2)
    , y2_(y2) {
    if (x1_ > x2_ || y1_ > y2_) {
      throw std::invalid_argument(
        "Invalid rectangle: require x1<=x2 and y1<=y2.");
    }
  }

  // Accessors
  int x1() const {
    return x1_;
  }
  int y1() const {
    return y1_;
  }
  int x2() const {
    return x2_;
  }
  int y2() const {
    return y2_;
  }

  // Dimensions and area (64-bit to prevent overflow).
  int64_t width() const {
    return static_cast<int64_t>(x2_) - static_cast<int64_t>(x1_);
  }
  int64_t height() const {
    return static_cast<int64_t>(y2_) - static_cast<int64_t>(y1_);
  }
  int64_t area() const {
    return width() * height();
  }

  // Degenerate = zero area (valid but line or point).
  bool is_degenerate() const {
    return width() == 0 || height() == 0;
  }

  // Intersection with another rectangle; returns std::nullopt if empty (no
  // common points).
  std::optional<Rectangle> intersect(const Rectangle& other) const {
    int nx1 = std::max(x1_, other.x1_);
    int ny1 = std::max(y1_, other.y1_);
    int nx2 = std::min(x2_, other.x2_);
    int ny2 = std::min(y2_, other.y2_);
    if (nx1 > nx2 || ny1 > ny2) {
      return std::nullopt;
    }
    return Rectangle(nx1, ny1, nx2, ny2);
  }

  // Minimal axis-aligned bounding box of *this and other.
  Rectangle combine(const Rectangle& other) const {
    int bx1 = std::min(x1_, other.x1_);
    int by1 = std::min(y1_, other.y1_);
    int bx2 = std::max(x2_, other.x2_);
    int by2 = std::max(y2_, other.y2_);
    return Rectangle(bx1, by1, bx2, by2);
  }
};

enum class IntersectionType { Empty, Degenerate, Proper };

IntersectionType classify(const Rectangle& r) {
  return (r.is_degenerate() ? IntersectionType::Degenerate
                            : IntersectionType::Proper);
}

// Returns (type, optional rectangle). If Empty, has std::nullopt.
std::pair<IntersectionType, std::optional<Rectangle>>
intersection(const std::vector<Rectangle>& rs) {
  if (rs.empty()) {
    return {IntersectionType::Empty, std::nullopt};
  }
  std::optional<Rectangle> acc = rs.front();
  for (std::size_t i = 1uz; i < rs.size(); ++i) {
    if (!acc)
      return {IntersectionType::Empty, std::nullopt};
    acc = acc->intersect(rs[i]);
    if (!acc)
      return {IntersectionType::Empty, std::nullopt};
  }
  return {classify(*acc), acc};
}

// Area of intersection of many rectangles (0 for empty or degenerate).
int64_t intersection_area(const std::vector<Rectangle>& rs) {
  auto [type, r] = intersection(rs);
  if (type != IntersectionType::Proper)
    return 0;
  return r->area();
}

// Minimal bounding rectangle that contains all input rectangles; std::nullopt
// if empty input.
std::optional<Rectangle> bounding_rectangle(const std::vector<Rectangle>& rs) {
  if (rs.empty())
    return std::nullopt;
  Rectangle acc = rs.front();
  for (std::size_t i = 1uz; i < rs.size(); ++i) {
    acc = acc.combine(rs[i]);
  }
  return acc;
}

namespace {

TEST(Rectangle, Invariants) {
  EXPECT_NO_THROW((Rectangle{0, 0, 0, 0}));
  EXPECT_NO_THROW((Rectangle{1, 2, 5, 2}));
  EXPECT_NO_THROW((Rectangle{-3, -1, 7, 10}));
  EXPECT_THROW((Rectangle{5, 0, 4, 1}), std::invalid_argument);
  EXPECT_THROW((Rectangle{0, 2, 1, 1}), std::invalid_argument);
}

TEST(Intersection, ProperOverlap) {
  Rectangle a{0, 0, 10, 10};
  Rectangle b{5, 2, 12, 12};
  auto [type, r] = intersection({a, b});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->x1(), 5LL);
  EXPECT_EQ(r->y1(), 2LL);
  EXPECT_EQ(r->x2(), 10LL);
  EXPECT_EQ(r->y2(), 10LL);
  EXPECT_EQ(r->area(), 5LL * 8LL);
  EXPECT_EQ(intersection_area({a, b}), 40LL);
}

TEST(Intersection, DegenerateTouchOnVerticalEdge) {
  Rectangle a{0, 0, 10, 10};
  Rectangle c{10, 0, 20, 10};
  auto [type, r] = intersection({a, c});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Degenerate);
  EXPECT_EQ(r->width(), 0LL);
  EXPECT_EQ(r->height(), 10LL);
  EXPECT_EQ(intersection_area({a, c}), 0LL);
}

TEST(Intersection, DegenerateTouchOnHorizontalEdge) {
  Rectangle a{0, 0, 10, 10};
  Rectangle c{0, 10, 10, 20};
  auto [type, r] = intersection({a, c});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Degenerate);
  EXPECT_EQ(r->width(), 10LL);
  EXPECT_EQ(r->height(), 0LL);
  EXPECT_EQ(intersection_area({a, c}), 0LL);
}

TEST(Intersection, DegeneratePointTouch) {
  Rectangle a{0, 0, 10, 10};
  Rectangle c{10, 10, 20, 20}; // touch at point (10,10)
  auto [type, r] = intersection({a, c});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Degenerate);
  EXPECT_EQ(r->width(), 0LL);
  EXPECT_EQ(r->height(), 0LL);
}

TEST(Intersection, EmptyNoOverlap) {
  Rectangle a{0, 0, 10, 10};
  Rectangle d{11, 0, 20, 10};
  auto [type, r] = intersection({a, d});
  EXPECT_EQ(type, IntersectionType::Empty);
  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(intersection_area({a, d}), 0LL);
}

TEST(Intersection, Containment) {
  Rectangle outer{0, 0, 10, 10};
  Rectangle inner{2, 3, 6, 7};
  auto [type, r] = intersection({outer, inner});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->x1(), inner.x1());
  EXPECT_EQ(r->y1(), inner.y1());
  EXPECT_EQ(r->x2(), inner.x2());
  EXPECT_EQ(r->y2(), inner.y2());
  EXPECT_EQ(r->area(), inner.area());
}

TEST(Intersection, Identical) {
  Rectangle a{1, 2, 8, 9};
  Rectangle b{1, 2, 8, 9};
  auto [type, r] = intersection({a, b});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->area(), a.area());
}

TEST(Intersection, MultipleProper) {
  Rectangle a{0, 0, 10, 10};
  Rectangle b{2, 1, 8, 11};
  Rectangle c{3, 4, 9, 14};
  auto [type, r] = intersection({a, b, c});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->x1(), 3);
  EXPECT_EQ(r->y1(), 4);
  EXPECT_EQ(r->x2(), 8);
  EXPECT_EQ(r->y2(), 10);
  EXPECT_EQ(r->area(), (8LL - 3LL) * (10LL - 4LL));
}

TEST(Intersection, MultipleDegenerate) {
  Rectangle a{0, 0, 10, 10};
  Rectangle b{5, 0, 10, 10};
  Rectangle c{10, 2, 10, 8};
  auto [type, r] = intersection({a, b, c});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Degenerate);
  EXPECT_EQ(r->x1(), 10);
  EXPECT_EQ(r->x2(), 10);
  EXPECT_EQ(r->y1(), 2);
  EXPECT_EQ(r->y2(), 8);
  EXPECT_EQ(intersection_area({a, b, c}), 0LL);
}

TEST(Intersection, MultipleEmpty) {
  Rectangle a{0, 0, 10, 10};
  Rectangle b{2, 1, 8, 11};
  Rectangle c{11, 0, 20, 10}; // disjoint with a and b
  auto [type, r] = intersection({a, b, c});
  EXPECT_EQ(type, IntersectionType::Empty);
  EXPECT_FALSE(r.has_value());
}

TEST(Intersection, NegativeCoordinates) {
  Rectangle a{-10, -10, 0, 0};
  Rectangle b{-5, -3, 10, 10};
  auto [type, r] = intersection({a, b});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->x1(), -5);
  EXPECT_EQ(r->y1(), -3);
  EXPECT_EQ(r->x2(), 0);
  EXPECT_EQ(r->y2(), 0);
}

TEST(Intersection, LargeArea_UsesInt64) {
  // Dimensions chosen to overflow 32-bit area if multiplied in int, but we use
  // int64 internally.
  Rectangle a{
    0, 0, 2'000'000'000,
    2'000'000'000}; // width/height fit in int (difference), area ~ 4e18
  Rectangle b{1'000'000'000, 1'000'000'000, 2'000'000'000, 2'000'000'000};
  auto [type, r] = intersection({a, b});
  ASSERT_TRUE(r.has_value());
  EXPECT_EQ(type, IntersectionType::Proper);
  EXPECT_EQ(r->width(), 1'000'000'000LL);
  EXPECT_EQ(r->height(), 1'000'000'000LL);
  EXPECT_EQ(r->area(), 1'000'000'000LL * 1'000'000'000LL); // 1e18
}

TEST(Bounding, EmptyInput) {
  std::vector<Rectangle> v;
  auto br = bounding_rectangle(v);
  EXPECT_FALSE(br.has_value());
}

TEST(Bounding, Single) {
  Rectangle a{1, 2, 5, 7};
  auto br = bounding_rectangle({a});
  ASSERT_TRUE(br.has_value());
  EXPECT_EQ(br->x1(), 1);
  EXPECT_EQ(br->y1(), 2);
  EXPECT_EQ(br->x2(), 5);
  EXPECT_EQ(br->y2(), 7);
}

TEST(Bounding, Multiple) {
  Rectangle a{0, 0, 10, 10};
  Rectangle b{-3, 2, 8, 12};
  Rectangle c{3, -5, 9, 14};
  auto br = bounding_rectangle({a, b, c});
  ASSERT_TRUE(br.has_value());
  EXPECT_EQ(br->x1(), -3);
  EXPECT_EQ(br->y1(), -5);
  EXPECT_EQ(br->x2(), 10);
  EXPECT_EQ(br->y2(), 14);
}

TEST(Bounding, DegenerateInputs) {
  Rectangle lineH{0, 1, 5, 1};
  Rectangle lineV{3, -2, 3, 4};
  auto br = bounding_rectangle({lineH, lineV});
  ASSERT_TRUE(br.has_value());
  EXPECT_EQ(br->x1(), 0);
  EXPECT_EQ(br->y1(), -2);
  EXPECT_EQ(br->x2(), 5);
  EXPECT_EQ(br->y2(), 4);
}

TEST(Bounding, IdenticalRectangles) {
  Rectangle a{2, 2, 6, 6};
  Rectangle b{2, 2, 6, 6};
  auto br = bounding_rectangle({a, b});
  ASSERT_TRUE(br.has_value());
  EXPECT_EQ(br->x1(), 2);
  EXPECT_EQ(br->y1(), 2);
  EXPECT_EQ(br->x2(), 6);
  EXPECT_EQ(br->y2(), 6);
}

TEST(Bounding, NegativeCoordinates) {
  Rectangle a{-10, -10, -5, -5};
  Rectangle b{-8, -12, -1, -6};
  auto br = bounding_rectangle({a, b});
  ASSERT_TRUE(br.has_value());
  EXPECT_EQ(br->x1(), -10);
  EXPECT_EQ(br->y1(), -12);
  EXPECT_EQ(br->x2(), -1);
  EXPECT_EQ(br->y2(), -5);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
