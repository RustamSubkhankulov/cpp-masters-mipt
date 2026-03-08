#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

class Integer {
public:
    Integer() = default;

    Integer(long long value) {
        assign_from_signed(value);
    }

    explicit Integer(std::string_view text) {
        parse(text);
    }

    [[nodiscard]] int sign() const noexcept {
        return sign_;
    }

    [[nodiscard]] Integer abs() const {
        Integer result(*this);
        if (result.sign_ < 0) {
            result.sign_ = 1;
        }
        return result;
    }

    [[nodiscard]] std::string str() const {
        if (sign_ == 0) {
            return "+0";
        }

        std::ostringstream output;
        output << (sign_ < 0 ? '-' : '+');
        output << digits_.back();
        for (std::size_t index = digits_.size() - 1; index > 0; --index) {
            output << std::setw(static_cast<int>(kBaseDigits)) << std::setfill('0')
                   << digits_[index - 1];
        }
        return output.str();
    }

    Integer& operator+=(const Integer& other) {
        if (other.sign_ == 0) {
            return *this;
        }
        if (sign_ == 0) {
            *this = other;
            return *this;
        }

        if (sign_ == other.sign_) {
            add_abs_to(*this, other);
            return *this;
        }

        const int comparison = compare_abs(*this, other);
        if (comparison == 0) {
            digits_.clear();
            sign_ = 0;
            return *this;
        }
        if (comparison > 0) {
            sub_abs_from(*this, other);
            return *this;
        }

        Integer result(other);
        sub_abs_from(result, *this);
        *this = result;
        return *this;
    }

    Integer& operator-=(const Integer& other) {
        return *this += (-other);
    }

    Integer& operator*=(const Integer& other) {
        if (sign_ == 0 || other.sign_ == 0) {
            digits_.clear();
            sign_ = 0;
            return *this;
        }

        std::vector<std::uint32_t> result(digits_.size() + other.digits_.size(), 0U);
        for (std::size_t left = 0; left < digits_.size(); ++left) {
            std::uint64_t carry = 0U;
            for (std::size_t right = 0; right < other.digits_.size() || carry != 0U; ++right) {
                const std::uint64_t current = result[left + right]
                    + carry
                    + static_cast<std::uint64_t>(digits_[left])
                          * (right < other.digits_.size() ? other.digits_[right] : 0ULL);
                result[left + right] = static_cast<std::uint32_t>(current % kBase);
                carry = current / kBase;
            }
        }

        digits_ = std::move(result);
        sign_ = sign_ * other.sign_;
        normalize();
        return *this;
    }

    Integer& operator/=(const Integer& other) {
        const auto [quotient, remainder] = divmod(*this, other);
        (void)remainder;
        *this = quotient;
        return *this;
    }

    Integer& operator%=(const Integer& other) {
        const auto [quotient, remainder] = divmod(*this, other);
        (void)quotient;
        *this = remainder;
        return *this;
    }

    Integer& operator++() {
        *this += Integer(1);
        return *this;
    }

    Integer operator++(int) {
        Integer before(*this);
        ++(*this);
        return before;
    }

    Integer& operator--() {
        *this -= Integer(1);
        return *this;
    }

    Integer operator--(int) {
        Integer before(*this);
        --(*this);
        return before;
    }

    [[nodiscard]] Integer operator+() const {
        return *this;
    }

    [[nodiscard]] Integer operator-() const {
        Integer result(*this);
        if (result.sign_ != 0) {
            result.sign_ = -result.sign_;
        }
        return result;
    }

    friend Integer operator+(Integer left, const Integer& right) {
        left += right;
        return left;
    }

    friend Integer operator-(Integer left, const Integer& right) {
        left -= right;
        return left;
    }

    friend Integer operator*(Integer left, const Integer& right) {
        left *= right;
        return left;
    }

    friend Integer operator/(Integer left, const Integer& right) {
        left /= right;
        return left;
    }

    friend Integer operator%(Integer left, const Integer& right) {
        left %= right;
        return left;
    }

    [[nodiscard]] int compare(const Integer& other) const noexcept {
        if (sign_ != other.sign_) {
            return sign_ < other.sign_ ? -1 : 1;
        }
        if (sign_ == 0) {
            return 0;
        }
        const int comparison = compare_abs(*this, other);
        return sign_ > 0 ? comparison : -comparison;
    }

