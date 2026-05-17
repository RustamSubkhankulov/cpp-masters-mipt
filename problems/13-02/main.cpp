#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

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
    double factor = 2.0;

    while (factor <= value) {
      result *= factor;
      factor += 1.0;
    }

    return result;
  }

  std::unordered_map<std::string, double> variables_;
};

struct FileEvaluation {
  std::size_t line_number = 0U;
  std::string instruction;
  double expected = 0.0;
  double actual = 0.0;
};

[[nodiscard]] auto is_blank(std::string_view text) -> bool {
  for (char symbol : text) {
    if (symbol != ' ' && symbol != '\t' && symbol != '\r' && symbol != '\n') {
      return false;
    }
  }

  return true;
}

[[nodiscard]] auto trim_copy(std::string_view text) -> std::string {
  std::size_t first = 0U;

  while (first < text.size() && (text[first] == ' ' || text[first] == '\t' ||
                                 text[first] == '\r' || text[first] == '\n')) {
    ++first;
  }

  std::size_t last = text.size();

  while (last > first && (text[last - 1U] == ' ' || text[last - 1U] == '\t' ||
                          text[last - 1U] == '\r' || text[last - 1U] == '\n')) {
    --last;
  }

  return std::string(text.substr(first, last - first));
}

[[nodiscard]] auto result_separator() -> std::string_view {
  return "=>";
}

[[nodiscard]] auto parse_expected_result(std::string const& text,
                                         std::size_t line_number) -> double {
  std::istringstream input(text);
  double value = 0.0;

  if (!(input >> value)) {
    throw std::runtime_error("expected result is not a number at line " +
                             std::to_string(line_number));
  }

  std::string extra;

  if (input >> extra) {
    throw std::runtime_error("unexpected text after result at line " +
                             std::to_string(line_number));
  }

  return value;
}

[[nodiscard]] auto parse_file_line(std::string const& line,
                                   std::size_t line_number) -> FileEvaluation {
  std::size_t separator = line.find(result_separator());

  if (separator == std::string::npos) {
    throw std::runtime_error("result separator expected at line " +
                             std::to_string(line_number));
  }

  std::string instruction =
    trim_copy(std::string_view(line).substr(0U, separator));
  std::string result_text = trim_copy(
    std::string_view(line).substr(separator + result_separator().size()));

  if (instruction.empty()) {
    throw std::runtime_error("empty instruction at line " +
                             std::to_string(line_number));
  }

  FileEvaluation evaluation;
  evaluation.line_number = line_number;
  evaluation.instruction = std::move(instruction);
  evaluation.expected = parse_expected_result(result_text, line_number);
  return evaluation;
}

[[nodiscard]] auto
evaluate_instruction_file(std::filesystem::path const& file_name)
  -> std::vector<FileEvaluation> {
  std::fstream input(file_name, std::ios::in);

  if (!input.is_open()) {
    throw std::runtime_error("cannot open instruction file");
  }

  Calculator calculator;
  std::vector<FileEvaluation> results;
  std::string line;
  std::size_t line_number = 1U;

  while (std::getline(input, line)) {
    if (!is_blank(line)) {
      FileEvaluation evaluation = parse_file_line(line, line_number);
      evaluation.actual = calculator.evaluate(evaluation.instruction);
      results.push_back(std::move(evaluation));
    }

    ++line_number;
  }

  if (input.bad()) {
    throw std::runtime_error("cannot read instruction file");
  }

  return results;
}

} // namespace calculator_impl

namespace test_support {

[[nodiscard]] auto temporary_file_path(std::string const& file_name)
  -> std::filesystem::path {
  return std::filesystem::temp_directory_path() / file_name;
}

void remove_file(std::filesystem::path const& file_name) {
  std::error_code error;
  static_cast<void>(std::filesystem::remove(file_name, error));
}

void write_text_file(std::filesystem::path const& file_name,
                     std::string const& content) {
  std::fstream output(file_name, std::ios::out | std::ios::trunc);

  if (!output.is_open()) {
    throw std::runtime_error("cannot create test file");
  }

  output << content;

  if (!output) {
    throw std::runtime_error("cannot write test file");
  }
}

} // namespace test_support

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

TEST(CalculatorFileInput, ReadsInstructionsAndResultsFromFile) {
  std::filesystem::path file_name =
    test_support::temporary_file_path("calculator_file_input_success.txt");

  test_support::remove_file(file_name);
  test_support::write_text_file(file_name, "1 + 2 * 3 => 7\n"
                                           "set x = 2 + 3 => 5\n"
                                           "x * 4 => 20\n"
                                           "set n = 4; => 4\n"
                                           "n! + 2 ^ n => 40\n"
                                           "[2 ^ 3 + 4!] % 10 => 2\n");

  std::vector<calculator_impl::FileEvaluation> results =
    calculator_impl::evaluate_instruction_file(file_name);

  constexpr std::size_t expected_count = 6U;
  ASSERT_EQ(results.size(), expected_count);

  for (calculator_impl::FileEvaluation const& result : results) {
    EXPECT_DOUBLE_EQ(result.actual, result.expected);
  }

  test_support::remove_file(file_name);
}

TEST(CalculatorFileInput, IgnoresBlankLinesAndKeepsLineNumbers) {
  std::filesystem::path file_name =
    test_support::temporary_file_path("calculator_file_input_blank_lines.txt");

  test_support::remove_file(file_name);
  test_support::write_text_file(file_name, "\n"
                                           "  \t\n"
                                           "4 + 1 => 5\n");

  std::vector<calculator_impl::FileEvaluation> results =
    calculator_impl::evaluate_instruction_file(file_name);

  constexpr std::size_t expected_count = 1U;
  constexpr std::size_t expected_line_number = 3U;
  ASSERT_EQ(results.size(), expected_count);
  EXPECT_EQ(results.front().line_number, expected_line_number);
  EXPECT_DOUBLE_EQ(results.front().actual, 5.0);
  EXPECT_DOUBLE_EQ(results.front().expected, 5.0);

  test_support::remove_file(file_name);
}

TEST(CalculatorFileInput, RejectsMalformedFileLines) {
  std::filesystem::path file_name =
    test_support::temporary_file_path("calculator_file_input_bad_line.txt");

  test_support::remove_file(file_name);
  test_support::write_text_file(file_name, "1 + 2 = 3\n");

  EXPECT_THROW(
    static_cast<void>(calculator_impl::evaluate_instruction_file(file_name)),
    std::runtime_error);

  test_support::remove_file(file_name);
}

TEST(CalculatorFileInput, RejectsInvalidExpectedResult) {
  std::filesystem::path file_name =
    test_support::temporary_file_path("calculator_file_input_bad_result.txt");

  test_support::remove_file(file_name);
  test_support::write_text_file(file_name, "1 + 2 => three\n");

  EXPECT_THROW(
    static_cast<void>(calculator_impl::evaluate_instruction_file(file_name)),
    std::runtime_error);

  test_support::remove_file(file_name);
}

TEST(CalculatorFileInput, RejectsMissingFile) {
  std::filesystem::path file_name =
    test_support::temporary_file_path("calculator_file_input_missing.txt");

  test_support::remove_file(file_name);

  EXPECT_THROW(
    static_cast<void>(calculator_impl::evaluate_instruction_file(file_name)),
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
