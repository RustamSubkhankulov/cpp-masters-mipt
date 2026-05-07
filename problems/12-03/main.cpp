#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::size_t CheckedSquareSize(std::size_t value) {
    if (value != std::size_t{0} &&
        value > std::numeric_limits<std::size_t>::max() / value) {
        throw std::length_error("palindrome cache is too large");
    }

    return value * value;
}

std::size_t CacheIndex(
    std::size_t row,
    std::size_t column,
    std::size_t side_length
) {
    return row * side_length + column;
}

std::string_view LongestPalindromicSubstring(std::string_view text) {
    constexpr auto kEmptyLength = std::size_t{0};
    constexpr auto kSingleCharacterLength = std::size_t{1};
    constexpr auto kTwoCharacterLength = std::size_t{2};

    const std::size_t text_size = text.size();

    if (text_size == kEmptyLength) {
        return {};
    }

    std::vector<bool> cache(CheckedSquareSize(text_size), false);

    std::size_t best_begin = kEmptyLength;
    std::size_t best_length = kSingleCharacterLength;

    for (std::size_t current_length = kSingleCharacterLength;
         current_length <= text_size;
         ++current_length) {
        const std::size_t last_begin = text_size - current_length;

        for (std::size_t begin = kEmptyLength; begin <= last_begin; ++begin) {
            const std::size_t end = begin + current_length - kSingleCharacterLength;
            const bool matching_edges = text[begin] == text[end];
            const bool inner_is_palindrome =
                current_length <= kTwoCharacterLength ||
                cache[CacheIndex(
                    begin + kSingleCharacterLength,
                    end - kSingleCharacterLength,
                    text_size
                )];

            const bool current_is_palindrome = matching_edges && inner_is_palindrome;
            cache[CacheIndex(begin, end, text_size)] = current_is_palindrome;

            if (current_is_palindrome && current_length > best_length) {
                best_begin = begin;
                best_length = current_length;
            }
        }
    }

    return text.substr(best_begin, best_length);
}

TEST(LongestPalindromicSubstringTest, HandlesEmptyString) {
    EXPECT_EQ(LongestPalindromicSubstring(""), std::string_view());
}

TEST(LongestPalindromicSubstringTest, HandlesSingleCharacter) {
    EXPECT_EQ(LongestPalindromicSubstring("x"), std::string_view("x"));
}

TEST(LongestPalindromicSubstringTest, HandlesStringWithoutLongPalindrome) {
    EXPECT_EQ(LongestPalindromicSubstring("abcdef"), std::string_view("a"));
}

TEST(LongestPalindromicSubstringTest, FindsOddLengthPalindrome) {
    EXPECT_EQ(LongestPalindromicSubstring("babad"), std::string_view("bab"));
}

TEST(LongestPalindromicSubstringTest, FindsEvenLengthPalindrome) {
    EXPECT_EQ(
        LongestPalindromicSubstring("forgeeksskeegfor"),
        std::string_view("geeksskeeg")
    );
}

TEST(LongestPalindromicSubstringTest, HandlesWholeStringPalindrome) {
    EXPECT_EQ(LongestPalindromicSubstring("racecar"), std::string_view("racecar"));
}

TEST(LongestPalindromicSubstringTest, HandlesAllEqualCharacters) {
    EXPECT_EQ(LongestPalindromicSubstring("aaaaaa"), std::string_view("aaaaaa"));
}

TEST(LongestPalindromicSubstringTest, KeepsFirstLongestPalindromeOnTie) {
    EXPECT_EQ(
        LongestPalindromicSubstring("abacdfgdcaba"),
        std::string_view("aba")
    );
}

TEST(LongestPalindromicSubstringTest, PreservesStringViewRange) {
    const std::string text = "zzabbaqq";
    const std::string_view expected = "abba";
    const std::size_t expected_offset = 2;

    const std::string_view result = LongestPalindromicSubstring(text);

    EXPECT_EQ(result, expected);
    EXPECT_EQ(result.data(), text.data() + expected_offset);
    EXPECT_EQ(result.size(), expected.size());
}

TEST(LongestPalindromicSubstringDemoTest, DemonstratesTypicalUsage) {
    const std::string input = "abcxyzyxdef";
    const std::string_view result = LongestPalindromicSubstring(input);

    EXPECT_EQ(result, std::string_view("xyzyx"));
}

}  // namespace

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