    [[nodiscard]] bool operator==(const Integer& other) const noexcept {
        return sign_ == other.sign_ && digits_ == other.digits_;
    }

    [[nodiscard]] bool operator!=(const Integer& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] bool operator<(const Integer& other) const noexcept {
        return compare(other) < 0;
    }

    [[nodiscard]] bool operator>(const Integer& other) const noexcept {
        return compare(other) > 0;
    }

    [[nodiscard]] bool operator<=(const Integer& other) const noexcept {
        return compare(other) <= 0;
    }

    [[nodiscard]] bool operator>=(const Integer& other) const noexcept {
        return compare(other) >= 0;
    }

    friend std::ostream& operator<<(std::ostream& output, const Integer& value) {
        if (value.sign_ == 0) {
            output << '0';
            return output;
        }

        if (value.sign_ < 0) {
            output << '-';
        }

        const char previous_fill = output.fill('0');
        output << value.digits_.back();
        for (std::size_t index = value.digits_.size() - 1; index > 0; --index) {
            output << std::setw(static_cast<int>(kBaseDigits)) << value.digits_[index - 1];
        }
        output.fill(previous_fill);
        return output;
    }

    friend std::istream& operator>>(std::istream& input, Integer& value) {
        std::string token;
        input >> token;
        if (!input) {
            return input;
        }
        try {
            value.parse(token);
        } catch (const std::invalid_argument&) {
            input.setstate(std::ios::failbit);
        }
        return input;
    }

    friend Integer pow(Integer base, unsigned int exponent) {
        Integer result(1);
        while (exponent != 0U) {
            if ((exponent & 1U) != 0U) {
                result *= base;
            }
            exponent >>= 1U;
            if (exponent != 0U) {
                base *= base;
            }
        }
        return result;
    }

    friend Integer multiply(const Integer& left, const Integer& right) {
        return left * right;
    }

    friend Integer sqrt(const Integer& value) {
        if (value.sign_ < 0) {
            throw std::domain_error("square root of a negative Integer");
        }

        Integer low(0);
        Integer high(value);
        Integer answer(0);
        const Integer one(1);
        const Integer two(2);

        while (low <= high) {
            const Integer middle = (low + high) / two;
            const Integer square = middle * middle;
            if (square <= value) {
                answer = middle;
                low = middle + one;
            } else {
                high = middle - one;
            }
        }

        return answer;
    }

private:
    static constexpr std::uint32_t kBase = 1000000000U;
    static constexpr std::size_t kBaseDigits = 9U;

    int sign_ = 0;
    std::vector<std::uint32_t> digits_;

    void assign_from_signed(long long value) {
        digits_.clear();
        if (value == 0) {
            sign_ = 0;
            return;
        }

        sign_ = value < 0 ? -1 : 1;
        unsigned long long magnitude = 0ULL;
        if (value < 0) {
            magnitude = static_cast<unsigned long long>(-(value + 1));
            ++magnitude;
        } else {
            magnitude = static_cast<unsigned long long>(value);
        }

        while (magnitude != 0ULL) {
            digits_.push_back(static_cast<std::uint32_t>(magnitude % kBase));
            magnitude /= kBase;
        }
    }

    void parse(std::string_view text) {
        if (text.empty()) {
            throw std::invalid_argument("empty Integer string");
        }

        std::size_t offset = 0U;
        int parsed_sign = 1;
        if (text.front() == '+' || text.front() == '-') {
            parsed_sign = text.front() == '-' ? -1 : 1;
            offset = 1U;
        }
        if (offset == text.size()) {
            throw std::invalid_argument("missing Integer digits");
        }

        while (offset < text.size() && text[offset] == '0') {
            ++offset;
        }
        if (offset == text.size()) {
            digits_.clear();
            sign_ = 0;
            return;
        }

        digits_.clear();
        for (std::size_t block_end = text.size(); block_end > offset; ) {
            const std::size_t available = block_end - offset;
            const std::size_t block_size = available >= kBaseDigits ? kBaseDigits : available;
            const std::size_t block_begin = block_end - block_size;

            std::uint32_t block_value = 0U;
            for (std::size_t index = block_begin; index < block_end; ++index) {
                const char symbol = text[index];
                if (symbol < '0' || symbol > '9') {
                    throw std::invalid_argument("invalid Integer digit");
                }
                block_value = static_cast<std::uint32_t>(
                    block_value * 10U + static_cast<unsigned int>(symbol - '0'));
            }

            digits_.push_back(block_value);
            block_end = block_begin;
        }

        sign_ = parsed_sign;
        normalize();
    }

