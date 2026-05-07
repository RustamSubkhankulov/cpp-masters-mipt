#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>
#include <iomanip>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

struct ConversionRate {
    long double rub_per_usd;
};

namespace {

std::string_view RussianLocaleName() {
    return "ru_RU.utf8";
}

std::string_view AmericanLocaleName() {
    return "en_US.utf8";
}

std::string_view RubCurrencyCode() {
    return "RUB";
}

bool IsAsciiSpace(char character) {
    return character == ' ' || character == '\t' || character == '\n' ||
           character == '\r' || character == '\f' || character == '\v';
}

bool IsAsciiLetter(char character) {
    return (character >= 'A' && character <= 'Z') ||
           (character >= 'a' && character <= 'z');
}

std::string_view TrimAscii(std::string_view text) {
    std::size_t first_index = 0;
    std::size_t last_index = text.size();

    while (first_index < last_index && IsAsciiSpace(text[first_index])) {
        ++first_index;
    }

    while (last_index > first_index && IsAsciiSpace(text[last_index - 1])) {
        --last_index;
    }

    return text.substr(first_index, last_index - first_index);
}

bool HasLeadingRubCode(std::string_view text) {
    const std::string_view code = RubCurrencyCode();

    if (!text.starts_with(code)) {
        return false;
    }

    return text.size() == code.size() || !IsAsciiLetter(text[code.size()]);
}

bool HasTrailingRubCode(std::string_view text) {
    const std::string_view code = RubCurrencyCode();

    if (!text.ends_with(code)) {
        return false;
    }

    if (text.size() == code.size()) {
        return true;
    }

    const std::size_t previous_index = text.size() - code.size() - 1;
    return !IsAsciiLetter(text[previous_index]);
}

std::string ExtractRubAmountText(std::string_view input_text) {
    const std::string_view trimmed_text = TrimAscii(input_text);
    const std::string_view code = RubCurrencyCode();

    if (HasLeadingRubCode(trimmed_text)) {
        return std::string(TrimAscii(trimmed_text.substr(code.size())));
    }

    if (HasTrailingRubCode(trimmed_text)) {
        return std::string(TrimAscii(
            trimmed_text.substr(0, trimmed_text.size() - code.size())));
    }

    throw std::invalid_argument("RUB currency code is missing");
}

void ValidateRate(ConversionRate rate) {
    if (!std::isfinite(rate.rub_per_usd) || rate.rub_per_usd <= 0.0L) {
        throw std::invalid_argument("Conversion rate must be positive");
    }
}

long double ParseRubKopecks(const std::string& input_text,
                            const std::locale& rub_locale) {
    const std::string amount_text = ExtractRubAmountText(input_text);

    std::stringstream input_stream(amount_text);
    input_stream.imbue(rub_locale);

    long double rub_kopecks = 0.0L;
    input_stream >> std::get_money(rub_kopecks, false);

    if (input_stream.fail()) {
        throw std::invalid_argument("Invalid RUB money amount");
    }

    input_stream >> std::ws;
    if (!input_stream.eof()) {
        throw std::invalid_argument("Unexpected text after RUB money amount");
    }

    if (!std::isfinite(rub_kopecks) || rub_kopecks < 0.0L) {
        throw std::invalid_argument("RUB money amount must be non-negative");
    }

    return rub_kopecks;
}

long long ConvertRubKopecksToUsdCents(long double rub_kopecks,
                                      ConversionRate rate) {
    const long double usd_cents_value = rub_kopecks / rate.rub_per_usd;
    const long double max_cents_value =
        static_cast<long double>(std::numeric_limits<long long>::max()) - 1.0L;

    if (usd_cents_value > max_cents_value) {
        throw std::out_of_range("USD money amount is too large");
    }

    return std::llround(usd_cents_value);
}

std::string FormatUsdCents(long long usd_cents, const std::locale& usd_locale) {
    std::stringstream output_stream;
    output_stream.imbue(usd_locale);

    output_stream << std::showbase
                  << std::put_money(static_cast<long double>(usd_cents), true);

    if (output_stream.fail()) {
        throw std::runtime_error("Could not format USD money amount");
    }

    return output_stream.str();
}

struct LocalePair {
    std::locale rub_locale;
    std::locale usd_locale;
};

std::optional<LocalePair> TryCreateRequiredLocales() {
    try {
        return LocalePair{
            std::locale(std::string(RussianLocaleName())),
            std::locale(std::string(AmericanLocaleName()))
        };
    } catch (const std::runtime_error&) {
        return std::nullopt;
    }
}

long long ParseUsdCentsForTest(const std::string& output_text,
                               const std::locale& usd_locale) {
    std::stringstream input_stream(output_text);
    input_stream.imbue(usd_locale);

    long double usd_cents = 0.0L;
    input_stream >> std::showbase >> std::get_money(usd_cents, true);

    if (input_stream.fail()) {
        throw std::invalid_argument("Invalid USD money amount");
    }

    input_stream >> std::ws;
    if (!input_stream.eof()) {
        throw std::invalid_argument("Unexpected text after USD money amount");
    }

    return std::llround(usd_cents);
}

}  // namespace

