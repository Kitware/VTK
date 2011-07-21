#include <stdio.h>
#include <stdlib.h>

int main()
{
  /* Test whether gcc/clang __sync_ builtins are defined */
  /* These are for gcc >= 4.1.2 and for clang >= 2.0.1 */
  /* Only certain platforms support them intrinsically */
  /* (guaranteed support on x86_64, ia64, mips, alpha) */

  static volatile unsigned long v = 0;
  unsigned long u = __sync_add_and_fetch(&v, 1);

  if (u - 1 == 0) { return 0; }

  return 1;
}
