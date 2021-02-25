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
constexpr int MaxExtent = 5;
constexpr int GridWidth = 2 * MaxExtent + 1;
constexpr double XCoordinates[GridWidth] = { -40.0, -25.0, -12.0, -10.0, -4.0, -3.0, 2.0, 10.0,
  12.0, 20.0, 21.0 };
constexpr double YCoordinates[GridWidth] = { -13.0, -12.0, -11.0, -10.0, -6.0, -3.0, -1.0, 4.0, 5.0,
  10.0, 11.0 };
constexpr double ZCoordinates[GridWidth] = { -9.0, -5.0, -3.0, 0, 2.0, 3.0, 4.0, 6.0, 15.0, 20.0,
  21.0 };
constexpr char GridArrayName[] = "grid_data";

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
template <class GridDataSetT>
void CopyGrid(vtkNew<GridDataSetT>& src, vtkStructuredGrid* dest)
{
  const int* extent = src->GetExtent();
  int ijk[3];
  vtkNew<vtkPoints> destPoints;
  destPoints->SetNumberOfPoints(
    (extent[5] - extent[4] + 1) * (extent[3] - extent[2] + 1) * (extent[1] - extent[0] + 1));
  for (int k = extent[4]; k <= extent[5]; ++k)
  {
    ijk[2] = k;
    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      ijk[1] = j;
      for (int i = extent[0]; i <= extent[1]; ++i)
      {
        ijk[0] = i;
        vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
        destPoints->SetPoint(pointId, src->GetPoint(pointId));
      }
    }
  }
  dest->SetPoints(destPoints);
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
template <class GridDataSetT1, class GridDataSetT2>
bool TestImageCellData(vtkPartitionedDataSet* pds, GridDataSetT2* refImage)
{
  const int* refExtent = refImage->GetExtent();
  vtkDataArray* refArray = refImage->GetCellData()->GetArray(GridArrayName);
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    GridDataSetT1* part = GridDataSetT1::SafeDownCast(pds->GetPartition(partitionId));
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
    for (int k = extent[4] + 1; k < extent[5]; ++k)
    {
      for (int j = extent[2] + 1; j < extent[3]; ++j)
      {
        for (int i = extent[0] + 1; i < extent[1]; ++i)
        {
          int ijk[3] = { i, j, k };
          vtkIdType refCellId = vtkStructuredData::ComputeCellIdForExtent(refExtent, ijk);
          vtkIdType cellId = vtkStructuredData::ComputeCellIdForExtent(extent, ijk);
          if (array->GetTuple1(cellId) != refArray->GetTuple1(refCellId))
          {
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
bool TestImagePointData(vtkPartitionedDataSet* pds, vtkImageData* refImage)
{
  const int* refExtent = refImage->GetExtent();
  vtkDataArray* refArray = refImage->GetPointData()->GetArray(GridArrayName);
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    GridDataSetT* part = GridDataSetT::SafeDownCast(pds->GetPartition(partitionId));
    if (!part)
    {
      vtkLog(ERROR, "No part!!");
      return false;
    }
    vtkDataArray* array = part->GetPointData()->GetArray(GridArrayName);
    if (!array)
    {
      vtkLog(ERROR, "NO ARRAY");
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
          vtkIdType refPointId = vtkStructuredData::ComputePointIdForExtent(refExtent, ijk);
          vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
          if (array->GetTuple1(pointId) != refArray->GetTuple1(refPointId))
          {
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
bool TestGridPoints(vtkPartitionedDataSet* pds, vtkRectilinearGrid* refGrid)
{
  const int* refExtent = refGrid->GetExtent();
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    GridDataSetT* part = GridDataSetT::SafeDownCast(pds->GetPartition(partitionId));
    if (!part)
    {
      vtkLog(ERROR, "No part!!");
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
          vtkIdType refPointId = vtkStructuredData::ComputePointIdForExtent(refExtent, ijk);
          vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
          double* p1 = part->GetPoint(pointId);
          double* p2 = refGrid->GetPoint(refPointId);
          if (p1[0] != p2[0] || p1[1] != p2[1] || p1[2] != p2[2])
          {
            return false;
          }
        }
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool TestExtent(const int* extent1, const int* extent2)
{
  return extent1[0] == extent2[0] && extent1[1] == extent2[1] && extent1[2] == extent2[2] &&
    extent1[3] == extent2[3] && extent1[4] == extent2[4] && extent1[5] == extent2[5];
}

//----------------------------------------------------------------------------
bool Test1DGrids(int myrank)
{
  bool retVal = true;

  int xmin, xmax;

  switch (myrank)
  {
    case 0:
      xmin = -MaxExtent;
      xmax = 0;
      break;
    case 1:
      xmin = 0;
      xmax = MaxExtent;
      break;
    default:
      xmin = 1;
      xmax = -1;
      break;
  }

  vtkNew<vtkImageData> refImage;
  refImage->SetExtent(-MaxExtent, MaxExtent, 0, 0, 0, 0);
  FillImage(refImage);

  vtkNew<vtkPointDataToCellData> refImagePointToCell;
  refImagePointToCell->SetInputData(refImage);
  refImagePointToCell->Update();
  vtkImageData* refImagePointToCellDO =
    vtkImageData::SafeDownCast(refImagePointToCell->GetOutputDataObject(0));

  int numberOfGhostLayers = 2;

  const int newExtent[6] = { xmin != 0 ? xmin : -numberOfGhostLayers,
    xmax != 0 ? xmax : numberOfGhostLayers, 0, 0, 0, 0 };

  vtkNew<vtkImageData> image;
  image->SetExtent(xmin, xmax, 0, 0, 0, 0);
  FillImage(image);

  {
    vtkNew<vtkPointDataToCellData> point2cell;
    point2cell->SetInputData(image);
    point2cell->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, point2cell->GetOutputDataObject(0));

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 1D vtkImageData in rank " << myrank);
    if (!TestImageCellData<vtkImageData>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 1D vtkImageData in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(newExtent, vtkImageData::SafeDownCast(outPDS->GetPartition(0))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 1D vtkImageData in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, image);

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost points for 1D vtkImageData in rank " << myrank);
    if (!TestImagePointData<vtkImageData>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 1D vtkImageData in rank " << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkRectilinearGrid> refGrid;
  refGrid->SetExtent(-MaxExtent, MaxExtent, 0, 0, 0, 0);
  vtkNew<vtkDoubleArray> X, Y, Z;
  refGrid->SetXCoordinates(X);
  refGrid->SetYCoordinates(Y);
  refGrid->SetZCoordinates(Z);
  SetCoordinates(X, -MaxExtent, MaxExtent, XCoordinates);
  SetCoordinates(Y, 0, 0, YCoordinates);
  SetCoordinates(Z, 0, 0, ZCoordinates);

  vtkNew<vtkRectilinearGrid> rgImage;
  rgImage->SetExtent(image->GetExtent());
  vtkNew<vtkDoubleArray> X0, Y0, Z0;
  rgImage->SetXCoordinates(X0);
  rgImage->SetYCoordinates(Y0);
  rgImage->SetZCoordinates(Z0);
  SetCoordinates(rgImage->GetXCoordinates(), xmin, xmax, XCoordinates);
  SetCoordinates(rgImage->GetYCoordinates(), 0, 0, YCoordinates);
  SetCoordinates(rgImage->GetZCoordinates(), 0, 0, ZCoordinates);
  rgImage->DeepCopy(image);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, rgImage);

    vtkLog(INFO, "Testing ghost points for 1D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkRectilinearGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 1D vtkRectilinearGrid in rank" << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 1D vtkRectilinearGrid in rank" << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkRectilinearGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 1D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell;
    point2cell->SetInputData(rgImage);
    point2cell->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, point2cell->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 1D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkRectilinearGrid>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 1D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkStructuredGrid> sgImage;
  sgImage->SetExtent(image->GetExtent());
  CopyGrid(rgImage, sgImage);
  sgImage->DeepCopy(image);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, sgImage);

    vtkLog(INFO, "Testing ghost points for 1D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkStructuredGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 1D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 1D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkStructuredGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 1D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell;
    point2cell->SetInputData(sgImage);
    point2cell->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(1);

    pds->SetPartition(0, point2cell->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 1D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkNew<vtkStructuredGrid> sgRefImage;
    sgRefImage->SetExtent(refImage->GetExtent());
    CopyGrid(refImage, sgRefImage);
    sgRefImage->ShallowCopy(refImage);

    vtkNew<vtkPointDataToCellData> sgRefImagePointToCell;
    sgRefImagePointToCell->SetInputData(sgRefImage);
    sgRefImagePointToCell->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkStructuredGrid>(
          outPDS, vtkStructuredGrid::SafeDownCast(sgRefImagePointToCell->GetOutputDataObject(0))))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 1D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool Test2DGrids(int myrank)
{
  bool retVal = true;

  int ymin, ymax;

  switch (myrank)
  {
    case 0:
      ymin = -MaxExtent;
      ymax = 0;
      break;
    case 1:
      ymin = 0;
      ymax = MaxExtent;
      break;
    default:
      ymin = 1;
      ymax = -1;
      break;
  }

  vtkNew<vtkImageData> refImage;
  refImage->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, 0, 0);
  FillImage(refImage);

  vtkNew<vtkPointDataToCellData> refImagePointToCell;
  refImagePointToCell->SetInputData(refImage);
  refImagePointToCell->Update();
  vtkImageData* refImagePointToCellDO =
    vtkImageData::SafeDownCast(refImagePointToCell->GetOutputDataObject(0));

  int numberOfGhostLayers = 2;

  const int newExtent0[6] = { -MaxExtent, numberOfGhostLayers,
    ymin != 0 ? ymin : -numberOfGhostLayers, ymax != 0 ? ymax : numberOfGhostLayers, 0, 0 };

  const int newExtent1[6] = { -numberOfGhostLayers, MaxExtent,
    ymin != 0 ? ymin : -numberOfGhostLayers, ymax != 0 ? ymax : numberOfGhostLayers, 0, 0 };

  vtkNew<vtkImageData> image0;
  image0->SetExtent(-MaxExtent, 0, ymin, ymax, 0, 0);
  FillImage(image0);

  vtkNew<vtkImageData> image1;
  image1->SetExtent(0, MaxExtent, ymin, ymax, 0, 0);
  FillImage(image1);

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(image0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(image1);
    point2cell1->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 2D vtkImageData in rank " << myrank);
    if (!TestImageCellData<vtkImageData>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 2D vtkImageData in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(newExtent0, vtkImageData::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(newExtent1, vtkImageData::SafeDownCast(outPDS->GetPartition(1))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 2D vtkImageData in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, image0);
    pds->SetPartition(1, image1);

    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost points for 2D vtkImageData in rank " << myrank);
    if (!TestImagePointData<vtkImageData>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 2D vtkImageData in rank " << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkRectilinearGrid> refGrid;
  refGrid->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, 0, 0);
  vtkNew<vtkDoubleArray> X, Y, Z;
  refGrid->SetXCoordinates(X);
  refGrid->SetYCoordinates(Y);
  refGrid->SetZCoordinates(Z);
  SetCoordinates(X, -MaxExtent, MaxExtent, XCoordinates);
  SetCoordinates(Y, -MaxExtent, MaxExtent, YCoordinates);
  SetCoordinates(Z, 0, 0, ZCoordinates);

  vtkNew<vtkRectilinearGrid> rgImage0;
  rgImage0->SetExtent(image0->GetExtent());
  vtkNew<vtkDoubleArray> X0, Y0, Z0;
  rgImage0->SetXCoordinates(X0);
  rgImage0->SetYCoordinates(Y0);
  rgImage0->SetZCoordinates(Z0);
  SetCoordinates(rgImage0->GetXCoordinates(), -MaxExtent, 0, XCoordinates);
  SetCoordinates(rgImage0->GetYCoordinates(), ymin, ymax, YCoordinates);
  SetCoordinates(rgImage0->GetZCoordinates(), 0, 0, ZCoordinates);
  rgImage0->DeepCopy(image0);

  vtkNew<vtkRectilinearGrid> rgImage1;
  rgImage1->SetExtent(image1->GetExtent());
  vtkNew<vtkDoubleArray> X1, Y1, Z1;
  rgImage1->SetXCoordinates(X1);
  rgImage1->SetYCoordinates(Y1);
  rgImage1->SetZCoordinates(Z1);
  SetCoordinates(rgImage1->GetXCoordinates(), 0, MaxExtent, XCoordinates);
  SetCoordinates(rgImage1->GetYCoordinates(), ymin, ymax, YCoordinates);
  SetCoordinates(rgImage1->GetZCoordinates(), 0, 0, ZCoordinates);
  rgImage1->DeepCopy(image1);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, rgImage0);
    pds->SetPartition(1, rgImage1);

    vtkLog(INFO, "Testing ghost points for 2D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkRectilinearGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 2D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent0, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(
        newExtent1, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(1))->GetExtent()))
    {
      vtkLog(
        ERROR, "Wrong extent when adding ghosts on a 2D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkRectilinearGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 2D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(rgImage0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(rgImage1);
    point2cell1->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 2D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkRectilinearGrid>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 2D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkStructuredGrid> sgImage0;
  sgImage0->SetExtent(image0->GetExtent());
  CopyGrid(rgImage0, sgImage0);
  sgImage0->DeepCopy(image0);

  vtkNew<vtkStructuredGrid> sgImage1;
  sgImage1->SetExtent(image1->GetExtent());
  CopyGrid(rgImage1, sgImage1);
  sgImage1->DeepCopy(image1);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, sgImage0);
    pds->SetPartition(1, sgImage1);

    vtkLog(INFO, "Testing ghost points for 2D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkStructuredGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 2D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent0, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(
        newExtent1, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(1))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 2D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkStructuredGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 2D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(sgImage0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(sgImage1);
    point2cell1->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(2);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 2D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkNew<vtkStructuredGrid> sgRefImage;
    sgRefImage->SetExtent(refImage->GetExtent());
    CopyGrid(refImage, sgRefImage);
    sgRefImage->ShallowCopy(refImage);

    vtkNew<vtkPointDataToCellData> sgRefImagePointToCell;
    sgRefImagePointToCell->SetInputData(sgRefImage);
    sgRefImagePointToCell->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkStructuredGrid>(
          outPDS, vtkStructuredGrid::SafeDownCast(sgRefImagePointToCell->GetOutputDataObject(0))))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 2D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool Test3DGrids(int myrank)
{
  bool retVal = true;
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

  const int newExtent0[6] = { -MaxExtent, numberOfGhostLayers, -MaxExtent, numberOfGhostLayers,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent1[6] = { -numberOfGhostLayers, MaxExtent, -MaxExtent, numberOfGhostLayers,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent2[6] = { -numberOfGhostLayers, MaxExtent, -numberOfGhostLayers, MaxExtent,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent3[6] = { -MaxExtent, numberOfGhostLayers, -numberOfGhostLayers, MaxExtent,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

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

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 3D vtkImageData in rank " << myrank);
    if (!TestImageCellData<vtkImageData>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 3D vtkImageData in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(newExtent0, vtkImageData::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(newExtent1, vtkImageData::SafeDownCast(outPDS->GetPartition(1))->GetExtent()) ||
      !TestExtent(newExtent2, vtkImageData::SafeDownCast(outPDS->GetPartition(2))->GetExtent()) ||
      !TestExtent(newExtent3, vtkImageData::SafeDownCast(outPDS->GetPartition(3))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 3D vtkImageData in rank " << myrank);
      retVal = false;
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

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost points for 3D vtkImageData in rank " << myrank);
    if (!TestImagePointData<vtkImageData>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 3D vtkImageData in rank " << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkRectilinearGrid> refGrid;
  refGrid->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, -MaxExtent, MaxExtent);
  vtkNew<vtkDoubleArray> X, Y, Z;
  refGrid->SetXCoordinates(X);
  refGrid->SetYCoordinates(Y);
  refGrid->SetZCoordinates(Z);
  SetCoordinates(X, -MaxExtent, MaxExtent, XCoordinates);
  SetCoordinates(Y, -MaxExtent, MaxExtent, YCoordinates);
  SetCoordinates(Z, -MaxExtent, MaxExtent, ZCoordinates);

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

    vtkLog(INFO, "Testing ghost points for 3D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkRectilinearGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 3D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent0, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(
        newExtent1, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(1))->GetExtent()) ||
      !TestExtent(
        newExtent2, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(2))->GetExtent()) ||
      !TestExtent(
        newExtent3, vtkRectilinearGrid::SafeDownCast(outPDS->GetPartition(3))->GetExtent()))
    {
      vtkLog(
        ERROR, "Wrong extent when adding ghosts on a 3D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkRectilinearGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 3D vtkRectilinearGrid in rank " << myrank);
      retVal = false;
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

    vtkLog(INFO, "Testing ghost cells for 3D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkRectilinearGrid>(outPDS, refImagePointToCellDO))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 3D vtkRectilinearGrid in rank" << myrank);
      retVal = false;
    }
  }

  vtkNew<vtkStructuredGrid> sgImage0;
  sgImage0->SetExtent(image0->GetExtent());
  CopyGrid(rgImage0, sgImage0);
  sgImage0->DeepCopy(image0);

  vtkNew<vtkStructuredGrid> sgImage1;
  sgImage1->SetExtent(image1->GetExtent());
  CopyGrid(rgImage1, sgImage1);
  sgImage1->DeepCopy(image1);

  vtkNew<vtkStructuredGrid> sgImage2;
  sgImage2->SetExtent(image2->GetExtent());
  CopyGrid(rgImage2, sgImage2);
  sgImage2->DeepCopy(image2);

  vtkNew<vtkStructuredGrid> sgImage3;
  sgImage3->SetExtent(image3->GetExtent());
  CopyGrid(rgImage3, sgImage3);
  sgImage3->DeepCopy(image3);

  {
    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, sgImage0);
    pds->SetPartition(1, sgImage1);
    pds->SetPartition(2, sgImage2);
    pds->SetPartition(3, sgImage3);

    vtkLog(INFO, "Testing ghost points for 3D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImagePointData<vtkStructuredGrid>(outPDS, refImage))
    {
      vtkLog(ERROR, "Failed to create ghost points on a 3D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestExtent(
          newExtent0, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(0))->GetExtent()) ||
      !TestExtent(
        newExtent1, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(1))->GetExtent()) ||
      !TestExtent(
        newExtent2, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(2))->GetExtent()) ||
      !TestExtent(
        newExtent3, vtkStructuredGrid::SafeDownCast(outPDS->GetPartition(3))->GetExtent()))
    {
      vtkLog(ERROR, "Wrong extent when adding ghosts on a 3D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    if (!TestGridPoints<vtkStructuredGrid>(outPDS, refGrid))
    {
      vtkLog(ERROR,
        "Ghost point positions were wrongly sent on a 3D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }

  {
    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputData(sgImage0);
    point2cell0->Update();

    vtkNew<vtkPointDataToCellData> point2cell1;
    point2cell1->SetInputData(sgImage1);
    point2cell1->Update();

    vtkNew<vtkPointDataToCellData> point2cell2;
    point2cell2->SetInputData(sgImage2);
    point2cell2->Update();

    vtkNew<vtkPointDataToCellData> point2cell3;
    point2cell3->SetInputData(sgImage3);
    point2cell3->Update();

    vtkNew<vtkPartitionedDataSet> pds;
    pds->SetNumberOfPartitions(4);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 3D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkCompositeDataSetGhostCellsGenerator> generator;
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkNew<vtkStructuredGrid> sgRefImage;
    sgRefImage->SetExtent(refImage->GetExtent());
    CopyGrid(refImage, sgRefImage);
    sgRefImage->ShallowCopy(refImage);

    vtkNew<vtkPointDataToCellData> sgRefImagePointToCell;
    sgRefImagePointToCell->SetInputData(sgRefImage);
    sgRefImagePointToCell->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkStructuredGrid>(
          outPDS, vtkStructuredGrid::SafeDownCast(sgRefImagePointToCell->GetOutputDataObject(0))))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 3D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }
  }
  return retVal;
}
} // anonymous namespace

//----------------------------------------------------------------------------
int TestCompositeDataSetGhostCellsGenerator(int argc, char* argv[])
{
#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  vtkNew<vtkMPIController> contr;
#else
  vtkNew<vtkDummyController> contr;
#endif

  contr->Initialize(&argc, &argv);
  vtkMultiProcessController::SetGlobalController(contr);

  int retVal = EXIT_SUCCESS;
  int myrank = contr->GetLocalProcessId();

  if (!Test1DGrids(myrank))
  {
    retVal = EXIT_FAILURE;
  }

  if (!Test2DGrids(myrank))
  {
    retVal = EXIT_FAILURE;
  }

  if (!Test3DGrids(myrank))
  {
    retVal = EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}

#undef _USE_MATH_DEFINES
