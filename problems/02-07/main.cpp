#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>

int main() {
  unsigned elemsNumber;
  std::cin >> elemsNumber;

  if (std::cin.fail() || elemsNumber == 0) {
    std::cerr << "Invalid input" << std::endl;
    return EXIT_FAILURE;
  }

  auto numbers = std::make_unique<double[]>(elemsNumber);

  for (unsigned ind = 0; std::cin.good() && ind < elemsNumber; ++ind) {
    std::cin >> numbers[ind];
  }

  if (std::cin.fail()) {
    std::cerr << "Invalid input" << std::endl;
    return EXIT_FAILURE;
  }

  double minVal = numbers[0];
  double maxVal = numbers[0];
  double summ = numbers[0];

  for (unsigned ind = 1; ind < elemsNumber; ++ind) {

    const auto& curNum = numbers[ind];

    minVal = std::min(minVal, curNum);
    maxVal = std::max(maxVal, curNum);

    summ += curNum;
  }

  double mean = summ / elemsNumber;

  double variance = 0.;
  for (unsigned ind = 0; ind < elemsNumber; ++ind) {
    const auto& curNum = numbers[ind];
    auto tmp = curNum - mean;
    variance += tmp * tmp;
  }
  double stdDev = std::sqrt(variance / elemsNumber);

  // Вывод результатов
  std::cout << "Min: " << minVal << std::endl;
  std::cout << "Max: " << maxVal << std::endl;
  std::cout << "Mean: " << mean << std::endl;
  std::cout << "Standard deviation: " << stdDev << std::endl;

  return 0;
}
