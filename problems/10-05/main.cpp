// Suggested build flags: -O3 -m32

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

/*
Analysis (see 'has_research' dir for output files):

The obtained dependencies have the expected shape: the number of collisions
increases monotonically as the number of hashed strings grows. This is explained
by the fact that the number of buckets is fixed at 65521, while the number of
input strings increases up to 100000. At the beginning of the experiment, many
buckets are still empty, so the probability of a collision is relatively low and
the curves grow more slowly. As the hash table becomes more filled, new strings
are more likely to be mapped to buckets that are already occupied, so the number
of collisions starts growing faster. For hash functions with distribution close
to random, the growth is close to quadratic at small loads and then becomes almost
linear when the table is significantly filled. If a hash function produces stronger
clustering, its curve becomes steeper because similar input strings are sent to
the same buckets more often.

According to the final results, the best hash function in this set is DJBHash,
which produced 40729 collisions. The next best results were shown by RSHash with
42366 collisions and SDBMHash with 43331 collisions. These functions therefore
distributed the decimal strings from 0 to 99999 more uniformly than the others.
The summary file also gives the expected number of collisions for a near-random
distribution as 48720.1, so DJBHash performed even better than this reference
value on the tested dataset.

The worst hash function in the tested set is DEKHash, which produced 79230 collisions.
Two other clearly weak functions are ELFHash and PJWHash, each with 73023 collisions.
These values are much higher than the reference level of 48720.1, which means that on
this dataset these functions distributed values across buckets much less uniformly
and caused much stronger clustering.

The full ranking from best to worst is exactly the following: DJBHash — 40729,
RSHash — 42366, SDBMHash — 43331, BKDRHash — 47345, JSHash — 47997, APHash — 48725,
ELFHash — 73023, PJWHash — 73023, DEKHash — 79230. The graph was built for all nine
functions from the dataset.
*/

namespace hash_research
{

using HashValue = std::uint32_t;
using HashFunction = HashValue (*)(std::string_view);

struct HashDescriptor
{
    std::string_view name;
    HashFunction function;
};

HashValue RSHash(std::string_view text)
{
    HashValue factor = 63689U;
    constexpr HashValue multiplier = 378551U;
    HashValue hash = 0U;

    for (unsigned char character : text)
    {
        hash = hash * factor + static_cast<HashValue>(character);
        factor *= multiplier;
    }

    return hash;
}

HashValue JSHash(std::string_view text)
{
    HashValue hash = 1315423911U;

    for (unsigned char character : text)
    {
        hash ^= (hash << 5U) + static_cast<HashValue>(character) + (hash >> 2U);
    }

    return hash;
}

HashValue PJWHash(std::string_view text)
{
    constexpr std::size_t kBitsInUnsignedInt = std::numeric_limits<HashValue>::digits;
    constexpr std::size_t kThreeQuarters = (kBitsInUnsignedInt * 3U) / 4U;
    constexpr std::size_t kOneEighth = kBitsInUnsignedInt / 8U;
    constexpr HashValue kHighBits = 0xFFFFFFFFU << (kBitsInUnsignedInt - kOneEighth);

    HashValue hash = 0U;

    for (unsigned char character : text)
    {
        hash = (hash << kOneEighth) + static_cast<HashValue>(character);
        const HashValue high = hash & kHighBits;

        if (high != 0U)
        {
            hash = (hash ^ (high >> kThreeQuarters)) & ~kHighBits;
        }
    }

    return hash;
}

HashValue ELFHash(std::string_view text)
{
    HashValue hash = 0U;

    for (unsigned char character : text)
    {
        hash = (hash << 4U) + static_cast<HashValue>(character);
        const HashValue high = hash & 0xF0000000U;

        if (high != 0U)
        {
            hash ^= high >> 24U;
        }

        hash &= ~high;
    }

    return hash;
}

HashValue BKDRHash(std::string_view text)
{
    constexpr HashValue kSeed = 131U;
    HashValue hash = 0U;

    for (unsigned char character : text)
    {
        hash = hash * kSeed + static_cast<HashValue>(character);
    }

    return hash;
}

HashValue SDBMHash(std::string_view text)
{
    HashValue hash = 0U;

    for (unsigned char character : text)
    {
        hash = static_cast<HashValue>(character) + (hash << 6U) + (hash << 16U) - hash;
    }

    return hash;
}

HashValue DJBHash(std::string_view text)
{
    HashValue hash = 5381U;

    for (unsigned char character : text)
    {
        hash = ((hash << 5U) + hash) + static_cast<HashValue>(character);
    }

    return hash;
}

HashValue DEKHash(std::string_view text)
{
    HashValue hash = static_cast<HashValue>(text.size());

    for (unsigned char character : text)
    {
        hash = ((hash << 5U) ^ (hash >> 27U)) ^ static_cast<HashValue>(character);
    }

    return hash;
}

HashValue APHash(std::string_view text)
{
    HashValue hash = 0xAAAAAAAAU;

    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const HashValue character = static_cast<HashValue>(static_cast<unsigned char>(text[index]));

        if ((index & 1U) == 0U)
        {
            hash ^= (hash << 7U) ^ (character * (hash >> 3U));
        }
        else
        {
            hash ^= ~((hash << 11U) + (character ^ (hash >> 5U)));
        }
    }

