#include <gtest/gtest.h>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <numeric>
#include <vector>

namespace {

struct CapacityChange {
    std::size_t size_before;
    std::size_t size_after;
    std::size_t capacity_before;
    std::size_t capacity_after;
};

struct Rational {
    std::size_t numerator;
    std::size_t denominator;

    auto operator<=>(const Rational&) const = default;
};

Rational ReduceRatio(const std::size_t numerator, const std::size_t denominator) {
    const std::size_t divisor = std::gcd(numerator, denominator);
    return {numerator / divisor, denominator / divisor};
}

std::vector<CapacityChange> TrackVectorCapacityChanges(const std::size_t required_change_count) {
    std::vector<int> values;
    std::vector<CapacityChange> changes;
    changes.reserve(required_change_count);

    for (int value = 0; changes.size() < required_change_count; ++value) {
        const std::size_t size_before = values.size();
        const std::size_t capacity_before = values.capacity();

        values.push_back(value);

        const std::size_t capacity_after = values.capacity();
        if (capacity_after != capacity_before) {
            changes.push_back({
                size_before,
                values.size(),
                capacity_before,
                capacity_after,
            });
        }
    }

    return changes;
}

std::vector<Rational> BuildVectorGrowthRatios(const std::vector<CapacityChange>& changes) {
    std::vector<Rational> ratios;
    ratios.reserve(changes.size());

    for (const CapacityChange& change : changes) {
        if (change.capacity_before != 0U) {
            ratios.push_back(ReduceRatio(change.capacity_after, change.capacity_before));
        }
    }

    return ratios;
}

Rational FindMostFrequentRatio(const std::vector<Rational>& ratios, const std::size_t start_index) {
    std::map<Rational, std::size_t> frequencies;
    Rational best_ratio{0U, 1U};
    std::size_t best_count = 0U;

    for (std::size_t index = start_index; index < ratios.size(); ++index) {
        const std::size_t count = ++frequencies[ratios[index]];
        if (count > best_count) {
            best_ratio = ratios[index];
            best_count = count;
        }
    }

    return best_ratio;
}

Rational DetectVectorGrowthFactor(const std::size_t required_change_count) {
    const std::vector<CapacityChange> changes = TrackVectorCapacityChanges(required_change_count);
    const std::vector<Rational> ratios = BuildVectorGrowthRatios(changes);
    const std::size_t warmup_count = std::min<std::size_t>(3U, ratios.size() / 2U);
    return FindMostFrequentRatio(ratios, warmup_count);
}

template <class T>
const void* TypeTag() {
    static const int tag = 0;
    return &tag;
}

struct AllocationRecord {
    const void* type_tag;
    std::size_t element_size;
    std::size_t element_count;
};

class AllocationRegistry {
public:
    static void Clear() {
        Records().clear();
    }

    static void Add(const AllocationRecord record) {
        Records().push_back(record);
    }

    static const std::vector<AllocationRecord>& Get() {
        return Records();
    }

private:
    static std::vector<AllocationRecord>& Records() {
        static std::vector<AllocationRecord> records;
        return records;
    }
};

template <class T>
class TrackingAllocator {
public:
    using value_type = T;

    TrackingAllocator() noexcept = default;

    template <class U>
    TrackingAllocator(const TrackingAllocator<U>&) noexcept {
    }

    [[nodiscard]] T* allocate(const std::size_t count) {
        AllocationRegistry::Add({TypeTag<T>(), sizeof(T), count});
        return std::allocator<T>{}.allocate(count);
    }

    void deallocate(T* const pointer, const std::size_t count) noexcept {
        std::allocator<T>{}.deallocate(pointer, count);
    }

    template <class U>
    bool operator==(const TrackingAllocator<U>&) const noexcept {
        return true;
    }

