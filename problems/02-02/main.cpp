#include <cmath>
#include <iostream>

namespace {

const double epsilon = 1e-10;

} // namespace

auto normilizeNegZeroIfRequired(double arg) {
  auto abs = std::abs(arg);
  if (abs < epsilon) {
    return abs;
  }
  return arg;
}

int main() {
  double a, b, c;

  std::cin >> a >> b >> c;

  if (std::abs(a) < epsilon) {
    // Linear equation bx + c = 0
    if (std::abs(b) < epsilon) {

      if (std::abs(c) < epsilon) {
        std::cout << "R - all real numbers are solutions" << std::endl;

      } else {
        std::cout << "No solution" << std::endl;
      }
    } else {
      auto root = -c / b;
      root = normilizeNegZeroIfRequired(root);

      std::cout << root << std::endl;
    }
  } else {
    // Quadratic equation
    auto discriminant = b * b - 4 * a * c;

    if (discriminant > epsilon) {

      auto sqrt_d = std::sqrt(discriminant);

      auto root1 = (-b - sqrt_d) / (2 * a);
      auto root2 = (-b + sqrt_d) / (2 * a);

      root1 = normilizeNegZeroIfRequired(root1);
      root2 = normilizeNegZeroIfRequired(root2);

      std::cout << root1 << " " << root2 << std::endl;

    } else if (discriminant < -epsilon) {
      std::cout << "No real roots - only complex" << std::endl;

    } else {
      auto root = -b / (2 * a);
      root = normilizeNegZeroIfRequired(root);
      std::cout << root << std::endl;
    }
  }

  return 0;
}