#include <cstddef>
#include <gtest/gtest.h>
#include <new>

template <typename Derived>
class Entity {
public:
  struct MemoryStatistics {
    std::size_t single_new_calls = 0;
    std::size_t single_delete_calls = 0;
    std::size_t array_new_calls = 0;
    std::size_t array_delete_calls = 0;
    std::size_t single_new_nothrow_calls = 0;
    std::size_t single_delete_nothrow_calls = 0;
    std::size_t array_new_nothrow_calls = 0;
    std::size_t array_delete_nothrow_calls = 0;
  };

  static auto operator new(std::size_t size) -> void* {
    ++statistics_.single_new_calls;
    return ::operator new(size);
  }

  static void operator delete(void* pointer) noexcept {
    ++statistics_.single_delete_calls;
    ::operator delete(pointer);
  }

  static auto operator new[](std::size_t size) -> void* {
    ++statistics_.array_new_calls;
    return ::operator new[](size);
  }

  static void operator delete[](void* pointer) noexcept {
    ++statistics_.array_delete_calls;
    ::operator delete[](pointer);
  }

  static auto operator new(std::size_t size, const std::nothrow_t& tag) noexcept
    -> void* {
    ++statistics_.single_new_nothrow_calls;
    return ::operator new(size, tag);
  }

  static void operator delete(void* pointer,
                              const std::nothrow_t& tag) noexcept {
    ++statistics_.single_delete_nothrow_calls;
    ::operator delete(pointer, tag);
  }

  static auto operator new[](std::size_t size,
                             const std::nothrow_t& tag) noexcept -> void* {
    ++statistics_.array_new_nothrow_calls;
    return ::operator new[](size, tag);
  }

  static void operator delete[](void* pointer,
                                const std::nothrow_t& tag) noexcept {
    ++statistics_.array_delete_nothrow_calls;
    ::operator delete[](pointer, tag);
  }

  static void ResetMemoryStatistics() noexcept {
    statistics_ = MemoryStatistics{};
  }

  static auto GetMemoryStatistics() noexcept -> const MemoryStatistics& {
    return statistics_;
  }

protected:
  Entity() = default;
  ~Entity() = default;

private:
  inline static MemoryStatistics statistics_{};
};

class Client : private Entity<Client> {
public:
  Client() noexcept = default;
  ~Client() noexcept = default;

  using Entity<Client>::operator new;
  using Entity<Client>::operator delete;
  using Entity<Client>::operator new[];
  using Entity<Client>::operator delete[];

  static void ResetMemoryStatistics() noexcept {
    Entity<Client>::ResetMemoryStatistics();
  }

  static auto GetMemoryStatistics() noexcept
    -> const Entity<Client>::MemoryStatistics& {
    return Entity<Client>::GetMemoryStatistics();
  }
};

TEST(EntityMemoryOverloadTest, SingleObjectUsesClassSpecificOperators) {
  Client::ResetMemoryStatistics();

  Client* pointer = new Client();
  ASSERT_NE(pointer, nullptr);

  delete pointer;

  const auto& statistics = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics.single_new_calls, 1U);
  EXPECT_EQ(statistics.single_delete_calls, 1U);
  EXPECT_EQ(statistics.array_new_calls, 0U);
  EXPECT_EQ(statistics.array_delete_calls, 0U);
  EXPECT_EQ(statistics.single_new_nothrow_calls, 0U);
  EXPECT_EQ(statistics.single_delete_nothrow_calls, 0U);
  EXPECT_EQ(statistics.array_new_nothrow_calls, 0U);
  EXPECT_EQ(statistics.array_delete_nothrow_calls, 0U);
}

TEST(EntityMemoryOverloadTest, DynamicArrayUsesClassSpecificArrayOperators) {
  Client::ResetMemoryStatistics();

  constexpr std::size_t count = 4;
  Client* pointer = new Client[count];
  ASSERT_NE(pointer, nullptr);

  delete[] pointer;

  const auto& statistics = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics.single_new_calls, 0U);
  EXPECT_EQ(statistics.single_delete_calls, 0U);
  EXPECT_EQ(statistics.array_new_calls, 1U);
  EXPECT_EQ(statistics.array_delete_calls, 1U);
  EXPECT_EQ(statistics.single_new_nothrow_calls, 0U);
  EXPECT_EQ(statistics.single_delete_nothrow_calls, 0U);
  EXPECT_EQ(statistics.array_new_nothrow_calls, 0U);
  EXPECT_EQ(statistics.array_delete_nothrow_calls, 0U);
}

TEST(EntityMemoryOverloadTest, NothrowSingleAllocationIsAvailable) {
  Client::ResetMemoryStatistics();

  Client* pointer = new (std::nothrow) Client();
  ASSERT_NE(pointer, nullptr);

  const auto& statistics_after_new = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics_after_new.single_new_nothrow_calls, 1U);
  EXPECT_EQ(statistics_after_new.single_delete_nothrow_calls, 0U);

  delete pointer;

  const auto& statistics_after_delete = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics_after_delete.single_delete_calls, 1U);
}

TEST(EntityMemoryOverloadTest, NothrowSingleDeallocationFunctionIsAvailable) {
  Client::ResetMemoryStatistics();

  void* pointer = Client::operator new(sizeof(Client), std::nothrow);
  ASSERT_NE(pointer, nullptr);

  Client::operator delete(pointer, std::nothrow);

  const auto& statistics = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics.single_new_nothrow_calls, 1U);
  EXPECT_EQ(statistics.single_delete_nothrow_calls, 1U);
}

TEST(EntityMemoryOverloadTest, NothrowArrayDeallocationFunctionIsAvailable) {
  Client::ResetMemoryStatistics();

  constexpr std::size_t count = 3;
  void* pointer = Client::operator new[](sizeof(Client) * count, std::nothrow);
  ASSERT_NE(pointer, nullptr);

  Client::operator delete[](pointer, std::nothrow);

  const auto& statistics = Client::GetMemoryStatistics();
  EXPECT_EQ(statistics.array_new_nothrow_calls, 1U);
  EXPECT_EQ(statistics.array_delete_nothrow_calls, 1U);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
