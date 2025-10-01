/*
Реализуйте алгоритмы вычисления максимального и минимального значений, среднего
арифметического и стандартного отклонения коллекции чисел типа double.
Используйте встроенный статический массив. Исполь- зуйте стандартный символьный
поток ввода std::cin для ввода коллекции чисел. Используйте стандартный
символьный поток вывода std::cout для вывода максимального и минимального
значений, среднего арифмети- ческого и стандартного отклонения коллекции чисел.
Не сопровождайте Ваше решение данной задачи тестами.
*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

constexpr std::size_t cInputMaxSize = 1000U;

} // namespace

int main() {
  double numbers[cInputMaxSize];
  unsigned elemsNumber;
  std::cin >> elemsNumber;

  if (std::cin.fail() || elemsNumber == 0 || elemsNumber > cInputMaxSize) {
    std::cerr << "Invalid input" << std::endl;
    return EXIT_FAILURE;
  }

  for (unsigned ind = 0; std::cin.good() && ind < elemsNumber; ++ind) {
    std::cin >> numbers[ind];
  }

  if (std::cin.fail()) {
    std::cerr << "Invalid input" << std::endl;
    return EXIT_FAILURE;
  }

  double minVal = std::numeric_limits<double>::max();
  double maxVal = std::numeric_limits<double>::lowest();
  double summ = 0.;

  for (unsigned ind = 0; ind < elemsNumber; ++ind) {

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
