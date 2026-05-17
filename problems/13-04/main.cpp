#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <ostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

struct EntryInfo {
  char type;
  std::string permissions;
  std::string size;
  std::int64_t modified_seconds;
  std::string name;
};

auto make_type(fs::file_status status) -> char {
  if (fs::is_directory(status)) {
    return 'd';
  }

  if (fs::is_regular_file(status)) {
    return 'f';
  }

  if (fs::is_symlink(status)) {
    return 'l';
  }

  return '?';
}

auto make_permissions(fs::perms permissions) -> std::string {
  auto make_symbol = [permissions](fs::perms bit, char symbol) {
    return (permissions & bit) == fs::perms::none ? '-' : symbol;
  };

  std::string result;
  constexpr std::size_t permission_count = 3U;
  result.reserve(permission_count);

  result.push_back(make_symbol(fs::perms::owner_read, 'r'));
  result.push_back(make_symbol(fs::perms::owner_write, 'w'));
  result.push_back(make_symbol(fs::perms::owner_exec, 'x'));

  return result;
}

auto regular_file_size(fs::path const& path) -> std::uintmax_t {
  std::error_code error;
  auto const result = fs::file_size(path, error);
  return error ? 0U : result;
}

auto directory_size(fs::path const& path) -> std::uintmax_t {
  std::error_code error;

  if (!fs::exists(path, error) || !fs::is_directory(path, error)) {
    return 0U;
  }

  constexpr auto options = fs::directory_options::skip_permission_denied;
  fs::recursive_directory_iterator iterator(path, options, error);
  fs::recursive_directory_iterator end;

  std::uintmax_t result = 0U;

  while (!error && iterator != end) {
    auto const entry_path = iterator->path();
    auto const status = iterator->status(error);

    if (!error && fs::is_regular_file(status)) {
      result += regular_file_size(entry_path);
    }

    iterator.increment(error);
  }

  return result;
}

auto entry_size(fs::directory_entry const& entry, fs::file_status status)
  -> std::uintmax_t {
  if (fs::is_regular_file(status)) {
    return regular_file_size(entry.path());
  }

  if (fs::is_directory(status)) {
    return directory_size(entry.path());
  }

  return 0U;
}

auto format_size(std::uintmax_t bytes) -> std::string {
  constexpr std::uintmax_t bytes_per_unit = 1024U;
  constexpr std::array<char, 4U> units = {'B', 'K', 'M', 'G'};

  std::size_t unit_index = 0U;

  while (unit_index + 1U < units.size() && bytes >= bytes_per_unit) {
    bytes /= bytes_per_unit;
    ++unit_index;
  }

  std::ostringstream stream;
  constexpr int size_width = 4;
  stream << std::setw(size_width) << bytes << " (" << units[unit_index] << ')';

  return stream.str();
}

auto modified_seconds(fs::directory_entry const& entry) -> std::int64_t {
  std::error_code error;
  auto const time = entry.last_write_time(error);

  if (error) {
    return 0;
  }

  using seconds = std::chrono::seconds;
  return std::chrono::duration_cast<seconds>(time.time_since_epoch()).count();
}

auto make_entry_info(fs::directory_entry const& entry, std::regex const& filter)
  -> std::optional<EntryInfo> {
  auto const name = entry.path().filename().string();

  if (!std::regex_search(name, filter)) {
    return std::nullopt;
  }

  std::error_code error;
  auto const status = entry.status(error);

  if (error) {
    return std::nullopt;
  }

  auto const symlink_status = entry.symlink_status(error);
  auto const type_status = error ? status : symlink_status;

  return EntryInfo{
    make_type(type_status), make_permissions(status.permissions()),
    format_size(entry_size(entry, status)), modified_seconds(entry), name};
}

auto collect_matching_entries(fs::path const& path, std::regex const& filter)
  -> std::vector<EntryInfo> {
  std::vector<EntryInfo> entries;
  std::error_code error;

  if (!fs::exists(path, error) || !fs::is_directory(path, error)) {
    return entries;
  }

  constexpr auto options = fs::directory_options::skip_permission_denied;
  fs::directory_iterator iterator(path, options, error);
  fs::directory_iterator end;

  while (!error && iterator != end) {
    auto entry = make_entry_info(*iterator, filter);

    if (entry.has_value()) {
      entries.push_back(*entry);
    }

    iterator.increment(error);
  }

  std::sort(entries.begin(), entries.end(),
            [](EntryInfo const& left, EntryInfo const& right) {
              return left.name < right.name;
            });

  return entries;
}

void show(fs::path const& path, std::string const& pattern,
          std::ostream& output) {
  std::regex const filter(pattern, std::regex_constants::extended);
  auto const entries = collect_matching_entries(path, filter);

  for (auto const& entry : entries) {
    output << "show : entry : " << entry.type << " | " << entry.permissions
           << " | " << entry.size << " | " << entry.modified_seconds << " | "
           << entry.name << '\n';
  }
}

