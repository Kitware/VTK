/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAbstractArraySize.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkStringArray.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

int TestAbstractArraySize(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int status = 0;
  vtkNew<vtkStringArray> stringArray;
  stringArray->SetNumberOfComponents(2);
  stringArray->SetNumberOfTuples(1);
  std::cout << "Size is now " << stringArray->GetSize() << "\n";
  if (stringArray->GetMaxId() < 1)
  {
    std::cerr << "Allocation failed: number of tuples requested not provided.\n";
    status = 1;
  }
  // Test for regression
  stringArray->SetValue(0, "This value is OK.");
  stringArray->SetValue(1, "This used to crash, even though GetMaxId reported a proper size.");

  // Test for desired behavior
  stringArray->SetNumberOfValues(3);
  std::cout << "Size is now " << stringArray->GetSize() << "\n";
  if (stringArray->GetSize() < 4)
  {
    std::cerr
      << "Allocation failed: SetNumberOfValues should always allocate to a tuple boundary.\n";
    status = 1;
  }
  // Same as above, but test a vtkDataArray subclass.
  vtkNew<vtkDoubleArray> doubleArray;
  doubleArray->SetNumberOfComponents(3);
  doubleArray->SetNumberOfValues(7);
  if (doubleArray->GetSize() != 9)
  {
    std::cerr
      << "Allocation failed: SetNumberOfValues should always allocate to a tuple boundary.\n";
    status = 1;
  }

  return status;
}
