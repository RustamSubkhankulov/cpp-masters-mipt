#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

struct alignas(std::max_align_t) Header
{
    std::size_t size = 0;
};

struct Node
{
    std::size_t size = 0;
    Node * next = nullptr;
};

class Allocator
{
public:
    Allocator() = default;
    Allocator(const Allocator &) = delete;
    Allocator & operator=(const Allocator &) = delete;
    Allocator(Allocator &&) = delete;
    Allocator & operator=(Allocator &&) = delete;

    virtual ~Allocator() = default;

    virtual void * allocate(
        std::size_t size,
        std::size_t alignment = kDefaultAlignment) = 0;

    virtual void deallocate(void * pointer) = 0;

protected:
    static constexpr std::size_t kDefaultAlignment = alignof(std::max_align_t);

    template <class T>
    static T * get(void * address) noexcept
    {
        static_assert(
            std::is_same_v<T, std::byte> ||
            std::is_same_v<T, Header> ||
            std::is_same_v<T, Node>);

        return static_cast<T *>(address);
    }

    template <class T>
    static const T * get(const void * address) noexcept
    {
        static_assert(
            std::is_same_v<T, std::byte> ||
            std::is_same_v<T, Header> ||
            std::is_same_v<T, Node>);

        return static_cast<const T *>(address);
    }

    static constexpr bool is_supported_alignment(std::size_t alignment) noexcept
    {
        return alignment != 0 &&
               (alignment & (alignment - 1)) == 0 &&
               alignment <= kDefaultAlignment;
    }

    static constexpr std::size_t align_up(
        std::size_t value,
        std::size_t alignment) noexcept
    {
        return ((value + alignment - 1) / alignment) * alignment;
    }
};

class LinearAllocator final : public Allocator
{
public:
    explicit LinearAllocator(std::size_t capacity)
        : m_capacity(capacity),
          m_memory(::operator new(m_capacity, std::align_val_t(kDefaultAlignment)))
    {
    }

    ~LinearAllocator() override
    {
        ::operator delete(m_memory, std::align_val_t(kDefaultAlignment));
    }

    void * allocate(
        std::size_t size,
        std::size_t alignment = kDefaultAlignment) override
    {
        if (size == 0 || !is_supported_alignment(alignment) || m_offset > m_capacity)
        {
            return nullptr;
        }

        void * current = get<std::byte>(m_memory) + m_offset;
        std::size_t space = m_capacity - m_offset;
        void * aligned = std::align(alignment, size, current, space);

        if (aligned == nullptr)
        {
            return nullptr;
        }

        m_offset = m_capacity - space + size;
        return aligned;
    }

    void deallocate(void * pointer) override
    {
        static_cast<void>(pointer);
    }

private:
    std::size_t m_capacity = 0;
    std::size_t m_offset = 0;
    void * m_memory = nullptr;
};

class StackAllocator final : public Allocator
{
public:
    explicit StackAllocator(std::size_t capacity)
        : m_capacity(capacity),
          m_memory(::operator new(m_capacity, std::align_val_t(kDefaultAlignment)))
    {
    }

    ~StackAllocator() override
    {
        ::operator delete(m_memory, std::align_val_t(kDefaultAlignment));
    }

    void * allocate(
        std::size_t size,
        std::size_t alignment = kDefaultAlignment) override
    {
        if (size == 0 ||
            !is_supported_alignment(alignment) ||
            m_offset + sizeof(Header) > m_capacity)
        {
            return nullptr;
        }

        const std::size_t actual_alignment =
            std::max(alignment, alignof(Header));

        void * current = get<std::byte>(m_memory) + m_offset + sizeof(Header);
        std::size_t space = m_capacity - m_offset - sizeof(Header);
        void * aligned = std::align(actual_alignment, size, current, space);

        if (aligned == nullptr)
        {
            return nullptr;
        }

        auto * header =
            get<Header>(get<std::byte>(aligned) - sizeof(Header));

        ::new (static_cast<void *>(header))
            Header{static_cast<std::size_t>(get<std::byte>(aligned) -
                                            (get<std::byte>(m_memory) + m_offset))};

        m_offset =
            static_cast<std::size_t>(get<std::byte>(aligned) -
                                     get<std::byte>(m_memory)) + size;

        return aligned;
    }

