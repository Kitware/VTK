#include "token/Token.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
  for (int ii = 1; ii < argc; ++ii)
  {
    std::string arg(argv[ii]);
    token_NAMESPACE::Token t(arg);
    std::cout << "0x" << std::hex << t.getId() << " = \"" << t.data() << "\"\n";
  }
  return 0;
}
