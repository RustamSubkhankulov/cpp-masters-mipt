#include <gtest/gtest.h>

#include <type_traits>

class Wrapper;
using FunctionPointer = Wrapper (*)();

class Wrapper {
public:
  explicit Wrapper(FunctionPointer pointer = nullptr) noexcept
    : pointer_(pointer) {}

  operator FunctionPointer() const noexcept {
    return pointer_;
  }

private:
  FunctionPointer pointer_;
};

Wrapper test() {
  return Wrapper(&test);
}

static_assert(std::is_convertible_v<Wrapper, FunctionPointer>);
static_assert(std::is_same_v<decltype(test()), Wrapper>);

TEST(WrapperTest, SupportsRequiredSyntax) {
  Wrapper function = test();
  Wrapper next = (*function)();
  Wrapper one_more = (*next)();

  EXPECT_EQ(static_cast<FunctionPointer>(function), &test);
  EXPECT_EQ(static_cast<FunctionPointer>(next), &test);
  EXPECT_EQ(static_cast<FunctionPointer>(one_more), &test);
}

TEST(WrapperTest, DefaultConstructedWrapperStoresNullPointer) {
  const Wrapper function;
  EXPECT_EQ(static_cast<FunctionPointer>(function), nullptr);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