    void deallocate(void * pointer) override
    {
        if (pointer == nullptr)
        {
            return;
        }

        const auto * header =
            get<Header>(get<std::byte>(pointer) - sizeof(Header));

        m_offset =
            static_cast<std::size_t>(get<std::byte>(pointer) -
                                     get<std::byte>(m_memory)) - header->size;
    }

private:
    std::size_t m_capacity = 0;
    std::size_t m_offset = 0;
    void * m_memory = nullptr;
};

class ListAllocator final : public Allocator
{
public:
    ListAllocator(std::size_t page_size, std::size_t block_size)
        : m_block_size(align_up(
              std::max<std::size_t>(block_size, sizeof(Node)),
              kDefaultAlignment)),
          m_page_size(align_up(
              std::max<std::size_t>(page_size, m_block_size),
              m_block_size))
    {
    }

    ~ListAllocator() override
    {
        for (void * page : m_pages)
        {
            ::operator delete(page, std::align_val_t(kDefaultAlignment));
        }
    }

    void * allocate(
        std::size_t size,
        std::size_t alignment = kDefaultAlignment) override
    {
        if (size == 0 ||
            size > m_block_size ||
            !is_supported_alignment(alignment))
        {
            return nullptr;
        }

        if (m_free_list == nullptr)
        {
            add_page();
        }

        Node * node = m_free_list;
        m_free_list = m_free_list->next;
        return node;
    }

    void deallocate(void * pointer) override
    {
        if (pointer == nullptr)
        {
            return;
        }

        auto * node = ::new (pointer) Node{m_block_size, m_free_list};
        m_free_list = node;
    }

private:
    void add_page()
    {
        void * page =
            ::operator new(m_page_size, std::align_val_t(kDefaultAlignment));
        m_pages.push_back(page);

        auto * bytes = get<std::byte>(page);
        const std::size_t block_count = m_page_size / m_block_size;

        for (std::size_t index = 0; index < block_count; ++index)
        {
            void * block = bytes + index * m_block_size;
            auto * node = ::new (block) Node{m_block_size, m_free_list};
            m_free_list = node;
        }
    }

    std::size_t m_block_size = 0;
    std::size_t m_page_size = 0;
    Node * m_free_list = nullptr;
    std::vector<void *> m_pages;
};

class FreeListAllocator final : public Allocator
{
public:
    explicit FreeListAllocator(std::size_t capacity)
        : m_capacity(std::max(capacity, minimum_total_block_size())),
          m_memory(::operator new(m_capacity, std::align_val_t(kDefaultAlignment)))
    {
        m_free_list = ::new (m_memory) Node{m_capacity - sizeof(Header), nullptr};
    }

    ~FreeListAllocator() override
    {
        ::operator delete(m_memory, std::align_val_t(kDefaultAlignment));
    }

    void * allocate(
        std::size_t size,
        std::size_t alignment = kDefaultAlignment) override
    {
        if (size == 0 || !is_supported_alignment(alignment))
        {
            return nullptr;
        }

        const std::size_t payload_size =
            std::max(size, minimum_payload_size());

        Node * previous = nullptr;
        Node * current = m_free_list;

        while (current != nullptr && current->size < payload_size)
        {
            previous = current;
            current = current->next;
        }

        if (current == nullptr)
        {
            return nullptr;
        }

        if (current->size >= payload_size + minimum_total_block_size())
        {
            auto * next_block = get<std::byte>(current) +
                                sizeof(Header) + payload_size;

            auto * next_node = ::new (static_cast<void *>(next_block))
                Node{current->size - payload_size - sizeof(Header),
                     current->next};

            if (previous == nullptr)
            {
                m_free_list = next_node;
            }
            else
            {
                previous->next = next_node;
            }

            auto * header = ::new (static_cast<void *>(current))
                Header{payload_size};

            return get<std::byte>(header) + sizeof(Header);
        }

        if (previous == nullptr)
        {
            m_free_list = current->next;
        }
        else
        {
            previous->next = current->next;
        }

        auto * header = ::new (static_cast<void *>(current))
            Header{current->size};

        return get<std::byte>(header) + sizeof(Header);
    }

