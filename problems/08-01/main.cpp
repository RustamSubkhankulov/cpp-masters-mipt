#include <gtest/gtest.h>

#include <type_traits>

class Entity_v1 final {
public:
  explicit constexpr Entity_v1(const int value) noexcept
    : value_(value) {}

  [[nodiscard]] constexpr int Read() const noexcept {
    return value_;
  }

private:
  int value_;
};

struct Entity_v2 final {
  int value;
};

static_assert(std::is_standard_layout_v<Entity_v1>);
static_assert(std::is_standard_layout_v<Entity_v2>);
static_assert(sizeof(Entity_v1) == sizeof(Entity_v2));
static_assert(alignof(Entity_v1) == alignof(Entity_v2));

void FraudulentlyRewriteWithReinterpretCast(Entity_v1& entity,
                                            const int new_value) noexcept {
  auto& public_view = reinterpret_cast<Entity_v2&>(entity);
  public_view.value = new_value;
}

using RewriteFunction = void (*)(Entity_v1&, int) noexcept;

void CheckRewrite(const RewriteFunction rewrite) {
  constexpr int kInitialValue = 10;
  constexpr int kFraudulentValue = 77;

  Entity_v1 entity{kInitialValue};

  ASSERT_EQ(entity.Read(), kInitialValue);
  rewrite(entity, kFraudulentValue);
  EXPECT_EQ(entity.Read(), kFraudulentValue);
}

TEST(EntityFraudTest, ReinterpretCastAllowsExternalOverwrite) {
  CheckRewrite(&FraudulentlyRewriteWithReinterpretCast);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