    template <class U>
    bool operator!=(const TrackingAllocator<U>&) const noexcept {
        return false;
    }
};

template <class T>
std::size_t FindMostFrequentElementAllocationCount(
    const std::vector<AllocationRecord>& records) {
    std::map<std::size_t, std::size_t> frequencies;
    std::size_t best_count = 0U;
    std::size_t best_allocation = 0U;

    for (const AllocationRecord& record : records) {
        if (record.type_tag != TypeTag<T>()) {
            continue;
        }
        if (record.element_count <= 1U) {
            continue;
        }

        const std::size_t count = ++frequencies[record.element_count];
        if (count > best_count || (count == best_count && record.element_count > best_allocation)) {
            best_count = count;
            best_allocation = record.element_count;
        }
    }

    return best_allocation;
}

template <class T>
std::vector<AllocationRecord> TrackDequeAllocations(const std::size_t element_count) {
    AllocationRegistry::Clear();

    std::deque<T, TrackingAllocator<T>> values;
    for (std::size_t index = 0; index < element_count; ++index) {
        values.push_back(static_cast<T>(index));
    }

    return AllocationRegistry::Get();
}

template <class T>
std::size_t DetectDequePageSizeInElements(const std::size_t element_count) {
    const std::vector<AllocationRecord> records = TrackDequeAllocations<T>(element_count);
    return FindMostFrequentElementAllocationCount<T>(records);
}

template <class T>
std::size_t DetectDequePageSizeInBytes(const std::size_t element_count) {
    return DetectDequePageSizeInElements<T>(element_count) * sizeof(T);
}

template <class T>
std::uintptr_t ToAddress(const T& value) {
    return reinterpret_cast<std::uintptr_t>(&value);
}

template <class T>
std::vector<std::uintptr_t> TrackDequeBackAddresses(const std::size_t element_count) {
    std::deque<T> values;
    std::vector<std::uintptr_t> addresses;
    addresses.reserve(element_count);

    for (std::size_t index = 0; index < element_count; ++index) {
        values.push_back(static_cast<T>(index));
        addresses.push_back(ToAddress(values.back()));
    }

    return addresses;
}

template <class T>
std::vector<std::size_t> BuildContiguousRunLengths(const std::vector<std::uintptr_t>& addresses) {
    std::vector<std::size_t> run_lengths;
    if (addresses.empty()) {
        return run_lengths;
    }

    std::size_t current_run_length = 1U;
    for (std::size_t index = 1; index < addresses.size(); ++index) {
        const std::uintptr_t expected_next_address = addresses[index - 1U] + sizeof(T);
        if (addresses[index] == expected_next_address) {
            ++current_run_length;
        } else {
            run_lengths.push_back(current_run_length);
            current_run_length = 1U;
        }
    }

    run_lengths.push_back(current_run_length);
    return run_lengths;
}

template <class T>
void ExpectStableDequeAddressesAfterPushBack(
    const std::size_t prefix_size,
    const std::size_t extra_insertions) {
    std::deque<T> values;
    std::vector<std::uintptr_t> saved_addresses;
    saved_addresses.reserve(prefix_size);

    for (std::size_t index = 0; index < prefix_size; ++index) {
        values.push_back(static_cast<T>(index));
        saved_addresses.push_back(ToAddress(values[index]));
    }

    for (std::size_t index = 0; index < extra_insertions; ++index) {
        values.push_back(static_cast<T>(prefix_size + index));
    }

    for (std::size_t index = 0; index < prefix_size; ++index) {
        EXPECT_EQ(ToAddress(values[index]), saved_addresses[index]);
    }
}

constexpr std::size_t kTrackedVectorCapacityChanges = 16U;
constexpr std::size_t kTrackedDequeElements = 6000U;
constexpr std::size_t kTrackedDequePrefix = 256U;
using DequeValue = std::uint64_t;

}  // namespace

TEST(VectorCapacityInvestigation, CapacityChangesAreObservedOnlyWhenStorageIsFull) {
    const std::vector<CapacityChange> changes =
        TrackVectorCapacityChanges(kTrackedVectorCapacityChanges);

    ASSERT_GE(changes.size(), kTrackedVectorCapacityChanges);
    ASSERT_FALSE(changes.empty());

    EXPECT_EQ(changes.front().size_before, 0U);
    EXPECT_EQ(changes.front().capacity_before, 0U);

    for (std::size_t index = 0; index < changes.size(); ++index) {
        const CapacityChange& change = changes[index];

        EXPECT_EQ(change.size_before, change.capacity_before);
        EXPECT_EQ(change.size_after, change.size_before + 1U);
        EXPECT_GT(change.capacity_after, change.capacity_before);

        if (index != 0U) {
            EXPECT_EQ(change.capacity_before, changes[index - 1U].capacity_after);
        }
    }
}

TEST(VectorCapacityInvestigation, StableGrowthFactorIsDerivedFromTheObservedCapacities) {
    const std::vector<CapacityChange> changes =
        TrackVectorCapacityChanges(kTrackedVectorCapacityChanges);
    const std::vector<Rational> ratios = BuildVectorGrowthRatios(changes);
    const std::size_t warmup_count = std::min<std::size_t>(3U, ratios.size() / 2U);
    const Rational growth_factor = DetectVectorGrowthFactor(kTrackedVectorCapacityChanges);

    ASSERT_GE(ratios.size(), 8U);
    EXPECT_GT(growth_factor.numerator, growth_factor.denominator);

    for (std::size_t index = warmup_count; index < ratios.size(); ++index) {
        EXPECT_EQ(ratios[index], growth_factor);
    }
}

TEST(DequePageInvestigation, RepeatedElementAllocationsRevealPageSize) {
    const std::vector<AllocationRecord> records = TrackDequeAllocations<DequeValue>(kTrackedDequeElements);
    const std::size_t page_size_in_elements =
        FindMostFrequentElementAllocationCount<DequeValue>(records);
    const std::size_t page_size_in_bytes = DetectDequePageSizeInBytes<DequeValue>(kTrackedDequeElements);

    EXPECT_GT(page_size_in_elements, 1U);
    EXPECT_GT(page_size_in_bytes, sizeof(DequeValue));

    std::size_t page_allocation_count = 0U;
    for (const AllocationRecord& record : records) {
        if (record.type_tag == TypeTag<DequeValue>() &&
            record.element_count == page_size_in_elements) {
            ++page_allocation_count;
        }
    }

    EXPECT_GE(page_allocation_count, 2U);
}

TEST(DequePageInvestigation, RecordedAddressesContainAContiguousSegmentAtLeastOnePageLong) {
    const std::size_t page_size_in_elements =
        DetectDequePageSizeInElements<DequeValue>(kTrackedDequeElements);
    const std::vector<std::uintptr_t> addresses =
        TrackDequeBackAddresses<DequeValue>(kTrackedDequeElements);
    const std::vector<std::size_t> run_lengths =
        BuildContiguousRunLengths<DequeValue>(addresses);

    ASSERT_FALSE(run_lengths.empty());
    EXPECT_GT(page_size_in_elements, 1U);

    const std::size_t longest_run =
        *std::max_element(run_lengths.begin(), run_lengths.end());
    EXPECT_GE(longest_run, page_size_in_elements);
}

TEST(DequePageInvestigation, ExistingElementAddressesRemainStableAfterFurtherInsertions) {
    ExpectStableDequeAddressesAfterPushBack<DequeValue>(
        kTrackedDequePrefix,
        kTrackedDequeElements);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
