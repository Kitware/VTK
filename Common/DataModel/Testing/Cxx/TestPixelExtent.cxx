// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPixelExtent.h"
#include "vtkPixelExtentIO.h"

#include <deque>
#include <iostream>

using std::cerr;
using std::deque;
using std::endl;

int TestPixelExtent(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  std::cerr << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << std::endl;

  // small extent in the middle of the region of interest
  vtkPixelExtent A(4, 8, 4, 8);

  // larger region that covers A
  vtkPixelExtent B(A);
  B.Grow(4);

  // shift C to origin
  vtkPixelExtent C(A);
  C.Shift();

  // shift D to upper right corner of larger region
  vtkPixelExtent D(A);
  int s1[2] = { 4, 4 };
  D.Shift(s1);

  bool testPass = true;

  vtkPixelExtent tmp1;
  vtkPixelExtent tmp2;
  vtkPixelExtent tmp3;

  // shift, intersect
  tmp1 = C;
  tmp2 = D;

  tmp1 &= tmp2;

  std::cerr << C << " & " << D << " = " << tmp1 << std::endl;

  if (!tmp1.Empty())
  {
    std::cerr << "Test empty intersection failed" << std::endl;
    testPass = false;
  }

  tmp1 = A;
  int s2[2] = { -2, -2 };
  tmp1.Shift(s2);

  tmp2 = A;
  int s3[2] = { 2, 2 };
  tmp2.Shift(s3);

  tmp3 = tmp1;
  tmp3 &= tmp2;

  std::cerr << tmp1 << " & " << tmp2 << " = " << tmp3 << std::endl;

  if (!(tmp3 == vtkPixelExtent(6, 6, 6, 6)))
  {
    std::cerr << "Test intersection failed" << std::endl;
    testPass = false;
  }

  // shift, grow, union
  tmp1 = C;
  tmp2 = D;
  tmp3 = tmp1;
  tmp3 |= tmp2;

  std::cerr << tmp1 << " | " << tmp2 << " = " << tmp3 << std::endl;

  if (!(tmp3 == B))
  {
    std::cerr << "Test union fails" << std::endl;
    testPass = false;
  }

  // subtraction
  deque<vtkPixelExtent> tmp4;
  vtkPixelExtent::Subtract(B, A, tmp4);

  deque<vtkPixelExtent> tmp5;
  tmp5.emplace_back(4, 8, 9, 12);
  tmp5.emplace_back(9, 12, 9, 12);
  tmp5.emplace_back(9, 12, 4, 8);
  tmp5.emplace_back(0, 3, 4, 8);
  tmp5.emplace_back(0, 3, 9, 12);
  tmp5.emplace_back(4, 8, 0, 3);
  tmp5.emplace_back(9, 12, 0, 3);
  tmp5.emplace_back(0, 3, 0, 3);

  size_t n = tmp4.size();
  for (size_t i = 0; i < n; ++i)
  {
    if (!(tmp4[i] == tmp5[i]))
    {
      std::cerr << "Test subtraction failed" << std::endl;
      testPass = false;
      break;
    }
  }

  std::cerr << B << " - " << A << " = ";
  if (n)
  {
    std::cerr << tmp4[0];
    for (size_t i = 1; i < n; ++i)
    {
      std::cerr << ", " << tmp4[i];
    }
  }
  std::cerr << std::endl;

  if (!testPass)
  {
    std::cerr << "Test fails" << std::endl;
    return 1;
  }

  std::cerr << "Test passes" << std::endl;
  return 0;
}
