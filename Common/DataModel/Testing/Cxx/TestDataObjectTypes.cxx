// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkDataObjectTypes.h"

class TestDataObjectTypesTester : public vtkDataObjectTypes
{
public:
  static int Test() { return vtkDataObjectTypes::Validate(); }
};

int TestDataObjectTypes(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  return TestDataObjectTypesTester::Test();
}
