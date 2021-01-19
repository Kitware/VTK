/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCompositeDataSetGhostCellsGenerator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#define _USE_MATH_DEFINES

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif

#include "vtkCellData.h"
#include "vtkCompositeDataSetGhostCellsGenerator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

#include <cmath>

namespace
{
static constexpr int MaxExtent = 5;
static constexpr int GridWidth = 2 * MaxExtent + 1;
static constexpr double XCoordinates[GridWidth] = { -40.0, -25.0, -12.0, -10.0, -4.0, -3.0, 2.0,
  10.0, 12.0, 20.0, 21.0 };
static constexpr double YCoordinates[GridWidth] = { -13.0, -12.0, -11.0, -10.0 - 6.0, -3.0, -1.0,
  4.0, 5.0, 10.0, 11.0 };
static constexpr double ZCoordinates[GridWidth] = { -9.0, -5.0, -3.0, 0.0, 2.0, 3.0, 4.0, 6.0, 15.0,
  20.0, 21.0 };
static constexpr char GridArrayName[] = "grid_data";

//----------------------------------------------------------------------------
double GetGridValue(double i, double j, double k)
{
  return std::cos(i * M_PI / MaxExtent) * std::sin(j * M_PI / MaxExtent) *
    std::exp(-std::abs(k) * std::abs(k) / 9.0);
}

//----------------------------------------------------------------------------
void FillImage(vtkImageData* image)
{
  const int* extent = image->GetExtent();
  vtkNew<vtkDoubleArray> array;
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(
    (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1));
  array->SetName(GridArrayName);
  image->GetPointData()->AddArray(array);
  for (int k = extent[4]; k <= extent[5]; ++k)
  {
    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      for (int i = extent[0]; i <= extent[1]; ++i)
      {
        int ijk[3] = { i, j, k };
        vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
        array->SetValue(pointId, GetGridValue(i, j, k));
      }
    }
  }
}

//----------------------------------------------------------------------------
void SetCoordinates(vtkDataArray* array, int min, int max, const double* coordinates)
{
  int i = 0;
  for (int id = min; id <= max; ++id, ++i)
  {
    array->InsertTuple1(i, coordinates[MaxExtent + id]);
  }
}

