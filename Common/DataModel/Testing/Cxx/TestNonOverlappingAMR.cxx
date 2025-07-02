// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Inspired by OverlappingAMR example from VTK examples

#include "vtkAMRMetaData.h"
#include "vtkFloatArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkNonOverlappingAMR.h"
#include "vtkPointData.h"
#include "vtkSphere.h"
#include "vtkUniformGrid.h"

//------------------------------------------------------------------------------
namespace
{
void MakeScalars(
  int dims[3], double const origin[3], double const spacing[3], vtkFloatArray* scalars)
{
  // Implicit function used to compute scalars.
  vtkNew<vtkSphere> sphere;
  sphere->SetRadius(3);
  sphere->SetCenter(5, 5, 5);

  scalars->SetNumberOfTuples(dims[0] * dims[1] * dims[2]);
  for (int k = 0; k < dims[2]; k++)
  {
    auto z = origin[2] + spacing[2] * k;
    for (int j = 0; j < dims[1]; j++)
    {
      auto y = origin[1] + spacing[1] * j;
      for (int i = 0; i < dims[0]; i++)
      {
        auto x = origin[0] + spacing[0] * i;
        scalars->SetValue(
          k * dims[0] * dims[1] + j * dims[0] + i, sphere->EvaluateFunction(x, y, z));
      }
    }
  }
}
} // namespace

//------------------------------------------------------------------------------
int TestNonOverlappingAMR(int, char*[])
{
  // Create and populate the AMR dataset.
  vtkNew<vtkNonOverlappingAMR> amr;
  int blocksPerLevel[] = { 1, 2 };
  amr->Initialize(2, blocksPerLevel);

  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 6 };

  vtkNew<vtkUniformGrid> ug1;
  // Geometry
  ug1->SetOrigin(origin);
  ug1->SetSpacing(spacing);
  ug1->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars;
  ug1->GetPointData()->SetScalars(scalars);
  MakeScalars(dims, origin, spacing, scalars);

  amr->SetDataSet(0, 0, ug1);

  double origin2[3] = { 0.0, 0.0, 5.0 };
  double spacing2[3] = { 1.0, 0.5, 1.0 };

  vtkNew<vtkUniformGrid> ug2;
  // Geometry
  ug2->SetOrigin(origin2);
  ug2->SetSpacing(spacing2);
  ug2->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars2;
  ug2->GetPointData()->SetScalars(scalars2);
  MakeScalars(dims, origin, spacing2, scalars2);

  amr->SetDataSet(1, 0, ug2);

  double origin3[3] = { 0.0, 5.0, 5.0 };

  vtkNew<vtkUniformGrid> ug3;
  // Geometry
  ug3->SetOrigin(origin3);
  ug3->SetSpacing(spacing2);
  ug3->SetDimensions(dims);

  // Data
  vtkNew<vtkFloatArray> scalars3;
  ug3->GetPointData()->SetScalars(scalars3);
  MakeScalars(dims, origin3, spacing2, scalars3);

  amr->SetDataSet(1, 1, ug3);

  if (amr->GetNumberOfPoints() != 2178)
  {
    vtkLogF(ERROR, "Invalid number of points");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfCells() != 1500)
  {
    vtkLogF(ERROR, "Invalid number of cells");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfLevels() != 2)
  {
    vtkLogF(ERROR, "Invalid number of levels");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfDataSets(1) != 2)
  {
    vtkLogF(ERROR, "Invalid number of datasets");
    return EXIT_FAILURE;
  }

  if (amr->GetTotalNumberOfBlocks() != 3)
  {
    vtkLogF(ERROR, "Invalid total number of blocks");
    return EXIT_FAILURE;
  }

  const double* bounds = amr->GetBounds();
  if (!bounds || bounds[0] != 0 || bounds[1] != 10 || bounds[2] != 0 || bounds[3] != 10 ||
    bounds[4] != 0 || bounds[5] != 10)
  {
    vtkLogF(ERROR, "Unexpected GetBounds result");
    return EXIT_FAILURE;
  }

  unsigned int level = 1;
  unsigned int index = 1;
  unsigned int compIdx = amr->GetCompositeIndex(level, index);
  if (compIdx != 2)
  {
    vtkLogF(ERROR, "Unexpected GetCompositeIndex result");
    return EXIT_FAILURE;
  }

  level = index = 0;
  amr->GetLevelAndIndex(compIdx, level, index);
  if (level != 1 || index != 1)
  {
    vtkLogF(ERROR, "Unexpected GetLevelAndIndex result");
    return EXIT_FAILURE;
  }
  if (amr->GetDataSet(level, index) != ug3.Get())
  {
    vtkLogF(ERROR, "Unexpected GetDataSet result");
    return EXIT_FAILURE;
  }

  if (amr->GetAMRMetaData() == nullptr)
  {
    vtkLogF(ERROR, "Unexpected GetAMRMetaData result");
    return EXIT_FAILURE;
  }

  vtkNew<vtkAMRMetaData> anotherMetaData;
  amr->SetAMRMetaData(anotherMetaData);
  if (amr->GetAMRMetaData() != anotherMetaData.Get())
  {
    vtkLogF(ERROR, "Unexpected SetAMRMetaData result");
    return EXIT_FAILURE;
  }

  vtkNew<vtkNonOverlappingAMR> amr2;
  amr2->ShallowCopy(amr);
  if (amr->GetAMRMetaData() != amr2->GetAMRMetaData())
  {
    vtkLogF(ERROR, "Unexpected ShallowCopy result");
    return EXIT_FAILURE;
  }

  amr2->Initialize();
  if (amr->GetAMRMetaData() == amr2->GetAMRMetaData())
  {
    vtkLogF(ERROR, "Unexpected Initialize result");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