    return hash;
}

constexpr std::array<HashDescriptor, 9> kHashCatalog = {{
    {"RSHash", &RSHash},
    {"JSHash", &JSHash},
    {"PJWHash", &PJWHash},
    {"ELFHash", &ELFHash},
    {"BKDRHash", &BKDRHash},
    {"SDBMHash", &SDBMHash},
    {"DJBHash", &DJBHash},
    {"DEKHash", &DEKHash},
    {"APHash", &APHash},
}};

struct CollisionPoint
{
    std::size_t string_count = 0U;
    std::array<std::size_t, kHashCatalog.size()> collisions = {};
};

struct RankingEntry
{
    std::string_view name;
    std::size_t collisions = 0U;
};

std::array<HashValue, kHashCatalog.size()> EvaluateAll(std::string_view text)
{
    std::array<HashValue, kHashCatalog.size()> values = {};

    for (std::size_t index = 0; index < kHashCatalog.size(); ++index)
    {
        values[index] = kHashCatalog[index].function(text);
    }

    return values;
}

std::vector<std::string> MakeSequentialDecimalStrings(std::size_t count)
{
    std::vector<std::string> strings;
    strings.reserve(count);

    for (std::size_t index = 0; index < count; ++index)
    {
        strings.emplace_back(std::to_string(index));
    }

    return strings;
}

std::vector<CollisionPoint> MeasureCollisionCurves(
    const std::vector<std::string>& strings,
    std::size_t bucket_count,
    std::size_t sample_step)
{
    if (bucket_count == 0U)
    {
        throw std::invalid_argument("bucket_count must be positive");
    }

    if (sample_step == 0U)
    {
        throw std::invalid_argument("sample_step must be positive");
    }

    std::array<std::vector<unsigned char>, kHashCatalog.size()> occupied = {};

    for (auto& table : occupied)
    {
        table.assign(bucket_count, static_cast<unsigned char>(0));
    }

    std::array<std::size_t, kHashCatalog.size()> collision_counts = {};
    std::vector<CollisionPoint> points;
    points.reserve((strings.size() + sample_step - 1U) / sample_step);

    for (std::size_t string_index = 0; string_index < strings.size(); ++string_index)
    {
        const std::string_view text = strings[string_index];

        for (std::size_t hash_index = 0; hash_index < kHashCatalog.size(); ++hash_index)
        {
            const std::size_t bucket =
                static_cast<std::size_t>(kHashCatalog[hash_index].function(text) % bucket_count);

            if (occupied[hash_index][bucket] != 0U)
            {
                ++collision_counts[hash_index];
            }
            else
            {
                occupied[hash_index][bucket] = static_cast<unsigned char>(1);
            }
        }

        const std::size_t processed = string_index + 1U;

        if ((processed % sample_step) == 0U || processed == strings.size())
        {
            points.push_back(CollisionPoint{processed, collision_counts});
        }
    }

    return points;
}

