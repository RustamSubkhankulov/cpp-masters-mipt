#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dawkins {

constexpr char kFirstLowercaseLetter = 'a';
constexpr char kLastLowercaseLetter = 'z';
constexpr std::size_t kBookPopulationSize = 100;
constexpr double kBookMutationProbability = 0.05;
constexpr std::size_t kDefaultMaxGenerations = 1000;

struct Settings {
  std::string target;
  std::size_t population_size = kBookPopulationSize;
  double mutation_probability = kBookMutationProbability;
  std::size_t max_generations = kDefaultMaxGenerations;
};

struct RunResult {
  std::string initial_string;
  std::string best_string;
  std::size_t initial_distance = 0;
  std::size_t best_distance = 0;
  std::size_t generations = 0;
  bool reached_target = false;
};

void ValidateSettings(const Settings& settings) {
  if (settings.target.empty()) {
    throw std::invalid_argument("target must not be empty");
  }
  if (settings.population_size == 0) {
    throw std::invalid_argument("population_size must be positive");
  }
  if (settings.mutation_probability < 0.0 ||
      settings.mutation_probability > 1.0) {
    throw std::invalid_argument("mutation_probability must be in [0, 1]");
  }
}

std::size_t HammingDistance(const std::string& lhs, const std::string& rhs) {
  if (lhs.size() != rhs.size()) {
    throw std::invalid_argument("strings must have equal length");
  }

  std::size_t distance = 0;
  for (std::size_t index = 0; index < lhs.size(); ++index) {
    if (lhs[index] != rhs[index]) {
      ++distance;
    }
  }
  return distance;
}

bool IsLowercaseEnglishString(const std::string& value) {
  for (char symbol : value) {
    if (symbol < kFirstLowercaseLetter || symbol > kLastLowercaseLetter) {
      return false;
    }
  }
  return true;
}

char GenerateRandomLowercaseLetter(
  std::default_random_engine& engine,
  std::uniform_int_distribution<int>& letter_distribution) {
  return static_cast<char>(letter_distribution(engine));
}

char GenerateRandomDifferentLowercaseLetter(
  char current, std::default_random_engine& engine,
  std::uniform_int_distribution<int>& letter_distribution) {
  char candidate = current;
  while (candidate == current) {
    candidate = GenerateRandomLowercaseLetter(engine, letter_distribution);
  }
  return candidate;
}

std::string GenerateRandomString(std::size_t length,
                                 std::default_random_engine& engine) {
  std::uniform_int_distribution<int> letter_distribution(
    static_cast<int>(kFirstLowercaseLetter),
    static_cast<int>(kLastLowercaseLetter));

  std::string value(length, kFirstLowercaseLetter);
  for (char& symbol : value) {
    symbol = GenerateRandomLowercaseLetter(engine, letter_distribution);
  }
  return value;
}

std::string MutateString(const std::string& parent, double mutation_probability,
                         std::default_random_engine& engine) {
  std::uniform_int_distribution<int> letter_distribution(
    static_cast<int>(kFirstLowercaseLetter),
    static_cast<int>(kLastLowercaseLetter));
  std::uniform_real_distribution<double> mutation_distribution(0.0, 1.0);

  std::string child = parent;
  for (char& symbol : child) {
    if (mutation_distribution(engine) < mutation_probability) {
      symbol = GenerateRandomDifferentLowercaseLetter(symbol, engine,
                                                      letter_distribution);
    }
  }
  return child;
}

std::vector<std::string>
GeneratePopulation(const std::string& parent, const Settings& settings,
                   std::default_random_engine& engine) {
  std::vector<std::string> population;
  population.reserve(settings.population_size);

  for (std::size_t index = 0; index < settings.population_size; ++index) {
    population.push_back(
      MutateString(parent, settings.mutation_probability, engine));
  }
  return population;
}

std::pair<std::string, std::size_t>
SelectBestCandidate(const std::vector<std::string>& population,
                    const std::string& target) {
  const std::string* best_candidate = nullptr;
  std::size_t best_distance = target.size() + 1;

  for (const std::string& candidate : population) {
    const std::size_t candidate_distance = HammingDistance(candidate, target);
    if (candidate_distance < best_distance) {
      best_distance = candidate_distance;
      best_candidate = &candidate;
    }
  }

  if (best_candidate == nullptr) {
    throw std::logic_error("population must not be empty");
  }

  return {*best_candidate, best_distance};
}

