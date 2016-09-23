/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSmartPointer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of vtkSmartPointer.
// .SECTION Description
// Tests instantiations of the vtkSmartPointer class template.

#include "vtkDebugLeaks.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkSmartPointer.h"

int TestSmartPointer(int,char *[])
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

  vtkSmartPointer<vtkIntArray>  da2(ia);
  vtkSmartPointer<vtkFloatArray> da3;
  vtkSmartPointer<vtkDataArray> da1(da2);
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
  da1 = vtkSmartPointer<vtkDataArray>::NewInstance(ia);
  da1.TakeReference(vtkIntArray::New());
  vtkSmartPointer<vtkIntArray> da4 =
    vtkSmartPointer<vtkIntArray>::Take(vtkIntArray::New());
  (void)da4;
  ia->Delete();

  return rval;
}