std::vector<RankingEntry> RankByFinalCollisions(const std::vector<CollisionPoint>& points)
{
    if (points.empty())
    {
        return {};
    }

    std::vector<RankingEntry> ranking;
    ranking.reserve(kHashCatalog.size());

    for (std::size_t index = 0; index < kHashCatalog.size(); ++index)
    {
        ranking.push_back(RankingEntry{kHashCatalog[index].name, points.back().collisions[index]});
    }

    std::sort(
        ranking.begin(),
        ranking.end(),
        [](const RankingEntry& left, const RankingEntry& right)
        {
            if (left.collisions != right.collisions)
            {
                return left.collisions < right.collisions;
            }

            return left.name < right.name;
        });

    return ranking;
}

double ExpectedRandomCollisions(std::size_t string_count, std::size_t bucket_count)
{
    const double bucket_count_as_double = static_cast<double>(bucket_count);
    const double string_count_as_double = static_cast<double>(string_count);
    const double one_minus_inverse_bucket_count = 1.0 - 1.0 / bucket_count_as_double;
    const double occupied_expectation =
        bucket_count_as_double * (1.0 - std::pow(one_minus_inverse_bucket_count, string_count_as_double));

    return string_count_as_double - occupied_expectation;
}

void WriteCollisionCsv(const std::filesystem::path& file_path, const std::vector<CollisionPoint>& points)
{
    std::ofstream output(file_path);

    if (!output)
    {
        throw std::runtime_error("cannot open csv file");
    }

    output << "string_count";

    for (const HashDescriptor& descriptor : kHashCatalog)
    {
        output << ',' << descriptor.name;
    }

    output << '\n';

    for (const CollisionPoint& point : points)
    {
        output << point.string_count;

        for (std::size_t value : point.collisions)
        {
            output << ',' << value;
        }

        output << '\n';
    }
}

void WriteSummary(
    const std::filesystem::path& file_path,
    const std::vector<CollisionPoint>& points,
    std::size_t bucket_count)
{
    if (points.empty())
    {
        throw std::invalid_argument("points must not be empty");
    }

    const std::vector<RankingEntry> ranking = RankByFinalCollisions(points);
    const std::size_t final_string_count = points.back().string_count;
    const double expected_random_collisions = ExpectedRandomCollisions(final_string_count, bucket_count);

    std::ofstream output(file_path);

    if (!output)
    {
        throw std::runtime_error("cannot open summary file");
    }

    output << "Dataset: decimal strings from 0 to N - 1\n";
    output << "Buckets: " << bucket_count << '\n';
    output << "Final string count: " << final_string_count << '\n';
    output << "Expected random collisions at final point: " << expected_random_collisions << "\n\n";
    output << "Best function: " << ranking.front().name << " with " << ranking.front().collisions
           << " collisions\n";
    output << "Worst function: " << ranking.back().name << " with " << ranking.back().collisions
           << " collisions\n\n";

    output << "Ranking:\n";

    for (const RankingEntry& entry : ranking)
    {
        output << entry.name << ',' << entry.collisions << '\n';
    }
}

