// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAvmeshReader.h"
#include <algorithm>
#include <vtkCellData.h>
#include <vtkCellSizeFilter.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkPartitionedDataSet.h>
#include <vtkPartitionedDataSetCollection.h>
#include <vtkTestUtilities.h>
#include <vtkUnstructuredGrid.h>

#define vtk_assert(x)                                                                              \
  do                                                                                               \
  {                                                                                                \
    if (!(x))                                                                                      \
    {                                                                                              \
      std::cerr << "On line " << __LINE__ << " ERROR: Condition FAILED!! : " << #x << std::endl;   \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (false)

double GetMinVolume(vtkUnstructuredGrid* ugIn)
{
  vtkNew<vtkCellSizeFilter> cellSizeFilter;
  cellSizeFilter->SetComputeVertexCount(false);
  cellSizeFilter->SetComputeLength(false);
  cellSizeFilter->SetComputeArea(false);
  cellSizeFilter->SetComputeVolume(true);
  cellSizeFilter->SetInputDataObject(ugIn);
  cellSizeFilter->Update();
  auto ugOut = vtkUnstructuredGrid::SafeDownCast(cellSizeFilter->GetOutput());
  auto vol = ugOut->GetCellData()->GetArray("Volume");
  return vol->GetRange()[0];
}

// Intended to be the same as NumPy's isclose with default args for tolerances.
// https://numpy.org/doc/2.1/reference/generated/numpy.isclose.html
bool IsClose(double a, double b)
{
  const double rtol = 1e-5;
  const double atol = 1e-8;
  return fabs(a - b) <= (atol + rtol * fabs(b));
}

bool BoundsMatch2D(double boundsA[6], double boundsB[6])
{
  return std::equal(boundsA, boundsA + 4, boundsB, boundsB + 4, IsClose);
}

bool BoundsMatch3D(double boundsA[6], double boundsB[6])
{
  return std::equal(boundsA, boundsA + 6, boundsB, boundsB + 6, IsClose);
}

int TestAvmeshReader(int argc, char* argv[])
{
  // 3D volume test =========================================

  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vwing_hexle.avm");
  std::string vwing = fname ? fname : "";
  delete[] fname;

  vtkNew<vtkAvmeshReader> reader;
  vtk_assert(reader->CanReadFile(vwing.c_str()));
  reader->SetFileName(vwing.c_str());
  reader->Update();

  vtkPartitionedDataSetCollection* pdsc = reader->GetOutput();

  vtk_assert(pdsc->GetNumberOfPartitionedDataSets() == 4);

  // Check name of the flowfield collection
  unsigned int collectionNum = 0;
  std::string name = pdsc->GetMetaData(collectionNum)->Get(vtkCompositeDataSet::NAME());
  vtk_assert(name == "Flowfield");

  // Check number of points and cells of flowfield collection
  auto flow =
    vtkUnstructuredGrid::SafeDownCast(pdsc->GetPartitionedDataSet(collectionNum)->GetPartition(0));
  vtk_assert(flow->GetNumberOfPoints() == 16989);
  vtk_assert(flow->GetNumberOfCells() == 41146);

  // Check bounds
  double volBounds3D[6] = { -2.5, 7.5, 0.0, 10.0, -5.0, 5.0 };
  double bounds[6];
  flow->GetBounds(bounds);
  vtk_assert(BoundsMatch3D(bounds, volBounds3D));

  // To make sure we got the cell winding correct, make sure volumes are all positive.
  double minVol = GetMinVolume(flow);
  vtk_assert(minVol > 0.0);

  // Check name and bounds of the wing collection
  collectionNum = 1;
  name = pdsc->GetMetaData(collectionNum)->Get(vtkCompositeDataSet::NAME());
  vtk_assert(name == "wing");
  auto wing =
    vtkUnstructuredGrid::SafeDownCast(pdsc->GetPartitionedDataSet(collectionNum)->GetPartition(0));
  vtk_assert(wing->GetNumberOfPoints() == 570);
  vtk_assert(wing->GetNumberOfCells() == 999);
  double wingBounds[6] = { 0.0, 4.5, 0.0, 2.0, -0.5, 0.5 };
  wing->GetBounds(bounds);
  vtk_assert(BoundsMatch3D(bounds, wingBounds));

  // 3D surface only =======================================================

  reader->SetSurfaceOnly(true);
  reader->Update();
  pdsc = reader->GetOutput();

  vtk_assert(pdsc->GetNumberOfPartitionedDataSets() == 3);

  vtk_assert(pdsc->GetNumberOfCells() == 4087);
  pdsc->GetBounds(bounds);
  vtk_assert(BoundsMatch3D(bounds, volBounds3D));

  // 2D (also happens to be rev1) ==========================================

  fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vwing_2d.avm");
  std::string vwing2d = fname ? fname : "";
  delete[] fname;

  reader->SetSurfaceOnly(false);
  reader->SetFileName(vwing2d.c_str());
  reader->Update();
  pdsc = reader->GetOutput();

  collectionNum = 0;
  flow =
    vtkUnstructuredGrid::SafeDownCast(pdsc->GetPartitionedDataSet(collectionNum)->GetPartition(0));
  vtk_assert(flow->GetNumberOfCells() == 1359);

  double volBounds2D[6] = { -2.5, 7.5, -5.0, 5.0, 0.0, 0.0 };
  pdsc->GetBounds(bounds);
  vtk_assert(BoundsMatch2D(bounds, volBounds2D));

  return EXIT_SUCCESS;
}