class TemporaryDirectory {
public:
  TemporaryDirectory()
    : path_(make_unique_directory()) {}

  TemporaryDirectory(TemporaryDirectory const&) = delete;
  auto operator=(TemporaryDirectory const&) -> TemporaryDirectory& = delete;

  TemporaryDirectory(TemporaryDirectory&&) = delete;
  auto operator=(TemporaryDirectory&&) -> TemporaryDirectory& = delete;

  ~TemporaryDirectory() {
    std::error_code error;
    fs::remove_all(path_, error);
  }

  [[nodiscard]] auto path() const -> fs::path const& {
    return path_;
  }

private:
  static auto make_unique_directory() -> fs::path {
    auto const base = fs::temp_directory_path();
    auto const tick =
      std::chrono::steady_clock::now().time_since_epoch().count();

    constexpr int max_attempt_count = 1000;

    for (int attempt = 0; attempt < max_attempt_count; ++attempt) {
      auto candidate = base / ("directory_regex_test_" + std::to_string(tick) +
                               "_" + std::to_string(attempt));

      std::error_code error;

      if (fs::create_directory(candidate, error)) {
        return candidate;
      }
    }

    throw std::runtime_error("cannot create temporary directory");
  }

  fs::path path_;
};

void write_file(fs::path const& path, std::string_view content) {
  std::ofstream file(path, std::ios::binary);
  file << content;
}

TEST(DirectoryRegexTests, FiltersEntriesByExtendedRegex) {
  TemporaryDirectory directory;

  write_file(directory.path() / "main.cpp", "int main() {}\n");
  write_file(directory.path() / "notes.txt", "notes\n");
  write_file(directory.path() / "library.hpp", "#pragma once\n");
  fs::create_directory(directory.path() / "build");

  std::ostringstream output;
  show(directory.path(), ".*[.](cpp|hpp)$", output);

  auto const text = output.str();

  EXPECT_NE(text.find("main.cpp"), std::string::npos);
  EXPECT_NE(text.find("library.hpp"), std::string::npos);
  EXPECT_EQ(text.find("notes.txt"), std::string::npos);
  EXPECT_EQ(text.find("build"), std::string::npos);
}

TEST(DirectoryRegexTests, SupportsAnchorsForFullNameMatching) {
  TemporaryDirectory directory;

  write_file(directory.path() / "abc.txt", "match\n");
  write_file(directory.path() / "xabc.txt", "skip\n");
  write_file(directory.path() / "abc.log", "skip\n");

  std::ostringstream output;
  show(directory.path(), "^abc[.]txt$", output);

  auto const text = output.str();

  EXPECT_NE(text.find("abc.txt"), std::string::npos);
  EXPECT_EQ(text.find("xabc.txt"), std::string::npos);
  EXPECT_EQ(text.find("abc.log"), std::string::npos);
}

TEST(DirectoryRegexTests, MatchesDirectoriesByName) {
  TemporaryDirectory directory;

  fs::create_directory(directory.path() / "src");
  write_file(directory.path() / "source.txt", "skip\n");

  std::ostringstream output;
  show(directory.path(), "^src$", output);

  auto const text = output.str();

  EXPECT_NE(text.find("show : entry : d | "), std::string::npos);
  EXPECT_NE(text.find("src"), std::string::npos);
  EXPECT_EQ(text.find("source.txt"), std::string::npos);
}

TEST(DirectoryRegexTests, MissingDirectoryProducesNoOutput) {
  TemporaryDirectory directory;

  std::ostringstream output;
  show(directory.path() / "missing", ".*", output);

  EXPECT_TRUE(output.str().empty());
}

TEST(DirectoryRegexTests, InvalidRegexIsReportedByStdRegex) {
  TemporaryDirectory directory;
  std::ostringstream output;

  EXPECT_THROW(show(directory.path(), "[", output), std::regex_error);
}

TEST(DirectoryRegexDemo, ListsOnlyHeaderAndSourceFiles) {
  TemporaryDirectory directory;

  write_file(directory.path() / "program.cpp", "int main() { return 0; }\n");
  write_file(directory.path() / "program.hpp", "#pragma once\n");
  write_file(directory.path() / "readme.md", "text\n");

  std::ostringstream output;
  show(directory.path(), "program[.](cpp|hpp)$", output);

  auto const text = output.str();

  EXPECT_NE(text.find("program.cpp"), std::string::npos);
  EXPECT_NE(text.find("program.hpp"), std::string::npos);
  EXPECT_EQ(text.find("readme.md"), std::string::npos);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
