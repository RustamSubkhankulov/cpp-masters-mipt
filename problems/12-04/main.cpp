#include <gtest/gtest.h>

#include <iterator>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

struct EmailMatch {
  std::string address;
  std::string domain;

  bool operator==(EmailMatch const&) const = default;
};

std::vector<EmailMatch> ExtractEmailMatches(std::string const& text) {
  std::regex const email_pattern(
    R"([A-Za-z0-9._%+-]+@([A-Za-z0-9.-]+\.[A-Za-z]{2,}))");

  std::vector<EmailMatch> matches;

  std::sregex_token_iterator current(text.cbegin(), text.cend(), email_pattern,
                                     std::vector<int>{0, 1});

  std::sregex_token_iterator const end;

  while (current != end) {
    EmailMatch match;
    match.address = *current;
    ++current;

    if (current == end) {
      break;
    }

    match.domain = *current;
    ++current;

    matches.push_back(match);
  }

  return matches;
}

std::vector<std::string> ExtractEmailAddresses(std::string const& text) {
  std::vector<std::string> addresses;

  for (EmailMatch const& match : ExtractEmailMatches(text)) {
    addresses.push_back(match.address);
  }

  return addresses;
}

std::vector<std::string> ExtractEmailDomains(std::string const& text) {
  std::vector<std::string> domains;

  for (EmailMatch const& match : ExtractEmailMatches(text)) {
    domains.push_back(match.domain);
  }

  return domains;
}

TEST(EmailExtractionTests, ExtractsAddressesAndDomainsFromText) {
  std::string const text = R"(
        Contact first.user@example.com and support-42@mail.company.org.
        Backup address: admin_01@test.net.
    )";

  std::vector<EmailMatch> const expected = {
    {"first.user@example.com", "example.com"},
    {"support-42@mail.company.org", "mail.company.org"},
    {"admin_01@test.net", "test.net"}};

  EXPECT_EQ(ExtractEmailMatches(text), expected);
}

TEST(EmailExtractionTests, ExtractsOnlyAddresses) {
  std::string const text = R"(
        Send results to alpha@example.com, beta.user@sub.domain.io.
    )";

  std::vector<std::string> const expected = {"alpha@example.com",
                                             "beta.user@sub.domain.io"};

  EXPECT_EQ(ExtractEmailAddresses(text), expected);
}

TEST(EmailExtractionTests, ExtractsOnlyDomainsUsingGroup) {
  std::string const text = R"(
        Owners: one@first.com; two@second.org; three@deep.third.net.
    )";

  std::vector<std::string> const expected = {"first.com", "second.org",
                                             "deep.third.net"};

  EXPECT_EQ(ExtractEmailDomains(text), expected);
}

TEST(EmailExtractionTests, IgnoresTextWithoutEmails) {
  std::string const text = R"(
        This text contains names, numbers, and punctuation, but no email address.
    )";

  EXPECT_TRUE(ExtractEmailMatches(text).empty());
  EXPECT_TRUE(ExtractEmailAddresses(text).empty());
  EXPECT_TRUE(ExtractEmailDomains(text).empty());
}

TEST(EmailExtractionTests, IgnoresIncompleteAddresses) {
  std::string const text = R"(
        Invalid examples: user@, @example.com, user@example, user.name@.
        Valid example: correct.user@example.com.
    )";

  std::vector<EmailMatch> const expected = {
    {"correct.user@example.com", "example.com"}};

  EXPECT_EQ(ExtractEmailMatches(text), expected);
}

TEST(EmailExtractionDemo, DemonstratesTypicalUsage) {
  std::string const text = R"(
        Personal: john.smith@gmail.com
        Work: john_smith@research.example.edu
    )";

  std::vector<EmailMatch> const matches = ExtractEmailMatches(text);

  ASSERT_EQ(matches.size(), 2U);

  EXPECT_EQ(matches[0].address, "john.smith@gmail.com");
  EXPECT_EQ(matches[0].domain, "gmail.com");

  EXPECT_EQ(matches[1].address, "john_smith@research.example.edu");
  EXPECT_EQ(matches[1].domain, "research.example.edu");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
