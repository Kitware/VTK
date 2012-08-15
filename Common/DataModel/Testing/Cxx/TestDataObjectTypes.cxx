/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDataObjectTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataObjectTypes.h"

class TestDataObjectTypesTester: public vtkDataObjectTypes
{
public:
  static int Test() { return vtkDataObjectTypes::Validate(); };
};

int TestDataObjectTypes(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  return TestDataObjectTypesTester::Test();
}
