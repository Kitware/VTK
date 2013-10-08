/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestWeakPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkWeakPointer.
// .SECTION Description
// Tests instantiations of the vtkWeakPointer class template.

#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkWeakPointer.h"

int TestWeakPointer(int,char *[])
{
  int rval = 0;
  vtkIntArray* ia = vtkIntArray::New();

  // Coverage:
  unsigned int testbits = 0;
  unsigned int correctbits = 0x004d3953;
  const char *tests[] = {
    "da2 == da3", "da2 != da3", "da2 < da3", "da2 <= da3", "da2 > da3",
      "da2 >= da3",
    "ia == da3", "ia != da3", "ia < da3", "ia <= da3", "ia > da3", "ia >= da3",
    "da2 == ia", "da2 != ia", "da2 < ia", "da2 <= ia", "da2 > ia", "da2 <= ia",
      "da2 > ia", "da2 >= ia",
    "da1 == 0", "da1 != 0", "da1 < 0", "da1 <= 0", "da1 > 0", "da1 >= 0",
    NULL };

  vtkWeakPointer<vtkIntArray>  da2(ia);
  vtkWeakPointer<vtkFloatArray> da3;
  vtkWeakPointer<vtkDataArray> da1(da2);
  da1 = ia;
  da1 = da2;
  testbits = (testbits << 1) | ((da2 == da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 != da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 < da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 <= da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 > da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 >= da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia == da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia != da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia < da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia <= da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia > da3) ? 1 : 0);
  testbits = (testbits << 1) | ((ia >= da3) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 == ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 != ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 < ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 <= ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 > ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 >= ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 == 0) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 != 0) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 < 0) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 <= 0) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 > 0) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 >= 0) ? 1 : 0);
  if (testbits != correctbits)
    {
    unsigned int diffbits = (testbits ^ correctbits);
    int bitcount = 0;
    while (tests[bitcount] != NULL)
      {
      bitcount++;
      }
    for (int ib = 0; ib < bitcount; ++ib)
      {
      if (((diffbits >> (bitcount - ib - 1)) & 1) != 0)
        {
        cerr << "comparison (" << tests[ib] << ") failed!\n";
        }
      }
    rval = 1;
    }

  (*da1).SetNumberOfComponents(1);
  if(da2)
    {
    da2->SetNumberOfComponents(1);
    }
  if(!da2)
    {
    cerr << "da2 is NULL!" << "\n";
    rval = 1;
    }
  cout << "IntArray: " << da2 << "\n";

  if (da1.GetPointer() == 0)
    {
    cerr << "da1.GetPointer() is NULL\n";
    rval = 1;
    }
  if (da2.GetPointer() == 0)
    {
    cerr << "da2.GetPointer() is NULL\n";
    rval = 1;
    }
  if (da3.Get() != 0)
    {
    cerr << "da3.GetPointer() is not NULL\n";
    rval = 1;
    }

  da2 = 0;
  ia->Delete();

  if (da1.GetPointer() != NULL)
    {
    cerr << "da1.GetPointer() is not NULL\n";
    rval = 1;
    }

  return rval;
}
