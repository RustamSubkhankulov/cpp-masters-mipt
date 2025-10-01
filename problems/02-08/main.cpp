/*
Реализуйте алгоритм вычисления наибольшей длины последовательности Коллатца
среди всех последова- тельностей, начинающихся со значений от 1 до 100.
Используйте тип unsigned long long int для значений последовательностей Коллатца
и стандартный псевдоним std::std::size_t для их длин. Используйте кэширова- ние
длин последовательностей Коллатца в стандартном контейнере std::vector для
оптимизации вычисления длины каждой новой последовательности на основе
предыдущих последовательностей. Используйте стандарт- ный символьный поток
вывода std::cout для вывода наибольшей длины последовательности Коллатца среди
всех рассмотренных, а также ее начального значения. Не сопровождайте Ваше
решение данной задачи тестами.
*/

#include <iostream>
#include <stdexcept>
#include <vector>

std::size_t collatzSequenceLength(unsigned long long int n,
                                  std::vector<std::size_t>& cache) {
  if (n == 1) {
    return 1;
  }

  if (n < cache.size() && cache[n] != 0) {
    return cache[n];
  }

  unsigned long long int next = (n % 2 == 0) ? n / 2 : 3 * n + 1;
  std::size_t length = 1U + collatzSequenceLength(next, cache);

  if (n >= cache.size() && n < cache.max_size()) {
    try {
      cache.resize(n, 0);
    } catch (const std::length_error& exc) {
      std::cerr << exc.what() << std::endl;
    }
  }

  if (n < cache.size()) {
    cache[n] = length;
  }

  return length;
}

int main() {
  constexpr std::size_t maxValue = 100U;
  constexpr std::size_t initCacheSize = 10000U;

  std::vector<std::size_t> cache(initCacheSize, 0);

  std::size_t highestLen = 0;
  unsigned long long int bestVal = 0;

  for (unsigned long long int start = 1; start <= maxValue; ++start) {
    std::size_t length = collatzSequenceLength(start, cache);

    if (length > highestLen) {
      highestLen = length;
      bestVal = start;
    }
  }

  std::cout << "Winnes is " << bestVal << " with len of " << highestLen
            << std::endl;
  return 0;
}
