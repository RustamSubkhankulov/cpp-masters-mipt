#include <benchmark/benchmark.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

class Allocator
{
private:
    struct Node
    {
        std::size_t size = 0U;
        Node * next = nullptr;
    };

    struct alignas(std::max_align_t) Header
    {
        std::size_t size = 0U;
    };

    static_assert(sizeof(Node) >= sizeof(Header));
    static_assert(alignof(Node) <= alignof(std::max_align_t));

public:
    enum class SearchPolicy
    {
        FirstFit,
        BestFit
    };

    explicit Allocator(std::size_t size, SearchPolicy policy = SearchPolicy::FirstFit)
        : m_size(size),
          m_policy(policy)
    {
        if (m_size < sizeof(Node) + 1U)
        {
            throw std::invalid_argument("Allocator buffer is too small");
        }

        m_begin = static_cast<std::byte *>(::operator new(m_size, std::align_val_t(kAlignment)));
        m_head = as_node(m_begin);
        m_head->size = m_size - sizeof(Header);
        m_head->next = nullptr;
    }

    Allocator(const Allocator &) = delete;
    Allocator & operator=(const Allocator &) = delete;
    Allocator(Allocator &&) = delete;
    Allocator & operator=(Allocator &&) = delete;

    ~Allocator()
    {
        ::operator delete(m_begin, std::align_val_t(kAlignment));
    }

    [[nodiscard]] void * allocate(std::size_t requested_size)
    {
        if (requested_size == 0U)
        {
            return nullptr;
        }

        constexpr std::size_t kMaxRequest =
            std::numeric_limits<std::size_t>::max() - sizeof(Header) - (kAlignment - 1U);
        if (requested_size > kMaxRequest)
        {
            return nullptr;
        }

        std::size_t payload_size = adjusted_payload_size(requested_size);
        auto [current, previous] = find(payload_size);
        if (current == nullptr)
        {
            return nullptr;
        }

        Node * replacement = current->next;
        std::size_t stored_size = current->size;

        if (current->size >= payload_size + sizeof(Node) + 1U)
        {
            const std::size_t step = sizeof(Header) + payload_size;
            Node * tail = as_node(as_byte(current) + step);
            tail->size = current->size - step;
            tail->next = current->next;
            replacement = tail;
            stored_size = payload_size;
        }

        if (previous == nullptr)
        {
            m_head = replacement;
        }
        else
        {
            previous->next = replacement;
        }

        Header * header = as_header(current);
        header->size = stored_size;
        return as_byte(current) + sizeof(Header);
    }

    void deallocate(void * pointer)
    {
        if (pointer == nullptr)
        {
            return;
        }

        Node * node = as_node(as_byte(pointer) - sizeof(Header));
        Node * previous = nullptr;
        Node * current = m_head;

        while (current != nullptr && current < node)
        {
            previous = current;
            current = current->next;
        }

        node->next = current;

        if (previous == nullptr)
        {
            m_head = node;
        }
        else
        {
            previous->next = node;
        }

        merge(previous, node);
    }

private:
    static constexpr std::size_t kAlignment = alignof(std::max_align_t);

    [[nodiscard]] static std::byte * as_byte(void * pointer)
    {
        return static_cast<std::byte *>(pointer);
    }

    [[nodiscard]] static Node * as_node(void * pointer)
    {
        return static_cast<Node *>(pointer);
    }

    [[nodiscard]] static Header * as_header(void * pointer)
    {
        return static_cast<Header *>(pointer);
    }

    [[nodiscard]] static std::size_t alignment_padding(std::size_t size)
    {
        const std::size_t remainder = size % kAlignment;
        return remainder == 0U ? 0U : (kAlignment - remainder);
    }

    [[nodiscard]] static std::size_t adjusted_payload_size(std::size_t requested_size)
    {
        return requested_size + alignment_padding(sizeof(Header) + requested_size);
    }

    [[nodiscard]] std::pair<Node *, Node *> find(std::size_t size) const
    {
        return m_policy == SearchPolicy::FirstFit ? find_first(size) : find_best(size);
    }

    [[nodiscard]] std::pair<Node *, Node *> find_first(std::size_t size) const
    {
        Node * previous = nullptr;
        Node * current = m_head;

        while (current != nullptr && current->size < size)
        {
            previous = current;
            current = current->next;
        }

        return {current, previous};
    }

    [[nodiscard]] std::pair<Node *, Node *> find_best(std::size_t size) const
    {
        Node * best = nullptr;
        Node * best_previous = nullptr;
        Node * previous = nullptr;
        Node * current = m_head;

        while (current != nullptr)
        {
            if (current->size >= size)
            {
                if (best == nullptr || current->size < best->size)
                {
                    best = current;
                    best_previous = previous;
                    if (current->size == size)
                    {
                        break;
                    }
                }
            }

            previous = current;
            current = current->next;
        }

        return {best, best_previous};
    }

    void merge(Node * previous, Node * node)
    {
        if (node->next != nullptr &&
            as_byte(node) + sizeof(Header) + node->size == as_byte(node->next))
        {
            node->size += sizeof(Header) + node->next->size;
            node->next = node->next->next;
        }

        if (previous != nullptr &&
            as_byte(previous) + sizeof(Header) + previous->size == as_byte(node))
        {
            previous->size += sizeof(Header) + node->size;
            previous->next = node->next;
        }
    }

    std::size_t m_size = 0U;
    SearchPolicy m_policy = SearchPolicy::FirstFit;
    std::byte * m_begin = nullptr;
    Node * m_head = nullptr;
};

