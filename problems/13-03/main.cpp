#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace comment_deletion {

bool IsRawDelimiterCharacter(char character) {
  return character != '(' && character != ')' && character != '\\' &&
         character != ' ' && character != '\t' && character != '\n' &&
         character != '\r' && character != '\f' && character != '\v';
}

std::size_t FindRawStringEnd(std::string_view source, std::size_t position) {
  constexpr std::size_t raw_prefix_size = 2;
  constexpr std::size_t max_raw_delimiter_size = 16;

  if (position + raw_prefix_size > source.size() || source[position] != 'R' ||
      source[position + 1] != '"') {
    return std::string_view::npos;
  }

  const std::size_t delimiter_begin = position + raw_prefix_size;
  std::size_t delimiter_end = delimiter_begin;

  while (delimiter_end < source.size() && source[delimiter_end] != '(') {
    if (!IsRawDelimiterCharacter(source[delimiter_end])) {
      return std::string_view::npos;
    }

    if (delimiter_end - delimiter_begin >= max_raw_delimiter_size) {
      return std::string_view::npos;
    }

    ++delimiter_end;
  }

  if (delimiter_end == source.size()) {
    return std::string_view::npos;
  }

  std::string closing_sequence;
  closing_sequence.reserve(delimiter_end - delimiter_begin + raw_prefix_size);
  closing_sequence.push_back(')');
  closing_sequence.append(source.data() + delimiter_begin,
                          delimiter_end - delimiter_begin);
  closing_sequence.push_back('"');

  const std::size_t closing_position =
    source.find(closing_sequence, delimiter_end + 1);

  if (closing_position == std::string_view::npos) {
    return source.size();
  }

  return closing_position + closing_sequence.size();
}

void CopyRange(std::string_view source, std::size_t begin, std::size_t end,
               std::string& output) {
  output.append(source.data() + begin, end - begin);
}

void CopyQuotedLiteral(std::string_view source, std::size_t& position,
                       std::string& output) {
  const char literal_delimiter = source[position];

  output.push_back(source[position]);
  ++position;

  bool is_escaped = false;

  while (position < source.size()) {
    const char current = source[position];

    output.push_back(current);
    ++position;

    if (current == literal_delimiter && !is_escaped) {
      return;
    }

    if (current == '\\') {
      is_escaped = !is_escaped;
    } else {
      is_escaped = false;
    }
  }
}

void CopyRawStringLiteral(std::string_view source, std::size_t& position,
                          std::string& output) {
  const std::size_t raw_string_end = FindRawStringEnd(source, position);

  if (raw_string_end == std::string_view::npos) {
    output.push_back(source[position]);
    ++position;
    return;
  }

  CopyRange(source, position, raw_string_end, output);
  position = raw_string_end;
}

void SkipLineComment(std::string_view source, std::size_t& position) {
  while (position < source.size() && source[position] != '\n') {
    ++position;
  }
}

void SkipBlockComment(std::string_view source, std::size_t& position,
                      std::string& output) {
  constexpr std::size_t comment_open_size = 2;
  constexpr std::size_t comment_close_size = 2;

  const std::size_t close_position =
    source.find("*/", position + comment_open_size);

  position = close_position == std::string_view::npos
               ? source.size()
               : close_position + comment_close_size;

  output.push_back(' ');
}

std::string DeleteComments(std::string_view source) {
  std::string output;
  output.reserve(source.size());

  std::size_t position = 0;

  while (position < source.size()) {
    if (source[position] == 'R' &&
        FindRawStringEnd(source, position) != std::string_view::npos) {
      CopyRawStringLiteral(source, position, output);
      continue;
    }

    if (source[position] == '\'' || source[position] == '"') {
      CopyQuotedLiteral(source, position, output);
      continue;
    }

    if (source[position] == '/' && position + 1 < source.size()) {
      if (source[position + 1] == '/') {
        SkipLineComment(source, position);
        continue;
      }

      if (source[position + 1] == '*') {
        SkipBlockComment(source, position, output);
        continue;
      }
    }

    output.push_back(source[position]);
    ++position;
  }

  return output;
}

bool IsWhitespaceLine(std::string_view line) {
  return std::ranges::all_of(line, [](char character) {
    return character == ' ' || character == '\t' || character == '\r' ||
           character == '\f' || character == '\v';
  });
}

std::string RemoveBlankLines(std::string_view source) {
  std::string output;
  output.reserve(source.size());

  std::size_t position = 0;

  while (position < source.size()) {
    const std::size_t newline_position = source.find('\n', position);
    const bool has_newline = newline_position != std::string_view::npos;
    const std::size_t line_end = has_newline ? newline_position : source.size();

    const std::string_view line(source.data() + position, line_end - position);

    if (!IsWhitespaceLine(line)) {
      output.append(line);
      if (has_newline) {
        output.push_back('\n');
      }
    }

    if (!has_newline) {
      break;
    }

    position = newline_position + 1;
  }

  return output;
}

