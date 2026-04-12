#include <boost/multi_array.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

class GameOfLife {
public:
    static constexpr std::size_t kHeight = 10;
    static constexpr std::size_t kWidth = 10;

    using Field = boost::multi_array<std::uint8_t, 2>;

    GameOfLife()
        : field_(boost::extents[kHeight][kWidth]),
          next_field_(boost::extents[kHeight][kWidth]) {
        Clear(field_);
        Clear(next_field_);
    }

    void SetAlive(const std::size_t row, const std::size_t column) {
        field_[row][column] = kAlive;
    }

    bool IsAlive(const std::size_t row, const std::size_t column) const {
        return field_[row][column] == kAlive;
    }

    void Step() {
        for (std::size_t row = 0; row < kHeight; ++row) {
            for (std::size_t column = 0; column < kWidth; ++column) {
                const std::size_t alive_neighbors = CountAliveNeighbors(row, column);
                const bool is_alive_now = IsAlive(row, column);

                if (is_alive_now) {
                    next_field_[row][column] =
                        (alive_neighbors == 2U || alive_neighbors == 3U) ? kAlive : kDead;
                } else {
                    next_field_[row][column] = (alive_neighbors == 3U) ? kAlive : kDead;
                }
            }
        }

        for (std::size_t row = 0; row < kHeight; ++row) {
            for (std::size_t column = 0; column < kWidth; ++column) {
                field_[row][column] = next_field_[row][column];
            }
        }
    }

    void Print(std::ostream& output) const {
        for (std::size_t row = 0; row < kHeight; ++row) {
            for (std::size_t column = 0; column < kWidth; ++column) {
                output << (IsAlive(row, column) ? '#' : '.');
            }
            output << '\n';
        }
    }

    std::string ToString() const {
        std::string result;
        result.reserve(kHeight * (kWidth + 1U));

        for (std::size_t row = 0; row < kHeight; ++row) {
            for (std::size_t column = 0; column < kWidth; ++column) {
                result.push_back(IsAlive(row, column) ? '#' : '.');
            }
            result.push_back('\n');
        }

        return result;
    }

private:
    static constexpr std::uint8_t kDead = 0;
    static constexpr std::uint8_t kAlive = 1;

    static void Clear(Field& field) {
        for (std::size_t row = 0; row < kHeight; ++row) {
            for (std::size_t column = 0; column < kWidth; ++column) {
                field[row][column] = kDead;
            }
        }
    }

    std::size_t CountAliveNeighbors(const std::size_t row, const std::size_t column) const {
        std::size_t alive_neighbors = 0;

        const std::size_t row_begin = (row == 0U) ? 0U : row - 1U;
        const std::size_t row_end = (row + 1U < kHeight) ? row + 1U : kHeight - 1U;
        const std::size_t column_begin = (column == 0U) ? 0U : column - 1U;
        const std::size_t column_end = (column + 1U < kWidth) ? column + 1U : kWidth - 1U;

        for (std::size_t neighbor_row = row_begin; neighbor_row <= row_end; ++neighbor_row) {
            for (std::size_t neighbor_column = column_begin; neighbor_column <= column_end;
                 ++neighbor_column) {
                if (neighbor_row == row && neighbor_column == column) {
                    continue;
                }

                if (IsAlive(neighbor_row, neighbor_column)) {
                    ++alive_neighbors;
                }
            }
        }

        return alive_neighbors;
    }

    Field field_;
    Field next_field_;
};

TEST(GameOfLifeRulesTest, LonelyCellDies) {
    GameOfLife game;
    game.SetAlive(4U, 4U);

    game.Step();

    EXPECT_FALSE(game.IsAlive(4U, 4U));
}

TEST(GameOfLifeRulesTest, DeadCellWithThreeNeighborsBecomesAlive) {
    GameOfLife game;
    game.SetAlive(4U, 3U);
    game.SetAlive(3U, 4U);
    game.SetAlive(5U, 4U);

    game.Step();

    EXPECT_TRUE(game.IsAlive(4U, 4U));
}

TEST(GameOfLifePatternsTest, BlockRemainsStable) {
    GameOfLife game;
    game.SetAlive(4U, 4U);
    game.SetAlive(4U, 5U);
    game.SetAlive(5U, 4U);
    game.SetAlive(5U, 5U);

    const std::string before = game.ToString();

    game.Step();

    EXPECT_EQ(game.ToString(), before);
}

TEST(GameOfLifePatternsTest, BlinkerOscillatesWithPeriodTwo) {
    GameOfLife game;
    game.SetAlive(4U, 5U);
    game.SetAlive(5U, 5U);
    game.SetAlive(6U, 5U);

    const std::string initial = game.ToString();

    game.Step();

    EXPECT_TRUE(game.IsAlive(5U, 4U));
    EXPECT_TRUE(game.IsAlive(5U, 5U));
    EXPECT_TRUE(game.IsAlive(5U, 6U));
    EXPECT_FALSE(game.IsAlive(4U, 5U));
    EXPECT_FALSE(game.IsAlive(6U, 5U));

    game.Step();

    EXPECT_EQ(game.ToString(), initial);
}

TEST(GameOfLifeDemoTest, PrintAllIterationsToTerminal) {
    GameOfLife game;

    game.SetAlive(1U, 2U);
    game.SetAlive(2U, 3U);
    game.SetAlive(3U, 1U);
    game.SetAlive(3U, 2U);
    game.SetAlive(3U, 3U);

    constexpr std::size_t kIterations = 6U;

    for (std::size_t iteration = 0; iteration <= kIterations; ++iteration) {
        std::cout << "Iteration " << iteration << '\n';
        game.Print(std::cout);
        std::cout << '\n';

        if (iteration != kIterations) {
            game.Step();
        }
    }

    EXPECT_TRUE(game.IsAlive(3U, 4U));
    EXPECT_TRUE(game.IsAlive(4U, 2U));
    EXPECT_TRUE(game.IsAlive(4U, 4U));
    EXPECT_TRUE(game.IsAlive(5U, 3U));
    EXPECT_TRUE(game.IsAlive(5U, 4U));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
