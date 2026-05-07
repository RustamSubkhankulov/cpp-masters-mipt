#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <gtest/gtest.h>

namespace calculator_impl {

class Token {
public:
  enum class Kind {
    end,
    number,
    identifier,
    keyword_set,
    plus,
    minus,
    multiply,
    divide,
    remainder,
    power,
    factorial,
    assign,
    semicolon,
    left_parenthesis,
    right_parenthesis,
    left_square,
    right_square,
    left_brace,
    right_brace
  };

  explicit Token(Kind kind)
    : kind_(kind) {}

  explicit Token(double value)
    : kind_(Kind::number)
    , value_(value) {}

  Token(Kind kind, std::string text)
    : kind_(kind)
    , text_(std::move(text)) {}

  [[nodiscard]] auto kind() const -> Kind {
    return kind_;
  }

  [[nodiscard]] auto value() const -> double {
    return value_;
  }

  [[nodiscard]] auto text() const -> std::string const& {
    return text_;
  }

private:
  Kind kind_ = Kind::end;
  double value_ = 0.0;
  std::string text_;
};

class TokenStream {
public:
  explicit TokenStream(std::string input)
    : input_(std::move(input)) {}

  auto get() -> Token {
    if (buffer_.has_value()) {
      Token token = *buffer_;
      buffer_.reset();
      return token;
    }

    skip_spaces();

    if (position_ == input_.size()) {
      return Token(Token::Kind::end);
    }

    char symbol = input_[position_];
    ++position_;

    switch (symbol) {
    case '+':
      return Token(Token::Kind::plus);
    case '-':
      return Token(Token::Kind::minus);
    case '*':
      return Token(Token::Kind::multiply);
    case '/':
      return Token(Token::Kind::divide);
    case '%':
      return Token(Token::Kind::remainder);
    case '^':
      return Token(Token::Kind::power);
    case '!':
      return Token(Token::Kind::factorial);
    case '=':
      return Token(Token::Kind::assign);
    case ';':
      return Token(Token::Kind::semicolon);
    case '(':
      return Token(Token::Kind::left_parenthesis);
    case ')':
      return Token(Token::Kind::right_parenthesis);
    case '[':
      return Token(Token::Kind::left_square);
    case ']':
      return Token(Token::Kind::right_square);
    case '{':
      return Token(Token::Kind::left_brace);
    case '}':
      return Token(Token::Kind::right_brace);
    default:
      break;
    }

    if (is_number_start(symbol)) {
      --position_;
      return read_number();
    }

    if (is_name_start(symbol)) {
      --position_;
      return read_identifier();
    }

    throw std::runtime_error("bad token");
  }

  void put(Token token) {
    if (buffer_.has_value()) {
      throw std::logic_error("token buffer overflow");
    }

    buffer_ = std::move(token);
  }

private:
  [[nodiscard]] static auto is_space(char symbol) -> bool {
    return symbol == ' ' || symbol == '\n' || symbol == '\r' || symbol == '\t';
  }

  [[nodiscard]] static auto is_digit(char symbol) -> bool {
    return symbol >= '0' && symbol <= '9';
  }

  [[nodiscard]] static auto is_letter(char symbol) -> bool {
    return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
  }

  [[nodiscard]] static auto is_name_start(char symbol) -> bool {
    return is_letter(symbol) || symbol == '_';
  }

  [[nodiscard]] static auto is_name_part(char symbol) -> bool {
    return is_name_start(symbol) || is_digit(symbol);
  }

  [[nodiscard]] static auto is_number_start(char symbol) -> bool {
    return is_digit(symbol) || symbol == '.';
  }

  void skip_spaces() {
    while (position_ < input_.size() && is_space(input_[position_])) {
      ++position_;
    }
  }

