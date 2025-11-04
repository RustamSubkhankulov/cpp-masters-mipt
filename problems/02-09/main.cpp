#include <iostream>
#include <numeric>
#include <vector>
#include <utility>

// Greatest Common Divisor (recursive)
int gcd_recursive(int a, int b) {
    if (b == 0) {
        return a;
    }
    return gcd_recursive(b, a % b);
}

// Greatest Common Divisor (iterative)
int gcd_iterative(int a, int b) {
    while (b != 0) {
        auto temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

// Least Common Multiple
int lcm(int a, int b) {
    return (a / gcd_iterative(a, b)) * b;
}

int main() {
    std::vector<std::pair<int, int>> tests = {
        {24, 36},
        {12, 18},
        {7, 13},
        {21, 6},
        {100, 25},
        {48, 180}
    };

    for (const auto& [a, b] : tests) {
        int gcd_res_rec = gcd_recursive(a, b);
        int gcd_res_it = gcd_iterative(a, b);
        int lcm_res = lcm(a, b);

        int g_std = std::gcd(a, b);
        int l_std = std::lcm(a, b);

        std::cout << "Pair (" << a << ", " << b << "):\n";
        std::cout << "\t" << "GCD (recursive): " << gcd_res_rec << '\n';
        std::cout << "\t" << "GCD (iterative): " << gcd_res_it << '\n';
        std::cout << "\t" << "LCM: " << lcm_res << '\n';

        std::cout << "\t" << "std::gcd = " << g_std << '\n';
        std::cout << "\t" << "std::lcm = " << l_std << '\n';

        bool ok = (gcd_res_rec == g_std) && (gcd_res_it == g_std) && (lcm_res == l_std);
        std::cout << "\t" << (ok ? "Match" : "Mismatch") << "\n";
    }

    return 0;
}
