#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string MakeTraceMessage(std::string_view phase,
                             std::string_view function_name,
                             std::string_view file_name, unsigned int line) {
  std::ostringstream stream;
  stream << "[trace] " << phase << " function=\"" << function_name << '"'
         << " file=\"" << file_name << '"' << " line=" << line << '\n';
  return stream.str();
}

std::string MakeTraceMessage(std::string_view phase,
                             const std::source_location& location) {
  return MakeTraceMessage(phase, location.function_name(), location.file_name(),
                          location.line());
}

class Tracer final {
public:
  explicit Tracer(
    const std::source_location& location = std::source_location::current())
    : location_(location) {
    Print("enter");
  }

  Tracer(const Tracer&) = delete;
  Tracer& operator=(const Tracer&) = delete;
  Tracer(Tracer&&) = delete;
  Tracer& operator=(Tracer&&) = delete;

  ~Tracer() noexcept {
    try {
      Print("leave");
    } catch (...) {
    }
  }

private:
  void Print(std::string_view phase) const {
    std::cout << MakeTraceMessage(phase, location_);
  }

  std::source_location location_;
};

#define TRACER_CONCAT_IMPL(left, right) left##right
#define TRACER_CONCAT(left, right) TRACER_CONCAT_IMPL(left, right)

#ifdef NDEBUG
#  define trace() ((void)0)
#else
#  define trace()                                                     \
    [[maybe_unused]] ::Tracer TRACER_CONCAT(tracer_instance_at_line_, \
                                            __LINE__) {               \
      std::source_location::current()                                 \
    }
#endif

template <class Function>
std::string CaptureStandardOutput(Function function) {
  std::ostringstream buffer;
  std::streambuf* const old_buffer = std::cout.rdbuf(buffer.rdbuf());

  try {
    function();
  } catch (...) {
    std::cout.rdbuf(old_buffer);
    throw;
  }

  std::cout.rdbuf(old_buffer);
  return buffer.str();
}

std::size_t FindTraceLinePosition(const std::string& output,
                                  std::string_view phase,
                                  std::string_view function_fragment) {
  const std::string prefix = "[trace] " + std::string(phase) + " function=\"";
  std::size_t position = output.find(prefix);

  while (position != std::string::npos) {
    const std::size_t line_end = output.find('\n', position);
    const std::string line = output.substr(position, line_end - position);

    if (line.find(function_fragment) != std::string::npos) {
      return position;
    }

    position = output.find(prefix, position + 1U);
  }

  return std::string::npos;
}

std::string CaptureDefaultConstructorOutput(std::string* expected_output) {
  std::ostringstream buffer;
  std::streambuf* const old_buffer = std::cout.rdbuf(buffer.rdbuf());

  {
    const auto current = std::source_location::current();
    const unsigned int constructor_line = __LINE__ + 1U;
    Tracer tracer;

    if (expected_output != nullptr) {
      *expected_output =
        MakeTraceMessage("enter", current.function_name(), current.file_name(),
                         constructor_line) +
        MakeTraceMessage("leave", current.function_name(), current.file_name(),
                         constructor_line);
    }
  }

  std::cout.rdbuf(old_buffer);
  return buffer.str();
}

std::string CaptureTraceMacroOutput() {
  return CaptureStandardOutput([] { trace(); });
}

void ExampleLeaf() {
  trace();
  std::cout << "leaf body\n";
}

void ExampleRoot() {
  trace();
  std::cout << "root body begin\n";
  ExampleLeaf();
  std::cout << "root body end\n";
}

void ExampleThrowingFunction() {
  trace();
  std::cout << "before exception\n";
  throw std::runtime_error("failure");
}

TEST(TracerTest, DefaultConstructorAndDestructorPrintPairedMessages) {
  std::string expected_output;
  const std::string output = CaptureDefaultConstructorOutput(&expected_output);
  EXPECT_EQ(output, expected_output);
}

TEST(TracerTest, TraceMacroCanBeDisabledByNDebug) {
  const std::string output = CaptureTraceMacroOutput();

#ifdef NDEBUG
  EXPECT_TRUE(output.empty());
#else
  const std::size_t enter_position =
    FindTraceLinePosition(output, "enter", "CaptureTraceMacroOutput");
  const std::size_t leave_position =
    FindTraceLinePosition(output, "leave", "CaptureTraceMacroOutput");

  ASSERT_NE(enter_position, std::string::npos);
  ASSERT_NE(leave_position, std::string::npos);
  EXPECT_LT(enter_position, leave_position);
#endif
}

TEST(TracerTest, NestedTracingProducesStackLikeOrder) {
  const std::string output = CaptureStandardOutput(ExampleRoot);

#ifdef NDEBUG
  EXPECT_EQ(output, "root body begin\nleaf body\nroot body end\n");
#else
  const std::size_t root_enter =
    FindTraceLinePosition(output, "enter", "ExampleRoot");
  const std::size_t leaf_enter =
    FindTraceLinePosition(output, "enter", "ExampleLeaf");
  const std::size_t leaf_leave =
    FindTraceLinePosition(output, "leave", "ExampleLeaf");
  const std::size_t root_leave =
    FindTraceLinePosition(output, "leave", "ExampleRoot");

  const std::size_t root_body_begin = output.find("root body begin\n");
  const std::size_t leaf_body = output.find("leaf body\n");
  const std::size_t root_body_end = output.find("root body end\n");

  ASSERT_NE(root_enter, std::string::npos);
  ASSERT_NE(leaf_enter, std::string::npos);
  ASSERT_NE(leaf_leave, std::string::npos);
  ASSERT_NE(root_leave, std::string::npos);
  ASSERT_NE(root_body_begin, std::string::npos);
  ASSERT_NE(leaf_body, std::string::npos);
  ASSERT_NE(root_body_end, std::string::npos);

  EXPECT_LT(root_enter, root_body_begin);
  EXPECT_LT(root_body_begin, leaf_enter);
  EXPECT_LT(leaf_enter, leaf_body);
  EXPECT_LT(leaf_body, leaf_leave);
  EXPECT_LT(leaf_leave, root_body_end);
  EXPECT_LT(root_body_end, root_leave);
#endif
}

TEST(TracerTest, RAIIKeepsPairingEvenWhenExceptionIsThrown) {
  std::ostringstream buffer;
  std::streambuf* const old_buffer = std::cout.rdbuf(buffer.rdbuf());

  try {
    ExampleThrowingFunction();
    std::cout.rdbuf(old_buffer);
    FAIL() << "Exception was expected";
  } catch (const std::runtime_error& error) {
    std::cout.rdbuf(old_buffer);
    EXPECT_STREQ(error.what(), "failure");
  } catch (...) {
    std::cout.rdbuf(old_buffer);
    throw;
  }

  const std::string output = buffer.str();

#ifdef NDEBUG
  EXPECT_EQ(output, "before exception\n");
#else
  const std::size_t enter_position =
    FindTraceLinePosition(output, "enter", "ExampleThrowingFunction");
  const std::size_t message_position = output.find("before exception\n");
  const std::size_t leave_position =
    FindTraceLinePosition(output, "leave", "ExampleThrowingFunction");

  ASSERT_NE(enter_position, std::string::npos);
  ASSERT_NE(message_position, std::string::npos);
  ASSERT_NE(leave_position, std::string::npos);

  EXPECT_LT(enter_position, message_position);
  EXPECT_LT(message_position, leave_position);
#endif
}

} // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