std::string ConvertRubToUsd(const std::string& rub_text,
                            ConversionRate rate,
                            const std::locale& rub_locale,
                            const std::locale& usd_locale) {
    ValidateRate(rate);

    const long double rub_kopecks = ParseRubKopecks(rub_text, rub_locale);
    const long long usd_cents = ConvertRubKopecksToUsdCents(rub_kopecks, rate);

    return FormatUsdCents(usd_cents, usd_locale);
}

std::string ConvertRubToUsd(const std::string& rub_text, ConversionRate rate) {
    const std::locale rub_locale{std::string{RussianLocaleName()}};
    const std::locale usd_locale{std::string{AmericanLocaleName()}};

    return ConvertRubToUsd(rub_text, rate, rub_locale, usd_locale);
}

std::string ConvertRubToUsd(const std::string& rub_text,
                            long double rub_per_usd) {
    return ConvertRubToUsd(rub_text, ConversionRate{rub_per_usd});
}

TEST(RubToUsdConversionTest, ConvertsInputWithLeadingRubCode) {
    const std::optional<LocalePair> locales = TryCreateRequiredLocales();
    if (!locales.has_value()) {
        GTEST_SKIP() << "Required ru_RU.utf8 and en_US.utf8 locales are not "
                        "installed";
    }

    const ConversionRate rate{100.0L};
    const std::string input_text = "RUB 1234,56";
    const long long expected_usd_cents = 1235LL;

    const std::string output_text =
        ConvertRubToUsd(input_text, rate, locales->rub_locale,
                        locales->usd_locale);

    EXPECT_EQ(ParseUsdCentsForTest(output_text, locales->usd_locale),
              expected_usd_cents);
}

TEST(RubToUsdConversionTest, ConvertsInputWithTrailingRubCode) {
    const std::optional<LocalePair> locales = TryCreateRequiredLocales();
    if (!locales.has_value()) {
        GTEST_SKIP() << "Required ru_RU.utf8 and en_US.utf8 locales are not "
                        "installed";
    }

    const ConversionRate rate{100.0L};
    const std::string input_text = "1234,56 RUB";
    const long long expected_usd_cents = 1235LL;

    const std::string output_text =
        ConvertRubToUsd(input_text, rate, locales->rub_locale,
                        locales->usd_locale);

    EXPECT_EQ(ParseUsdCentsForTest(output_text, locales->usd_locale),
              expected_usd_cents);
}

TEST(RubToUsdConversionTest, RoundsUsdCentsToNearestCent) {
    const std::optional<LocalePair> locales = TryCreateRequiredLocales();
    if (!locales.has_value()) {
        GTEST_SKIP() << "Required ru_RU.utf8 and en_US.utf8 locales are not "
                        "installed";
    }

    const ConversionRate rate{90.0L};
    const std::string input_text = "RUB 100,00";
    const long long expected_usd_cents = 111LL;

    const std::string output_text =
        ConvertRubToUsd(input_text, rate, locales->rub_locale,
                        locales->usd_locale);

    EXPECT_EQ(ParseUsdCentsForTest(output_text, locales->usd_locale),
              expected_usd_cents);
}

TEST(RubToUsdConversionTest, RejectsMissingRubCode) {
    const ConversionRate rate{100.0L};
    const std::string input_text = "100.00";

    EXPECT_THROW(
        ConvertRubToUsd(input_text, rate, std::locale::classic(),
                        std::locale::classic()),
        std::invalid_argument);
}

TEST(RubToUsdConversionTest, RejectsNonPositiveRate) {
    const ConversionRate rate{0.0L};
    const std::string input_text = "RUB 100.00";

    EXPECT_THROW(
        ConvertRubToUsd(input_text, rate, std::locale::classic(),
                        std::locale::classic()),
        std::invalid_argument);
}

TEST(RubToUsdConversionTest, RejectsUnexpectedTextAfterAmount) {
    const std::optional<LocalePair> locales = TryCreateRequiredLocales();
    if (!locales.has_value()) {
        GTEST_SKIP() << "Required ru_RU.utf8 and en_US.utf8 locales are not "
                        "installed";
    }

    const ConversionRate rate{100.0L};
    const std::string input_text = "RUB 100,00 extra";

    EXPECT_THROW(
        ConvertRubToUsd(input_text, rate, locales->rub_locale,
                        locales->usd_locale),
        std::invalid_argument);
}

TEST(RubToUsdDemonstrationTest, SupportsBothRubCodePositions) {
    const std::optional<LocalePair> locales = TryCreateRequiredLocales();
    if (!locales.has_value()) {
        GTEST_SKIP() << "Required ru_RU.utf8 and en_US.utf8 locales are not "
                        "installed";
    }

    const long double rub_per_usd = 100.0L;
    const std::string leading_input = "RUB 250,00";
    const std::string trailing_input = "250,00 RUB";
    const long long expected_usd_cents = 250LL;

    const std::string leading_output =
        ConvertRubToUsd(leading_input, rub_per_usd);
    const std::string trailing_output =
        ConvertRubToUsd(trailing_input, rub_per_usd);

    EXPECT_EQ(ParseUsdCentsForTest(leading_output, locales->usd_locale),
              expected_usd_cents);
    EXPECT_EQ(ParseUsdCentsForTest(trailing_output, locales->usd_locale),
              expected_usd_cents);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
