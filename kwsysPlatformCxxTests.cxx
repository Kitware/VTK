#ifdef TEST_KWSYS_STL_HAVE_STD
#include <list>
void f(std::list<int>*) {}
int main() { return 0; }
#endif

#ifdef TEST_KWSYS_IOS_HAVE_STD
#include <iosfwd>
void f(std::ostream*) {}
int main() { return 0; }
#endif
