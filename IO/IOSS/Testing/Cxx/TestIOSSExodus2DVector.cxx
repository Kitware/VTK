// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkIOSSReader.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkRegressionTestImage.h>
#include <vtkTestUtilities.h>

int TestIOSSExodus2DVector(int argc, char* argv[])
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Exodus/exo_cell_vec2.e");

  vtkNew<vtkIOSSReader> reader0;
  reader0->AddFileName(fileNameC);
  delete[] fileNameC;

  reader0->Update();

  // The "result" array is a 2-components Cell array vector in the dataset.
  // We check that it is properly read as a 3D vector, so we can apply filters such as "glyphs" on
  // it properly.
  auto partitionedDS =
    vtkPartitionedDataSetCollection::SafeDownCast(reader0->GetOutputDataObject(0));
  auto dataset = vtkDataSet::SafeDownCast(partitionedDS->GetPartitionAsDataObject(0, 0));
  auto array2D = dataset->GetCellData()->GetArray("result");
  if (array2D->GetNumberOfComponents() != 3)
  {
    vtkLog(ERROR,
      "Expected 'result' array to have 3 components when read, but got "
        << array2D->GetNumberOfComponents() << " instead.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
