// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// .NAME
// .SECTION Description
// this program tests vtkHyperTreeGrid

#include "vtkHyperTreeGrid.h"
#include "vtkLogger.h"
#include "vtkNew.h"

int otherHyperTreeGrid(int, char*[])
{
  vtkNew<vtkHyperTreeGrid> htg;
  if (htg->SupportsGhostArray(vtkDataObject::POINT) ||
    !htg->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