    void normalize() {
        while (!digits_.empty() && digits_.back() == 0U) {
            digits_.pop_back();
        }
        if (digits_.empty()) {
            sign_ = 0;
        }
    }

    static int compare_abs(const Integer& left, const Integer& right) noexcept {
        if (left.digits_.size() != right.digits_.size()) {
            return left.digits_.size() < right.digits_.size() ? -1 : 1;
        }
        for (std::size_t index = left.digits_.size(); index > 0; --index) {
            const std::uint32_t left_digit = left.digits_[index - 1];
            const std::uint32_t right_digit = right.digits_[index - 1];
            if (left_digit != right_digit) {
                return left_digit < right_digit ? -1 : 1;
            }
        }
        return 0;
    }

    static void add_abs_to(Integer& left, const Integer& right) {
        const std::size_t result_size = std::max(left.digits_.size(), right.digits_.size());
        left.digits_.resize(result_size, 0U);

        std::uint64_t carry = 0U;
        for (std::size_t index = 0; index < result_size || carry != 0U; ++index) {
            if (index == left.digits_.size()) {
                left.digits_.push_back(0U);
            }
            const std::uint64_t sum = carry
                + left.digits_[index]
                + (index < right.digits_.size() ? right.digits_[index] : 0U);
            left.digits_[index] = static_cast<std::uint32_t>(sum % kBase);
            carry = sum / kBase;
        }

        left.normalize();
    }

    static void sub_abs_from(Integer& left, const Integer& right) {
        std::int64_t borrow = 0;
        for (std::size_t index = 0; index < left.digits_.size(); ++index) {
            const std::int64_t current = static_cast<std::int64_t>(left.digits_[index])
                - (index < right.digits_.size() ? static_cast<std::int64_t>(right.digits_[index]) : 0)
                - borrow;
            if (current < 0) {
                left.digits_[index] = static_cast<std::uint32_t>(current + static_cast<std::int64_t>(kBase));
                borrow = 1;
            } else {
                left.digits_[index] = static_cast<std::uint32_t>(current);
                borrow = 0;
            }
        }

        left.normalize();
    }

    static Integer multiply_abs_by_digit(const Integer& value, std::uint32_t digit) {
        if (value.sign_ == 0 || digit == 0U) {
            return Integer(0);
        }

        Integer result;
        result.sign_ = 1;
        result.digits_.resize(value.digits_.size(), 0U);

        std::uint64_t carry = 0U;
        for (std::size_t index = 0; index < value.digits_.size(); ++index) {
            const std::uint64_t product = static_cast<std::uint64_t>(value.digits_[index]) * digit + carry;
            result.digits_[index] = static_cast<std::uint32_t>(product % kBase);
            carry = product / kBase;
        }
        if (carry != 0U) {
            result.digits_.push_back(static_cast<std::uint32_t>(carry));
        }

        result.normalize();
        return result;
    }

    void shift_base_add(std::uint32_t digit) {
        if (sign_ == 0) {
            if (digit != 0U) {
                sign_ = 1;
                digits_.push_back(digit);
            }
            return;
        }

        digits_.insert(digits_.begin(), digit);
    }

    static std::pair<Integer, Integer> divmod(const Integer& dividend, const Integer& divisor) {
        if (divisor.sign_ == 0) {
            throw std::domain_error("division by zero");
        }
        if (dividend.sign_ == 0) {
            return {Integer(0), Integer(0)};
        }

        Integer left = dividend.abs();
        Integer right = divisor.abs();
        if (compare_abs(left, right) < 0) {
            return {Integer(0), dividend};
        }

        Integer quotient;
        quotient.sign_ = 1;
        quotient.digits_.assign(left.digits_.size(), 0U);
        Integer remainder;

        for (std::size_t index = left.digits_.size(); index > 0; --index) {
            remainder.shift_base_add(left.digits_[index - 1]);

            std::uint32_t low = 0U;
            std::uint32_t high = kBase - 1U;
            std::uint32_t best = 0U;
            while (low <= high) {
                const std::uint32_t middle = low + (high - low) / 2U;
                const Integer candidate = multiply_abs_by_digit(right, middle);
                if (compare_abs(candidate, remainder) <= 0) {
                    best = middle;
                    if (middle == kBase - 1U) {
                        break;
                    }
                    low = middle + 1U;
                } else {
                    if (middle == 0U) {
                        break;
                    }
                    high = middle - 1U;
                }
            }

            quotient.digits_[index - 1] = best;
            if (best != 0U) {
                const Integer candidate = multiply_abs_by_digit(right, best);
                sub_abs_from(remainder, candidate);
            }
        }

        quotient.normalize();
        remainder.normalize();

        if (quotient.sign_ != 0) {
            quotient.sign_ = dividend.sign_ * divisor.sign_;
        }
        if (remainder.sign_ != 0) {
            remainder.sign_ = dividend.sign_;
        }

        return {quotient, remainder};
    }
};

