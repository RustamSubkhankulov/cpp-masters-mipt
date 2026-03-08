#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <boost/dll/import.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/dll/shared_library.hpp>

#include <gtest/gtest.h>

namespace {
using imported_function_type = char const*();

constexpr std::string_view kSymbolName = "test";
constexpr std::string_view kLibraryOneName = "problem_06_05_library_v1";
constexpr std::string_view kLibraryTwoName = "problem_06_05_library_v2";

enum class RunStatus { kSuccess = 0, kInputError = 1, kLoadError = 2 };

using library_path = boost::dll::fs::path;

library_path GetExecutableDirectory() {
  return boost::dll::program_location().parent_path();
}

library_path ResolveLibraryPath(std::string_view library_name,
                                const library_path& library_directory) {
  const library_path input_path{std::string(library_name)};
  std::vector<library_path> candidates;

  if (input_path.is_absolute()) {
    candidates.push_back(input_path);
  } else {
    candidates.push_back(library_directory / input_path);
  }

  if (!input_path.has_parent_path() && !input_path.has_extension()) {
    candidates.push_back(
      library_directory /
      boost::dll::shared_library::decorate(input_path.string()));
  }

  for (const auto& candidate : candidates) {
    if (boost::dll::fs::exists(candidate)) {
      return candidate;
    }
  }

  return candidates.back();
}

std::string LoadMessageFromLibrary(std::string_view library_name,
                                   const library_path& library_directory) {
  const library_path selected_library_path =
    ResolveLibraryPath(library_name, library_directory);

  auto imported_function = boost::dll::import_symbol<imported_function_type>(
    selected_library_path, kSymbolName.data());

  return imported_function();
}

RunStatus RunSelectedLibrary(std::istream& input, std::ostream& output,
                             const library_path& library_directory) {
  std::string library_name;

  if (!(input >> library_name)) {
    return RunStatus::kInputError;
  }

  try {
    output << LoadMessageFromLibrary(library_name, library_directory) << '\n';
    return RunStatus::kSuccess;
  } catch (const std::exception& exception) {
    output << "error: " << exception.what() << '\n';
    return RunStatus::kLoadError;
  }
}

TEST(DynamicLibrarySelectionTest, LoadsFirstLibraryByName) {
  std::istringstream input{std::string(kLibraryOneName)};
  std::ostringstream output;

  const RunStatus status =
    RunSelectedLibrary(input, output, GetExecutableDirectory());

  EXPECT_EQ(status, RunStatus::kSuccess);
  EXPECT_EQ(output.str(), "loaded library version one\n");
}

TEST(DynamicLibrarySelectionTest, LoadsSecondLibraryByName) {
  std::istringstream input{std::string(kLibraryTwoName)};
  std::ostringstream output;

  const RunStatus status =
    RunSelectedLibrary(input, output, GetExecutableDirectory());

  EXPECT_EQ(status, RunStatus::kSuccess);
  EXPECT_EQ(output.str(), "loaded library version two\n");
}

TEST(DynamicLibrarySelectionTest, LoadsLibraryByDecoratedFileName) {
  const std::string file_name =
    boost::dll::shared_library::decorate(std::string(kLibraryOneName))
      .filename()
      .string();

  std::istringstream input{file_name};
  std::ostringstream output;

  const RunStatus status =
    RunSelectedLibrary(input, output, GetExecutableDirectory());

  EXPECT_EQ(status, RunStatus::kSuccess);
  EXPECT_EQ(output.str(), "loaded library version one\n");
}

TEST(DynamicLibrarySelectionTest, ReportsLoadingError) {
  std::istringstream input{"missing_library"};
  std::ostringstream output;

  const RunStatus status =
    RunSelectedLibrary(input, output, GetExecutableDirectory());

  EXPECT_EQ(status, RunStatus::kLoadError);
  EXPECT_NE(output.str().find("error: "), std::string::npos);
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
