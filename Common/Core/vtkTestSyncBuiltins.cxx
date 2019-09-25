#include <cstdio>
#include <cstdlib>

int main()
{
  /* Test whether gcc/clang __atomic_ builtins are defined */

  static volatile unsigned long v = 0;
  unsigned long u = __atomic_add_fetch(&v, 1);

  if (u - 1 == 0) { return 0; }

  return 1;
}
