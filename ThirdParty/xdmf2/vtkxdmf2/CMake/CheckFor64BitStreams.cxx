#ifdef NO_ANSI
# include <iostream.h>
#else
# include <iostream>
using std::cin;
#endif

#if defined(SIZEOF_LONG_LONG)
typedef long long t64;
#elif defined(SIZEOF___INT64)
typedef __int64 t64;
#else
#error No 64 bit types
#endif

int main()
{
  t64 foo;
  cin >> foo;
  return 0;
}
