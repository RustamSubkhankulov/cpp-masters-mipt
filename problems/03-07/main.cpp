#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <utility>

#include <gtest/gtest.h>

class Vector final {
public:
  using value_type = int;
  using size_type = std::size_t;

  Vector() noexcept = default;

  Vector(std::initializer_list<value_type> list)
    : m_array(list.size() ? new value_type[list.size()]{} : nullptr)
    , m_size(list.size())
    , m_capacity(list.size()) {
    if (m_array) {
      std::copy(list.begin(), list.end(), m_array);
    }
  }

  Vector(const Vector& other)
    : m_array(other.m_size ? new value_type[other.m_size]{} : nullptr)
    , m_size(other.m_size)
    , m_capacity(other.m_size) {
    if (m_array) {
      std::copy(other.m_array, other.m_array + other.m_size, m_array);
    }
  }

  Vector(Vector&& other) noexcept
    : m_array(std::exchange(other.m_array, nullptr))
    , m_size(std::exchange(other.m_size, 0uz))
    , m_capacity(std::exchange(other.m_capacity, 0uz)) {}

  ~Vector() {
    delete[] m_array;
  }

  Vector& operator=(const Vector& other) {
    if (this == &other) {
      return *this;
    }

    if (other.m_size > m_capacity) {
      // Need a bigger buffer
      auto* new_block = other.m_size ? new int[other.m_size]{} : nullptr;

      if (other.m_size) {
        std::copy(other.m_array, other.m_array + other.m_size, new_block);
      }

      delete[] m_array;
      m_array = new_block;

      m_capacity = other.m_size;
      m_size = other.m_size;
    } else {
      // Reuse existing buffer
      if (other.m_size) {
        std::copy(other.m_array, other.m_array + other.m_size, m_array);
      }
      // Shrink logical size if needed
      m_size = other.m_size;
    }

    return *this;
  }

  Vector& operator=(Vector&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    delete[] m_array;

    m_array = other.m_array;
    m_size = other.m_size;
    m_capacity = other.m_capacity;

    other.m_array = nullptr;
    other.m_size = 0uz;
    other.m_capacity = 0uz;

    return *this;
  }

  void push_back(const value_type& v) {
    ensure_capacity_for_growth();
    m_array[m_size++] = v;
  }

  void clear() noexcept {
    // Do not free memory. Just reset the logical size.
    m_size = 0uz;
  }

  void swap(Vector& other) noexcept {
    using std::swap;
    swap(m_array, other.m_array);
    swap(m_size, other.m_size);
    swap(m_capacity, other.m_capacity);
  }

  [[nodiscard]] size_type size() const noexcept {
    return m_size;
  }
  [[nodiscard]] size_type capacity() const noexcept {
    return m_capacity;
  }
  [[nodiscard]] bool empty() const noexcept {
    return m_size == 0uz;
  }

  value_type& operator[](size_type i) noexcept {
    return m_array[i];
  }
  const value_type& operator[](size_type i) const noexcept {
    return m_array[i];
  }
  value_type* data() noexcept {
    return m_array;
  }
  const value_type* data() const noexcept {
    return m_array;
  }

private:
  void ensure_capacity_for_growth() {
    if (m_size < m_capacity)
      return;

    // Growth policy: double previous capacity; if zero, grow to 1.
    size_type new_cap = (m_capacity == 0uz) ? 1uz : (m_capacity * 2uz);

    auto* new_block = new value_type[new_cap]{};
    if (m_array && m_size) {
      std::copy(m_array, m_array + m_size, new_block);
    }
    
    delete[] m_array;
    m_array = new_block;
    m_capacity = new_cap;
  }

private:
  value_type* m_array = nullptr;
  size_type m_size = 0uz;
  size_type m_capacity = 0uz;
};

inline void swap(Vector& a, Vector& b) noexcept {
  a.swap(b);
}

