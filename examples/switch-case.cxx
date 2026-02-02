#include <token/Token.h>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  using namespace token_NAMESPACE ::literals;
  // Tokenize the first argument passed on the command line:
  token::Token userData(argc > 1 ? argv[1] : "example");

  // Compare userData to some "expected" values:
  std::string message;
  switch (userData.getId())
  {
  case "foo"_hash: message = "bar is next"; break;
  case "bar"_hash: message = "baz is next"; break;
  case "baz"_hash: message = "foo is next"; break;
  default:
    message = "Unexpected option " + userData.data() + ".";
    break;
  }
  std::cout << message << "\n";
  return 0;
}
