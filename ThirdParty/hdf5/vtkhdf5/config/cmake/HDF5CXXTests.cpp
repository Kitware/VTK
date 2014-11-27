
#ifdef OLD_HEADER_FILENAME

#include <iostream>

int main(void) { return 0; }

#endif


#ifdef H5_NO_NAMESPACE

namespace H5 {
int fnord;
}

int main(void) {
   using namespace H5;
   fnord = 37;
   return 0;
}

#endif

#ifdef H5_NO_STD

#include <string>

using namespace std;

int main(void) {
   string myString("testing namespace std");
   return 0;
}

#endif

#ifdef BOOL_NOTDEFINED
int main(void) {
   bool flag;
   return 0;
}

#endif

#ifdef NO_STATIC_CAST

int main(void) {
   float test_float;
   int test_int;
   test_float = 37.0;
   test_int = static_cast <int> (test_float);
   return 0;
}

#endif
