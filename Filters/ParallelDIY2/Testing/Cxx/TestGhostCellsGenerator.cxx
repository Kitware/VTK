/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGhostCellsGenerator.cxx

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

#include "vtkAbstractPointLocator.h"
#include "vtkCellArray.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCommunicator.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGhostCellsGenerator.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStaticPointLocator.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <array>
#include <cmath>
#include <set>

namespace
{
constexpr int MaxExtent = 5;
constexpr int GridWidth = 2 * MaxExtent + 1;
constexpr vtkIdType NumberOfPoints = GridWidth * GridWidth * GridWidth;
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
  return std::cos(i * M_PI / MaxExtent + 1.0) * std::sin(j * M_PI / MaxExtent + 1.0) *
    std::exp(-(k - 1.0) * (k - 1.0) / 11.0);
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
bool TestImageCellData(
  vtkPartitionedDataSet* pds, GridDataSetT2* refImage, bool skipLastPartition = false)
{
  const int* refExtent = refImage->GetExtent();
  vtkDataArray* refArray = refImage->GetCellData()->GetArray(GridArrayName);
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions() - skipLastPartition;
       ++partitionId)
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
            std::cout << array->GetTuple1(pointId) << " != " << refArray->GetTuple1(refPointId)
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
bool TestGhostPointsTagging(
  vtkMultiProcessController* controller, vtkPartitionedDataSet* pds, vtkIdType numberOfPoints)
{
  vtkIdType numberOfNonGhostPoints = 0;
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    vtkDataSet* ps = vtkDataSet::SafeDownCast(pds->GetPartition(partitionId));
    int foo;
    vtkUnsignedCharArray* ghosts = vtkArrayDownCast<vtkUnsignedCharArray>(
      ps->GetPointData()->GetAbstractArray(vtkDataSetAttributes::GhostArrayName(), foo));
    for (vtkIdType pointId = 0; pointId < ps->GetNumberOfPoints(); ++pointId)
    {
      if (!ghosts->GetValue(pointId))
      {
        ++numberOfNonGhostPoints;
      }
    }
  }

  vtkIdType globalNumberOfNonGhostPoints;

  controller->AllReduce(
    &numberOfNonGhostPoints, &globalNumberOfNonGhostPoints, 1, vtkCommunicator::SUM_OP);

