#include <array>
#include <bit>
#include <boost/noncopyable.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>

#include <gtest/gtest.h>

class Entity : private boost::noncopyable
{
public:
    Entity();

    Entity(Entity&& other) noexcept;

    ~Entity();

    auto& operator=(Entity&& other) noexcept;

    void test() const;

    class Implementation;

    Implementation* get() noexcept;
    Implementation const* get() const noexcept;

private:
    static constexpr std::size_t kStorageSize = 16U;

    alignas(std::max_align_t) std::array<std::byte, kStorageSize> m_storage{};
    bool m_has_value = false;

    void reset() noexcept;

    friend struct EntityInspector;
};

class Entity::Implementation
{
public:
    static constexpr std::uint32_t kCookie = 0x13579BDFU;

    Implementation() = default;
    Implementation(Implementation&&) noexcept = default;
    auto operator=(Implementation&&) noexcept -> Implementation& = default;
    ~Implementation() = default;

    void test() const
    {
        if (m_cookie != kCookie)
        {
            throw std::logic_error("invalid implementation state");
        }
    }

    void set_value(int value) noexcept
    {
        m_value = value;
    }

    int value() const noexcept
    {
        return m_value;
    }

private:
    int m_value = 0;
    std::uint32_t m_cookie = kCookie;
};

struct EntityInspector
{
    static bool has_value(Entity const& entity)
    {
        return entity.m_has_value;
    }

    static void* implementation_address(Entity& entity)
    {
        return static_cast<void*>(entity.get());
    }

    static void const* implementation_address(Entity const& entity)
    {
        return static_cast<void const*>(entity.get());
    }

    static void* storage_address(Entity& entity)
    {
        return static_cast<void*>(entity.m_storage.data());
    }

    static void const* storage_address(Entity const& entity)
    {
        return static_cast<void const*>(entity.m_storage.data());
    }

    static void set_value(Entity& entity, int value)
    {
        entity.get()->set_value(value);
    }

    static int value(Entity const& entity)
    {
        return entity.get()->value();
    }
};

Entity::Entity()
{
    static_assert(sizeof(Implementation) <= kStorageSize);
    static_assert(alignof(Implementation) <= alignof(std::max_align_t));

    ::new (static_cast<void*>(m_storage.data())) Implementation();
    m_has_value = true;
}

Entity::Entity(Entity&& other) noexcept
{
    if (other.m_has_value)
    {
        ::new (static_cast<void*>(m_storage.data())) Implementation(std::move(*other.get()));
        m_has_value = true;
        std::destroy_at(other.get());
        other.m_has_value = false;
    }
}

Entity::~Entity()
{
    reset();
}

auto& Entity::operator=(Entity&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    if (m_has_value && other.m_has_value)
    {
        *get() = std::move(*other.get());
        std::destroy_at(other.get());
        other.m_has_value = false;
        return *this;
    }

    if (!m_has_value && other.m_has_value)
    {
        ::new (static_cast<void*>(m_storage.data())) Implementation(std::move(*other.get()));
        m_has_value = true;
        std::destroy_at(other.get());
        other.m_has_value = false;
        return *this;
    }

    if (m_has_value && !other.m_has_value)
    {
        reset();
    }

    return *this;
}

void Entity::test() const
{
    get()->test();
}

Entity::Implementation* Entity::get() noexcept
{
    return std::launder(std::bit_cast<Implementation*>(m_storage.data()));
}

Entity::Implementation const* Entity::get() const noexcept
{
    return std::launder(std::bit_cast<Implementation const*>(m_storage.data()));
}

void Entity::reset() noexcept
{
    if (m_has_value)
    {
        std::destroy_at(get());
        m_has_value = false;
    }
}

TEST(EntityTest, ConstructsImplementationInsideReservedStorage)
{
    Entity entity;

    EXPECT_TRUE(EntityInspector::has_value(entity));
    EXPECT_EQ(EntityInspector::implementation_address(entity), EntityInspector::storage_address(entity));
    EXPECT_NO_THROW(entity.test());
}

TEST(EntityTest, MoveConstructorTransfersState)
{
    Entity source;
    EntityInspector::set_value(source, 73);

    Entity target(std::move(source));

    EXPECT_FALSE(EntityInspector::has_value(source));
    EXPECT_TRUE(EntityInspector::has_value(target));
    EXPECT_EQ(EntityInspector::value(target), 73);
    EXPECT_EQ(EntityInspector::implementation_address(target), EntityInspector::storage_address(target));
    EXPECT_NO_THROW(target.test());
}

TEST(EntityTest, MoveAssignmentReusesExistingStorage)
{
    Entity source;
    Entity target;

    EntityInspector::set_value(source, 91);

    void const* const target_address_before = EntityInspector::implementation_address(target);

    target = std::move(source);

    EXPECT_FALSE(EntityInspector::has_value(source));
    EXPECT_TRUE(EntityInspector::has_value(target));
    EXPECT_EQ(EntityInspector::value(target), 91);
    EXPECT_EQ(EntityInspector::implementation_address(target), target_address_before);
    EXPECT_EQ(EntityInspector::implementation_address(target), EntityInspector::storage_address(target));
    EXPECT_NO_THROW(target.test());
}

TEST(EntityTest, MoveAssignmentFromSelfReferenceKeepsObjectValid)
{
    Entity entity;
    EntityInspector::set_value(entity, 17);

    void const* const address_before = EntityInspector::implementation_address(entity);

    Entity& reference = entity;
    entity = std::move(reference);

    EXPECT_TRUE(EntityInspector::has_value(entity));
    EXPECT_EQ(EntityInspector::value(entity), 17);
    EXPECT_EQ(EntityInspector::implementation_address(entity), address_before);
    EXPECT_NO_THROW(entity.test());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