void PrintGeneration(std::size_t generation, const std::string& candidate,
                     std::size_t distance, std::ostream& output) {
  output << "generation=" << generation << " string=" << candidate
         << " distance=" << distance << '\n';
}

RunResult Run(const Settings& settings, std::default_random_engine& engine,
              std::ostream& output) {
  ValidateSettings(settings);

  RunResult result;
  result.initial_string = GenerateRandomString(settings.target.size(), engine);
  result.best_string = result.initial_string;
  result.initial_distance =
    HammingDistance(result.initial_string, settings.target);
  result.best_distance = result.initial_distance;
  result.reached_target = (result.best_distance == 0);

  PrintGeneration(0, result.best_string, result.best_distance, output);

  if (result.reached_target) {
    return result;
  }

  for (std::size_t generation = 1; generation <= settings.max_generations;
       ++generation) {
    const std::vector<std::string> population =
      GeneratePopulation(result.best_string, settings, engine);
    const auto [best_candidate, best_distance] =
      SelectBestCandidate(population, settings.target);

    result.best_string = best_candidate;
    result.best_distance = best_distance;
    result.generations = generation;
    result.reached_target = (best_distance == 0);

    PrintGeneration(generation, result.best_string, result.best_distance,
                    output);

    if (result.reached_target) {
      break;
    }
  }

  return result;
}

Settings
MakeBookSettings(std::size_t max_generations = kDefaultMaxGenerations) {
  Settings settings;
  settings.target = "methinksitislikeaweasel";
  settings.population_size = kBookPopulationSize;
  settings.mutation_probability = kBookMutationProbability;
  settings.max_generations = max_generations;
  return settings;
}

RunResult RunBookExperiment(std::ostream& output) {
  std::random_device entropy_source;
  std::default_random_engine engine(entropy_source());
  return Run(MakeBookSettings(), engine, output);
}

} // namespace dawkins

TEST(MetricTest, CountsPositionalDifferences) {
  EXPECT_EQ(dawkins::HammingDistance("abc", "abc"), 0U);
  EXPECT_EQ(dawkins::HammingDistance("abc", "axz"), 2U);
  EXPECT_EQ(dawkins::HammingDistance("methinksitislikeaweasel",
                                     "meahinksjrislikebweasel"),
            4U);
}

TEST(GenerationTest, ProducesLowercaseInitialStringOfExpectedLength) {
  std::default_random_engine engine(7U);
  const std::string value = dawkins::GenerateRandomString(23U, engine);

  EXPECT_EQ(value.size(), 23U);
  EXPECT_TRUE(dawkins::IsLowercaseEnglishString(value));
}

TEST(MutationTest, KeepsStringUnchangedWhenProbabilityIsZero) {
  std::default_random_engine engine(11U);
  const std::string parent = "methinksitislikeaweasel";
  const std::string child = dawkins::MutateString(parent, 0.0, engine);

  EXPECT_EQ(child, parent);
}

TEST(MutationTest, ChangesEveryPositionWhenProbabilityIsOne) {
  std::default_random_engine engine(19U);
  const std::string parent = "aaaaaaaaaaaaaaaaaaaaaaa";
  const std::string child = dawkins::MutateString(parent, 1.0, engine);

  ASSERT_EQ(child.size(), parent.size());
  EXPECT_TRUE(dawkins::IsLowercaseEnglishString(child));
  EXPECT_NE(child, parent);

  for (std::size_t index = 0; index < parent.size(); ++index) {
    EXPECT_NE(child[index], parent[index]);
  }
}

TEST(AlgorithmTest, ReachesBookTargetWithDeterministicSeed) {
  const dawkins::Settings settings = dawkins::MakeBookSettings(1000U);
  std::default_random_engine engine(0U);

  const dawkins::RunResult result = dawkins::Run(settings, engine, std::cout);

  EXPECT_TRUE(result.reached_target);
  EXPECT_EQ(result.best_string, settings.target);
  EXPECT_EQ(result.best_distance, 0U);
  EXPECT_TRUE(result.generations <= settings.max_generations);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
