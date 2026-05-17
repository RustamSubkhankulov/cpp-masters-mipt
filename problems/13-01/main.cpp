#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

bool IsLowercaseHexDigit(char symbol) {
  const char zero = '0';
  const char nine = '9';
  const char letter_a = 'a';
  const char letter_f = 'f';

  return (zero <= symbol && symbol <= nine) ||
         (letter_a <= symbol && symbol <= letter_f);
}

std::uint8_t HexDigitValue(char symbol) {
  const char zero = '0';
  const char nine = '9';
  const char letter_a = 'a';
  const int decimal_digits_count = 10;

  if (!IsLowercaseHexDigit(symbol)) {
    throw std::invalid_argument("invalid hexadecimal digit");
  }

  if (zero <= symbol && symbol <= nine) {
    return static_cast<std::uint8_t>(symbol - zero);
  }

  return static_cast<std::uint8_t>(decimal_digits_count + symbol - letter_a);
}

std::string BytesToHexString(const std::vector<std::uint8_t>& bytes) {
  const int byte_width = 2;
  const char fill_symbol = '0';

  std::stringstream output;
  output << std::hex << std::right << std::setfill(fill_symbol);

  for (const std::uint8_t byte : bytes) {
    output << std::setw(byte_width) << static_cast<unsigned int>(byte);
  }

  return output.str();
}

std::vector<std::uint8_t> HexStringToBytes(const std::string& text) {
  const std::size_t digits_per_byte = 2;
  const int bits_per_hex_digit = 4;
  const std::size_t high_digit_offset = 0;
  const std::size_t low_digit_offset = 1;

  if (text.size() % digits_per_byte != 0U) {
    throw std::invalid_argument("hexadecimal string length must be even");
  }

  std::vector<std::uint8_t> bytes;
  bytes.reserve(text.size() / digits_per_byte);

  for (std::size_t index = 0; index < text.size(); index += digits_per_byte) {
    const std::uint8_t high = HexDigitValue(text[index + high_digit_offset]);
    const std::uint8_t low = HexDigitValue(text[index + low_digit_offset]);

    bytes.push_back(
      static_cast<std::uint8_t>((high << bits_per_hex_digit) | low));
  }

  return bytes;
}

TEST(ByteHexConversionTests, ConvertsEmptyCollectionToEmptyString) {
  const std::vector<std::uint8_t> bytes;

  EXPECT_EQ(BytesToHexString(bytes), "");
}

TEST(ByteHexConversionTests, PadsSingleDigitBytesWithZero) {
  const std::vector<std::uint8_t> bytes{std::uint8_t{0x00}, std::uint8_t{0x01},
                                        std::uint8_t{0x0a}, std::uint8_t{0x0f}};

  EXPECT_EQ(BytesToHexString(bytes), "00010a0f");
}

TEST(ByteHexConversionTests, ConvertsBytesToLowercaseHexString) {
  const std::vector<std::uint8_t> bytes{std::uint8_t{0x10}, std::uint8_t{0xab},
                                        std::uint8_t{0xcd}, std::uint8_t{0xef},
                                        std::uint8_t{0xff}};

  EXPECT_EQ(BytesToHexString(bytes), "10abcdefff");
}

TEST(ByteHexConversionTests, ConvertsLowercaseHexStringToBytes) {
  const std::vector<std::uint8_t> expected{
    std::uint8_t{0x00}, std::uint8_t{0x01}, std::uint8_t{0x7f},
    std::uint8_t{0x80}, std::uint8_t{0xab}, std::uint8_t{0xff}};

  EXPECT_EQ(HexStringToBytes("00017f80abff"), expected);
}

TEST(ByteHexConversionTests, RoundTripPreservesAllByteValues) {
  const int values_count = 256;

  std::vector<std::uint8_t> bytes;
  bytes.reserve(values_count);

  for (int value = 0; value < values_count; ++value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
  }

  EXPECT_EQ(HexStringToBytes(BytesToHexString(bytes)), bytes);
}

TEST(ByteHexConversionTests, RejectsOddLengthString) {
  EXPECT_THROW(HexStringToBytes("abc"), std::invalid_argument);
}

TEST(ByteHexConversionTests, RejectsUppercaseHexDigits) {
  EXPECT_THROW(HexStringToBytes("0A"), std::invalid_argument);
}

TEST(ByteHexConversionTests, RejectsNonHexDigits) {
  EXPECT_THROW(HexStringToBytes("0g"), std::invalid_argument);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
