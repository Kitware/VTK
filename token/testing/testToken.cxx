//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "token/Token.h"
#include "token/json/jsonToken.h"

#include "token/testing/helpers.h"

#include <iostream>
#include <set>
#include <unordered_set>

int testToken(int, char*[])
{
  using namespace token_NAMESPACE::literals; // for ""_token
  using json = nlohmann::json;

  // Test that construction from a std::string works.
  std::string dab = "bad";
  token_NAMESPACE::Token bad = dab;

  // Test that construction from a string literal works.
  token_NAMESPACE::Token tmp = "tmp";

  // Test that construction from our string literal operator works.
  auto foo = "foo"_token;
  auto bar = "bar"_token;
  auto oof = "foo"_token;

  std::cout << "Testing comparison operators for:\n";
  std::cout << "  " << bad.data() << " "
            << "0x" << std::hex << bad.getId() << std::dec << "\n";
  std::cout << "  " << tmp.data() << " "
            << "0x" << std::hex << tmp.getId() << std::dec << "\n";
  std::cout << "  " << foo.data() << " "
            << "0x" << std::hex << foo.getId() << std::dec << "\n";
  std::cout << "  " << oof.data() << " "
            << "0x" << std::hex << oof.getId() << std::dec << "\n";
  std::cout << "  " << bar.data() << " "
            << "0x" << std::hex << bar.getId() << std::dec << "\n";

  // Ensure we have string in the manager for our test strings.
  // (They are not inserted when the ""_token string-literal operator
  // is used.)
  token_NAMESPACE::Token dummy = "foo";
  dummy = "bar";
  // Test comparison operators
  test(foo == oof, "String comparison incorrect.");
  test(bar != foo, "String comparison incorrect.");
  test(bar <= foo, "String lexical order must be preserved.");
  test(foo >= bar, "String lexical order must be preserved.");
  test(bar < foo, "String lexical order must be preserved.");
  test(foo > bar, "String lexical order must be preserved.");

  // Test string-literal comparison operators
  test("foo" == oof, "String comparison incorrect.");
  test("bar" != foo, "String comparison incorrect.");
  test("bar" <= foo, "String lexical order must be preserved.");
  test("foo" >= bar, "String lexical order must be preserved.");
  test("bar" < foo, "String lexical order must be preserved.");
  test("foo" > bar, "String lexical order must be preserved.");

  test(foo == "foo", "String comparison incorrect.");
  test(bar != "foo", "String comparison incorrect.");
  test(bar <= "foo", "String lexical order must be preserved.");
  test(foo >= "bar", "String lexical order must be preserved.");
  test(bar < "foo", "String lexical order must be preserved.");
  test(foo > "bar", "String lexical order must be preserved.");

  // Test hash functor (verify that unordered containers are supported).
  std::unordered_set<token_NAMESPACE::Token> set;
  set.insert("foo"_token);
  set.insert(foo);
  set.insert(bar);
  set.insert("baz");
  set.insert(bad);
  set.insert(tmp);
  test(set.size() == 5, "Expected set to have 5 members.");

  // Test construction from a hash.
  token_NAMESPACE::Token foo2 = token_NAMESPACE::Token(foo.getId());
  (void)foo2;

  // Test json serialization/deserialization.
  json j = foo;
  bar = j;
  test(bar.data() == foo.data(), "Expected JSON assignment to work.");

#if __cplusplus >= 201402L
  // Test constexpr-ness (i.e., that tokens can be used in switch statements).
  // Really, the test here is at compile time; it ensures that no one
  // breaks anything constexpr as needed for the switch() use-case:
  //
  // Note: This only works for C++14 or later; it is untested for older platforms.
  std::cout << "Testing switch statement with hash cases: ";
  bool ok = false;
  switch (foo.getId())
  {
    case "foo"_hash:
      ok = true;
      break;
    case "bar"_hash:
      ok = false;
      break;
    default:
      ok = false;
      break;
  }
  std::cout << (ok ? "pass" : "fail") << "\n";
  test(ok, "Expected switch statement to work.");
#endif

  // Test that set<Token> works as expected (with slow alphanumeric sorting).
  std::cout << "Testing slow alphanumeric sorting for set<Token>:\n";
  std::set<token_NAMESPACE::Token> candies;
  std::vector<std::string> expected{
    { "gumdrop", "mike&ike", "pixie_stick", "tootsie_roll", "twinkie" }
  };
  // Insert in reverse order just to be sure sorting occurs:
  candies.insert(expected.rbegin(), expected.rend());
  auto expectedCandy = expected.begin();
  for (const auto& candy : candies)
  {
    std::cout << "  " << std::hex << candy.getId() << std::dec << ":  " << candy.data() << "\n";
    test(candy.data() == *expectedCandy, "Unexpected order for sorted tokens.");
    ++expectedCandy;
  }

  token_NAMESPACE::Token naughty(token_NAMESPACE::Invalid());
  token_NAMESPACE::Token uninitialized;
  test(!naughty.valid(), "Improper validity check.");
  test(!uninitialized.valid(), "Uninitialized tokens should be invalid.");

  token_NAMESPACE::Token emptyString("");
  std::cout << "Empty string has ID " << std::hex << emptyString.getId() << "\n";
  test(emptyString.getId() == token_NAMESPACE::Invalid(), "Expected zero-length string to have invalid hash.");
  return 0;
}