  auto read_number() -> Token {
    char* end = nullptr;
    char const* start = input_.c_str() + position_;
    double value = std::strtod(start, &end);

    if (end == start) {
      throw std::runtime_error("number expected");
    }

    position_ += static_cast<std::size_t>(end - start);
    return Token(value);
  }

  auto read_identifier() -> Token {
    std::size_t start = position_;

    while (position_ < input_.size() && is_name_part(input_[position_])) {
      ++position_;
    }

    std::string name = input_.substr(start, position_ - start);

    if (name == set_keyword()) {
      return Token(Token::Kind::keyword_set);
    }

    return Token(Token::Kind::identifier, std::move(name));
  }

  [[nodiscard]] static auto set_keyword() -> std::string_view {
    return "set";
  }

  std::string input_;
  std::size_t position_ = 0U;
  std::optional<Token> buffer_;
};

class Calculator {
public:
  auto evaluate(std::string const& text) -> double {
    TokenStream stream(text);
    double result = statement(stream);
    Token token = stream.get();

    if (token.kind() == Token::Kind::semicolon) {
      token = stream.get();
    }

    if (token.kind() != Token::Kind::end) {
      throw std::runtime_error("unexpected token after expression");
    }

    return result;
  }

private:
  auto statement(TokenStream& stream) -> double {
    Token token = stream.get();

    if (token.kind() == Token::Kind::keyword_set) {
      return declaration(stream);
    }

    stream.put(std::move(token));
    return expression(stream);
  }

  auto declaration(TokenStream& stream) -> double {
    Token name = stream.get();

    if (name.kind() != Token::Kind::identifier) {
      throw std::runtime_error("name expected in declaration");
    }

    std::string variable_name = name.text();
    Token token = stream.get();

    if (token.kind() != Token::Kind::assign) {
      stream.put(std::move(token));
    }

    double value = expression(stream);
    variables_[variable_name] = value;
    return value;
  }

  auto expression(TokenStream& stream) -> double {
    double left = term(stream);
    Token token = stream.get();

    while (true) {
      switch (token.kind()) {
      case Token::Kind::plus:
        left += term(stream);
        break;
      case Token::Kind::minus:
        left -= term(stream);
        break;
      default:
        stream.put(std::move(token));
        return left;
      }

      token = stream.get();
    }
  }

  auto term(TokenStream& stream) -> double {
    double left = unary(stream);
    Token token = stream.get();

    while (true) {
      switch (token.kind()) {
      case Token::Kind::multiply:
        left *= unary(stream);
        break;
      case Token::Kind::divide:
        left = divide(left, unary(stream));
        break;
      case Token::Kind::remainder:
        left = remainder(left, unary(stream));
        break;
      default:
        stream.put(std::move(token));
        return left;
      }

      token = stream.get();
    }
  }

  auto unary(TokenStream& stream) -> double {
    Token token = stream.get();

    switch (token.kind()) {
    case Token::Kind::plus:
      return unary(stream);
    case Token::Kind::minus:
      return -unary(stream);
    default:
      stream.put(std::move(token));
      return power(stream);
    }
  }

  auto power(TokenStream& stream) -> double {
    double base = postfix(stream);
    Token token = stream.get();

    if (token.kind() != Token::Kind::power) {
      stream.put(std::move(token));
      return base;
    }

    return std::pow(base, unary(stream));
  }

  auto postfix(TokenStream& stream) -> double {
    double value = primary(stream);
    Token token = stream.get();

    while (token.kind() == Token::Kind::factorial) {
      value = factorial(value);
      token = stream.get();
    }

    stream.put(std::move(token));
    return value;
  }

  auto primary(TokenStream& stream) -> double {
    Token token = stream.get();

    switch (token.kind()) {
    case Token::Kind::number:
      return token.value();
    case Token::Kind::identifier:
      return variable_value(token.text());
    case Token::Kind::left_parenthesis:
      return bracketed_expression(stream, Token::Kind::right_parenthesis);
    case Token::Kind::left_square:
      return bracketed_expression(stream, Token::Kind::right_square);
    case Token::Kind::left_brace:
      return bracketed_expression(stream, Token::Kind::right_brace);
    default:
      throw std::runtime_error("primary expected");
    }
  }

