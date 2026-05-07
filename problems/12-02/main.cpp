#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

namespace {

constexpr int kEndOfFile = EOF;
constexpr char kReadMode[] = "r";
constexpr const char* kSourceFileName = __FILE__;

struct FileCloser {
  void operator()(std::FILE* file) const {
    if (file != nullptr) {
      std::fclose(file);
    }
  }
};

using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

FilePtr OpenFileForReading(const char* file_name) {
  FilePtr file(std::fopen(file_name, kReadMode));
  if (file == nullptr) {
    throw std::runtime_error("Cannot open source file");
  }
  return file;
}

void PrintCharacter(const int symbol) {
  std::printf("%c", symbol);
}

void PrintFileToStdout(const char* file_name) {
  FilePtr file = OpenFileForReading(file_name);

  int symbol = std::fgetc(file.get());
  while (symbol != kEndOfFile) {
    PrintCharacter(symbol);
    symbol = std::fgetc(file.get());
  }

  if (std::ferror(file.get()) != 0) {
    throw std::runtime_error("Cannot read source file");
  }
}

std::string ReadFile(const char* file_name) {
  FilePtr file = OpenFileForReading(file_name);

  std::string content;
  int symbol = std::fgetc(file.get());
  while (symbol != kEndOfFile) {
    content.push_back(static_cast<char>(symbol));
    symbol = std::fgetc(file.get());
  }

  if (std::ferror(file.get()) != 0) {
    throw std::runtime_error("Cannot read source file");
  }

  return content;
}

void PrintOwnSourceToStdout() {
  PrintFileToStdout(kSourceFileName);
}

bool ContainsOnlyAsciiCharacters(const std::string& text) {
  constexpr int kMaxAsciiCode = 127;

  for (const unsigned char symbol : text) {
    if (static_cast<int>(symbol) > kMaxAsciiCode) {
      return false;
    }
  }

  return true;
}

} // namespace

TEST(SelfSourcePrintingTest, PrintsOwnSourceToStdout) {
  testing::internal::CaptureStdout();
  PrintOwnSourceToStdout();
  const std::string printed_text = testing::internal::GetCapturedStdout();

  EXPECT_EQ(ReadFile(kSourceFileName), printed_text);
}

TEST(SelfSourcePrintingTest, SourceFileIsNotEmpty) {
  const std::string source_text = ReadFile(kSourceFileName);

  ASSERT_FALSE(source_text.empty());
}

TEST(SelfSourcePrintingTest, SourceUsesOnlyAsciiCharacters) {
  const std::string source_text = ReadFile(kSourceFileName);

  EXPECT_TRUE(ContainsOnlyAsciiCharacters(source_text));
}

TEST(SelfSourcePrintingDemoTest, PrintingFunctionCanBeCalledDirectly) {
  testing::internal::CaptureStdout();
  PrintOwnSourceToStdout();
  const std::string printed_text = testing::internal::GetCapturedStdout();

  ASSERT_FALSE(printed_text.empty());
  EXPECT_EQ(printed_text, ReadFile(kSourceFileName));
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
