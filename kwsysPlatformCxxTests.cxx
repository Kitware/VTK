#ifdef TEST_KWSYS_HAVE_STL_STD
#include <list>
void f(std::list<int>*) {}
int main() { return 0; }
#endif

#ifdef TEST_KWSYS_HAVE_ANSI_STREAMS_STD
#include <iosfwd>
void f(std::ostream*) {}
int main() { return 0; }
#endif