void WriteGnuplotScript(
    const std::filesystem::path& file_path,
    const std::filesystem::path& csv_file_path,
    const std::filesystem::path& image_file_path)
{
    std::ofstream output(file_path);

    if (!output)
    {
        throw std::runtime_error("cannot open gnuplot file");
    }

    output << "set terminal pngcairo size 1600,900\n";
    output << "set output '" << image_file_path.filename().string() << "'\n";
    output << "set datafile separator ','\n";
    output << "set title 'Hash collision curves for decimal strings'\n";
    output << "set xlabel 'Number of hashed strings'\n";
    output << "set ylabel 'Collisions'\n";
    output << "set key outside\n";
    output << "plot \\\n";

    for (std::size_t index = 0; index < kHashCatalog.size(); ++index)
    {
        output << "    '" << csv_file_path.filename().string() << "' using 1:" << (index + 2U)
               << " with lines lw 2 title '" << kHashCatalog[index].name << "'";

        if (index + 1U != kHashCatalog.size())
        {
            output << ", \\\n";
        }
        else
        {
            output << '\n';
        }
    }
}

struct ResearchArtifacts
{
    std::filesystem::path csv_file_path;
    std::filesystem::path summary_file_path;
    std::filesystem::path gnuplot_file_path;
    std::filesystem::path image_file_path;
    std::vector<CollisionPoint> points;
    std::vector<RankingEntry> ranking;
};

ResearchArtifacts RunResearch(
    const std::filesystem::path& output_directory,
    std::size_t string_count,
    std::size_t bucket_count,
    std::size_t sample_step)
{
    std::filesystem::create_directories(output_directory);

    const std::vector<std::string> strings = MakeSequentialDecimalStrings(string_count);
    std::vector<CollisionPoint> points = MeasureCollisionCurves(strings, bucket_count, sample_step);
    std::vector<RankingEntry> ranking = RankByFinalCollisions(points);

    const std::filesystem::path csv_file_path = output_directory / "hash_collisions_decimal.csv";
    const std::filesystem::path summary_file_path = output_directory / "hash_summary_decimal.txt";
    const std::filesystem::path gnuplot_file_path = output_directory / "plot_hash_collisions_decimal.gnuplot";
    const std::filesystem::path image_file_path = output_directory / "hash_collisions_decimal.png";

    WriteCollisionCsv(csv_file_path, points);
    WriteSummary(summary_file_path, points, bucket_count);
    WriteGnuplotScript(gnuplot_file_path, csv_file_path, image_file_path);

    return ResearchArtifacts{
        csv_file_path,
        summary_file_path,
        gnuplot_file_path,
        image_file_path,
        std::move(points),
        std::move(ranking),
    };
}

}

TEST(HashResearchTest, MiniDecimalDatasetCollisionCountsAreStable)
{
    constexpr std::size_t kBucketCount = 23U;
    constexpr std::size_t kStringCount = 20U;

    const std::vector<std::string> strings = hash_research::MakeSequentialDecimalStrings(kStringCount);
    const std::vector<hash_research::CollisionPoint> points =
        hash_research::MeasureCollisionCurves(strings, kBucketCount, kStringCount);

    ASSERT_EQ(points.size(), 1U);

    constexpr std::array<std::size_t, hash_research::kHashCatalog.size()> expected = {
        9U,
        6U,
        8U,
        8U,
        8U,
        1U,
        1U,
        6U,
        4U,
    };

    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_EQ(points.front().collisions[index], expected[index]);
    }
}

TEST(HashResearchTest, DefaultResearchIdentifiesBestAndWorstFunctions)
{
    constexpr std::size_t kStringCount = 100000U;
    constexpr std::size_t kBucketCount = 65521U;
    constexpr std::size_t kSampleStep = 1000U;

    const auto artifacts = hash_research::RunResearch(
        std::filesystem::current_path() / "hash_research_output",
        kStringCount,
        kBucketCount,
        kSampleStep);

    ASSERT_FALSE(artifacts.ranking.empty());
    EXPECT_EQ(artifacts.ranking.front().name, "DJBHash");
    EXPECT_EQ(artifacts.ranking.back().name, "DEKHash");
    EXPECT_TRUE(std::filesystem::exists(artifacts.csv_file_path));
    EXPECT_TRUE(std::filesystem::exists(artifacts.summary_file_path));
    EXPECT_TRUE(std::filesystem::exists(artifacts.gnuplot_file_path));
    EXPECT_FALSE(artifacts.points.empty());
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
