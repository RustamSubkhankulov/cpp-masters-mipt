#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

/**
 * Note: '-Wunused-parameter' to fix following warnings:
 * include/boost/graph/adjacency_matrix.hpp:1113:68: warning: unused parameter
 * 'g' [-Wunused-parameter] 1113 |     static type
 * get_nonconst(adjacency_matrix< D, VP, EP, GP, A >& g, Tag tag) | ^
 *  include/boost/graph/adjacency_matrix.hpp:1119:53: warning: unused parameter
 * 'g' [-Wunused-parameter] 1119 |         const adjacency_matrix< D, VP, EP,
 * GP, A >& g, Tag tag) |                                                     ^
 *  include/boost/graph/adjacency_matrix.hpp:1125:47: warning: unused parameter
 * 'g' [-Wunused-parameter] 1125 |         adjacency_matrix< D, VP, EP, GP, A >&
 * g, Tag tag, edge_descriptor e) | ^
 * include/boost/graph/adjacency_matrix.hpp:1132:53: warning: unused parameter
 * 'g' [-Wunused-parameter] 1132 |         const adjacency_matrix< D, VP, EP,
 * GP, A >& g, Tag tag,
 *       |
 */

#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/properties.hpp>

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <ostream>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::size_t VertexCount() {
  return 10U;
}

int MinEdgeWeight() {
  return 1;
}

int MaxEdgeWeight() {
  return 10;
}

using EdgeWeightProperty = boost::property<boost::edge_weight_t, int>;
using Graph = boost::adjacency_matrix<boost::undirectedS, boost::no_property,
                                      EdgeWeightProperty>;
using VertexIndex = std::size_t;
using VertexPath = std::vector<VertexIndex>;
using EdgePair = std::pair<VertexIndex, VertexIndex>;

struct TourResult {
  VertexPath path;
  int cost;
};

struct GeneratedGraph {
  Graph graph;
  std::vector<std::vector<int>> weights;
};

GeneratedGraph MakeCompleteGraph(std::default_random_engine& generator) {
  const std::size_t vertices = VertexCount();
  Graph graph(vertices);
  std::vector<std::vector<int>> weights(vertices,
                                        std::vector<int>(vertices, 0));
  std::uniform_int_distribution<int> distribution(MinEdgeWeight(),
                                                  MaxEdgeWeight());

  for (std::size_t from = 0U; from < vertices; ++from) {
    for (std::size_t to = from + 1U; to < vertices; ++to) {
      const int weight = distribution(generator);
      const auto edge =
        boost::add_edge(from, to, EdgeWeightProperty(weight), graph);
      if (!edge.second) {
        throw std::logic_error("Complete graph edge was not inserted");
      }

      weights[from][to] = weight;
      weights[to][from] = weight;
    }
  }

  return GeneratedGraph{std::move(graph), std::move(weights)};
}

std::vector<EdgePair> MakeEdgePairs() {
  std::vector<EdgePair> edges;
  edges.reserve(VertexCount() * (VertexCount() - 1U) / 2U);

  for (std::size_t from = 0U; from < VertexCount(); ++from) {
    for (std::size_t to = from + 1U; to < VertexCount(); ++to) {
      edges.emplace_back(from, to);
    }
  }

  return edges;
}

int GetEdgeWeight(const Graph& graph, VertexIndex from, VertexIndex to) {
  const auto edge = boost::edge(from, to, graph);
  if (!edge.second) {
    throw std::logic_error("Missing edge in complete graph");
  }

  const auto weights = boost::get(boost::edge_weight, graph);
  return boost::get(weights, edge.first);
}

int CalculateTourCost(const Graph& graph, const VertexPath& path) {
  if (path.empty()) {
    throw std::invalid_argument("Path must contain at least one vertex");
  }

  int cost = 0;
  for (std::size_t index = 1U; index < path.size(); ++index) {
    cost += GetEdgeWeight(graph, path[index - 1U], path[index]);
  }

  cost += GetEdgeWeight(graph, path.back(), path.front());
  return cost;
}

TourResult SolveTravelingSalesman(const Graph& graph) {
  const std::size_t vertices = boost::num_vertices(graph);
  if (vertices == 0U) {
    throw std::invalid_argument("Graph must contain vertices");
  }

  VertexPath current_path(vertices);
  std::iota(current_path.begin(), current_path.end(), VertexIndex{0U});

  TourResult best_result{current_path, std::numeric_limits<int>::max()};
  const auto first_permuted = current_path.begin() + 1;

  do {
    const int current_cost = CalculateTourCost(graph, current_path);
    if (current_cost < best_result.cost) {
      best_result = TourResult{current_path, current_cost};
    }
  } while (std::next_permutation(first_permuted, current_path.end()));

  return best_result;
}

void PrintWeightMatrix(const std::vector<std::vector<int>>& weights,
                       std::ostream& output) {
  output << "Weight matrix:\n";
  for (const auto& row : weights) {
    for (std::size_t column = 0U; column < row.size(); ++column) {
      if (column != 0U) {
        output << ' ';
      }

      output << row[column];
    }

    output << '\n';
  }
}

void PrintIncidenceMatrix(std::ostream& output) {
  const std::vector<EdgePair> edges = MakeEdgePairs();

  output << "Incidence matrix:\n";
  for (std::size_t vertex = 0U; vertex < VertexCount(); ++vertex) {
    for (std::size_t edge_index = 0U; edge_index < edges.size(); ++edge_index) {
      if (edge_index != 0U) {
        output << ' ';
      }

      const EdgePair edge = edges[edge_index];
      const bool incident = vertex == edge.first || vertex == edge.second;
      output << (incident ? 1 : 0);
    }

    output << '\n';
  }
}

