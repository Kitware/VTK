#include <token/Token.h>
#include <iostream>
#include <set>
#include <unordered_set>

template<typename Container>
void print_set(const Container& container, const std::string& title)
{
  std::cout << title << "\n";
  for (const auto& item : container)
  {
    std::cout << "  0x" << std::hex << item.getId() << " (" << item.data() << ")\n";
  }
}

int main(int argc, char* argv[])
{
  using namespace token_NAMESPACE;
  using namespace token_NAMESPACE ::literals;
  // Using the less-than comparator causes underlying strings
  // (if they exist) to be sorted. All tokens with no underlying
  // string will evaluate as equivalent to an invalid token and
  // thus have a single container entry among them.
  std::set<Token> sorted{
    // These strings are turned into tokens at run-time:
    "foo", "bar", "baz",
    // These strings are turned into tokens at compile-time:
    "foo"_token, "xyzzy"_token
  };
  print_set(sorted, "sorted (and smushed)");

  // Using the hashing comparator causes token identifiers
  // to be indexed. Tokens with no underlying string will
  // have a separate entry, but it is not safe to assume their
  // underlying strings are unique (because some may be empty):
  std::unordered_set<Token> hashed{
    // These strings are turned into tokens at run-time:
    "foo", "bar", "baz",
    // These strings are turned into tokens at compile-time:
    "foo"_token, "xyzzy"_token, "same"_token, ""_token
  };
  print_set(hashed, "hashed (and maybe blank)");

  // Now let's insert strings into the token manager
  // (by hashing the same strings as above at run-time)
  // and re-print the contents of the set:
  Token dummy;
  dummy = "xyzzy";
  dummy = "same";
  print_set(hashed, "hashed (and no longer blank)");

  return 0;
}