std::string TransformText(std::string_view source) {
  return RemoveBlankLines(DeleteComments(source));
}

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary);

  if (!input) {
    throw std::runtime_error("Cannot open input file");
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void WriteFile(const std::filesystem::path& path, std::string_view text) {
  std::ofstream output(path, std::ios::binary);

  if (!output) {
    throw std::runtime_error("Cannot open output file");
  }

  output << text;
}

void transform(const std::filesystem::path& input_path,
               const std::filesystem::path& output_path) {
  WriteFile(output_path, TransformText(ReadFile(input_path)));
}

std::filesystem::path MakeTemporaryPath(std::string_view suffix) {
  const auto timestamp =
    std::chrono::steady_clock::now().time_since_epoch().count();

  std::string filename = "task_13_03_";
  filename += std::to_string(timestamp);
  filename.append(suffix);

  return std::filesystem::temp_directory_path() / filename;
}

} // namespace comment_deletion

TEST(CommentDeletionTests, RemovesLineCommentsAndBlankLines) {
  const std::string input = "\n"
                            "int value = 1; // trailing comment\n"
                            "    // full line comment\n"
                            "\t  \n"
                            "int next = 2;\n";

  const std::string expected = "int value = 1; \n"
                               "int next = 2;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, RemovesBlockCommentsAndBlankLines) {
  const std::string input = "int first = 1;/* removed */int second = 2;\n"
                            "/* removed full line */\n"
                            "   \t \n"
                            "int third = 3;\n";

  const std::string expected = "int first = 1; int second = 2;\n"
                               "int third = 3;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, PreservesCommentMarkersInsideOrdinaryLiterals) {
  const std::string input = "auto url = \"https://example.com/a//b\";\n"
                            "auto block = \"/* not a comment */\";\n"
                            "auto slash = '/';\n"
                            "auto quote = '\\\'';\n"
                            "// removed\n"
                            "int value = 7;\n";

  const std::string expected = "auto url = \"https://example.com/a//b\";\n"
                               "auto block = \"/* not a comment */\";\n"
                               "auto slash = '/';\n"
                               "auto quote = '\\\'';\n"
                               "int value = 7;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, HandlesEscapedQuotesInsideStringLiterals) {
  const std::string input =
    "auto text = \"quote: \\\" and comment marker // kept\"; // removed\n"
    "int value = 1;\n";

  const std::string expected =
    "auto text = \"quote: \\\" and comment marker // kept\"; \n"
    "int value = 1;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, PreservesRawStringLiteralWithEmptyDelimiter) {
  const std::string input = "auto text = R\"(// not a comment\n"
                            "/* not a comment */\n"
                            ")\";\n"
                            "// removed\n"
                            "int value = 1;\n";

  const std::string expected = "auto text = R\"(// not a comment\n"
                               "/* not a comment */\n"
                               ")\";\n"
                               "int value = 1;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, PreservesRawStringLiteralWithCustomDelimiter) {
  const std::string input = "auto text = R\"tag(not closed by )\" and keeps // "
                            "and /* */)tag\"; // tail\n"
                            "int value = 2;\n";

  const std::string expected =
    "auto text = R\"tag(not closed by )\" and keeps // and /* */)tag\"; \n"
    "int value = 2;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, SupportsEncodedRawStringPrefixes) {
  const std::string input = "auto first = u8R\"tag(// not a comment)tag\";\n"
                            "auto second = LR\"tag(/* not a comment */)tag\";\n"
                            "/* removed */\n"
                            "int value = 3;\n";

  const std::string expected =
    "auto first = u8R\"tag(// not a comment)tag\";\n"
    "auto second = LR\"tag(/* not a comment */)tag\";\n"
    "int value = 3;\n";

  EXPECT_EQ(comment_deletion::TransformText(input), expected);
}

TEST(CommentDeletionTests, WritesTransformedFile) {
  const std::filesystem::path input_path =
    comment_deletion::MakeTemporaryPath("_input.cpp");
  const std::filesystem::path output_path =
    comment_deletion::MakeTemporaryPath("_output.cpp");

  const std::string input = "int value = 0; // removed\n"
                            "\n"
                            "auto raw = R\"(/* kept */)\";\n";

  const std::string expected = "int value = 0; \n"
                               "auto raw = R\"(/* kept */)\";\n";

  comment_deletion::WriteFile(input_path, input);
  comment_deletion::transform(input_path, output_path);

  EXPECT_EQ(comment_deletion::ReadFile(output_path), expected);

  const bool input_removed = std::filesystem::remove(input_path);
  const bool output_removed = std::filesystem::remove(output_path);

  EXPECT_TRUE(input_removed);
  EXPECT_TRUE(output_removed);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
