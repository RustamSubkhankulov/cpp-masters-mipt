#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

#include <boost/numeric/ublas/matrix.hpp>
#include <gtest/gtest.h>

/*
Complexity summary.

Let M be the standard 2x2 Fibonacci matrix:
[1 1]
[1 0]

The algorithm computes M^N by exponentiation by squaring.
It performs O(log N) matrix multiplications. Since the matrix size is fixed
(2x2), one multiplication costs O(1), so the total time complexity is O(log N).
The auxiliary space complexity is O(1).

Comparison with other common approaches:
1. Naive recursive Fibonacci:
   time O(phi^N), stack O(N).
2. Iterative linear recurrence:
   time O(N), space O(1).
3. Dynamic programming with memoization/table:
   time O(N), space O(N).
4. Binet formula:
   O(1) arithmetic operations, but it is not exact in integer arithmetic and
   becomes unreliable because of floating-point rounding.
5. Fast doubling:
   time O(log N), space O(log N) in recursive form or O(1) in iterative form.
   It has the same asymptotic time complexity as matrix exponentiation, usually
   with a smaller constant factor.

Because the required value type is unsigned long long int, exact computation is
possible only up to F(93). F(94) does not fit into this type.
*/

namespace fibonacci_matrix {

using ValueType = unsigned long long int;
using MatrixType = boost::numeric::ublas::matrix<ValueType>;

constexpr std::size_t kMatrixSize = 2U;
constexpr ValueType kMaxSupportedIndex = 93ULL;

MatrixType MakeZeroMatrix() {
    MatrixType matrix(kMatrixSize, kMatrixSize);
    for (std::size_t row = 0; row < kMatrixSize; ++row) {
        for (std::size_t column = 0; column < kMatrixSize; ++column) {
            matrix(row, column) = 0ULL;
        }
    }
    return matrix;
}

MatrixType MakeIdentityMatrix() {
    MatrixType matrix = MakeZeroMatrix();
    for (std::size_t index = 0; index < kMatrixSize; ++index) {
        matrix(index, index) = 1ULL;
    }
    return matrix;
}

MatrixType MakeBaseMatrix() {
    MatrixType matrix = MakeZeroMatrix();
    matrix(0U, 0U) = 1ULL;
    matrix(0U, 1U) = 1ULL;
    matrix(1U, 0U) = 1ULL;
    matrix(1U, 1U) = 0ULL;
    return matrix;
}

MatrixType MultiplyMatrices(const MatrixType& left, const MatrixType& right) {
    MatrixType result = MakeZeroMatrix();

    for (std::size_t row = 0; row < kMatrixSize; ++row) {
        for (std::size_t column = 0; column < kMatrixSize; ++column) {
            ValueType value = 0ULL;
            for (std::size_t inner = 0; inner < kMatrixSize; ++inner) {
                value += left(row, inner) * right(inner, column);
            }
            result(row, column) = value;
        }
    }

    return result;
}

MatrixType PowerMatrix(ValueType exponent) {
    MatrixType result = MakeIdentityMatrix();
    MatrixType factor = MakeBaseMatrix();

    while (exponent > 0ULL) {
        if ((exponent & 1ULL) != 0ULL) {
            result = MultiplyMatrices(result, factor);
        }

        exponent >>= 1ULL;
        if (exponent > 0ULL) {
            factor = MultiplyMatrices(factor, factor);
        }
    }

    return result;
}

ValueType Compute(ValueType index) {
    if (index > kMaxSupportedIndex) {
        throw std::overflow_error("Fibonacci value does not fit into unsigned long long int");
    }

    if (index == 0ULL) {
        return 0ULL;
    }

    const MatrixType powered = PowerMatrix(index);
    return powered(0U, 1U);
}

ValueType ComputeReferenceIterative(ValueType index) {
    if (index > kMaxSupportedIndex) {
        throw std::overflow_error("Reference Fibonacci value does not fit into unsigned long long int");
    }

    if (index == 0ULL) {
        return 0ULL;
    }

    ValueType previous = 0ULL;
    ValueType current = 1ULL;

    for (ValueType step = 1ULL; step < index; ++step) {
        const ValueType next = previous + current;
        previous = current;
        current = next;
    }

    return current;
}

}  // namespace fibonacci_matrix

TEST(FibonacciMatrixTest, ReturnsCorrectValuesForDemoExamples) {
    using fibonacci_matrix::Compute;

    EXPECT_EQ(Compute(0ULL), 0ULL);
    EXPECT_EQ(Compute(1ULL), 1ULL);
    EXPECT_EQ(Compute(2ULL), 1ULL);
    EXPECT_EQ(Compute(3ULL), 2ULL);
    EXPECT_EQ(Compute(10ULL), 55ULL);
    EXPECT_EQ(Compute(20ULL), 6765ULL);
    EXPECT_EQ(Compute(30ULL), 832040ULL);
}

TEST(FibonacciMatrixTest, MatchesReferenceImplementationForAllSupportedIndices) {
    using fibonacci_matrix::Compute;
    using fibonacci_matrix::ComputeReferenceIterative;
    using fibonacci_matrix::kMaxSupportedIndex;

    for (unsigned long long int index = 0ULL; index <= kMaxSupportedIndex; ++index) {
        EXPECT_EQ(Compute(index), ComputeReferenceIterative(index)) << "index = " << index;
    }
}

TEST(FibonacciMatrixTest, ReturnsLargestExactlyRepresentableValue) {
    using fibonacci_matrix::Compute;

    EXPECT_EQ(Compute(93ULL), 12200160415121876738ULL);
}

TEST(FibonacciMatrixTest, ThrowsOnOverflowingIndex) {
    using fibonacci_matrix::Compute;

    EXPECT_THROW(static_cast<void>(Compute(94ULL)), std::overflow_error);
    EXPECT_THROW(static_cast<void>(Compute(100ULL)), std::overflow_error);
}

TEST(FibonacciMatrixTest, ZeroPowerProducesIdentityMatrix) {
    using fibonacci_matrix::MatrixType;
    using fibonacci_matrix::PowerMatrix;

    const MatrixType matrix = PowerMatrix(0ULL);

    EXPECT_EQ(matrix(0U, 0U), 1ULL);
    EXPECT_EQ(matrix(0U, 1U), 0ULL);
    EXPECT_EQ(matrix(1U, 0U), 0ULL);
    EXPECT_EQ(matrix(1U, 1U), 1ULL);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