TEST(AllocatorTests, ReusesFreedBlock)
{
    Allocator allocator(1024U, Allocator::SearchPolicy::FirstFit);

    void * first = allocator.allocate(128U);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(first) % alignof(std::max_align_t), 0U);

    allocator.deallocate(first);

    void * second = allocator.allocate(128U);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second, first);
}

TEST(AllocatorTests, DemoSequenceMergesAdjacentBlocks)
{
    Allocator allocator(1024U, Allocator::SearchPolicy::FirstFit);

    void * a = allocator.allocate(16U);
    void * x = allocator.allocate(16U);
    void * y = allocator.allocate(16U);
    void * b = allocator.allocate(16U);

    ASSERT_NE(a, nullptr);
    ASSERT_NE(x, nullptr);
    ASSERT_NE(y, nullptr);
    ASSERT_NE(b, nullptr);

    allocator.deallocate(y);
    allocator.deallocate(x);

    void * z = allocator.allocate(32U);
    ASSERT_NE(z, nullptr);
    EXPECT_EQ(z, x);

    allocator.deallocate(z);
    allocator.deallocate(b);
    allocator.deallocate(a);
}

TEST(AllocatorTests, BestFitChoosesSmallerSuitableHole)
{
    struct ScenarioResult
    {
        void * chosen = nullptr;
        void * first_hole = nullptr;
        void * best_hole = nullptr;
    };

    auto run = [](Allocator & allocator)
    {
        void * a = allocator.allocate(64U);
        void * b = allocator.allocate(256U);
        void * c = allocator.allocate(64U);
        void * d = allocator.allocate(160U);
        void * e = allocator.allocate(64U);

        EXPECT_NE(a, nullptr);
        EXPECT_NE(b, nullptr);
        EXPECT_NE(c, nullptr);
        EXPECT_NE(d, nullptr);
        EXPECT_NE(e, nullptr);

        allocator.deallocate(b);
        allocator.deallocate(d);

        return ScenarioResult{allocator.allocate(128U), b, d};
    };

    Allocator first_fit(2048U, Allocator::SearchPolicy::FirstFit);
    Allocator best_fit(2048U, Allocator::SearchPolicy::BestFit);

    const ScenarioResult first_result = run(first_fit);
    const ScenarioResult best_result = run(best_fit);

    ASSERT_NE(first_result.chosen, nullptr);
    ASSERT_NE(best_result.chosen, nullptr);
    EXPECT_EQ(first_result.chosen, first_result.first_hole);
    EXPECT_EQ(best_result.chosen, best_result.best_hole);
    EXPECT_NE(first_result.chosen, best_result.chosen);
}

TEST(AllocatorTests, ReturnsNullWhenMemoryIsExhausted)
{
    Allocator allocator(128U, Allocator::SearchPolicy::BestFit);

    void * block = allocator.allocate(1024U);
    EXPECT_EQ(block, nullptr);
}

struct Workload
{
    std::vector<std::size_t> initial_sizes;
    std::vector<std::size_t> refill_sizes;
};

const Workload & get_workload()
{
    static const Workload workload = []
    {
        constexpr std::size_t kBlockCount = 4096U;
        Workload result;
        result.initial_sizes.resize(kBlockCount);
        result.refill_sizes.resize(kBlockCount);

        std::mt19937_64 engine(123456789ULL);
        std::uniform_int_distribution<std::size_t> distribution(4U, 256U);

        for (std::size_t index = 0U; index < kBlockCount; ++index)
        {
            result.initial_sizes[index] = distribution(engine) * 16U;
            result.refill_sizes[index] = distribution(engine) * 16U;
        }

        return result;
    }();

    return workload;
}

void run_allocator_benchmark(benchmark::State & state, Allocator::SearchPolicy policy)
{
    constexpr std::size_t kPoolSize = 32U * 1024U * 1024U;
    constexpr std::size_t kReleaseStride = 7U;
    const Workload & workload = get_workload();
    std::vector<void *> blocks(workload.initial_sizes.size(), nullptr);

    for (auto _ : state)
    {
        (void)_;
        std::fill(blocks.begin(), blocks.end(), nullptr);
        Allocator allocator(kPoolSize, policy);
        bool failed = false;

        for (std::size_t index = 0U; index < workload.initial_sizes.size(); ++index)
        {
            blocks[index] = allocator.allocate(workload.initial_sizes[index]);
            if (blocks[index] == nullptr)
            {
                state.SkipWithError("initial allocation failed");
                failed = true;
                break;
            }
        }

        if (failed)
        {
            continue;
        }

        for (std::size_t index = 0U; index < blocks.size(); index += kReleaseStride)
        {
            allocator.deallocate(blocks[index]);
            blocks[index] = nullptr;
        }

        for (std::size_t index = 0U; index < blocks.size(); index += kReleaseStride)
        {
            blocks[index] = allocator.allocate(workload.refill_sizes[index]);
            if (blocks[index] == nullptr)
            {
                state.SkipWithError("refill allocation failed");
                failed = true;
                break;
            }
        }

        if (failed)
        {
            continue;
        }

        for (void * block : blocks)
        {
            allocator.deallocate(block);
        }

        benchmark::DoNotOptimize(blocks.data());
        benchmark::ClobberMemory();
    }
}

static void BM_AllocatorFirstFit(benchmark::State & state)
{
    run_allocator_benchmark(state, Allocator::SearchPolicy::FirstFit);
}

static void BM_AllocatorBestFit(benchmark::State & state)
{
    run_allocator_benchmark(state, Allocator::SearchPolicy::BestFit);
}

BENCHMARK(BM_AllocatorFirstFit)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_AllocatorBestFit)->Unit(benchmark::kMicrosecond);

int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    const int test_result = RUN_ALL_TESTS();
    if (test_result != 0)
    {
        return test_result;
    }

    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    {
        return 1;
    }

    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
