// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Inspired by OverlappingAMR example from VTK examples

#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkOverlappingAMR.h"
#include "vtkOverlappingAMRMetaData.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSphere.h"
#include "vtkUnsignedCharArray.h"

//------------------------------------------------------------------------------
namespace
{
void MakeCoords(int dims[3], double const origin[3], double const spacing[3],
  vtkDoubleArray* xCoords, vtkDoubleArray* yCoords, vtkDoubleArray* zCoords, double* bb)
{
  xCoords->SetNumberOfTuples(dims[0]);
  for (int i = 0; i < dims[0]; i++)
  {
    auto x = origin[0] + spacing[0] * i;
    xCoords->SetValue(i, x);
  }

  yCoords->SetNumberOfTuples(dims[1]);
  for (int j = 0; j < dims[1]; j++)
  {
    auto y = origin[1] + spacing[1] * j;
    yCoords->SetValue(j, y);
  }

  zCoords->SetNumberOfTuples(dims[2]);
  for (int k = 0; k < dims[2]; k++)
  {
    auto z = origin[2] + spacing[2] * k;
    zCoords->SetValue(k, z);
  }

  bb[0] = origin[0];
  bb[1] = origin[0] + spacing[0] * (dims[0] - 1);
  bb[2] = origin[1];
  bb[3] = origin[1] + spacing[1] * (dims[1] - 1);
  bb[4] = origin[2];
  bb[5] = origin[2] + spacing[2] * (dims[2] - 1);
}

void MakeScalars(
  int dims[3], double const origin[3], double const spacing[3], vtkDoubleArray* scalars)
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
int TestOverlappingAMRRectilinear(int, char*[])
{
  // Create and populate the AMR dataset.
  // The dataset should look like
  // Level 0
  //   rectilinear grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  // Level 1 - refinement ratio : 2
  //   rectilinear grid, dimensions 11, 11, 11, AMR box (0, 0, 0) - (9, 9, 9)
  //   rectilinear grid, dimensions 11, 11, 11, AMR box (10, 10, 10) - (19, 19, 19)
  // Use MakeScalars() above to fill the scalar arrays.

  vtkNew<vtkOverlappingAMR> amr;
  const std::vector<unsigned int> blocksPerLevel{ 1, 2 };
  amr->Initialize(blocksPerLevel);

  double bb[6];
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 1.0, 1.0, 1.0 };
  int dims[3] = { 11, 11, 11 };

  // Origin should be set as soon as it is known
  amr->SetOrigin(origin);

  vtkNew<vtkRectilinearGrid> rg1;
  // Geometry
  rg1->SetDimensions(dims);

  vtkNew<vtkDoubleArray> xCoords;
  vtkNew<vtkDoubleArray> yCoords;
  vtkNew<vtkDoubleArray> zCoords;
  ::MakeCoords(dims, origin, spacing, xCoords, yCoords, zCoords, bb);
  rg1->SetXCoordinates(xCoords);
  rg1->SetYCoordinates(yCoords);
  rg1->SetZCoordinates(zCoords);

  // Data
  vtkNew<vtkDoubleArray> scalars;
  rg1->GetPointData()->SetScalars(scalars);
  ::MakeScalars(dims, origin, spacing, scalars);

  int lo[3] = { 0, 0, 0 };
  int hi[3] = { 9, 9, 9 };
  vtkAMRBox box1(lo, hi);
  amr->SetAMRBox(0, 0, box1);
  amr->SetDataSet(0, 0, rg1);

  double spacing2[3] = { 0.5, 0.5, 0.5 };

  vtkNew<vtkRectilinearGrid> rg2;
  // Geometry
  rg2->SetDimensions(dims);

  vtkNew<vtkDoubleArray> xCoords2;
  vtkNew<vtkDoubleArray> yCoords2;
  vtkNew<vtkDoubleArray> zCoords2;
  ::MakeCoords(dims, origin, spacing2, xCoords2, yCoords2, zCoords2, bb);
  rg2->SetXCoordinates(xCoords2);
  rg2->SetYCoordinates(yCoords2);
  rg2->SetZCoordinates(zCoords2);

  // Data
  vtkNew<vtkDoubleArray> scalars2;
  rg2->GetPointData()->SetScalars(scalars2);
  ::MakeScalars(dims, origin, spacing2, scalars2);

  int lo2[3] = { 0, 0, 0 };
  int hi2[3] = { 9, 9, 9 };
  vtkAMRBox box2(lo2, hi2);
  amr->SetAMRBox(1, 0, box2);
  amr->SetDataSet(1, 0, rg2);

  double origin3[3] = { 5, 5, 5 };

  vtkNew<vtkRectilinearGrid> rg3;
  // Geometry
  rg3->SetDimensions(dims);

