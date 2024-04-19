// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"

//------------------------------------------------------------------------------
int TestCompositeDataSets(int, char*[])
{
  vtkNew<vtkMultiBlockDataSet> mb;
  if (!mb->SupportsGhostArray(vtkDataObject::POINT) || !mb->SupportsGhostArray(vtkDataObject::CELL))
  {
    vtkLog(ERROR, "Unexpected results on SupportsGhostArray");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