    void deallocate(void * pointer) override
    {
        if (pointer == nullptr)
        {
            return;
        }

        auto * header =
            get<Header>(get<std::byte>(pointer) - sizeof(Header));
        auto * node =
            ::new (static_cast<void *>(header)) Node{header->size, nullptr};

        Node * previous = nullptr;
        Node * current = m_free_list;

        while (current != nullptr &&
               get<std::byte>(current) < get<std::byte>(node))
        {
            previous = current;
            current = current->next;
        }

        node->next = current;

        if (previous == nullptr)
        {
            m_free_list = node;
        }
        else
        {
            previous->next = node;
        }

        merge_with_next(node);

        if (previous != nullptr)
        {
            merge_with_next(previous);
        }
    }

private:
    static constexpr std::size_t minimum_payload_size() noexcept
    {
        return sizeof(Node) > sizeof(Header)
                   ? sizeof(Node) - sizeof(Header)
                   : 1;
    }

    static constexpr std::size_t minimum_total_block_size() noexcept
    {
        return std::max<std::size_t>(sizeof(Node), sizeof(Header) + 1);
    }

    static void merge_with_next(Node * node) noexcept
    {
        if (node == nullptr || node->next == nullptr)
        {
            return;
        }

        auto * node_end =
            get<std::byte>(node) + sizeof(Header) + node->size;

        if (node_end == get<std::byte>(node->next))
        {
            node->size += sizeof(Header) + node->next->size;
            node->next = node->next->next;
        }
    }

    std::size_t m_capacity = 0;
    void * m_memory = nullptr;
    Node * m_free_list = nullptr;
};

static std::uintptr_t as_integer(void * pointer) noexcept
{
    return reinterpret_cast<std::uintptr_t>(pointer);
}

TEST(LinearAllocatorTest, AllocatesSequentiallyWithRequestedAlignment)
{
    LinearAllocator allocator(256);

    void * p1 = allocator.allocate(1, 1);
    void * p2 = allocator.allocate(2, 2);
    void * p3 = allocator.allocate(4, 4);
    void * p4 = allocator.allocate(8, 8);

    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    ASSERT_NE(p3, nullptr);
    ASSERT_NE(p4, nullptr);

    EXPECT_EQ(as_integer(p1) % 1U, 0U);
    EXPECT_EQ(as_integer(p2) % 2U, 0U);
    EXPECT_EQ(as_integer(p3) % 4U, 0U);
    EXPECT_EQ(as_integer(p4) % 8U, 0U);

    EXPECT_LT(as_integer(p1), as_integer(p2));
    EXPECT_LT(as_integer(p2), as_integer(p3));
    EXPECT_LT(as_integer(p3), as_integer(p4));

    allocator.deallocate(p4);
    EXPECT_EQ(allocator.allocate(1024), nullptr);
}

TEST(StackAllocatorTest, ReusesMemoryInLifoOrder)
{
    StackAllocator allocator(256);

    void * first = allocator.allocate(8, 8);
    void * second = allocator.allocate(16, 8);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    allocator.deallocate(second);
    allocator.deallocate(first);

    void * reused = allocator.allocate(8, 8);

    ASSERT_NE(reused, nullptr);
    EXPECT_EQ(reused, first);
}

TEST(ListAllocatorTest, ReusesReleasedFixedSizeBlock)
{
    ListAllocator allocator(64, 8);

    void * first = allocator.allocate(8);
    void * second = allocator.allocate(8);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    allocator.deallocate(second);

    void * reused = allocator.allocate(8);

    ASSERT_NE(reused, nullptr);
    EXPECT_EQ(reused, second);
}

TEST(FreeListAllocatorTest, MergesAdjacentFreeBlocks)
{
    FreeListAllocator allocator(256);

    void * first = allocator.allocate(16);
    void * second = allocator.allocate(16);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);

    allocator.deallocate(second);
    allocator.deallocate(first);

    void * merged = allocator.allocate(32);

    ASSERT_NE(merged, nullptr);
    EXPECT_EQ(merged, first);
}

TEST(AllocatorPolymorphismTest, WorksThroughBaseClassInterface)
{
    std::vector<std::unique_ptr<Allocator>> allocators;
    allocators.push_back(std::make_unique<LinearAllocator>(128));
    allocators.push_back(std::make_unique<StackAllocator>(128));
    allocators.push_back(std::make_unique<ListAllocator>(64, 8));
    allocators.push_back(std::make_unique<FreeListAllocator>(128));

    const std::vector<std::size_t> sizes{8, 8, 8, 8};

    for (std::size_t index = 0; index < allocators.size(); ++index)
    {
        void * pointer = allocators[index]->allocate(sizes[index], 8);
        ASSERT_NE(pointer, nullptr);
        allocators[index]->deallocate(pointer);
    }
}

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
