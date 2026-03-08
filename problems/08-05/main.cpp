#include <chrono>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

class Timer {
public:
  using duration_t = std::chrono::duration<double>;

  void start() {
    if (is_running_) {
      throw std::logic_error("Timer is already running.");
    }

    begin_ = clock_t::now();
    is_running_ = true;
  }

  void stop() {
    if (!is_running_) {
      throw std::logic_error("Timer is not running.");
    }

    measurements_.push_back(
      std::chrono::duration_cast<duration_t>(clock_t::now() - begin_));
    is_running_ = false;
  }

  [[nodiscard]] double average() const {
    if (measurements_.empty()) {
      return 0.0;
    }

    const duration_t total = std::accumulate(
      measurements_.begin(), measurements_.end(), duration_t::zero());

    return total.count() / static_cast<double>(measurements_.size());
  }

  [[nodiscard]] bool is_running() const noexcept {
    return is_running_;
  }

  [[nodiscard]] std::size_t measurement_count() const noexcept {
    return measurements_.size();
  }

private:
  using clock_t = std::chrono::steady_clock;

  bool is_running_ = false;
  clock_t::time_point begin_{};
  std::vector<duration_t> measurements_{};
};

double calculate(std::size_t size) {
  double result = 0.0;

  for (std::size_t index = 0; index < size; ++index) {
    const double value = static_cast<double>(index);
    const double sine_value = std::sin(value);
    const double cosine_value = std::cos(value);
    result += (sine_value * sine_value) + (cosine_value * cosine_value);
  }

  return result;
}

bool almost_equal(double lhs, double rhs) {
  constexpr double kEpsilon = 1e-6;
  return std::abs(lhs - rhs) < kEpsilon;
}

TEST(TimerTest, AverageIsZeroWithoutMeasurements) {
  const Timer timer;

  EXPECT_DOUBLE_EQ(timer.average(), 0.0);
  EXPECT_EQ(timer.measurement_count(), 0U);
  EXPECT_FALSE(timer.is_running());
}

TEST(TimerTest, StopWithoutStartThrows) {
  Timer timer;

  EXPECT_THROW(static_cast<void>(timer.stop()), std::logic_error);
  EXPECT_EQ(timer.measurement_count(), 0U);
  EXPECT_FALSE(timer.is_running());
}

TEST(TimerTest, StartWhileRunningThrows) {
  constexpr std::size_t kWorkloadSize = 100000U;

  Timer timer;

  timer.start();
  EXPECT_TRUE(timer.is_running());
  EXPECT_THROW(static_cast<void>(timer.start()), std::logic_error);

  const double result = calculate(kWorkloadSize);
  timer.stop();

  EXPECT_TRUE(almost_equal(result, static_cast<double>(kWorkloadSize)));
  EXPECT_EQ(timer.measurement_count(), 1U);
  EXPECT_FALSE(timer.is_running());
  EXPECT_GT(timer.average(), 0.0);
}

TEST(TimerTest, StartAndStopRecordSingleMeasurement) {
  constexpr std::size_t kWorkloadSize = 150000U;

  Timer timer;

  timer.start();
  const double result = calculate(kWorkloadSize);
  timer.stop();

  EXPECT_TRUE(almost_equal(result, static_cast<double>(kWorkloadSize)));
  EXPECT_EQ(timer.measurement_count(), 1U);
  EXPECT_FALSE(timer.is_running());
  EXPECT_GT(timer.average(), 0.0);
}

TEST(TimerTest, AverageIsComputedAcrossMultipleMeasurements) {
  constexpr std::size_t kRepeatCount = 3U;
  constexpr std::size_t kWorkloadSize = 120000U;

  Timer timer;

  for (std::size_t attempt = 0; attempt < kRepeatCount; ++attempt) {
    timer.start();
    const double result = calculate(kWorkloadSize);
    timer.stop();

    EXPECT_TRUE(almost_equal(result, static_cast<double>(kWorkloadSize)));
  }

  EXPECT_EQ(timer.measurement_count(), kRepeatCount);
  EXPECT_FALSE(timer.is_running());
  EXPECT_GT(timer.average(), 0.0);
}

TEST(TimerTest, HeavierWorkloadHasLargerAverageThanLighterWorkload) {
  constexpr std::size_t kRepeatCount = 5U;
  constexpr std::size_t kLightWorkloadSize = 20000U;
  constexpr std::size_t kHeavyWorkloadSize = 200000U;

  Timer light_timer;
  Timer heavy_timer;

  for (std::size_t attempt = 0; attempt < kRepeatCount; ++attempt) {
    light_timer.start();
    const double light_result = calculate(kLightWorkloadSize);
    light_timer.stop();

    heavy_timer.start();
    const double heavy_result = calculate(kHeavyWorkloadSize);
    heavy_timer.stop();

    EXPECT_TRUE(
      almost_equal(light_result, static_cast<double>(kLightWorkloadSize)));
    EXPECT_TRUE(
      almost_equal(heavy_result, static_cast<double>(kHeavyWorkloadSize)));
  }

  EXPECT_EQ(light_timer.measurement_count(), kRepeatCount);
  EXPECT_EQ(heavy_timer.measurement_count(), kRepeatCount);
  EXPECT_GT(light_timer.average(), 0.0);
  EXPECT_GT(heavy_timer.average(), 0.0);
  EXPECT_GT(heavy_timer.average(), light_timer.average());
}

} // namespace

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
