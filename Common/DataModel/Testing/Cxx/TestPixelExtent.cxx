/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPixelExtent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPixelExtent.h"
#include "vtkPixelExtentIO.h"

#include <iostream>
#include <deque>

using std::cerr;
using std::endl;
using std::deque;

int TestPixelExtent(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  cerr << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  // small extent in the middle of the region of interest
  vtkPixelExtent A(4,8,4,8);

  // larger region that covers A
  vtkPixelExtent B(A);
  B.Grow(4);

  // shift C to origin
  vtkPixelExtent C(A);
  C.Shift();

  // shift D to upper right corner of larger region
  vtkPixelExtent D(A);
  int s1[2]={4,4};
  D.Shift(s1);

  bool testPass = true;

  vtkPixelExtent tmp1;
  vtkPixelExtent tmp2;
  vtkPixelExtent tmp3;

  // shift, intersect
  tmp1 = C;
  tmp2 = D;

  tmp1 &= tmp2;

  cerr << C << " & " << D << " = " << tmp1 << endl;

  if (!tmp1.Empty())
    {
    cerr << "Test empty intersection failed" << endl;
    testPass = false;
    }


  tmp1 = A;
  int s2[2] = {-2,-2};
  tmp1.Shift(s2);

  tmp2 = A;
  int s3[2] = {2,2};
  tmp2.Shift(s3);

  tmp3 = tmp1;
  tmp3 &= tmp2;

  cerr << tmp1 << " & " << tmp2 << " = " << tmp3 << endl;

  if (!(tmp3 == vtkPixelExtent(6,6,6,6)))
    {
    cerr << "Test intersection failed" << endl;
    testPass = false;
    }

  // shift, grow, union
  tmp1 = C;
  tmp2 = D;
  tmp3 = tmp1;
  tmp3 |= tmp2;

  cerr << tmp1 << " | " << tmp2 << " = " << tmp3 << endl;

  if (!(tmp3 == B))
    {
    cerr << "Test union fails" << endl;
    testPass = false;
    }

  // subtraction
  deque<vtkPixelExtent> tmp4;
  vtkPixelExtent::Subtract(B, A, tmp4);

  deque<vtkPixelExtent> tmp5;
  tmp5.push_back(vtkPixelExtent(4, 8, 9, 12));
  tmp5.push_back(vtkPixelExtent(9, 12, 9, 12));
  tmp5.push_back(vtkPixelExtent(9, 12, 4, 8));
  tmp5.push_back(vtkPixelExtent(0, 3, 4, 8));
  tmp5.push_back(vtkPixelExtent(0, 3, 9, 12));
  tmp5.push_back(vtkPixelExtent(4, 8, 0, 3));
  tmp5.push_back(vtkPixelExtent(9, 12, 0, 3));
  tmp5.push_back(vtkPixelExtent(0, 3, 0, 3));

  size_t n = tmp4.size();
  for (size_t i=0; i<n; ++i)
    {
    if (!(tmp4[i] == tmp5[i]))
      {
      cerr << "Test subtraction failed" << endl;
      testPass = false;
      break;
      }
    }

  cerr << B << " - " << A << " = ";
  if (n)
    {
    cerr << tmp4[0];
    for (size_t i=1; i<n; ++i)
      {
      cerr << ", " << tmp4[i];
      }
    }
  cerr << endl;

  if (!testPass)
    {
    cerr << "Test fails" << endl;
    return 1;
    }

  cerr << "Test passes" << endl;
  return 0;
}
