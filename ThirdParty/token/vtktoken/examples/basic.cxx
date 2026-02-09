#include <token/Token.h>
#include <iostream>

int main(int argc, char* argv[])
{
  // Tokenize the first argument passed on the command line:
  token::Token userData(argc > 1 ? argv[1] : "");
  std::string input = userData.data();
  std::cout
    << '"' << input << '"' << " = 0x"
    << std::hex << userData.getId() << " = "
    << std::dec << userData.getId() << "\n"

    << "If userData was created at run-time:\n"
    << "  it should have data:" << (userData.hasData() ? "T" : "F") << " and,\n"
    << "  if non-empty, be valid:" << (userData.valid() ? "T" : "F") << ".\n";
  return 0;
}
