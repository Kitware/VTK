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
#include "vtkIntArray.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <vector>

int TestSmartPointer(int, char*[])
{
  int rval = 0;
  vtkIntArray* ia = vtkIntArray::New();

  // Coverage:
  unsigned int testbits = 0;
  unsigned int correctbits = 0x00000953;
  const char* tests[] = { "da2 == ia", "da2 != ia", "da2 < ia", "da2 <= ia", "da2 > ia",
    "da2 <= ia", "da2 > ia", "da2 >= ia", "da1 == 0", "da1 != 0", "da1 < 0", "da1 <= 0", "da1 > 0",
    "da1 >= 0", nullptr };

  auto da2 = vtk::MakeSmartPointer(ia); // da2 is a vtkSmartPointer<vtkIntArray>
  vtkSmartPointer<vtkDataArray> da1(da2);
  da1 = ia;
  da1 = da2;
  testbits = (testbits << 1) | ((da2 == ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 != ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 < ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 <= ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 > ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da2 >= ia) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 == nullptr) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 != nullptr) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 < nullptr) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 <= nullptr) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 > nullptr) ? 1 : 0);
  testbits = (testbits << 1) | ((da1 >= nullptr) ? 1 : 0);
  if (testbits != correctbits)
  {
    unsigned int diffbits = (testbits ^ correctbits);
    int bitcount = 0;
    while (tests[bitcount] != nullptr)
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
  if (da2)
  {
    da2->SetNumberOfComponents(1);
  }
  if (!da2)
  {
    cerr << "da2 is nullptr!"
         << "\n";
    rval = 1;
  }
  da1 = vtkSmartPointer<vtkDataArray>::NewInstance(ia);
  da1.TakeReference(vtkIntArray::New());
  auto da4 = vtk::TakeSmartPointer(vtkIntArray::New());
  (void)da4;
  ia->Delete();

  std::vector<vtkSmartPointer<vtkIntArray> > intarrays;
  { // local scope for vtkNew object
    vtkNew<vtkIntArray> vtknew;
    vtkSmartPointer<vtkIntArray> aa(vtknew);
    intarrays.push_back(vtknew);
  }
  if (intarrays[0]->GetReferenceCount() != 1)
  {
    cerr << "Didn't properly add vtkNew object to stl vector of smart pointers\n";
    rval = 1;
  }

  // Test move constructors
  {
    vtkSmartPointer<vtkIntArray> intArray{ vtkNew<vtkIntArray>{} };
    if (intArray == nullptr || intArray->GetReferenceCount() != 1)
    {
      std::cerr << "Move constructing a vtkSmartPointer from a vtkNew "
                   "failed.\n";
      rval = 1;
    }

    vtkSmartPointer<vtkIntArray> intArrayCopy(intArray);
    if (intArrayCopy != intArray || intArray->GetReferenceCount() != 2 ||
      intArrayCopy->GetReferenceCount() != 2)
    {
      std::cerr << "Copy constructing vtkSmartPointer yielded unexpected "
                   "result.\n";
      rval = 1;
    }

    vtkSmartPointer<vtkIntArray> intArrayMoved(std::move(intArrayCopy));
    if (intArrayCopy || !intArrayMoved || intArrayMoved->GetReferenceCount() != 2)
    {
      std::cerr << "Move constructing vtkSmartPointer yielded unexpected "
                   "result.\n";
      rval = 1;
    }

    vtkSmartPointer<vtkDataArray> dataArrayCopy(intArray);
    if (dataArrayCopy != intArray || intArray->GetReferenceCount() != 3 ||
      dataArrayCopy->GetReferenceCount() != 3)
    {
      std::cerr << "Cast constructing vtkSmartPointer failed.\n";
      rval = 1;
    }

    vtkSmartPointer<vtkDataArray> dataArrayMoved(std::move(intArrayMoved));
    if (!dataArrayMoved || intArrayMoved || dataArrayMoved->GetReferenceCount() != 3)
    {
      std::cerr << "Cast move-constructing vtkSmartPointer failed.\n";
      rval = 1;
    }
  }

  return rval;
}