  vtkNew<vtkDoubleArray> xCoords3;
  vtkNew<vtkDoubleArray> yCoords3;
  vtkNew<vtkDoubleArray> zCoords3;
  ::MakeCoords(dims, origin3, spacing2, xCoords3, yCoords3, zCoords3, bb);
  rg3->SetXCoordinates(xCoords3);
  rg3->SetYCoordinates(yCoords3);
  rg3->SetZCoordinates(zCoords3);

  // Data
  vtkNew<vtkDoubleArray> scalars3;
  rg3->GetPointData()->SetScalars(scalars3);
  ::MakeScalars(dims, origin3, spacing2, scalars3);

  int lo3[3] = { 10, 10, 10 };
  int hi3[3] = { 19, 19, 19 };
  vtkAMRBox box3(lo3, hi3);
  amr->SetAMRBox(1, 1, box3);
  amr->SetDataSet(1, 1, rg3);

  if (amr->GetNumberOfPoints() != 3993)
  {
    vtkLogF(ERROR, "Invalid number of points");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfCells() != 3000)
  {
    vtkLogF(ERROR, "Invalid number of cells");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfLevels() != 2)
  {
    vtkLogF(ERROR, "Invalid number of levels");
    return EXIT_FAILURE;
  }

  if (amr->GetNumberOfBlocks() != 3)
  {
    vtkLogF(ERROR, "Invalid total number of blocks");
    return EXIT_FAILURE;
  }

  if (amr->HasChildrenInformation())
  {
    vtkLogF(ERROR, "Unexpectedly contains children information");
    return EXIT_FAILURE;
  }

  amr->GenerateParentChildInformation();
  if (!amr->HasChildrenInformation())
  {
    vtkLogF(ERROR, "Unexpectedly doesn't contains children information");
    return EXIT_FAILURE;
  }

  unsigned int numParents;
  unsigned int* parents = amr->GetParents(1, 0, numParents);
  if (!parents || parents[0] != 0 || numParents != 1)
  {
    vtkLogF(ERROR, "Unexpected GetParents output");
    return EXIT_FAILURE;
  }

  unsigned int numChildren;
  unsigned int* children = amr->GetChildren(0, 0, numChildren);
  if (!children || children[0] != 0 || children[1] != 1 || numChildren != 2)
  {
    vtkLogF(ERROR, "Unexpected GetChildren output");
    return EXIT_FAILURE;
  }

  if (!amr->CheckValidity())
  {
    vtkLogF(ERROR, "Error with CheckValidity");
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
  unsigned int compIdx = amr->GetAbsoluteBlockIndex(level, index);
  if (compIdx != 2)
  {
    vtkLogF(ERROR, "Unexpected GetAbsoluteBlockIndex result");
    return EXIT_FAILURE;
  }
  level = index = 0;
  amr->ComputeIndexPair(compIdx, level, index);
  if (level != 1 || index != 1)
  {
    vtkLogF(ERROR, "Unexpected ComputeIndexPair result");
    return EXIT_FAILURE;
  }

  double x[3] = { 1, 1, 1 };
  if (!amr->FindGrid(x, level, index) || level != 1 || index != 0)
  {
    vtkLogF(ERROR, "Unexpected FindGrid result");
    return EXIT_FAILURE;
  }

  if (amr->GetDataSetAsCartesianGrid(level, index) != rg2.Get())
  {
    vtkLogF(ERROR, "Unexpected GetDataSet result");
    return EXIT_FAILURE;
  }

  if (amr->GetOverlappingAMRMetaData() == nullptr)
  {
    vtkLogF(ERROR, "Unexpected GetOverlappingAMRMetaData result");
    return EXIT_FAILURE;
  }

  // Check cell blanking
  vtkAMRUtilities::BlankCells(amr);
  vtkCartesianGrid* cg = amr->GetDataSetAsCartesianGrid(0, 0);
  vtkUnsignedCharArray* ghostCells = cg->GetCellGhostArray();
  vtkIdType ghostCount = 0;
  for (vtkIdType i = 0; i < ghostCells->GetNumberOfTuples(); i++)
  {
    if (ghostCells->GetValue(i) & vtkDataSetAttributes::REFINEDCELL)
    {
      ghostCount++;
    }
  }
  if (ghostCount != 250)
  {
    vtkLogF(ERROR, "Unexpected number of ghost cells, expecting 250, got %lld", ghostCount);
    return EXIT_FAILURE;
  }

  vtkNew<vtkOverlappingAMRMetaData> anotherMetaData;
  amr->SetAMRMetaData(anotherMetaData);
  if (amr->GetOverlappingAMRMetaData() != anotherMetaData.Get())
  {
    vtkLogF(ERROR, "Unexpected SetOverlappingAMRMetaData result");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