namespace {

TEST(Basic, Default) {
  Vector v;
  EXPECT_EQ(v.size(), 0uz);
  EXPECT_EQ(v.capacity(), 0uz);
  EXPECT_TRUE(v.empty());
  EXPECT_EQ(v.data(), nullptr);
}

TEST(Basic, InitFromList) {
  Vector v{1, 2, 3, 4};
  
  EXPECT_EQ(v.size(), 4uz);
  EXPECT_EQ(v.capacity(), 4uz);
  
  ASSERT_FALSE(v.empty());
  EXPECT_EQ(v[0], 1);
  EXPECT_EQ(v[1], 2);
  EXPECT_EQ(v[2], 3);
  EXPECT_EQ(v[3], 4);
}

TEST(Modify, Push) {
  Vector v;
  v.push_back(42);
  
  EXPECT_EQ(v.size(), 1uz);
  EXPECT_GE(v.capacity(), 1uz);
  EXPECT_FALSE(v.empty());
  EXPECT_EQ(v[0], 42);
  EXPECT_EQ(v.capacity(), 1uz);
}

TEST(Modify, MultiplePushes) {
  Vector v;
  v.push_back(1);
  auto cap1 = v.capacity();
  EXPECT_EQ(cap1, 1uz);

  v.push_back(2);
  auto cap2 = v.capacity();
  EXPECT_EQ(cap2, 2uz);

  auto* p_before = v.data();
  v.push_back(3);
  auto cap3 = v.capacity();
  EXPECT_EQ(cap3, 4uz);
  EXPECT_NE(v.data(), p_before);

  p_before = v.data();
  v.push_back(4);
  EXPECT_EQ(v.capacity(), 4uz);
  EXPECT_EQ(v.data(), p_before);

  v.push_back(5);
  EXPECT_EQ(v.capacity(), 8uz);

  EXPECT_EQ(v.size(), 5uz);
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(v[static_cast<std::size_t>(i)], i + 1);
  }
}

TEST(Modify, Clear) {
  Vector v{10, 20, 30};
  auto cap_before = v.capacity();
  v.clear();
  
  EXPECT_EQ(v.size(), 0uz);
  EXPECT_EQ(v.capacity(), cap_before);
  EXPECT_TRUE(v.empty());

  v.push_back(111);
  EXPECT_EQ(v.size(), 1uz);
  EXPECT_EQ(v.capacity(), cap_before);
  EXPECT_EQ(v[0], 111);
}

TEST(CopyCtor, Generic) {
  Vector a{7, 8, 9};
  a.push_back(10); // ensure non-trivial capacity
  
  Vector b = a;
  
  ASSERT_EQ(b.size(), a.size());

  a[0] = 100;
  EXPECT_EQ(b[0], 7);
}

TEST(MoveCtor, Generic) {
  Vector a{1, 2, 3};
  a.push_back(4);
  
  auto expected_size = a.size();
  auto expected_capacity = a.capacity();
  auto* expected_ptr = a.data();

  Vector b = std::move(a);
  
  EXPECT_EQ(b.size(), expected_size);
  EXPECT_EQ(b.capacity(), expected_capacity);
  EXPECT_EQ(b.data(), expected_ptr);

  EXPECT_EQ(a.size(), 0uz);
  EXPECT_EQ(a.capacity(), 0uz);
  EXPECT_EQ(a.data(), nullptr);

  b.push_back(99);
  EXPECT_EQ(b[b.size() - 1uz], 99);
}

TEST(CopyAssignment, Generic) {
  Vector a{1, 2, 3};
  Vector b{4, 5};
  
  auto* ptr_before = b.data();
  b = a;
  
  EXPECT_EQ(b.size(), a.size());

  EXPECT_EQ(b[0], 1);
  EXPECT_EQ(b[1], 2);
  EXPECT_EQ(b[2], 3);

  EXPECT_EQ(a.size(), 3uz);
  EXPECT_NE(b.data(), ptr_before);
}

TEST(Swap, Generic) {
  Vector a{1, 2, 3};
  Vector b;
  b.push_back(10); // capacity 1
  
  auto a_ptr = a.data();
  auto b_ptr = b.data();
  
  auto a_cap = a.capacity();
  auto b_cap = b.capacity();

  swap(a, b);
  
  EXPECT_EQ(a.data(), b_ptr);
  EXPECT_EQ(b.data(), a_ptr);
  EXPECT_EQ(a.capacity(), b_cap);
  EXPECT_EQ(b.capacity(), a_cap);

  EXPECT_EQ(a.size(), 1uz);
  EXPECT_EQ(b.size(), 3uz);
  
  EXPECT_EQ(a[0], 10);
  EXPECT_EQ(b[0], 1);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
