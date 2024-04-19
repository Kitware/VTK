// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
  if (!ug->SupportsGhostArray(vtkDataObject::POINT) || !ug->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    retVal = EXIT_FAILURE;
  }

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
