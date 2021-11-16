/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherUnstructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests vtkUnstructuredGrid

#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

int otherUnstructuredGrid(int, char*[])
{
  int retVal = EXIT_SUCCESS;

  vtkNew<vtkUnstructuredGrid> ug;
  vtkUnsignedCharArray* distinctCellTypes = ug->GetDistinctCellTypesArray();
  if (!distinctCellTypes)
  {
    vtkLog(ERROR, "vtkUnstrucutredGrid::GetDistinctCellTypesArray() should never return nullptr");
    retVal = EXIT_FAILURE;
  }
  else if (distinctCellTypes->GetNumberOfTuples())
  {
    vtkLog(ERROR, "vtkUnstructuredGrid::GetDistinctCellTypesArray() should return an empty array");
    retVal = EXIT_FAILURE;
  }

  return retVal;
}