  auto bracketed_expression(TokenStream& stream, Token::Kind closing_kind)
    -> double {
    double value = expression(stream);
    Token closing = stream.get();

    if (closing.kind() != closing_kind) {
      throw std::runtime_error("closing bracket expected");
    }

    return value;
  }

  auto variable_value(std::string const& name) const -> double {
    auto iterator = variables_.find(name);

    if (iterator == variables_.end()) {
      throw std::runtime_error("undefined variable");
    }

    return iterator->second;
  }

  [[nodiscard]] static auto divide(double left, double right) -> double {
    if (right == 0.0) {
      throw std::runtime_error("division by zero");
    }

    return left / right;
  }

  [[nodiscard]] static auto remainder(double left, double right) -> double {
    if (right == 0.0) {
      throw std::runtime_error("remainder by zero");
    }

    return std::fmod(left, right);
  }

  [[nodiscard]] static auto factorial(double value) -> double {
    if (!std::isfinite(value) || value < 0.0 || std::trunc(value) != value) {
      throw std::runtime_error(
        "factorial operand must be a non-negative integer");
    }

    double result = 1.0;

    for (double factor = 2.0; factor <= value; factor += 1.0) {
      result *= factor;
    }

    return result;
  }

  std::unordered_map<std::string, double> variables_;
};

} // namespace calculator_impl

TEST(CalculatorArithmetic, KeepsBasicPrecedence) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("1 + 2 * 3"), 7.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("(1 + 2) * 3"), 9.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("[1 + 2] * {3 + 4}"), 21.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("{[2 + 3] * (4 - 1)}"), 15.0);
}

TEST(CalculatorRemainder, ComputesRemainder) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("17 % 5"), 2.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("20 % 6 % 4"), 2.0);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("5 % 0")),
               std::runtime_error);
}

TEST(CalculatorPower, ComputesRightAssociativePower) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("2 ^ 3"), 8.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("2 ^ 3 ^ 2"), 512.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("2 ^ -3"), 0.125);
  EXPECT_DOUBLE_EQ(calculator.evaluate("-2 ^ 2"), -4.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("(-2) ^ 2"), 4.0);
}

TEST(CalculatorFactorial, ComputesPostfixFactorial) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("0!"), 1.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("5!"), 120.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("3!!"), 720.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("2 * 3!"), 12.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("-3!"), -6.0);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("(-3)!")),
               std::runtime_error);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("2.5!")),
               std::runtime_error);
}

TEST(CalculatorVariables, StoresAndUsesVariables) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("set x = 2 + 3"), 5.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("x * 4"), 20.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("set y [x + 1]"), 6.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("y! / x!"), 6.0);
}

TEST(CalculatorInputValidation, RejectsInvalidInput) {
  calculator_impl::Calculator calculator;

  EXPECT_THROW(static_cast<void>(calculator.evaluate("(1 + 2]")),
               std::runtime_error);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("unknown + 1")),
               std::runtime_error);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("1 + 2 3")),
               std::runtime_error);
  EXPECT_THROW(static_cast<void>(calculator.evaluate("set = 3")),
               std::runtime_error);
}

TEST(CalculatorDemoExamples, HandlesCompoundExamples) {
  calculator_impl::Calculator calculator;

  EXPECT_DOUBLE_EQ(calculator.evaluate("4 + 1"), 5.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("(5 - 2) / {6 * (8 + 14)}"),
                   3.0 / 132.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("[2 ^ 3 + 4!] % 10"), 2.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("set n = 4;"), 4.0);
  EXPECT_DOUBLE_EQ(calculator.evaluate("n! + 2 ^ n"), 40.0);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
