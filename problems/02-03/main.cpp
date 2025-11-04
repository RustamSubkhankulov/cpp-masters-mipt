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
    case ',':
    case ';':
    case ':':
    case '!':
    case '?':
    case '"':
    case '(':
    case ')':
    case '[':
    case ']':
    case '-':
    case '*':
    case '/':
    case '`':
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
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
      std::cout << "Uppercase letter\n";
      break;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      std::cout << "Lowercase letter\n";
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      std::cout << "Digit\n";
      break;

    case '.':
    case ',':
    case ';':
    case ':':
    case '!':
    case '?':
    case '"':
    case '(':
    case ')':
    case '[':
    case ']':
    case '-':
    case '*':
    case '/':
    case '`':
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
