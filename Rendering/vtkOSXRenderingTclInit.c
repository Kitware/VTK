//-----  This hack needed to compile using gcc3 on OSX until new stdc++.dylib
#include <stdio.h>
void oft_initRenOSXInit() 
{
  extern void _ZNSt8ios_base4InitC4Ev();
  _ZNSt8ios_base4InitC4Ev();
}

