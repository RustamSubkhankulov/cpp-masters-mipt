#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace example_0501 {

struct Entity {
  int x = 0;
  int y = 0;
};

class Builder {
public:
  virtual ~Builder() = default;

  std::unique_ptr<Entity> make_entity() {
    m_entity = std::make_unique<Entity>();
    set_x();
    set_y();
    return std::move(m_entity);
  }

protected:
  virtual void set_x() = 0;
  virtual void set_y() = 0;

  Entity& entity() {
    return *m_entity;
  }

private:
  std::unique_ptr<Entity> m_entity;
};

class Builder_Client final : public Builder {
protected:
  void set_x() override {
    entity().x = 1;
  }

  void set_y() override {
    entity().y = 1;
  }
};

class Builder_Server final : public Builder {
protected:
  void set_x() override {
    entity().x = 1;
  }

  void set_y() override {
    entity().y = 1;
  }
};

} // namespace example_0501

namespace example_0503 {

class Entity {
public:
  virtual ~Entity() = default;
};

class Client final : public Entity {};

class Server final : public Entity {};

class Factory {
public:
  virtual ~Factory() = default;

  virtual std::unique_ptr<Entity> make_entity() const = 0;
};

class Factory_Client final : public Factory {
public:
  std::unique_ptr<Entity> make_entity() const override {
    return std::make_unique<Client>();
  }
};

class Factory_Server final : public Factory {
public:
  std::unique_ptr<Entity> make_entity() const override {
    return std::make_unique<Server>();
  }
};

} // namespace example_0503

namespace example_0504 {

class Entity {
public:
  virtual ~Entity() = default;

  virtual std::unique_ptr<Entity> copy() const = 0;
};

class Client final : public Entity {
public:
  std::unique_ptr<Entity> copy() const override {
    return std::make_unique<Client>(*this);
  }
};

class Server final : public Entity {
public:
  std::unique_ptr<Entity> copy() const override {
    return std::make_unique<Server>(*this);
  }
};

class Prototype {
public:
  Prototype() {
    m_entities.push_back(std::make_unique<Client>());
    m_entities.push_back(std::make_unique<Server>());
  }

  std::unique_ptr<Entity> make_client() const {
    return m_entities.at(0)->copy();
  }

  std::unique_ptr<Entity> make_server() const {
    return m_entities.at(1)->copy();
  }

private:
  std::vector<std::unique_ptr<Entity>> m_entities;
};

} // namespace example_0504

namespace example_0509 {

class Entity {
public:
  virtual ~Entity() = default;

  virtual int test() const = 0;
};

class Client final : public Entity {
public:
  int test() const override {
    return 1;
  }
};

class Server final : public Entity {
public:
  int test() const override {
    return 2;
  }
};

class Composite final : public Entity {
public:
  void add(std::unique_ptr<Entity> entity) {
    if (entity != nullptr) {
      m_entities.push_back(std::move(entity));
    }
  }

  int test() const override {
    int result = 0;

    for (const auto& entity : m_entities) {
      if (entity != nullptr) {
        result += entity->test();
      }
    }

    return result;
  }

private:
  std::vector<std::unique_ptr<Entity>> m_entities;
};

std::unique_ptr<Entity> make_composite(std::size_t size_1, std::size_t size_2) {
  auto composite = std::make_unique<Composite>();

  for (std::size_t i = 0; i < size_1; ++i) {
    composite->add(std::make_unique<Client>());
  }

  for (std::size_t i = 0; i < size_2; ++i) {
    composite->add(std::make_unique<Server>());
  }

  return composite;
}

} // namespace example_0509

namespace example_0513 {

class Observer {
public:
  virtual ~Observer() = default;

  virtual void test(int x) = 0;
};

class Entity {
public:
  void add(std::shared_ptr<Observer> observer) {
    if (observer != nullptr) {
      m_observers.push_back(std::move(observer));
    }
  }

  void set(int x) {
    m_x = x;
    notify_all();
  }

private:
  void notify_all() {
    for (const auto& observer : m_observers) {
      if (observer != nullptr) {
        observer->test(m_x);
      }
    }
  }

  int m_x = 0;
  std::vector<std::shared_ptr<Observer>> m_observers;
};

class Client final : public Observer {
public:
  void test(int x) override {
    m_values.push_back(x);
  }

  const std::vector<int>& values() const {
    return m_values;
  }

private:
  std::vector<int> m_values;
};

class Server final : public Observer {
public:
  void test(int x) override {
    m_values.push_back(x);
  }

  const std::vector<int>& values() const {
    return m_values;
  }

private:
  std::vector<int> m_values;
};

} // namespace example_0513

TEST(Example0501Test, BuilderClientBuildsEntity) {
  auto builder = std::make_unique<example_0501::Builder_Client>();
  auto entity = builder->make_entity();

  ASSERT_NE(entity, nullptr);
  EXPECT_EQ(entity->x, 1);
  EXPECT_EQ(entity->y, 1);
}

TEST(Example0501Test, BuilderServerBuildsEntity) {
  auto builder = std::make_unique<example_0501::Builder_Server>();
  auto entity = builder->make_entity();

  ASSERT_NE(entity, nullptr);
  EXPECT_EQ(entity->x, 1);
  EXPECT_EQ(entity->y, 1);
}

TEST(Example0503Test, FactoryClientCreatesClient) {
  auto factory = std::make_unique<example_0503::Factory_Client>();
  auto entity = factory->make_entity();

  ASSERT_NE(entity, nullptr);
  EXPECT_NE(dynamic_cast<example_0503::Client*>(entity.get()), nullptr);
}

TEST(Example0503Test, FactoryServerCreatesServer) {
  auto factory = std::make_unique<example_0503::Factory_Server>();
  auto entity = factory->make_entity();

  ASSERT_NE(entity, nullptr);
  EXPECT_NE(dynamic_cast<example_0503::Server*>(entity.get()), nullptr);
}

TEST(Example0504Test, PrototypeCreatesIndependentCopies) {
  const example_0504::Prototype prototype;

  auto client = prototype.make_client();
  auto server = prototype.make_server();

  ASSERT_NE(client, nullptr);
  ASSERT_NE(server, nullptr);
  EXPECT_NE(dynamic_cast<example_0504::Client*>(client.get()), nullptr);
  EXPECT_NE(dynamic_cast<example_0504::Server*>(server.get()), nullptr);
  EXPECT_NE(client.get(), server.get());
}

TEST(Example0509Test, CompositeCalculatesRecursiveSum) {
  auto composite = std::make_unique<example_0509::Composite>();

  for (std::size_t i = 0; i < 5; ++i) {
    composite->add(example_0509::make_composite(1, 1));
  }

  std::unique_ptr<example_0509::Entity> entity = std::move(composite);

  ASSERT_NE(entity, nullptr);
  EXPECT_EQ(entity->test(), 15);
}

TEST(Example0513Test, EntityNotifiesAllObservers) {
  example_0513::Entity entity;

  auto client = std::make_shared<example_0513::Client>();
  auto server = std::make_shared<example_0513::Server>();

  entity.add(client);
  entity.add(server);

  entity.set(1);
  entity.set(2);

  const std::vector<int> expected = {1, 2};

  EXPECT_EQ(client->values(), expected);
  EXPECT_EQ(server->values(), expected);
}

TEST(Example0513Test, SharedObserversCanOutliveEntity) {
  auto client = std::make_shared<example_0513::Client>();

  {
    example_0513::Entity entity;
    entity.add(client);
    entity.set(7);
  }

  const std::vector<int> expected = {7};
  EXPECT_EQ(client->values(), expected);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
