// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkIOSSReader.h"
#include "vtkLogger.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

#include <string>

int TestIOSSGhostArray(int argc, char* argv[])
{
  vtkNew<vtkIOSSReader> reader;
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Exodus/ghost.ex2");
  reader->AddFileName(fileNameC);
  delete[] fileNameC;
  reader->Update();

  auto pdsc = vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  auto dataset = pdsc->GetPartition(0, 0);
  auto cd = dataset->GetCellData();
  auto ghostArray =
    vtkUnsignedCharArray::SafeDownCast(cd->GetArray(vtkDataSetAttributes::GhostArrayName()));
  if (ghostArray)
  {
    // Check the values in the ghost array - first should be 0, second should be 1
    if (ghostArray->GetValue(0) != 0)
    {
      vtkLog(ERROR, "First cell ghost value was not 0");
      return EXIT_FAILURE;
    }
    if (ghostArray->GetValue(1) != vtkDataSetAttributes::DUPLICATECELL)
    {
      vtkLog(ERROR, "Second cell ghost value was not 1");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