void PrintTour(const TourResult& result, std::ostream& output) {
  output << "Optimal tour:\n";
  for (std::size_t index = 0U; index < result.path.size(); ++index) {
    if (index != 0U) {
      output << " -> ";
    }

    output << result.path[index];
  }

  output << " -> " << result.path.front() << '\n';
  output << "Cost: " << result.cost << '\n';
}

void PrintSolution(const GeneratedGraph& generated_graph,
                   const TourResult& result, std::ostream& output) {
  PrintWeightMatrix(generated_graph.weights, output);
  PrintIncidenceMatrix(output);
  PrintTour(result, output);
}

GeneratedGraph MakeKnownGraph() {
  Graph graph(VertexCount());
  std::vector<std::vector<int>> weights(VertexCount(),
                                        std::vector<int>(VertexCount(), 0));

  for (std::size_t from = 0U; from < VertexCount(); ++from) {
    for (std::size_t to = from + 1U; to < VertexCount(); ++to) {
      const int weight = static_cast<int>(from + to + 1U);
      const auto edge =
        boost::add_edge(from, to, EdgeWeightProperty(weight), graph);
      if (!edge.second) {
        throw std::logic_error("Known graph edge was not inserted");
      }

      weights[from][to] = weight;
      weights[to][from] = weight;
    }
  }

  return GeneratedGraph{std::move(graph), std::move(weights)};
}

bool AreAdjacentInCycle(const VertexPath& cycle, VertexIndex first_vertex,
                        VertexIndex second_vertex) {
  const auto first = std::find(cycle.begin(), cycle.end(), first_vertex);
  const auto second = std::find(cycle.begin(), cycle.end(), second_vertex);

  const std::size_t first_index =
    static_cast<std::size_t>(std::distance(cycle.begin(), first));
  const std::size_t second_index =
    static_cast<std::size_t>(std::distance(cycle.begin(), second));

  const std::size_t direct_distance = first_index > second_index
                                        ? first_index - second_index
                                        : second_index - first_index;
  const std::size_t wrap_distance = cycle.size() - direct_distance;

  return direct_distance == 1U || wrap_distance == 1U;
}

} // namespace

TEST(GraphGeneration, CreatesCompleteUndirectedGraph) {
  std::default_random_engine generator(12345U);
  const GeneratedGraph generated_graph = MakeCompleteGraph(generator);

  EXPECT_EQ(boost::num_vertices(generated_graph.graph), VertexCount());
  EXPECT_EQ(boost::num_edges(generated_graph.graph),
            VertexCount() * (VertexCount() - 1U) / 2U);

  for (std::size_t from = 0U; from < VertexCount(); ++from) {
    EXPECT_EQ(generated_graph.weights[from][from], 0);

    for (std::size_t to = from + 1U; to < VertexCount(); ++to) {
      const int matrix_weight = generated_graph.weights[from][to];

      EXPECT_GE(matrix_weight, MinEdgeWeight());
      EXPECT_LE(matrix_weight, MaxEdgeWeight());
      EXPECT_EQ(matrix_weight, generated_graph.weights[to][from]);
      EXPECT_EQ(matrix_weight, GetEdgeWeight(generated_graph.graph, from, to));
    }
  }
}

TEST(TourCost, IncludesReturnToStartVertex) {
  const GeneratedGraph graph = MakeKnownGraph();
  const VertexPath path{0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

  int expected_cost = 0;
  for (std::size_t index = 1U; index < path.size(); ++index) {
    expected_cost += graph.weights[path[index - 1U]][path[index]];
  }

  expected_cost += graph.weights[path.back()][path.front()];

  EXPECT_EQ(CalculateTourCost(graph.graph, path), expected_cost);
}

TEST(TravelingSalesman, FindsOptimalKnownTour) {
  Graph graph(VertexCount());
  const int cheap_weight = 1;
  const int expensive_weight = 10;
  const VertexPath optimal_cycle{0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U};

  for (std::size_t from = 0U; from < VertexCount(); ++from) {
    for (std::size_t to = from + 1U; to < VertexCount(); ++to) {
      const bool adjacent = AreAdjacentInCycle(optimal_cycle, from, to);
      const int weight = adjacent ? cheap_weight : expensive_weight;
      const auto edge =
        boost::add_edge(from, to, EdgeWeightProperty(weight), graph);

      if (!edge.second) {
        throw std::logic_error("Test graph edge was not inserted");
      }
    }
  }

  const TourResult result = SolveTravelingSalesman(graph);

  EXPECT_EQ(result.cost, static_cast<int>(VertexCount()) * cheap_weight);
  EXPECT_EQ(CalculateTourCost(graph, result.path), result.cost);
}

TEST(Demonstration, PrintsRandomGraphSolution) {
  std::random_device entropy;
  std::default_random_engine generator(entropy());
  const GeneratedGraph generated_graph = MakeCompleteGraph(generator);
  const TourResult result = SolveTravelingSalesman(generated_graph.graph);

  PrintSolution(generated_graph, result, std::cout);

  EXPECT_EQ(result.path.size(), VertexCount());
  EXPECT_EQ(CalculateTourCost(generated_graph.graph, result.path), result.cost);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
