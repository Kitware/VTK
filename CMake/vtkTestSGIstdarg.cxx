#if defined(_COMPILER_VERSION) && _COMPILER_VERSION == 742
# include <cstdarg>
#endif
#include <stdarg.h>
void f(const char*, va_list);
int main() { return 0; }
