#include <string>
#include <gtest/gtest.h>

// Base classes, defining interfaces

struct Entity_v1 {
  virtual ~Entity_v1() = default;
  virtual std::string test() const {
    return "Entity_v1::test";
  }
};

struct Entity_v2 {
  virtual ~Entity_v2() = default;
  virtual std::string test() const {
    return "Entity_v2::test";
  }
};

// Adapters

struct Adapter_v1 : public Entity_v1 {
  virtual ~Adapter_v1() = default;

  virtual std::string test_v1() const {
    return "Adapter_v1::test_v1";
  }

  std::string test() const override {
    return test_v1();
  }
};

struct Adapter_v2 : public Entity_v2 {
  virtual ~Adapter_v2() = default;

  virtual std::string test_v2() const {
    return "Adapter_v2::test_v2";
  }

  std::string test() const override {
    return test_v2();
  }
};

// Client

struct Client final : public Adapter_v1, public Adapter_v2 {
  std::string test_v1() const override {
    return "Client::test_v1";
  }

  std::string test_v2() const override {
    return "Client::test_v2";
  }
};

TEST(Entity_v1, TestCheck) {
  Entity_v1 e;
  EXPECT_EQ(e.test(), "Entity_v1::test");
}

TEST(Entity_v2, TestCheck) {
  Entity_v2 e;
  EXPECT_EQ(e.test(), "Entity_v2::test");
}

TEST(Adapter_v1, TestCheck) {
  Adapter_v1 a;
  EXPECT_EQ(a.test(), "Adapter_v1::test_v1");
}

TEST(Adapter_v2, TestCheck) {
  Adapter_v2 a;
  EXPECT_EQ(a.test(), "Adapter_v2::test_v2");
}

TEST(ClientOverrides, AdaptorInterfaces) {
  Client c;

  // Call through Entity_v1 interface
  Entity_v1& v1 = c;
  EXPECT_EQ(v1.test(), "Client::test_v1");

  // Call through Entity_v2 interface
  Entity_v2& v2 = c;
  EXPECT_EQ(v2.test(), "Client::test_v2");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