  if (globalNumberOfNonGhostPoints != numberOfPoints)
  {
    vtkLog(ERROR,
      "Ghost point tagging failed. We have "
        << globalNumberOfNonGhostPoints
        << " points that are tagged as non ghost, but we should have " << numberOfPoints);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestGhostCellsTagging(
  vtkMultiProcessController* controller, vtkPartitionedDataSet* pds, vtkIdType numberOfCells)
{
  vtkIdType numberOfNonGhostCells = 0;
  for (unsigned int partitionId = 0; partitionId < pds->GetNumberOfPartitions(); ++partitionId)
  {
    vtkDataSet* ps = vtkDataSet::SafeDownCast(pds->GetPartition(partitionId));
    int foo;
    vtkUnsignedCharArray* ghosts = vtkArrayDownCast<vtkUnsignedCharArray>(
      ps->GetCellData()->GetAbstractArray(vtkDataSetAttributes::GhostArrayName(), foo));
    for (vtkIdType cellId = 0; cellId < ps->GetNumberOfCells(); ++cellId)
    {
      if (!ghosts->GetValue(cellId))
      {
        ++numberOfNonGhostCells;
      }
    }
  }

  vtkIdType globalNumberOfNonGhostCells;

  controller->AllReduce(
    &numberOfNonGhostCells, &globalNumberOfNonGhostCells, 1, vtkCommunicator::SUM_OP);

  if (globalNumberOfNonGhostCells != numberOfCells)
  {
    vtkLog(ERROR,
      "Ghost cell tagging failed. We have "
        << globalNumberOfNonGhostCells << " cells that are tagged as non ghost, but we should have "
        << numberOfCells);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
// Testing multiblock input with more than one depth
bool TestDeepMultiBlock()
{
  vtkNew<vtkMultiBlockDataSet> multiBlock;
  vtkNew<vtkMultiPieceDataSet> multiPiece;
  vtkNew<vtkUnstructuredGrid> ug;

  multiBlock->SetNumberOfBlocks(1);
  multiBlock->SetBlock(0, multiPiece);
  multiPiece->SetNumberOfPieces(1);
  multiPiece->SetPiece(0, ug);

  vtkNew<vtkGhostCellsGenerator> generator;
  generator->SetNumberOfGhostLayers(1);
  generator->BuildIfRequiredOff();
  generator->SetInputData(multiBlock);

  // We are just checking if the output structure is generated without crashing.
  // This will crash if the structure of the output doesn't take deep multi blocks into account.
  generator->Update();

  return true;
}

//----------------------------------------------------------------------------
bool TestMixedTypes(int myrank)
{
  vtkLog(INFO, "Testing mixed types");

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(myrank == 1);
  if (myrank == 1)
  {
    vtkNew<vtkImageData> ds;
    pds->SetPartition(0, ds);
  }
  else if (myrank == 0)
  {
    vtkNew<vtkRectilinearGrid> ds;
    pds->SetPartition(0, ds);
  }

  // If mixed types are mishandled, this will crash.
  vtkNew<vtkGhostCellsGenerator> generator;
  generator->SetInputData(pds);
  generator->BuildIfRequiredOff();
  generator->Update();

  return true;
}

//----------------------------------------------------------------------------
bool Test1DGrids(vtkMultiProcessController* controller, int myrank, int numberOfGhostLayers)
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

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool Test2DGrids(vtkMultiProcessController* controller, int myrank, int numberOfGhostLayers)
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

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool Test3DGrids(vtkMultiProcessController* controller, int myrank, int numberOfGhostLayers)
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
    // This preGenerator is testing if the peeling ghosts layers from input is done correctly
    // for grid data sets
    vtkNew<vtkGhostCellsGenerator> preGenerator;
    preGenerator->BuildIfRequiredOff();
    preGenerator->SetInputDataObject(image0);
    preGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);

    vtkNew<vtkPointDataToCellData> point2cell0;
    point2cell0->SetInputConnection(preGenerator->GetOutputPort());
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
    pds->SetNumberOfPartitions(5);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));
    pds->SetPartition(4, vtkNew<vtkImageData>()); // testing empty input

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
    generator->SetInputDataObject(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    vtkLog(INFO, "Testing ghost cells for 3D vtkImageData in rank " << myrank);
    if (!TestImageCellData<vtkImageData>(
          outPDS, refImagePointToCellDO, true /* skipLastPartition */))
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

    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
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
    pds->SetNumberOfPartitions(5);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));
    pds->SetPartition(4, vtkNew<vtkRectilinearGrid>()); // testing empty input

    vtkLog(INFO, "Testing ghost cells for 3D vtkRectilinearGrid in rank " << myrank);
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
    generator->SetInputData(pds);
    generator->SetNumberOfGhostLayers(numberOfGhostLayers);
    generator->Update();

    vtkPartitionedDataSet* outPDS =
      vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

    if (!TestImageCellData<vtkRectilinearGrid>(
          outPDS, refImagePointToCellDO, true /* skipLastPartition */))
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
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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
    pds->SetNumberOfPartitions(5);

    pds->SetPartition(0, point2cell0->GetOutputDataObject(0));
    pds->SetPartition(1, point2cell1->GetOutputDataObject(0));
    pds->SetPartition(2, point2cell2->GetOutputDataObject(0));
    pds->SetPartition(3, point2cell3->GetOutputDataObject(0));
    pds->SetPartition(4, vtkNew<vtkStructuredGrid>()); // testing empty input

    vtkLog(INFO, "Testing ghost cells for 3D vtkStructuredGrid in rank " << myrank);
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->BuildIfRequiredOff();
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

    if (!TestImageCellData<vtkStructuredGrid>(outPDS,
          vtkStructuredGrid::SafeDownCast(sgRefImagePointToCell->GetOutputDataObject(0)),
          true /* skipLastPartition */))
    {
      vtkLog(ERROR, "Failed to create ghost cells on a 3D vtkStructuredGrid in rank " << myrank);
      retVal = false;
    }

    vtkIdType pointsLength = 2 * MaxExtent + 1;
    vtkIdType numberOfPoints = pointsLength * pointsLength * pointsLength;
    vtkIdType cellsLength = 2 * MaxExtent;
    vtkIdType numberOfCells = cellsLength * cellsLength * cellsLength;

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

    if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
    {
      retVal = false;
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> Convert3DImageToUnstructuredGrid(
  vtkImageData* input, bool producePolyhedrons = true)
{
  vtkSmartPointer<vtkUnstructuredGrid> output = vtkSmartPointer<vtkUnstructuredGrid>::New();

  output->ShallowCopy(input);
  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetNumberOfPoints(input->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < points->GetNumberOfPoints(); ++pointId)
  {
    points->SetPoint(pointId, input->GetPoint(pointId));
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();

  using ArrayType32 = vtkCellArray::ArrayType32;
  vtkNew<vtkCellArray> cells;
  cells->Use32BitStorage();

  ArrayType32* offsets = cells->GetOffsetsArray32();
  offsets->SetNumberOfValues(numberOfCells + 1);
  for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
  {
    offsets->SetValue(id, 8 * id);
  }

  const int* extent = input->GetExtent();
  vtkNew<vtkIdTypeArray> faces;
  // half cells * number of faces in voxel [6] * (number of points in face + 1) [4 + 1]
  faces->SetNumberOfValues((numberOfCells / 2) * (6 * 5 + 1));

  vtkNew<vtkIdTypeArray> faceLocations;
  faceLocations->SetNumberOfValues(numberOfCells);

  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfValues(numberOfCells);

  ArrayType32* connectivity = cells->GetConnectivityArray32();
  connectivity->SetNumberOfValues(8 * numberOfCells);
  int ijkCell[3];
  int ijkPoint[3];
  vtkIdType connectivityId = 0;
  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkStructuredData::ComputeCellStructuredCoordsForExtent(cellId, extent, ijkCell);
    for (ijkPoint[2] = ijkCell[2]; ijkPoint[2] <= ijkCell[2] + 1; ++ijkPoint[2])
    {
      for (ijkPoint[1] = ijkCell[1]; ijkPoint[1] <= ijkCell[1] + 1; ++ijkPoint[1])
      {
        for (ijkPoint[0] = ijkCell[0]; ijkPoint[0] <= ijkCell[0] + 1; ++ijkPoint[0])
        {
          connectivity->SetValue(
            connectivityId++, vtkStructuredData::ComputePointIdForExtent(extent, ijkPoint));
        }
      }
    }

    if (producePolyhedrons && cellId % 2)
    {
      vtkIdType id = ((cellId / 2) * (5 * 6 + 1));
      faceLocations->SetValue(cellId, id);

      types->SetValue(cellId, VTK_POLYHEDRON);

      faces->SetValue(id, 6); // 6 faces.
      ++id;

      vtkIdType offsetId = connectivityId - 8;
      faces->SetValue(id, 4); // 4 points per face.
      // Bottom face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 0));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 1));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 3));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 2));

      id += 5;
      faces->SetValue(id, 4); // 4 points per face.
      // Top face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 4));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 5));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 7));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 6));

      id += 5;
      faces->SetValue(id, 4); // 4 points per face.
      // Front face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 0));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 1));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 5));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 4));

      id += 5;
      faces->SetValue(id, 4); // 4 points per face.
      // Back face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 2));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 3));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 7));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 6));

      id += 5;
      faces->SetValue(id, 4); // 4 points per face.
      // Left face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 0));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 2));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 6));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 4));

      id += 5;
      faces->SetValue(id, 4); // 4 points per face.
      // Right face
      faces->SetValue(id + 1, connectivity->GetValue(offsetId + 1));
      faces->SetValue(id + 2, connectivity->GetValue(offsetId + 3));
      faces->SetValue(id + 3, connectivity->GetValue(offsetId + 7));
      faces->SetValue(id + 4, connectivity->GetValue(offsetId + 5));
    }
    else
    {
      faceLocations->SetValue(cellId, -1);
      types->SetValue(cellId, VTK_VOXEL);
    }
  }

  if (producePolyhedrons)
  {
    output->SetCells(types, cells, faceLocations, faces);
  }
  else
  {
    output->SetCells(VTK_VOXEL, cells);
  }

  return output;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> Convert2DImageToPolyData(
  vtkImageData* input, bool produceStrips = false)
{
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();

  output->ShallowCopy(input);
  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetNumberOfPoints(input->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < points->GetNumberOfPoints(); ++pointId)
  {
    points->SetPoint(pointId, input->GetPoint(pointId));
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();

  using ArrayType32 = vtkCellArray::ArrayType32;
  vtkNew<vtkCellArray> polys, strips;
  polys->Use32BitStorage();
  strips->Use32BitStorage();

  {
    ArrayType32* offsets = polys->GetOffsetsArray32();
    offsets->SetNumberOfValues(
      produceStrips ? numberOfCells / 2 + numberOfCells % 2 + 1 : numberOfCells + 1);
    for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
    {
      offsets->SetValue(id, 4 * id);
    }
  }
  {
    ArrayType32* offsets = strips->GetOffsetsArray32();
    offsets->SetNumberOfValues(produceStrips ? numberOfCells / 2 + 1 : 0);
    for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
    {
      offsets->SetValue(id, 4 * id);
    }
  }

  const int* extent = input->GetExtent();
  constexpr vtkIdType pixel2hexMap[4] = { 0, 1, 3, 2 };

  ArrayType32* polyConnectivity = polys->GetConnectivityArray32();
  polyConnectivity->SetNumberOfValues((polys->GetOffsetsArray()->GetNumberOfValues() - 1) * 4);
  ArrayType32* stripConnectivity = strips->GetConnectivityArray32();
  stripConnectivity->SetNumberOfValues(
    produceStrips ? (strips->GetOffsetsArray()->GetNumberOfValues() - 1) * 4 : 0);

  int ijkCell[3] = { 0, 0, 0 };
  int ijkPoint[3] = { 0, 0, 0 };
  vtkIdType polyConnectivityId = 0, stripConnectivityId = 0;

  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkStructuredData::ComputeCellStructuredCoordsForExtent(cellId, extent, ijkCell);

    if (!produceStrips || (produceStrips && !(cellId % 2)))
    {
      int counter = 0;
      for (ijkPoint[0] = ijkCell[0]; ijkPoint[0] <= ijkCell[0] + 1; ++ijkPoint[0])
      {
        for (ijkPoint[1] = ijkCell[1]; ijkPoint[1] <= ijkCell[1] + 1; ++ijkPoint[1], ++counter)
        {
          vtkIdType id = vtkStructuredData::ComputePointIdForExtent(extent, ijkPoint);
          polyConnectivity->SetValue(polyConnectivityId + pixel2hexMap[counter], id);
        }
      }
      polyConnectivityId += 4;
    }
    else
    {
      int counter = 0;
      for (ijkPoint[0] = ijkCell[0]; ijkPoint[0] <= ijkCell[0] + 1; ++ijkPoint[0])
      {
        for (ijkPoint[1] = ijkCell[1]; ijkPoint[1] <= ijkCell[1] + 1; ++ijkPoint[1], ++counter)
        {
          vtkIdType id = vtkStructuredData::ComputePointIdForExtent(extent, ijkPoint);
          stripConnectivity->SetValue(stripConnectivityId + counter, id);
        }
      }
      stripConnectivityId += 4;
    }
  }

  if (produceStrips)
  {
    output->SetStrips(strips);
  }
  output->SetPolys(polys);

  return output;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> Convert1DImageToPolyData(vtkImageData* input)
{
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();

  output->ShallowCopy(input);
  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetNumberOfPoints(input->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < points->GetNumberOfPoints(); ++pointId)
  {
    points->SetPoint(pointId, input->GetPoint(pointId));
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();

  using ArrayType32 = vtkCellArray::ArrayType32;
  vtkNew<vtkCellArray> lines;
  lines->Use32BitStorage();

  ArrayType32* offsets = lines->GetOffsetsArray32();
  offsets->SetNumberOfValues(numberOfCells + 1);
  for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
  {
    offsets->SetValue(id, 2 * id);
  }

  ArrayType32* connectivity = lines->GetConnectivityArray32();
  connectivity->SetNumberOfValues(numberOfCells * 2);

  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    connectivity->SetValue(2 * cellId, cellId);
    connectivity->SetValue(2 * cellId + 1, cellId + 1);
  }

  output->SetLines(lines);

  return output;
}

//----------------------------------------------------------------------------
void GenerateGlobalIds(vtkPointSet* ps, const int localExtent[6])
{
  vtkNew<vtkIdTypeArray> gids;
  gids->SetNumberOfValues(ps->GetNumberOfPoints());
  gids->SetName("GlobalIds");
  int ijk[3];
  vtkIdType pointId = 0;
  constexpr int extent[6] = { -MaxExtent, MaxExtent, -MaxExtent, MaxExtent, -MaxExtent, MaxExtent };

  for (ijk[2] = localExtent[4]; ijk[2] <= localExtent[5]; ++ijk[2])
  {
    for (ijk[1] = localExtent[2]; ijk[1] <= localExtent[3]; ++ijk[1])
    {
      for (ijk[0] = localExtent[0]; ijk[0] <= localExtent[1]; ++ijk[0], ++pointId)
      {
        gids->SetValue(pointId, vtkStructuredData::ComputePointIdForExtent(extent, ijk));
      }
    }
  }

  ps->GetPointData()->SetGlobalIds(gids);
}

//----------------------------------------------------------------------------
bool TestVoxelCellsVolume(vtkDataSet* ds)
{
  double p1[3], p2[3], p3[3], p4[3], p5[4], p6[4], p7[4], p8[4];
  double diff[3];
  constexpr double eps = 0.00001;
  for (vtkIdType cellId = 0; cellId < ds->GetNumberOfCells(); ++cellId)
  {
    vtkCell* cell = ds->GetCell(cellId);
    vtkPoints* points = cell->GetPoints();

    points->GetPoint(0, p1);
    points->GetPoint(1, p2);
    points->GetPoint(2, p3);
    points->GetPoint(3, p4);
    points->GetPoint(4, p5);
    points->GetPoint(5, p6);
    points->GetPoint(6, p7);
    points->GetPoint(7, p8);

    vtkMath::Subtract(p2, p1, diff);
    if (std::fabs(diff[0] - 1.0) > eps || std::fabs(diff[1]) > eps || std::fabs(diff[2]) > eps)
    {
      vtkLog(INFO, "p2diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p3, p1, diff);
    if (std::fabs(diff[0]) > eps || std::fabs(diff[1] - 1.0) > eps || std::fabs(diff[2]) > eps)
    {
      vtkLog(INFO, "p3diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p4, p1, diff);
    if (std::fabs(diff[0] - 1.0) > eps || std::fabs(diff[1] - 1) > eps || std::fabs(diff[2]) > eps)
    {
      vtkLog(INFO, "p4diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p5, p1, diff);
    if (std::fabs(diff[0]) > eps || std::fabs(diff[1]) > eps || std::fabs(diff[2] - 1.0) > eps)
    {
      vtkLog(INFO, "p5diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p6, p1, diff);
    if (std::fabs(diff[0] - 1.0) > eps || std::fabs(diff[1]) > eps ||
      std::fabs(diff[2] - 1.0) > eps)
    {
      vtkLog(INFO, "p6diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p7, p1, diff);
    if (std::fabs(diff[0]) > eps || std::fabs(diff[1] - 1.0) > eps ||
      std::fabs(diff[2] - 1.0) > eps)
    {
      vtkLog(INFO, "p7diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }

    vtkMath::Subtract(p8, p1, diff);
    if (std::fabs(diff[0] - 1.0) > eps || std::fabs(diff[1] - 1.0) > eps ||
      std::fabs(diff[2] - 1.0) > eps)
    {
      vtkLog(INFO, "p8diff " << diff[0] << ", " << diff[1] << ", " << diff[2]);
      return false;
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestQueryReferenceToGenerated(vtkPointSet* ref, vtkAbstractPointLocator* refLocator,
  vtkPointSet* gen, bool centers = false, bool ignorePointPosition = false)
{
  vtkPoints* points = gen->GetPoints();
  vtkPoints* refPoints = ref->GetPoints();
  std::set<vtkIdType> passThroughAllPointsCheck;

  vtkDoubleArray* data =
    vtkArrayDownCast<vtkDoubleArray>(gen->GetPointData()->GetArray(GridArrayName));
  if (!data)
  {
    if (!centers)
    {
      vtkLog(ERROR, "Point data scalar field is absent from generated unstructured grid.");
    }
    else
    {
      vtkLog(ERROR, "Cell data scalar field is absent from generated unstructured grid.");
    }
    return false;
  }
  vtkDoubleArray* refData =
    vtkArrayDownCast<vtkDoubleArray>(ref->GetPointData()->GetArray(GridArrayName));

  for (vtkIdType pointId = 0; pointId < gen->GetNumberOfPoints(); ++pointId)
  {
    double* p = points->GetPoint(pointId);
    vtkIdType refPointId = refLocator->FindClosestPoint(p);
    passThroughAllPointsCheck.insert(refPointId);
    double* refp = refPoints->GetPoint(refPointId);
    if (!ignorePointPosition && (refp[0] != p[0] || refp[1] != p[1] || refp[2] != p[2]))
    {
      vtkLog(ERROR,
        "Generated point not present in reference data set: ("
          << p[0] << ", " << p[1] << ", " << p[2] << ") != (" << refp[0] << ", " << refp[1] << ", "
          << refp[2] << ").");
      return false;
    }

    // There can be rounding errors on triangle strips vs voxels in a poly data when executing
    // vtkPointDataToCellData
    if (std::abs(refData->GetValue(refPointId) - data->GetValue(pointId)) > 1e-15)
    {
      if (!centers)
      {
        vtkLog(ERROR, "Generated output for unstructured data failed to copy point data.");
      }
      else
      {
        vtkLog(ERROR, "Generated output for unstructured data failed to copy cell data.");
      }
      return false;
    }
  }

  if (static_cast<vtkIdType>(passThroughAllPointsCheck.size()) != points->GetNumberOfPoints())
  {
    if (!centers)
    {
      vtkLog(ERROR, "It seems that there are duplicate point locations in the generated points.");
    }
    else
    {
      vtkLog(ERROR, "Something's off with cell geometry in the generated output.");
    }
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestUnstructuredGrid(
  vtkMultiProcessController* controller, int myrank, int numberOfGhostLayers)
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

  {
    vtkNew<vtkUnstructuredGrid> emptyUG;
    // Calling Initialize sets GetCells to nullptr
    emptyUG->Initialize();

    // We are checking if the filter crashes in this instance.
    vtkNew<vtkGhostCellsGenerator> generator;
    generator->SetInputData(emptyUG);
    generator->BuildIfRequiredOff();
    generator->SetNumberOfGhostLayers(1);
    generator->Update();
  }

  // Generating an image englobing the extents of every blocks
  // to use as a reference
  vtkNew<vtkImageData> refImage;
  refImage->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, -MaxExtent, MaxExtent);
  FillImage(refImage);

  vtkSmartPointer<vtkUnstructuredGrid> refUG = Convert3DImageToUnstructuredGrid(refImage);

  vtkNew<vtkStaticPointLocator> refLocator;
  refLocator->SetDataSet(refUG);
  refLocator->BuildLocator();

  vtkNew<vtkPointDataToCellData> refPointToCell;
  refPointToCell->SetInputData(refUG);
  refPointToCell->Update();

  vtkNew<vtkImageData> image0;
  image0->SetExtent(-MaxExtent, 0, -MaxExtent, 0, zmin, zmax);
  FillImage(image0);
  vtkSmartPointer<vtkUnstructuredGrid> ug0 = Convert3DImageToUnstructuredGrid(image0, false);

  vtkNew<vtkImageData> image1;
  image1->SetExtent(0, MaxExtent, -MaxExtent, 0, zmin, zmax);
  FillImage(image1);
  vtkSmartPointer<vtkUnstructuredGrid> ug1 = Convert3DImageToUnstructuredGrid(image1);

  vtkNew<vtkImageData> image2;
  image2->SetExtent(0, MaxExtent, 0, MaxExtent, zmin, zmax);
  FillImage(image2);
  vtkSmartPointer<vtkUnstructuredGrid> ug2 = Convert3DImageToUnstructuredGrid(image2);

  vtkNew<vtkImageData> image3;
  image3->SetExtent(-MaxExtent, 0, 0, MaxExtent, zmin, zmax);
  FillImage(image3);
  vtkSmartPointer<vtkUnstructuredGrid> ug3 = Convert3DImageToUnstructuredGrid(image3);

  vtkNew<vtkPointDataToCellData> point2cell0;
  point2cell0->SetInputData(ug0);
  point2cell0->Update();

  vtkNew<vtkPointDataToCellData> point2cell1;
  point2cell1->SetInputData(ug1);
  point2cell1->Update();

  vtkNew<vtkPointDataToCellData> point2cell2;
  point2cell2->SetInputData(ug2);
  point2cell2->Update();

  vtkNew<vtkPointDataToCellData> point2cell3;
  point2cell3->SetInputData(ug3);
  point2cell3->Update();

  vtkLog(INFO, "Testing ghost cells for vtkUnstructuredGrid in rank " << myrank);

  vtkNew<vtkPartitionedDataSet> prePds;
  prePds->SetNumberOfPartitions(1);

  prePds->SetPartition(0, ug0);

  // We do a simple case with only one ug per rank.
  // We will use the output of this generator for the next more complex generation,
  // and ensure that when ghosts are present in the input, everything works fine.
  vtkNew<vtkGhostCellsGenerator> preGenerator;
  preGenerator->BuildIfRequiredOff();
  preGenerator->SetInputDataObject(prePds);
  preGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  preGenerator->Update();

  vtkPartitionedDataSet* outPrePds =
    vtkPartitionedDataSet::SafeDownCast(preGenerator->GetOutputDataObject(0));
  vtkUnstructuredGrid* preug = vtkUnstructuredGrid::SafeDownCast(outPrePds->GetPartition(0));

  if (preug->GetNumberOfCells() != (MaxExtent * MaxExtent * (MaxExtent + numberOfGhostLayers)))
  {
    vtkLog(ERROR,
      "Wrong number of output cells for a one to one ghost cell generation:"
        << " we should have " << (MaxExtent * MaxExtent * (MaxExtent + numberOfGhostLayers))
        << ", instead we have " << preug->GetNumberOfCells());
    retVal = false;
  }

  if (preug->GetNumberOfPoints() !=
    ((MaxExtent + 1) * (MaxExtent + 1) * (MaxExtent + 1 + numberOfGhostLayers)))
  {
    vtkLog(ERROR,
      "Wrong number of output points for a one to one ghost cell generation:"
        << " we should have "
        << ((MaxExtent + 1) * (MaxExtent + 1) * (MaxExtent + 1 + numberOfGhostLayers))
        << ", instead we have " << preug->GetNumberOfCells());
    retVal = false;
  }

  if (!TestQueryReferenceToGenerated(refUG, refLocator, preug))
  {
    retVal = false;
  }

  if (!TestVoxelCellsVolume(preug))
  {
    vtkLog(ERROR, "Generated cells have wrong geometry");
    retVal = false;
  }

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(5);

  pds->SetPartition(0, outPrePds->GetPartition(0));
  pds->SetPartition(1, ug1);
  pds->SetPartition(2, ug2);
  pds->SetPartition(3, ug3);
  pds->SetPartition(4, vtkNew<vtkUnstructuredGrid>()); // testing empty input

  // On this pass, we test point data when using the cells generator.
  vtkNew<vtkGhostCellsGenerator> generator;
  generator->BuildIfRequiredOff();
  generator->SetInputDataObject(pds);
  generator->SetNumberOfGhostLayers(numberOfGhostLayers);
  generator->Update();

  vtkPartitionedDataSet* outPDS =
    vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

  vtkLog(INFO, "Testing ghost points for vtkUnstructuredGrid in rank " << myrank);

  vtkNew<vtkGhostCellsGenerator> preCellGenerator;
  preCellGenerator->SetInputConnection(point2cell0->GetOutputPort());
  preCellGenerator->BuildIfRequiredOff();
  preCellGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  preCellGenerator->Update();

  vtkNew<vtkPartitionedDataSet> pdsPointToCell;
  pdsPointToCell->SetNumberOfPartitions(4);

  pdsPointToCell->SetPartition(0, preCellGenerator->GetOutputDataObject(0));
  pdsPointToCell->SetPartition(1, point2cell1->GetOutputDataObject(0));
  pdsPointToCell->SetPartition(2, point2cell2->GetOutputDataObject(0));
  pdsPointToCell->SetPartition(3, point2cell3->GetOutputDataObject(0));

  // On this pass, we test cell data when using the cells generator.
  vtkNew<vtkGhostCellsGenerator> cellGenerator;
  cellGenerator->BuildIfRequiredOff();
  cellGenerator->SetInputDataObject(pdsPointToCell);
  cellGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  cellGenerator->Update();

  vtkPartitionedDataSet* outCellPDS =
    vtkPartitionedDataSet::SafeDownCast(cellGenerator->GetOutputDataObject(0));

  vtkNew<vtkCellCenters> refCenters;
  refCenters->SetInputData(refPointToCell->GetOutputDataObject(0));
  refCenters->Update();

  vtkPointSet* refCentersPS = vtkPointSet::SafeDownCast(refCenters->GetOutputDataObject(0));

  vtkNew<vtkStaticPointLocator> refCellsLocator;
  refCellsLocator->SetDataSet(refCentersPS);
  refCellsLocator->BuildLocator();

  for (int id = 0; id < 4; ++id)
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(outPDS->GetPartition(id));

    if (!TestVoxelCellsVolume(ug))
    {
      vtkLog(ERROR, "Generated cells have wrong geometry");
      retVal = false;
    }

    vtkIdType numberOfCells = (MaxExtent + numberOfGhostLayers) *
      (MaxExtent + numberOfGhostLayers) * (MaxExtent + numberOfGhostLayers);
    if (ug->GetNumberOfCells() != numberOfCells)
    {
      vtkLog(ERROR,
        "Wrong number of output cells when generating ghost cells with "
          << "unstructured grid: " << ug->GetNumberOfCells() << " != " << numberOfCells);
      retVal = false;
    }
    vtkIdType numberOfPoints = (MaxExtent + numberOfGhostLayers + 1) *
      (MaxExtent + numberOfGhostLayers + 1) * (MaxExtent + numberOfGhostLayers + 1);
    if (ug->GetNumberOfPoints() != numberOfPoints)
    {
      vtkLog(ERROR,
        "Wrong number of output points when generating ghost cells with "
          << "unstructured grid: " << ug->GetNumberOfPoints() << " != " << numberOfPoints);
      retVal = false;
    }

    if (!TestQueryReferenceToGenerated(refUG, refLocator, ug))
    {
      retVal = false;
    }

    vtkNew<vtkCellCenters> centers;
    centers->SetInputData(outCellPDS->GetPartition(id));
    centers->Update();

    if (!TestQueryReferenceToGenerated(refCentersPS, refCellsLocator,
          vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0)), true /* centers */))
    {
      retVal = false;
    }
  }

  vtkIdType pointsLength = 2 * MaxExtent + 1;
  vtkIdType numberOfPoints = pointsLength * pointsLength * pointsLength;
  vtkIdType cellsLength = 2 * MaxExtent;
  vtkIdType numberOfCells = cellsLength * cellsLength * cellsLength;

  if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
  {
    retVal = false;
  }

  if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
  {
    retVal = false;
  }

  // Now we're going to test ghost cells generation when using point global ids.
  // We take the same input as previously, but add global ids, and edit some that should match
  // across partitions so they do not. The ghost cell generator should ignore point positions in
  // the presence of a global ids array.

  std::array<vtkImageData*, 4> images = { image0, image1, image2, image3 };
  pds->SetPartition(0, ug0);

  for (int id = 0; id < 4; ++id)

    if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
    {
      retVal = false;
    }

  if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
  {
    retVal = false;
  }

  for (int id = 0; id < 4; ++id)
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(pds->GetPartition(id));
    GenerateGlobalIds(ug, images[id]->GetExtent());
    vtkIdTypeArray* gids = vtkArrayDownCast<vtkIdTypeArray>(ug->GetPointData()->GetGlobalIds());

    // For the first partition, we mess up an edge with global ids that don't match the
    // corresponding points in other partitions.
    if (id == 0)
    {
      vtkIdType offset = NumberOfPoints + MaxExtent;
      int extent[6] = { -MaxExtent, 0, -MaxExtent, 0, zmin, zmax };
      for (int z = zmin; z <= zmax; ++z)
      {
        int ijk[3] = { 0, 0, z };
        vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
        gids->SetValue(pointId, offset + z);
      }
    }
  }

  generator->Modified();
  generator->Update();

  vtkPartitionedDataSet* outPDSWithGID =
    vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

  for (int id = 0; id < 4; ++id)
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::SafeDownCast(outPDSWithGID->GetPartition(id));
    bool error = false;

    // Number of points is hardcoded. The topology of the output is kind of weird because out of the
    // 4 partitions, the first partition has one edge that has global ids that don't match its
    // counter part in the other partitions. This test ensures that global ids trump point
    // positions in 3D.
    switch (id)
    {
      case 0:
        if (ug->GetNumberOfPoints() != 491)
        {
          error = true;
        }
        break;
      case 1:
        if (ug->GetNumberOfPoints() != 532)
        {
          error = true;
        }
        break;
      case 2:
        if (ug->GetNumberOfPoints() != 480)
        {
          error = true;
        }
        break;
      case 3:
        if (ug->GetNumberOfPoints() != 532)
        {
          error = true;
        }
        break;
    }

    if (error)
    {
      vtkLog(ERROR, "Ghost cells generation for unstructured grid failed when using global ids");
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestPolyData(vtkMultiProcessController* controller, int myrank, int numberOfGhostLayers)
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

  {
    vtkLog(INFO, "Testing ghost cells for vtkPolyData composed of lines only in rank " << myrank);

    // Generating an image englobing the extents of every blocks
    // to use as a reference
    vtkNew<vtkImageData> refImage;
    refImage->SetExtent(-MaxExtent, MaxExtent, 0, 0, 0, 0);
    FillImage(refImage);

    vtkSmartPointer<vtkPolyData> refPD = Convert1DImageToPolyData(refImage);

    vtkNew<vtkPointDataToCellData> refPointToCell;
    refPointToCell->SetInputData(refPD);
    refPointToCell->Update();
    vtkNew<vtkImageData> image;
    image->SetExtent(ymin, ymax, 0, 0, 0, 0);
    FillImage(image);

    vtkSmartPointer<vtkPolyData> pd = Convert1DImageToPolyData(image);

    {
      vtkNew<vtkPointDataToCellData> point2cell;
      point2cell->SetInputData(pd);
      point2cell->Update();

      vtkNew<vtkPartitionedDataSet> pds;
      pds->SetNumberOfPartitions(1);

      pds->SetPartition(0, point2cell->GetOutputDataObject(0));

      vtkNew<vtkGhostCellsGenerator> generator;
      generator->BuildIfRequiredOff();
      generator->SetInputDataObject(pds);
      generator->SetNumberOfGhostLayers(numberOfGhostLayers);
      generator->Update();

      vtkPartitionedDataSet* outPDS =
        vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

      vtkPolyData* out = vtkPolyData::SafeDownCast(outPDS->GetPartition(0));

      vtkNew<vtkCellCenters> refCenters;
      refCenters->SetInputData(refPointToCell->GetOutputDataObject(0));
      refCenters->Update();

      vtkPointSet* refCentersPS = vtkPointSet::SafeDownCast(refCenters->GetOutputDataObject(0));

      vtkNew<vtkStaticPointLocator> refCellsLocator;
      refCellsLocator->SetDataSet(refCentersPS);
      refCellsLocator->BuildLocator();

      vtkPolyData* outPD = vtkPolyData::SafeDownCast(outPDS->GetPartition(0));

      vtkIdType numberOfCells = (MaxExtent + numberOfGhostLayers);
      if (outPD->GetNumberOfCells() != numberOfCells)
      {
        vtkLog(ERROR,
          "Wrong number of output cells when generating ghost cells with "
            << "poly data: " << outPD->GetNumberOfCells() << " != " << numberOfCells);
        retVal = false;
      }
      vtkIdType numberOfPoints = (MaxExtent + numberOfGhostLayers + 1);
      if (outPD->GetNumberOfPoints() != numberOfPoints)
      {
        vtkLog(ERROR,
          "Wrong number of output points when generating ghost cells with "
            << "poly data: " << outPD->GetNumberOfPoints() << " != " << numberOfPoints);
        retVal = false;
      }

      vtkNew<vtkCellCenters> centers;
      centers->SetInputData(out);
      centers->Update();

      if (!TestQueryReferenceToGenerated(refCentersPS, refCellsLocator,
            vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0)), true))
      {
        retVal = false;
      }

      if (!TestGhostPointsTagging(controller, outPDS, GridWidth))
      {
        retVal = false;
      }

      if (!TestGhostCellsTagging(controller, outPDS, GridWidth - 1))
      {
        retVal = false;
      }
    }

    vtkLog(INFO, "Testing ghost points for vtkPolyData composed of lines only in rank " << myrank);

    {
      vtkNew<vtkPartitionedDataSet> pds;
      pds->SetNumberOfPartitions(1);

      pds->SetPartition(0, pd);

      vtkNew<vtkGhostCellsGenerator> generator;
      generator->BuildIfRequiredOff();
      generator->SetInputDataObject(pds);
      generator->SetNumberOfGhostLayers(numberOfGhostLayers);
      generator->Update();

      vtkPartitionedDataSet* outPDS =
        vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

      vtkPolyData* outPD = vtkPolyData::SafeDownCast(outPDS->GetPartition(0));

      vtkNew<vtkStaticPointLocator> refLocator;
      refLocator->SetDataSet(refPD);
      refLocator->BuildLocator();

      if (!TestQueryReferenceToGenerated(refPD, refLocator, outPD))
      {
        retVal = false;
      }
    }
  }

  // Generating an image englobing the extents of every blocks
  // to use as a reference
  vtkNew<vtkImageData> refImage;
  refImage->SetExtent(-MaxExtent, MaxExtent, -MaxExtent, MaxExtent, 0, 0);
  FillImage(refImage);

  vtkSmartPointer<vtkPolyData> refPD = Convert2DImageToPolyData(refImage);

  vtkNew<vtkStaticPointLocator> refLocator;
  refLocator->SetDataSet(refPD);
  refLocator->BuildLocator();

  vtkNew<vtkPointDataToCellData> refPointToCell;
  refPointToCell->SetInputData(refPD);
  refPointToCell->Update();

  vtkNew<vtkImageData> image0;
  image0->SetExtent(-MaxExtent, 0, ymin, ymax, 0, 0);
  FillImage(image0);
  vtkSmartPointer<vtkPolyData> pd0 = Convert2DImageToPolyData(image0, true);

  vtkNew<vtkImageData> image1;
  image1->SetExtent(0, MaxExtent, ymin, ymax, 0, 0);
  FillImage(image1);
  vtkSmartPointer<vtkPolyData> pd1 = Convert2DImageToPolyData(image1);

  vtkNew<vtkPointDataToCellData> point2cell0;
  point2cell0->SetInputData(pd0);
  point2cell0->Update();

  vtkNew<vtkPointDataToCellData> point2cell1;
  point2cell1->SetInputData(pd1);
  point2cell1->Update();

  vtkLog(INFO, "Testing ghost cells for vtkPolyData in rank " << myrank);

  vtkNew<vtkPartitionedDataSet> prePds;
  prePds->SetNumberOfPartitions(1);

  prePds->SetPartition(0, pd0);

  // We do a simple case with only one ug per rank.
  // We will use the output of this generator for the next more complex generation,
  // and ensure that when ghosts are present in the input, everything works fine.
  vtkNew<vtkGhostCellsGenerator> preGenerator;
  preGenerator->BuildIfRequiredOff();
  preGenerator->SetInputDataObject(prePds);
  preGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  preGenerator->Update();

  vtkPartitionedDataSet* outPrePds =
    vtkPartitionedDataSet::SafeDownCast(preGenerator->GetOutputDataObject(0));
  vtkPolyData* prepd = vtkPolyData::SafeDownCast(outPrePds->GetPartition(0));

  if (prepd->GetNumberOfCells() != (MaxExtent * (MaxExtent + numberOfGhostLayers)))
  {
    vtkLog(ERROR,
      "Wrong number of output cells for a one to one ghost cell generation:"
        << " we should have " << (MaxExtent * (MaxExtent + numberOfGhostLayers))
        << ", instead we have " << prepd->GetNumberOfCells());
    retVal = false;
  }

  if (prepd->GetNumberOfPoints() != ((MaxExtent + 1) * (MaxExtent + 1 + numberOfGhostLayers)))
  {
    vtkLog(ERROR,
      "Wrong number of output points for a one to one ghost cell generation:"
        << " we should have " << ((MaxExtent + 1) * (MaxExtent + 1 + numberOfGhostLayers))
        << ", instead we have " << prepd->GetNumberOfCells());
    retVal = false;
  }

  if (!TestQueryReferenceToGenerated(refPD, refLocator, prepd))
  {
    retVal = false;
  }

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(2);

  pds->SetPartition(0, outPrePds->GetPartition(0));
  pds->SetPartition(1, pd1);
  pds->SetPartition(2, vtkNew<vtkPolyData>()); // testing empty input

  // On this pass, we test point data when using the cells generator.
  vtkNew<vtkGhostCellsGenerator> generator;
  generator->BuildIfRequiredOff();
  generator->SetInputDataObject(pds);
  generator->SetNumberOfGhostLayers(numberOfGhostLayers);
  generator->Update();

  vtkPartitionedDataSet* outPDS =
    vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

  vtkLog(INFO, "Testing ghost points for vtkPolyData in rank " << myrank);

  vtkNew<vtkGhostCellsGenerator> preCellGenerator;
  preCellGenerator->BuildIfRequiredOff();
  preCellGenerator->SetInputConnection(point2cell0->GetOutputPort());
  preCellGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  preCellGenerator->Update();

  vtkNew<vtkPartitionedDataSet> pdsPointToCell;
  pdsPointToCell->SetNumberOfPartitions(2);

  pdsPointToCell->SetPartition(0, preCellGenerator->GetOutputDataObject(0));
  pdsPointToCell->SetPartition(1, point2cell1->GetOutputDataObject(0));

  // On this pass, we test cell data when using the cells generator.
  vtkNew<vtkGhostCellsGenerator> cellGenerator;
  cellGenerator->BuildIfRequiredOff();
  cellGenerator->SetInputDataObject(pdsPointToCell);
  cellGenerator->SetNumberOfGhostLayers(numberOfGhostLayers);
  cellGenerator->Update();

  vtkPartitionedDataSet* outCellPDS =
    vtkPartitionedDataSet::SafeDownCast(cellGenerator->GetOutputDataObject(0));

  vtkNew<vtkCellCenters> refCenters;
  refCenters->SetInputData(refPointToCell->GetOutputDataObject(0));
  refCenters->Update();

  vtkPointSet* refCentersPS = vtkPointSet::SafeDownCast(refCenters->GetOutputDataObject(0));

  vtkNew<vtkStaticPointLocator> refCellsLocator;
  refCellsLocator->SetDataSet(refCentersPS);
  refCellsLocator->BuildLocator();

  for (int id = 0; id < 2; ++id)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(outPDS->GetPartition(id));

    vtkIdType numberOfCells = (MaxExtent + numberOfGhostLayers) * (MaxExtent + numberOfGhostLayers);
    if (pd->GetNumberOfCells() != numberOfCells)
    {
      vtkLog(ERROR,
        "Wrong number of output cells when generating ghost cells with "
          << "poly data: " << pd->GetNumberOfCells() << " != " << numberOfCells);
      retVal = false;
    }
    vtkIdType numberOfPoints =
      (MaxExtent + numberOfGhostLayers + 1) * (MaxExtent + numberOfGhostLayers + 1);
    if (pd->GetNumberOfPoints() != numberOfPoints)
    {
      vtkLog(ERROR,
        "Wrong number of output points when generating ghost cells with "
          << "poly data: " << pd->GetNumberOfPoints() << " != " << numberOfPoints);
      retVal = false;
    }

    if (!TestQueryReferenceToGenerated(
          refPD, refLocator, pd, false /* centers */, true /* ignorePointPosition */))
    {
      retVal = false;
    }

    vtkNew<vtkCellCenters> centers;
    centers->SetInputData(outCellPDS->GetPartition(id));
    centers->Update();

    if (!TestQueryReferenceToGenerated(refCentersPS, refCellsLocator,
          vtkPointSet::SafeDownCast(centers->GetOutputDataObject(0)), true /* centers */,
          true /* ignorePointPosition */))
    {
      retVal = false;
    }
  }

  vtkIdType pointsLength = 2 * MaxExtent + 1;
  vtkIdType numberOfPoints = pointsLength * pointsLength;
  vtkIdType cellsLength = 2 * MaxExtent;
  vtkIdType numberOfCells = cellsLength * cellsLength;

  if (!TestGhostPointsTagging(controller, outPDS, numberOfPoints))
  {
    retVal = false;
  }

  if (!TestGhostCellsTagging(controller, outPDS, numberOfCells))
  {
    retVal = false;
  }

  // Now we're going to test ghost cells generation when using point global ids.
  // We take the same input as previously, but add global ids, and edit some that should match
  // across partitions so they do not. The ghost cell generator should ignore point positions in
  // the presence of a global ids array.

  std::array<vtkImageData*, 2> images = { image0, image1 };
  pds->SetPartition(0, pd0);

  for (int id = 0; id < 2; ++id)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(pds->GetPartition(id));
    GenerateGlobalIds(pd, images[id]->GetExtent());
    vtkIdTypeArray* gids = vtkArrayDownCast<vtkIdTypeArray>(pd->GetPointData()->GetGlobalIds());

    // For the first partition, we mess up an edge with global ids that don't match the
    // corresponding points in other partitions.
    if (id == 0)
    {
      vtkIdType offset = NumberOfPoints + MaxExtent;
      int extent[6] = { -MaxExtent, 0, ymin, ymax, 0, 0 };
      for (int y = ymin; y <= ymax; ++y)
      {
        int ijk[3] = { 0, y, 0 };
        vtkIdType pointId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
        gids->SetValue(pointId, offset + y);
      }
    }
  }

  generator->Modified();
  generator->Update();

  vtkPartitionedDataSet* outPDSWithGID =
    vtkPartitionedDataSet::SafeDownCast(generator->GetOutputDataObject(0));

  for (int id = 0; id < 2; ++id)
  {
    vtkPolyData* pd = vtkPolyData::SafeDownCast(outPDSWithGID->GetPartition(id));

    if (pd->GetNumberOfPoints() != (MaxExtent + 1) * (MaxExtent + 3))
    {
      vtkLog(ERROR, "Ghost cells generation for poly data failed when using global ids");
      retVal = false;
    }
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestPartitionedDataSetCollection(int myrank, int numberOfGhostLayers)
{
  // This test follows the same first steps as in Test3DGrids, but instead of computing ghosts on a
  // partitioned data set, we compute them on a partitioned data set collection, which means that
  // there should not be ghosts between the separate partitioned data sets. image0 and image1 belong
  // to the same collection, image2 and image3 belong to the same collection as well.
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

  const int newExtent0[6] = { -MaxExtent, numberOfGhostLayers, -MaxExtent, 0,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent1[6] = { -numberOfGhostLayers, MaxExtent, -MaxExtent, 0,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent2[6] = { -numberOfGhostLayers, MaxExtent, 0, MaxExtent,
    zmin != 0 ? zmin : -numberOfGhostLayers, zmax != 0 ? zmax : numberOfGhostLayers };

  const int newExtent3[6] = { -MaxExtent, numberOfGhostLayers, 0, MaxExtent,
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

  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  pdsc->SetNumberOfPartitionedDataSets(2);

  vtkPartitionedDataSet* pds0 = pdsc->GetPartitionedDataSet(0);
  pds0->SetNumberOfPartitions(2);
  pds0->SetPartition(0, image0);
  pds0->SetPartition(1, image1);

  vtkPartitionedDataSet* pds1 = pdsc->GetPartitionedDataSet(1);
  pds1->SetNumberOfPartitions(2);
  pds1->SetPartition(0, image2);
  pds1->SetPartition(1, image3);

  vtkLog(INFO, "Testing ghost points for vtkPartitionedDataSetCollection in rank " << myrank);

  vtkNew<vtkGhostCellsGenerator> generator;
  generator->BuildIfRequiredOff();
  generator->SetInputDataObject(pdsc);
  generator->SetNumberOfGhostLayers(numberOfGhostLayers);
  generator->Update();

  vtkPartitionedDataSetCollection* outPDSC =
    vtkPartitionedDataSetCollection::SafeDownCast(generator->GetOutputDataObject(0));

  vtkPartitionedDataSet* outPDS0 = outPDSC->GetPartitionedDataSet(0);
  vtkPartitionedDataSet* outPDS1 = outPDSC->GetPartitionedDataSet(1);

  if (!TestExtent(newExtent0, vtkImageData::SafeDownCast(outPDS0->GetPartition(0))->GetExtent()) ||
    !TestExtent(newExtent1, vtkImageData::SafeDownCast(outPDS0->GetPartition(1))->GetExtent()) ||
    !TestExtent(newExtent2, vtkImageData::SafeDownCast(outPDS1->GetPartition(0))->GetExtent()) ||
    !TestExtent(newExtent3, vtkImageData::SafeDownCast(outPDS1->GetPartition(1))->GetExtent()))
  {
    vtkLog(ERROR, "Generating ghosts in vtkPartitionedDataSetCollection failed" << myrank);
    retVal = false;
  }

  return retVal;
}

//----------------------------------------------------------------------------
bool TestPointPrecision(vtkMultiProcessController* controller, int myrank)
{
  vtkNew<vtkUnstructuredGrid> ug;
  vtkNew<vtkPoints> points;
  points->SetDataType(VTK_DOUBLE);
  points->SetNumberOfPoints(2);
  ug->SetPoints(points);

  vtkNew<vtkIdTypeArray> connectivity;
  connectivity->SetNumberOfValues(2);
  connectivity->SetValue(0, 0);
  connectivity->SetValue(1, 1);

  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfValues(2);
  offsets->SetValue(0, 0);
  offsets->SetValue(1, 2);

  vtkNew<vtkUnsignedCharArray> types;
  types->SetNumberOfValues(1);
  types->SetValue(0, VTK_LINE);

  vtkNew<vtkCellArray> cells;
  cells->SetData(offsets, connectivity);
  ug->SetCells(types, cells);

  if (myrank == 0)
  {
    double p[3] = { 0.0, 0.0, 0.0 };
    points->SetPoint(0, p);
    p[0] = 1.0;
    points->SetPoint(1, p);
  }
  else if (myrank == 1)
  {
    // double p[3] = { 1.0, 0.0, 0.0 };
    double p[3] = { 1.0 + VTK_DBL_EPSILON, 0.0, 0.0 };
    points->SetPoint(0, p);
    p[0] = 2.0;
    points->SetPoint(1, p);
  }

  vtkNew<vtkGhostCellsGenerator> generator;
  generator->SetInputData(ug);
  generator->SetNumberOfGhostLayers(1);
  generator->SetController(controller);
  generator->BuildIfRequiredOff();
  generator->Update();

  auto output = vtkUnstructuredGrid::SafeDownCast(generator->GetOutputDataObject(0));

  // The ghost cells generator would output one cell if it was sensitive to point precision.
  if (output->GetNumberOfCells() != 2)
  {
    vtkLog(ERROR, "Ghost cells generator is too sensitive to point precision");
    return false;
  }

  return true;
}
} // anonymous namespace

//----------------------------------------------------------------------------
int TestGhostCellsGenerator(int argc, char* argv[])
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
  int numberOfGhostLayers = 2;

  if (!TestPointPrecision(contr, myrank))
  {
    retVal = EXIT_FAILURE;
  }

  if (!TestDeepMultiBlock())
  {
    retVal = EXIT_FAILURE;
  }

  if (!TestMixedTypes(myrank))
  {
    retVal = EXIT_FAILURE;
  }

  if (!Test1DGrids(contr, myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  if (!Test2DGrids(contr, myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  if (!Test3DGrids(contr, myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  if (!TestPolyData(contr, myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  if (!TestUnstructuredGrid(contr, myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  if (!TestPartitionedDataSetCollection(myrank, numberOfGhostLayers))
  {
    retVal = EXIT_FAILURE;
  }

  vtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  return retVal;
}

#undef _USE_MATH_DEFINES
