// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkCellGrid

#include "vtkCellGrid.h"
#include "vtkLogger.h"
#include "vtkNew.h"

int otherCellGrid(int, char*[])
{
  vtkNew<vtkCellGrid> cg;
  if (cg->SupportsGhostArray(vtkDataObject::POINT) || !cg->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