TEST(IntegerTest, ArithmeticFromExample815) {
    Integer x(std::string(32, '1'));
    Integer y(std::string(32, '2'));

    EXPECT_EQ((x += y).str(), "+33333333333333333333333333333333");
    EXPECT_EQ((x -= y).str(), "+11111111111111111111111111111111");
    EXPECT_EQ((x *= y).str(), "+246913580246913580246913580246908641975308641975308641975308642");
    EXPECT_EQ((x /= y).str(), "+11111111111111111111111111111111");

    EXPECT_EQ((x++).str(), "+11111111111111111111111111111111");
    EXPECT_EQ((x--).str(), "+11111111111111111111111111111112");
    EXPECT_EQ((++y).str(), "+22222222222222222222222222222223");
    EXPECT_EQ((--y).str(), "+22222222222222222222222222222222");

    EXPECT_EQ((x + y).str(), "+33333333333333333333333333333333");
    EXPECT_EQ((x - y).str(), "-11111111111111111111111111111111");
    EXPECT_EQ((x * y).str(), "+246913580246913580246913580246908641975308641975308641975308642");
    EXPECT_EQ((x / y).str(), "+0");

    EXPECT_TRUE(x < y);
    EXPECT_FALSE(x > y);
    EXPECT_TRUE(x <= y);
    EXPECT_FALSE(x >= y);
    EXPECT_FALSE(x == y);
    EXPECT_TRUE(x != y);

    std::stringstream input(std::string(32, '1'));
    std::stringstream output;
    input >> x;
    output << x;

    EXPECT_EQ(output.str(), std::string(32, '1'));
    EXPECT_EQ(sqrt(multiply(x, x)).str(), x.str());
}

TEST(IntegerTest, SignAndAbs) {
    const Integer negative("-12345678901234567890");
    const Integer positive("12345678901234567890");
    const Integer zero(0);

    EXPECT_EQ(negative.sign(), -1);
    EXPECT_EQ(positive.sign(), 1);
    EXPECT_EQ(zero.sign(), 0);

    EXPECT_EQ(negative.abs().str(), "+12345678901234567890");
    EXPECT_EQ(positive.abs().str(), "+12345678901234567890");
    EXPECT_EQ(zero.abs().str(), "+0");
}

TEST(IntegerTest, ModuloAssignmentAndModulo) {
    Integer value("1000");
    value %= Integer("64");
    EXPECT_EQ(value.str(), "+40");

    EXPECT_EQ((Integer("1000") % Integer("64")).str(), "+40");
    EXPECT_EQ((Integer("-1000") % Integer("64")).str(), "-40");
    EXPECT_EQ((Integer("1000") % Integer("-64")).str(), "+40");
    EXPECT_EQ((Integer("-1000") % Integer("-64")).str(), "-40");
    EXPECT_EQ((Integer("17") % Integer("17")).str(), "+0");
}

TEST(IntegerTest, PowFunction) {
    EXPECT_EQ(pow(Integer("2"), 10U).str(), "+1024");
    EXPECT_EQ(pow(Integer("-2"), 9U).str(), "-512");
    EXPECT_EQ(pow(Integer("-2"), 10U).str(), "+1024");
    EXPECT_EQ(pow(Integer("123456789"), 0U).str(), "+1");
}

TEST(IntegerTest, ExceptionalCases) {
    EXPECT_THROW(static_cast<void>(Integer("10") / Integer(0)), std::domain_error);
    EXPECT_THROW(static_cast<void>(Integer("10") % Integer(0)), std::domain_error);
    EXPECT_THROW(static_cast<void>(sqrt(Integer("-1"))), std::domain_error);
    EXPECT_THROW(static_cast<void>(Integer("12a3")), std::invalid_argument);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