//----------------------------------------------------------------------------
template <class GridDataSetT>
bool TestImageCells(vtkPartitionedDataSet* pds, vtkImageData* refImage)
{
  const int* refExtent = refImage->GetExtent();
  vtkDataArray* refArray = refImage->GetCellData()->GetArray(GridArrayName);
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    GridDataSetT* part = GridDataSetT::SafeDownCast(pds->GetPartition(partitionId));
    if (!part)
    {
      return false;
    }
    vtkDataArray* array = part->GetCellData()->GetArray(GridArrayName);
    if (!array)
    {
      return false;
    }
    const int* extent = part->GetExtent();
    for (int k = extent[4] + 1; k < extent[5] - 1; ++k)
    {
      for (int j = extent[2] + 1; j < extent[3] - 1; ++j)
      {
        for (int i = extent[0] + 1; i < extent[1] - 1; ++i)
        {
          int ijk[3] = { i, j, k };
          vtkIdType refCellId = vtkStructuredData::ComputeCellIdForExtent(refExtent, ijk);
          vtkIdType cellId = vtkStructuredData::ComputeCellIdForExtent(extent, ijk);
          if (array->GetTuple1(cellId) != refArray->GetTuple1(refCellId))
          {
            std::cout << array->GetTuple1(cellId) << " != " << refArray->GetTuple1(refCellId)
                      << std::endl;
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
template <class GridDataSetT>
bool TestImagePoints(vtkPartitionedDataSet* pds, vtkImageData* refImage)
{
  const int* refExtent = refImage->GetExtent();
  vtkDataArray* refArray = refImage->GetPointData()->GetArray(GridArrayName);
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    GridDataSetT* part = GridDataSetT::SafeDownCast(pds->GetPartition(partitionId));
    if (!part)
    {
      return false;
    }
    vtkDataArray* array = part->GetPointData()->GetArray(GridArrayName);
    if (!array)
    {
      return false;
    }
    const int* extent = part->GetExtent();
    for (int k = extent[4]; k <= extent[5]; ++k)
    {
      for (int j = extent[2]; j <= extent[3]; ++j)
      {
        for (int i = extent[0]; i <= extent[1]; ++i)
        {
          int ijk[3] = { i, j, k };
          vtkIdType refCellId = vtkStructuredData::ComputePointIdForExtent(refExtent, ijk);
          vtkIdType cellId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
          if (array->GetTuple1(cellId) != refArray->GetTuple1(refCellId))
          {
            std::cout << array->GetTuple1(cellId) << " != " << refArray->GetTuple1(refCellId)
                      << std::endl;
            return false;
          }
        }
      }
    }
  }
  return true;
}
} // anonymous namespace

//----------------------------------------------------------------------------
int TestCompositeDataSetGhostCellsGenerator(int argc, char* argv[])
{
  vtkLogger::Init(argc, argv);

  // Put every log message in "everything.log":

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif

  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  int myrank = contr->GetLocalProcessId();

  int zmin, zmax;

  switch (myrank)
  {
    case 0:
      zmin = -MaxExtent;
      zmax = 0;
      break;
    case 1:
      zmin = 0;
      zmax = MaxExtent;
      break;
    default:
      zmin = 1;
      zmax = -1;
      break;
  }

  // Generating an image englobing the extents of every blocks
  // to use as a reference
  vtkNew<vtkImageData> refImage;
  refImage->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, -MaxExtent, MaxExtent);
  FillImage(refImage);

  vtkNew<vtkPointDataToCellData> refImagePointToCell;
  refImagePointToCell->SetInputData(refImage);
  refImagePointToCell->Update();
  vtkImageData* refImagePointToCellDO =
    vtkImageData::SafeDownCast(refImagePointToCell->GetOutputDataObject(0));

  int numberOfGhostLayers = 2;

  // Testing ghost cell generator on vtkImageData

  vtkNew<vtkImageData> image0;
  image0->SetExtent(-MaxExtent, 0, -MaxExtent, 0, zmin, zmax);
  FillImage(image0);

  vtkNew<vtkImageData> image1;
  image1->SetExtent(0, MaxExtent, -MaxExtent, 0, zmin, zmax);
  FillImage(image1);

  vtkNew<vtkImageData> image2;
  image2->SetExtent(0, MaxExtent, 0, MaxExtent, zmin, zmax);
  FillImage(image2);

  vtkNew<vtkImageData> image3;
  image3->SetExtent(-MaxExtent, 0, 0, MaxExtent, zmin, zmax);
  FillImage(image3);

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(image0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(image1);
    point2cell1->Update();

    vtkNew<vtkPointDataToCellData> point2cell2;
    point2cell2->SetInputData(image2);
    point2cell2->Update();

    vtkNew<vtkPointDataToCellData> point2cell3;
    point2cell3->SetInputData(image3);
    point2cell3->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    if (!TestImageCells<vtkImageData>(
          vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0)),
          refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a vtkImageData");
      retVal = EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, image0);
    pds->SetPartition(1, image1);
    pds->SetPartition(2, image2);
    pds->SetPartition(3, image3);

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    if (!TestImagePoints<vtkImageData>(
          vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0)), refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a vtkImageData");
      retVal = EXIT_FAILURE;
    }
  }

  // Testing ghost cell generator on vtkImageData

  vtkNew<vtkRectilinearGrid> rgImage0;
  rgImage0->SetExtent(image0->GetExtent());
  vtkNew<vtkDoubleArray> X0, Y0, Z0;
  rgImage0->SetXCoordinates(X0);
  rgImage0->SetYCoordinates(Y0);
  rgImage0->SetZCoordinates(Z0);
  SetCoordinates(rgImage0->GetXCoordinates(), -MaxExtent, 0, XCoordinates);
  SetCoordinates(rgImage0->GetYCoordinates(), -MaxExtent, 0, YCoordinates);
  SetCoordinates(rgImage0->GetZCoordinates(), zmin, zmax, ZCoordinates);
  rgImage0->DeepCopy(image0);

  vtkNew<vtkRectilinearGrid> rgImage1;
  rgImage1->SetExtent(image1->GetExtent());
  vtkNew<vtkDoubleArray> X1, Y1, Z1;
  rgImage1->SetXCoordinates(X1);
  rgImage1->SetYCoordinates(Y1);
  rgImage1->SetZCoordinates(Z1);
  SetCoordinates(rgImage1->GetXCoordinates(), 0, MaxExtent, XCoordinates);
  SetCoordinates(rgImage1->GetYCoordinates(), -MaxExtent, 0, YCoordinates);
  SetCoordinates(rgImage1->GetZCoordinates(), zmin, zmax, ZCoordinates);
  rgImage1->DeepCopy(image1);

  vtkNew<vtkRectilinearGrid> rgImage2;
  rgImage2->SetExtent(image2->GetExtent());
  vtkNew<vtkDoubleArray> X2, Y2, Z2;
  rgImage2->SetXCoordinates(X2);
  rgImage2->SetYCoordinates(Y2);
  rgImage2->SetZCoordinates(Z2);
  SetCoordinates(rgImage2->GetXCoordinates(), 0, MaxExtent, XCoordinates);
  SetCoordinates(rgImage2->GetYCoordinates(), 0, MaxExtent, YCoordinates);
  SetCoordinates(rgImage2->GetZCoordinates(), zmin, zmax, ZCoordinates);
  rgImage2->DeepCopy(image2);

  vtkNew<vtkRectilinearGrid> rgImage3;
  rgImage3->SetExtent(image3->GetExtent());
  vtkNew<vtkDoubleArray> X3, Y3, Z3;
  rgImage3->SetXCoordinates(X3);
  rgImage3->SetYCoordinates(Y3);
  rgImage3->SetZCoordinates(Z3);
  SetCoordinates(rgImage3->GetXCoordinates(), -MaxExtent, 0, XCoordinates);
  SetCoordinates(rgImage3->GetYCoordinates(), 0, MaxExtent, YCoordinates);
  SetCoordinates(rgImage3->GetZCoordinates(), zmin, zmax, ZCoordinates);
  rgImage3->DeepCopy(image3);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, rgImage0);
    pds->SetPartition(1, rgImage1);
    pds->SetPartition(2, rgImage2);
    pds->SetPartition(3, rgImage3);

    vtkLog(INFO, "Testing ghost points for vtkRectilinearGrid");
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    if (!TestImagePoints<vtkRectilinearGrid>(
          vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0)), refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a vtkRectilinearGrid");
      retVal = EXIT_FAILURE;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(rgImage0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(rgImage1);
    point2cell1->Update();

    vtkNew<vtkPointDataToCellData> point2cell2;
    point2cell2->SetInputData(rgImage2);
    point2cell2->Update();

    vtkNew<vtkPointDataToCellData> point2cell3;
    point2cell3->SetInputData(rgImage3);
    point2cell3->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost points for vtkRectilinearGrid");
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    if (!TestImageCells<vtkRectilinearGrid>(
          vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0)),
          refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a vtkRectilinearGrid");
      retVal = EXIT_FAILURE;
    }
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}

#undef _USE_MATH_DEFINES
