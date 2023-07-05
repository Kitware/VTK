// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataIterator.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"

#include <iostream>
#include <vector>

//------------------------------------------------------------------------------
int TestCompositeDataSets(int, char*[])
{
  int errors = 0;

  return (errors);
}
