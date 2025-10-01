/*
Реализуйте алгоритм классификации символов типа char из таблицы ASCII с
десятичными кодами от 32 до 127 включительно на пять следующих классов:
заглавные буквы, строчные буквы, десятичные цифры, знаки препинания, прочие
символы. Используйте ветвление switch с проваливанием и символьными литералами
типа char в качестве меток в секциях case. Протестируйте нестандартное
расширение компилятора g++ для диапазонов и флаг компилятора pedantic.
Используйте секцию default для пятого класса. Используйте стандартный символьный
поток ввода std::cin для ввода символов. Используйте стандартный символьный по-
ток вывода std::cout для вывода названий классов. Не сопровождайте Ваше решение
данной задачи тестами.
*/

#include <iostream>

// #define PROBLEM_02_03_USE_GCC_EXT

int main() {
  char c;
  while (std::cin.get(c)) {

#ifdef PROBLEM_02_03_USE_GCC_EXT
    switch (c) {
    // warning: use of GNU case range extension
    case 'A' ... 'Z':
      std::cout << "Uppercase letter\n";
      break;

    // warning: use of GNU case range extension
    case 'a' ... 'z':
      std::cout << "Lowercase letter\n";
      break;

    // warning: use of GNU case range extension
    case '0' ... '9':
      std::cout << "Digit\n";
      break;

    case '.':
      [[fallthrough]];
    case ',':
      [[fallthrough]];
    case ';':
      [[fallthrough]];
    case ':':
      [[fallthrough]];
    case '!':
      [[fallthrough]];
    case '?':
      [[fallthrough]];
    case '"':
      [[fallthrough]];
    case '(':
      [[fallthrough]];
    case ')':
      [[fallthrough]];
    case '[':
      [[fallthrough]];
    case ']':
      [[fallthrough]];
    case '-':
      [[fallthrough]];
    case '*':
      [[fallthrough]];
    case '/':
      [[fallthrough]];
    case '`':
      [[fallthrough]];
    case '\'':
      std::cout << "Punctuation\n";
      break;

    default:
      std::cout << "Other\n";
      break;
    }
  }
#else
    switch (c) {
    case 'A':
      [[fallthrough]];
    case 'B':
      [[fallthrough]];
    case 'C':
      [[fallthrough]];
    case 'D':
      [[fallthrough]];
    case 'E':
      [[fallthrough]];
    case 'F':
      [[fallthrough]];
    case 'G':
      [[fallthrough]];
    case 'H':
      [[fallthrough]];
    case 'I':
      [[fallthrough]];
    case 'J':
      [[fallthrough]];
    case 'K':
      [[fallthrough]];
    case 'L':
      [[fallthrough]];
    case 'M':
      [[fallthrough]];
    case 'N':
      [[fallthrough]];
    case 'O':
      [[fallthrough]];
    case 'P':
      [[fallthrough]];
    case 'Q':
      [[fallthrough]];
    case 'R':
      [[fallthrough]];
    case 'S':
      [[fallthrough]];
    case 'T':
      [[fallthrough]];
    case 'U':
      [[fallthrough]];
    case 'V':
      [[fallthrough]];
    case 'W':
      [[fallthrough]];
    case 'X':
      [[fallthrough]];
    case 'Y':
      [[fallthrough]];
    case 'Z':
      std::cout << "Uppercase letter\n";
      break;

    case 'a':
      [[fallthrough]];
    case 'b':
      [[fallthrough]];
    case 'c':
      [[fallthrough]];
    case 'd':
      [[fallthrough]];
    case 'e':
      [[fallthrough]];
    case 'f':
      [[fallthrough]];
    case 'g':
      [[fallthrough]];
    case 'h':
      [[fallthrough]];
    case 'i':
      [[fallthrough]];
    case 'j':
      [[fallthrough]];
    case 'k':
      [[fallthrough]];
    case 'l':
      [[fallthrough]];
    case 'm':
      [[fallthrough]];
    case 'n':
      [[fallthrough]];
    case 'o':
      [[fallthrough]];
    case 'p':
      [[fallthrough]];
    case 'q':
      [[fallthrough]];
    case 'r':
      [[fallthrough]];
    case 's':
      [[fallthrough]];
    case 't':
      [[fallthrough]];
    case 'u':
      [[fallthrough]];
    case 'v':
      [[fallthrough]];
    case 'w':
      [[fallthrough]];
    case 'x':
      [[fallthrough]];
    case 'y':
      [[fallthrough]];
    case 'z':
      std::cout << "Lowercase letter\n";
      break;

    case '0':
      [[fallthrough]];
    case '1':
      [[fallthrough]];
    case '2':
      [[fallthrough]];
    case '3':
      [[fallthrough]];
    case '4':
      [[fallthrough]];
    case '5':
      [[fallthrough]];
    case '6':
      [[fallthrough]];
    case '7':
      [[fallthrough]];
    case '8':
      [[fallthrough]];
    case '9':
      std::cout << "Digit\n";
      break;

    case '.':
      [[fallthrough]];
    case ',':
      [[fallthrough]];
    case ';':
      [[fallthrough]];
    case ':':
      [[fallthrough]];
    case '!':
      [[fallthrough]];
    case '?':
      [[fallthrough]];
    case '"':
      [[fallthrough]];
    case '(':
      [[fallthrough]];
    case ')':
      [[fallthrough]];
    case '[':
      [[fallthrough]];
    case ']':
      [[fallthrough]];
    case '-':
      [[fallthrough]];
    case '*':
      [[fallthrough]];
    case '/':
      [[fallthrough]];
    case '`':
      [[fallthrough]];
    case '\'':
      std::cout << "Punctuation\n";
      break;

    default:
      std::cout << "Other\n";
      break;
    }
  }
#endif // PROBLEM_02_03_USE_GCC_EXT

  return 0;
}
