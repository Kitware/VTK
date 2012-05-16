/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestStreamLongLong.cxx.in

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#if defined(_MSC_VER)
# pragma warning (push, 1)
#endif

// Include the streams library.
#include <iostream>
using std::ostream;
using std::istream;
using std::cout;
using std::cin;

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

#if defined(VTK_TEST_OSTREAM_LONG_LONG)
int test_ostream(ostream& os, long long x)
{
  return (os << x)? 1:0;
}
#endif

#if defined(VTK_TEST_ISTREAM_LONG_LONG)
int test_istream(istream& is, long long& x)
{
  return (is >> x)? 1:0;
}
#endif

int main()
{
#if defined(VTK_TEST_OSTREAM_LONG_LONG)
  long long x = 0;
  return test_ostream(cout, x);
#endif
#if defined(VTK_TEST_ISTREAM_LONG_LONG)
  long long x;
  return test_istream(cin, x);
#endif
}

