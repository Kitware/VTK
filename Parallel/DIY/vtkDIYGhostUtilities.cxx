/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYGhostUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDIYGhostUtilities.h"

#include "vtkArrayDispatch.h"
#include "vtkDIYExplicitAssigner.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMathUtilities.h"
#include "vtkMatrix3x3.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <vector>

// clang-format off
#include "vtk_diy2.h"
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
// clang-format off

namespace
{
//@{
/**
 * Convenient typedefs
 */
using ExtentType = vtkDIYGhostUtilities::ExtentType;
using VectorType = vtkDIYGhostUtilities::VectorType;
using QuaternionType = vtkDIYGhostUtilities::QuaternionType;
template <class T>
using BlockMapType = vtkDIYGhostUtilities::BlockMapType<T>;
using Links = vtkDIYGhostUtilities::Links;
using LinkMap = vtkDIYGhostUtilities::LinkMap;
template<class DataSetT>
using DataSetTypeToBlockTypeConverter =
    vtkDIYGhostUtilities::DataSetTypeToBlockTypeConverter<DataSetT>;
//@}

//@{
/**
 * Block typedefs
 */
using ImageDataBlockStructure = vtkDIYGhostUtilities::ImageDataBlockStructure;
using ImageDataBlock = vtkDIYGhostUtilities::ImageDataBlock;
using RectilinearGridBlock = vtkDIYGhostUtilities::RectilinearGridBlock;
using RectilinearGridBlockStructure = vtkDIYGhostUtilities::RectilinearGridBlockStructure;

using GridInformation = vtkDIYGhostUtilities::GridInformation;
using RectilinearGridInformation = vtkDIYGhostUtilities::RectilinearGridInformation;
//@}

//============================================================================
/**
 * Ajacency bits used for grids.
 * For instance, `Adjacency::Something` means that the neighboring block it refers to is on the
 * `Something` of current block
 * Naming is self-explanatory.
 */
enum Adjacency
{
  Left = 0x01,
  Right = 0x02,
  Front = 0x04,
  Back = 0x08,
  Bottom = 0x10,
  Top = 0x20
};

//============================================================================
/**
 * Bit arrangement encoding how neighboring grid blocks overlap. Two grids overlap in a dimension
 * if and only if the extent segment of the corresponding dimension intersect.
 */
enum Overlap
{
  X = 0x01,
  Y = 0x02,
  XY = 0x03,
  Z = 0x04,
  XZ = 0x05,
  YZ = 0x06
};

//----------------------------------------------------------------------------
bool IsExtentValid(const int* extent)
{
  return extent[0] <= extent[1] && extent[2] <= extent[3] && extent[4] <= extent[5];
}

//----------------------------------------------------------------------------
/**
 * This function fills an input cell `array` mapped with input `grid` given the input extent.
 * `array` needs to be already allocated.
 */
template <class ArrayT, class GridDataSetT>
void FillGridCellArray(ArrayT* array, GridDataSetT* grid, int imin, int imax, int jmin, int jmax,
  int kmin, int kmax, typename ArrayT::ValueType val)
{
  const int* gridExtent = grid->GetExtent();
  for (int k = kmin; k < kmax; ++k)
  {
    for (int j = jmin; j < jmax; ++j)
    {
      for (int i = imin; i < imax; ++i)
      {
        int ijk[3] = { i, j, k };
        array->SetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk), val);
      }
    }
  }
}

//----------------------------------------------------------------------------
/**
 * This function fills an input point `array` mapped with input `grid` given the input extent.
 * `array` needs to be already allocated.
 */
template <class ArrayT, class GridDataSetT>
void FillGridPointArray(ArrayT* array, GridDataSetT* grid, int imin, int imax, int jmin, int jmax,
  int kmin, int kmax, typename ArrayT::ValueType val)
{
  const int* gridExtent = grid->GetExtent();
  for (int k = kmin; k <= kmax; ++k)
  {
    for (int j = jmin; j <= jmax; ++j)
    {
      for (int i = imin; i <= imax; ++i)
      {
        int ijk[3] = { i, j, k };
        array->SetValue(vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk), val);
      }
    }
  }
}

//----------------------------------------------------------------------------
/**
 * Clone a `grid` into a `clone`. CloneGrid should have wider extents than grid.
 * This function does a deep copy of every scalar fields.
 */
template <class GridDataSetT>
void CloneGrid(GridDataSetT* grid, GridDataSetT* clone)
{
  vtkCellData* cloneCellData = clone->GetCellData();
  vtkCellData* gridCellData = grid->GetCellData();
  cloneCellData->CopyStructure(gridCellData);
  for (int arrayId = 0; arrayId < cloneCellData->GetNumberOfArrays(); ++arrayId)
  {
    cloneCellData->GetAbstractArray(arrayId)->SetNumberOfTuples(clone->GetNumberOfCells());
  }

  int* extent = grid->GetExtent();

  // We use `std::max` here to work for grids of dimension 2 and 1.
  // This gives "thickness" to the degenerate dimension
  int imin = extent[0];
  int imax = std::max(extent[1], extent[0] + 1);
  int jmin = extent[2];
  int jmax = std::max(extent[3], extent[2] + 1);
  int kmin = extent[4];
  int kmax = std::max(extent[5], extent[4] + 1);

  const int* cloneExtent = clone->GetExtent();
  const int* gridExtent = grid->GetExtent();
  for (int k = kmin; k < kmax; ++k)
  {
    for (int j = jmin; j < jmax; ++j)
    {
      for (int i = imin; i < imax; ++i)
      {
        int ijk[3] = { i, j, k };
        cloneCellData->SetTuple(vtkStructuredData::ComputeCellIdForExtent(cloneExtent, ijk),
          vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk), gridCellData);
      }
    }
  }

  vtkPointData* clonePointData = clone->GetPointData();
  vtkPointData* gridPointData = grid->GetPointData();
  clonePointData->CopyStructure(gridPointData);
  for (int arrayId = 0; arrayId < clonePointData->GetNumberOfArrays(); ++arrayId)
  {
    clonePointData->GetAbstractArray(arrayId)->SetNumberOfTuples(clone->GetNumberOfPoints());
  }

  imax = extent[1];
  jmax = extent[3];
  kmax = extent[5];

  for (int k = kmin; k <= kmax; ++k)
  {
    for (int j = jmin; j <= jmax; ++j)
    {
      for (int i = imin; i <= imax; ++i)
      {
        int ijk[3] = { i, j, k };
        clonePointData->SetTuple(vtkStructuredData::ComputePointIdForExtent(cloneExtent, ijk),
          vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk), gridPointData);
      }
    }
  }

  clone->GetFieldData()->ShallowCopy(grid->GetFieldData());
}

//----------------------------------------------------------------------------
/**
 * This function computes the extent of an input `grid` if ghosts are removed.
 * `ghostLevel` is the ghost level of input `grid`.
 */
template <class GridDataSetT>
ExtentType PeelOffGhostLayers(GridDataSetT* grid, int ghostLevel)
{
  ExtentType extent;
  vtkUnsignedCharArray* ghosts = vtkArrayDownCast<vtkUnsignedCharArray>(
    grid->GetGhostArray(vtkDataObject::FIELD_ASSOCIATION_CELLS));
  if (!ghosts)
  {
    grid->GetExtent(extent.data());
    return extent;
  }
  int* gridExtent = grid->GetExtent();

  // We use `std::max` here to work for grids of dimension 2 and 1.
  // This gives "thickness" to the degenerate dimension
  int imin = gridExtent[0];
  int imax = std::max(gridExtent[1], gridExtent[0] + 1);
  int jmin = gridExtent[2];
  int jmax = std::max(gridExtent[3], gridExtent[2] + 1);
  int kmin = gridExtent[4];
  int kmax = std::max(gridExtent[5], gridExtent[4] + 1);

  {
    // Strategy:
    // We create a cursor `ijk` that is at the bottom left front corner of the grid.
    // From there, we iterate each cursor dimension until the targetted brick is not a ghost.
    // When this happens on a dimension, we lock it.
    // As a result, the when this loop is over, `ijk` points to the last raws of ghosts in the input
    // `grid`.
    //
    // We use `std::min` to acknowledge that a ghost level can be bigger than a dimension's width.
    int ijk[3] = { std::min(imin + ghostLevel, imax - 1), std::min(jmin + ghostLevel, jmax - 1),
      std::min(kmin + ghostLevel, kmax - 1) };

    // We lock degenerate dimensions at start
    bool lock[3] = { gridExtent[0] == gridExtent[1], gridExtent[2] == gridExtent[3],
      gridExtent[4] == gridExtent[5] };

    while ((!lock[0] || !lock[1] || !lock[2]) && (lock[0] || ijk[0] > imin) &&
      (lock[1] || ijk[1] > jmin) && (lock[2] || ijk[2] > kmin) &&
      !ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk)))
    {
      for (int dim = 0; dim < 3; ++dim)
      {
        if (!lock[dim])
        {
          --ijk[dim];
          if (ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk)))
          {
            ++ijk[dim];
            lock[dim] = true;
          }
        }
      }
    }
    extent[0] = ijk[0];
    extent[2] = ijk[1];
    extent[4] = ijk[2];
  }
  {
    // This part follows the same process as the previous one, but on the top right back corner.
    int ijk[3] = { std::max(imax - 1 - ghostLevel, imin), std::max(jmax - 1 - ghostLevel, jmin),
      std::max(kmax - 1 - ghostLevel, kmin) };
    bool lock[3] = { gridExtent[0] == gridExtent[1], gridExtent[2] == gridExtent[3],
      gridExtent[4] == gridExtent[5] };
    while ((!lock[0] || !lock[1] || !lock[2]) && (lock[0] || ijk[0] < imax - 1) &&
      (lock[1] || ijk[1] < jmax - 1) && (lock[2] || ijk[2] < kmax - 1) &&
      !ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk)))
    {
      for (int dim = 0; dim < 3; ++dim)
      {
        if (!lock[dim])
        {
          ++ijk[dim];
          if (ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk)))
          {
            --ijk[dim];
            lock[dim] = true;
          }
        }
      }
    }
    extent[1] = ijk[0] + (gridExtent[0] != gridExtent[1]);
    extent[3] = ijk[1] + (gridExtent[2] != gridExtent[3]);
    extent[5] = ijk[2] + (gridExtent[4] != gridExtent[5]);
  }
  return extent;
}

//----------------------------------------------------------------------------
void AddGhostLayerOfGridPoints(int vtkNotUsed(extentIdx), GridInformation& vtkNotUsed(information),
  ImageDataBlockStructure& vtkNotUsed(blockStructure))
{
  // Do nothing for image data. Points are all implicit.
}

//----------------------------------------------------------------------------
void AddGhostLayerOfGridPoints(int extentIdx, RectilinearGridInformation& blockInformation,
  RectilinearGridBlockStructure& blockStructure)
{
  int layerThickness = blockInformation.ExtentGhostThickness[extentIdx];
  vtkSmartPointer<vtkDataArray>& coordinateGhosts = blockInformation.CoordinateGhosts[extentIdx];
  vtkDataArray* coordinates[3] = { blockStructure.XCoordinates, blockStructure.YCoordinates,
    blockStructure.ZCoordinates };
  vtkDataArray* coords = coordinates[extentIdx / 2];
  if (!coordinateGhosts)
  {
    coordinateGhosts = vtkSmartPointer<vtkDataArray>::Take(coords->NewInstance());
  }
  if (coordinateGhosts->GetNumberOfTuples() < layerThickness)
  {
    if (extentIdx == 0 || extentIdx == 2 || extentIdx == 4)
    {
      coordinateGhosts->InsertTuples(
        0, layerThickness, coords->GetNumberOfTuples() - layerThickness - 1, coords);
    }
    else
    {
      coordinateGhosts->InsertTuples(0, layerThickness, 0, coords);
    }
  }
}

//----------------------------------------------------------------------------
/**
 * This function is only used for grid inputs. It updates the extents of the output of current block
 * to account for an adjacency with a block at index `idx` inside the extent.
 * `outputExtentShift` is the accumulation of every needed shift to account for new ghost layers
 * over passes with all connected / adjacent neighboring blocks.
 * `neighborExtentWithNewGhosts` is the extent of the adjacent block to be updated with this new
 * ghost layer.
 */
// void AddGhostLayerToGrid(int idx, int outputGhostLevels, const ExtentType& extent,
//    ExtentType& outputExtentShift, ExtentType& neighborExtentWithNewGhosts)
template <class BlockStructureT, class BlockInformationT>
void AddGhostLayerToGrid(int idx, int outputGhostLevels, BlockStructureT& blockStructure,
  BlockInformationT& blockInformation)
{
  const ExtentType& extent = blockStructure.Extent;
  bool upperBound = idx % 2;
  int oppositeIdx = upperBound ? idx - 1 : idx + 1;
  int localOutputGhostLevels =
    std::min(outputGhostLevels, std::abs(extent[idx] - extent[oppositeIdx]));
  blockInformation.ExtentGhostThickness[idx] =
    std::max(blockInformation.ExtentGhostThickness[idx], localOutputGhostLevels);
  blockStructure.ExtentWithNewGhosts[oppositeIdx] +=
    (upperBound ? -1.0 : 1.0) * localOutputGhostLevels;

  AddGhostLayerOfGridPoints(idx, blockInformation, blockStructure);
}

//----------------------------------------------------------------------------
/**
 * This function is to be used with grids only.
 * At given position inside `blockStructures` pointed by iterator `it`, and given a computed
 * `adjacencyMask` and `overlapMask` and input ghost levels, this function updates the accumulated
 * extent shift (`outputExtentShift`) for the output grid, as well as the extent of the current
 * block's neighbor `neighborExtentWithNewGhosts`.
 */
template <class BlockStructuresT, class IteratorT, class BlockInformationT>
void LinkGrid(BlockStructuresT& blockStructures, IteratorT& it, BlockInformationT& blockInformation,
  Links& localLinks, unsigned char adjacencyMask, unsigned char overlapMask, int outputGhostLevels)
{
  int gid = it->first;
  auto& blockStructure = it->second;

  // Here we look at adjacency where faces overlap
  //   ______
  //  /__/__/|
  // |  |  | |
  // |__|__|/
  //
  if (((overlapMask == Overlap::YZ) && (adjacencyMask & (Adjacency::Left | Adjacency::Right))) ||
    ((overlapMask == Overlap::XZ) && (adjacencyMask & (Adjacency::Front | Adjacency::Back))) ||
    ((overlapMask == Overlap::XY) && (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))))
  {
    // idx is the index in extent of current block on which side the face overlap occurs
    int idx = -1;
    switch (adjacencyMask)
    {
      case Adjacency::Left:
        idx = 0;
        break;
      case Adjacency::Right:
        idx = 1;
        break;
      case Adjacency::Front:
        idx = 2;
        break;
      case Adjacency::Back:
        idx = 3;
        break;
      case Adjacency::Bottom:
        idx = 4;
        break;
      case Adjacency::Top:
        idx = 5;
        break;
      default:
        std::cerr << "WRONG adjacencyMask " << std::endl;
        break;
    }

    AddGhostLayerToGrid(idx, outputGhostLevels, blockStructure, blockInformation);
  }
  // Here we look at ajacency where edges overlaps but no face overlap occurs
  //   ___
  //  /__/|
  // |  | |__
  // |__|/__/|
  //    |  | |
  //    |__|/
  //
  else if (((overlapMask == Overlap::X) && (adjacencyMask & (Adjacency::Front | Adjacency::Back)) &&
             (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))) ||
    ((overlapMask == Overlap::Y) && (adjacencyMask & (Adjacency::Left | Adjacency::Right)) &&
      (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))) ||
    ((overlapMask == Overlap::Z) && (adjacencyMask & (Adjacency::Left | Adjacency::Right)) &&
      (adjacencyMask & (Adjacency::Front | Adjacency::Back))))
  {
    // idx1 and idx2 are the indices in extent of current block
    // such that the intersection of the 2 faces mapped by those 2 indices is the overlapping edge.
    int idx1 = -1, idx2 = -1;
    switch (adjacencyMask)
    {
      case Adjacency::Front | Adjacency::Bottom:
        idx1 = 2;
        idx2 = 4;
        break;
      case Adjacency::Front | Adjacency::Top:
        idx1 = 2;
        idx2 = 5;
        break;
      case Adjacency::Back | Adjacency::Bottom:
        idx1 = 3;
        idx2 = 4;
        break;
      case Adjacency::Back | Adjacency::Top:
        idx1 = 3;
        idx2 = 5;
        break;
      case Adjacency::Left | Adjacency::Bottom:
        idx1 = 0;
        idx2 = 4;
        break;
      case Adjacency::Left | Adjacency::Top:
        idx1 = 0;
        idx2 = 5;
        break;
      case Adjacency::Right | Adjacency::Bottom:
        idx1 = 1;
        idx2 = 4;
        break;
      case Adjacency::Right | Adjacency::Top:
        idx1 = 1;
        idx2 = 5;
        break;
      case Adjacency::Left | Adjacency::Front:
        idx1 = 0;
        idx2 = 2;
        break;
      case Adjacency::Left | Adjacency::Back:
        idx1 = 0;
        idx2 = 3;
        break;
      case Adjacency::Right | Adjacency::Front:
        idx1 = 1;
        idx2 = 2;
        break;
      case Adjacency::Right | Adjacency::Back:
        idx1 = 1;
        idx2 = 3;
        break;
      default:
        std::cerr << "WRONG adjacencyMask" << std::endl;
        break;
    }

    AddGhostLayerToGrid(idx1, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid(idx2, outputGhostLevels, blockStructure, blockInformation);
  }
  // Here we look at ajacency where corners touch but no edges / faces overlap
  //   ___
  //  /__/|
  // |  | |
  // |__|/__
  //    /__/|
  //   |  | |
  //   |__|/
  //
  else
  {
    // idx1, idx2 and idx3 are the indices in extent of current block
    // such that the intersection of the 3 faces mapped by those 3 indices is the concurrant corner.
    int idx1 = -1, idx2 = -1, idx3 = -1;
    switch (adjacencyMask)
    {
      case Adjacency::Left | Adjacency::Front | Adjacency::Bottom:
        idx1 = 0;
        idx2 = 2;
        idx3 = 4;
        break;
      case Adjacency::Left | Adjacency::Front | Adjacency::Top:
        idx1 = 0;
        idx2 = 2;
        idx3 = 5;
        break;
      case Adjacency::Left | Adjacency::Back | Adjacency::Bottom:
        idx1 = 0;
        idx2 = 3;
        idx3 = 4;
        break;
      case Adjacency::Left | Adjacency::Back | Adjacency::Top:
        idx1 = 0;
        idx2 = 3;
        idx3 = 5;
        break;
      case Adjacency::Right | Adjacency::Front | Adjacency::Bottom:
        idx1 = 1;
        idx2 = 2;
        idx3 = 4;
        break;
      case Adjacency::Right | Adjacency::Front | Adjacency::Top:
        idx1 = 1;
        idx2 = 2;
        idx3 = 5;
        break;
      case Adjacency::Right | Adjacency::Back | Adjacency::Bottom:
        idx1 = 1;
        idx2 = 3;
        idx3 = 4;
        break;
      case Adjacency::Right | Adjacency::Back | Adjacency::Top:
        idx1 = 1;
        idx2 = 3;
        idx3 = 5;
        break;
      // If we didn't reach any case in the 3 switches, then we do not need any connection with this
      // neighbor block. We remove this block from our local map.
      default:
        it = blockStructures.erase(it);
        return;
    }

    AddGhostLayerToGrid(idx1, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid(idx2, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid(idx3, outputGhostLevels, blockStructure, blockInformation);
  }

  // If we reach this point, then the current neighboring block is indeed adjacent to us.
  // We add it to our link map.
  localLinks.emplace(gid);

  // We need to iterate by hand here because of the potential iterator erasure in this function.
  ++it;
}

//----------------------------------------------------------------------------
/**
 * This function computes the adjacency and overlap masks mapping the configuration between the 2
 * input extents `localExtent` and `extent`
 */
void ComputeAdjacencyAndOverlapMasks(const ExtentType& localExtent, const ExtentType& extent,
  unsigned char& adjacencyMask, unsigned char& overlapMask)
{
  // AdjacencyMask is a binary mask that is trigger if 2
  // blocks are adjacent. Dimensionnality of the grid is carried away
  // by discarding any bit that is on a degenerate dimension
  adjacencyMask = (((localExtent[0] == extent[1]) * Adjacency::Left) |
                    ((localExtent[1] == extent[0]) * Adjacency::Right) |
                    ((localExtent[2] == extent[3]) * Adjacency::Front) |
                    ((localExtent[3] == extent[2]) * Adjacency::Back) |
                    ((localExtent[4] == extent[5]) * Adjacency::Bottom) |
                    ((localExtent[5] == extent[4]) * Adjacency::Top)) &
    (((Adjacency::Left | Adjacency::Right) * (localExtent[0] != localExtent[1])) |
      ((Adjacency::Front | Adjacency::Back) * (localExtent[2] != localExtent[3])) |
      ((Adjacency::Bottom | Adjacency::Top) * (localExtent[4] != localExtent[5])));

  overlapMask = ((localExtent[0] < extent[1] && extent[0] < localExtent[1])) |
    ((localExtent[2] < extent[3] && extent[2] < localExtent[3]) << 1) |
    ((localExtent[4] < extent[5] && extent[4] < localExtent[5]) << 2);
}

//----------------------------------------------------------------------------
/**
 * Function to be overloaded for each supported input grid data sets.
 * This function will return true if 2 input block structures are adjacent, false otherwise.
 */
bool SynchronizeGridExtents(const ImageDataBlockStructure& localBlockStructure,
  const ImageDataBlockStructure& blockStructure, ExtentType& shiftedExtent)
{
  // Images are spatially defined by origin, spacing, dimension, and orientation.
  // We make sure that they all connect well using those values.
  const VectorType& localOrigin = localBlockStructure.Origin;
  const VectorType& localSpacing = localBlockStructure.Spacing;
  const QuaternionType& localQ = localBlockStructure.OrientationQuaternion;

  const ExtentType& extent = blockStructure.Extent;
  const QuaternionType& q = blockStructure.OrientationQuaternion;
  const VectorType& spacing = blockStructure.Spacing;

  // We skip if spacing or quaternions don't match
  // spacing == localSpacing <=> dot(spacing, localSpacing) == norm(localSpacing)^2
  // q == localQ <=> dot(q, localQ) == 1 (both are unitary quaternions)
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5] ||
    !vtkMathUtilities::NearlyEqual(
      vtkMath::Dot(spacing, localSpacing), vtkMath::SquaredNorm(localSpacing)) ||
    !(std::fabs(vtkMath::Dot<double, 4>(q.GetData(), localQ.GetData()) - 1.0) < VTK_DBL_EPSILON))
  {
    return false;
  }

  // We reposition extent all together so we have a unified extent framework with the current
  // neighbor.
  const VectorType& origin = blockStructure.Origin;
  int originDiff[3] = { static_cast<int>(std::lround((origin[0] - localOrigin[0]) / spacing[0])),
    static_cast<int>(std::lround((origin[1] - localOrigin[1]) / spacing[1])),
    static_cast<int>(std::lround((origin[2] - localOrigin[2]) / spacing[2])) };

  shiftedExtent =
    ExtentType{ extent[0] - originDiff[0], extent[1] - originDiff[0], extent[2] - originDiff[1],
      extent[3] - originDiff[1], extent[4] - originDiff[2], extent[5] - originDiff[2] };
  return true;
}

//============================================================================
template <bool IsIntegerT>
struct Comparator;

//============================================================================
template <>
struct Comparator<true>
{
  template <class ValueT1, class ValueT2>
  static bool Equals(const ValueT1& localVal, const ValueT2& val)
  {
    return !(localVal - val);
  }
};

//============================================================================
template <>
struct Comparator<false>
{
  template <class ValueT>
  static bool Equals(const ValueT& val1, const ValueT& val2)
  {
    return std::fabs(val1 - val2) <
      std::max(std::numeric_limits<ValueT>::epsilon() *
            std::max(std::fabs(val1), std::fabs(val2)),
        std::numeric_limits<ValueT>::min());
  }
};

//============================================================================
struct RectilinearGridFittingWorker
{
  RectilinearGridFittingWorker(vtkDataArray* array)
    : Array(array)
  {
  }

  template <class ArrayT>
  void operator()(ArrayT* localArray)
  {
    ArrayT* array = ArrayT::SafeDownCast(this->Array);
    if (localArray->GetValue(localArray->GetNumberOfTuples() - 1) >
        array->GetValue(array->GetNumberOfTuples() - 1))
    {
      this->FitArrays(array, localArray);
    }
    else
    {
      this->FitArrays(localArray, array);
      std::swap(this->MinId, this->LocalMinId);
      std::swap(this->MaxId, this->LocalMaxId);
    }
  }

  template <class ArrayT>
  void FitArrays(ArrayT* lowerMaxArray, ArrayT* upperMaxArray)
  {
    using ValueType = typename ArrayT::ValueType;
    constexpr bool IsInteger = std::numeric_limits<ValueType>::is_integer;
    const auto& lowerMinArray = lowerMaxArray->GetValue(0) > upperMaxArray->GetValue(0)
    ? upperMaxArray : lowerMaxArray;
    const auto& upperMinArray = lowerMaxArray->GetValue(0) < upperMaxArray->GetValue(0)
    ? upperMaxArray : lowerMaxArray;
    vtkIdType id = 0;
    while (id < lowerMinArray->GetNumberOfTuples() &&
      (lowerMinArray->GetValue(id) < upperMinArray->GetValue(0) &&
        !Comparator<IsInteger>::Equals(lowerMinArray->GetValue(id), upperMinArray->GetValue(0))))
    {
      ++id;
    }
    if (this->SubArraysAreEqual(lowerMinArray, upperMinArray, id))
    {
      this->LocalMinId = 0;
      this->MinId = id;
      if (lowerMaxArray->GetValue(0) > upperMaxArray->GetValue(0))
      {
        std::swap(this->MaxId, this->LocalMaxId);
      }
    }
  }

  template <class ArrayT>
  bool SubArraysAreEqual(ArrayT* lowerArray, ArrayT* upperArray, vtkIdType lowerId)
  {
    vtkIdType upperId = 0;
    using ValueType = typename ArrayT::ValueType;
    constexpr bool IsInteger = std::numeric_limits<ValueType>::is_integer;
    while (lowerId < lowerArray->GetNumberOfTuples() && upperId < upperArray->GetNumberOfTuples() &&
      Comparator<IsInteger>::Equals(lowerArray->GetValue(lowerId), upperArray->GetValue(upperId)))
    {
      ++lowerId;
      ++upperId;
    }
    if (lowerId == lowerArray->GetNumberOfTuples())
    {
      this->MaxId = lowerId - 1;
      this->LocalMaxId = upperId - 1;
      this->Overlaps = true;
      return true;
    }
    return false;
  }

  vtkDataArray* Array;
  int MinId = 0, MaxId = -1, LocalMinId = 0, LocalMaxId = -1;
  bool Overlaps = false;
};

//----------------------------------------------------------------------------
/**
 * Function to be overloaded for each supported input grid data sets.
 * This function will return true if 2 input block structures are adjacent, false otherwise.
 */
bool SynchronizeGridExtents(const RectilinearGridBlockStructure& localBlockStructure,
  const RectilinearGridBlockStructure& blockStructure, ExtentType& shiftedExtent)
{
  // TODO comment
  const ExtentType& extent = blockStructure.Extent;
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    return false;
  }
  const ExtentType& localExtent = localBlockStructure.Extent;

  const vtkSmartPointer<vtkDataArray>& localXCoordinates = localBlockStructure.XCoordinates;
  const vtkSmartPointer<vtkDataArray>& localYCoordinates = localBlockStructure.YCoordinates;
  const vtkSmartPointer<vtkDataArray>& localZCoordinates = localBlockStructure.ZCoordinates;

  const vtkSmartPointer<vtkDataArray>& xCoordinates = blockStructure.XCoordinates;
  const vtkSmartPointer<vtkDataArray>& yCoordinates = blockStructure.YCoordinates;
  const vtkSmartPointer<vtkDataArray>& zCoordinates = blockStructure.ZCoordinates;

  using Dispatcher = vtkArrayDispatch::Dispatch;
  RectilinearGridFittingWorker xWorker(xCoordinates), yWorker(yCoordinates), zWorker(zCoordinates);

  Dispatcher::Execute(localXCoordinates, xWorker);
  Dispatcher::Execute(localYCoordinates, yWorker);
  Dispatcher::Execute(localZCoordinates, zWorker);

  // The overlap between the 2 grids needs to have at least one degenerate dimension in order
  // for them to be adjacent.
  if ((!xWorker.Overlaps || !yWorker.Overlaps || !zWorker.Overlaps) &&
    (xWorker.MinId != xWorker.MaxId || yWorker.MinId != yWorker.MaxId ||
      zWorker.MinId != zWorker.MaxId))
  {
    return false;
  }

  int originDiff[3] = { extent[0] + xWorker.MinId - localExtent[0] - xWorker.LocalMinId,
    extent[2] + yWorker.MinId - localExtent[2] - yWorker.LocalMinId,
    extent[4] + zWorker.MinId - localExtent[4] - zWorker.LocalMinId };

  shiftedExtent =
    ExtentType{ extent[0] - originDiff[0], extent[1] - originDiff[0], extent[2] - originDiff[1],
      extent[3] - originDiff[1], extent[4] - originDiff[2], extent[5] - originDiff[2] };
  return true;
}

//----------------------------------------------------------------------------
void UpdateOutputGridPoints(
  vtkImageData* vtkNotUsed(output), GridInformation& vtkNotUsed(blockInformation))
{
  // Points are implicit in an vtkImageData. We do nothing.
}

//----------------------------------------------------------------------------
void AppendGhostPointsForRectilinearGrid(
  vtkDataArray*& coordinates, vtkDataArray*& preCoordinates, vtkDataArray* postCoordinates)
{
  if (preCoordinates)
  {
    std::swap(preCoordinates, coordinates);
    coordinates->InsertTuples(
      coordinates->GetNumberOfTuples(), preCoordinates->GetNumberOfTuples(), 0, preCoordinates);
  }
  if (postCoordinates)
  {
    coordinates->InsertTuples(
      coordinates->GetNumberOfTuples(), postCoordinates->GetNumberOfTuples(), 0, postCoordinates);
  }
}

//----------------------------------------------------------------------------
void UpdateOutputGridPoints(
  vtkRectilinearGrid* output, RectilinearGridInformation& blockInformation)
{
  auto& coordinateGhosts = blockInformation.CoordinateGhosts;

  vtkDataArray* xCoordinates = output->GetXCoordinates();
  vtkDataArray* preXCoordinates = coordinateGhosts[0];
  AppendGhostPointsForRectilinearGrid(xCoordinates, preXCoordinates, coordinateGhosts[1]);
  output->SetXCoordinates(xCoordinates);

  vtkDataArray* yCoordinates = output->GetYCoordinates();
  vtkDataArray* preYCoordinates = coordinateGhosts[2];
  AppendGhostPointsForRectilinearGrid(yCoordinates, preYCoordinates, coordinateGhosts[3]);
  output->SetYCoordinates(yCoordinates);

  vtkDataArray* zCoordinates = output->GetZCoordinates();
  vtkDataArray* preZCoordinates = coordinateGhosts[4];
  AppendGhostPointsForRectilinearGrid(zCoordinates, preZCoordinates, coordinateGhosts[5]);
  output->SetZCoordinates(zCoordinates);
}

//----------------------------------------------------------------------------
template <class GridDataSetT, class BlockInformationT>
void UpdateOutputGridStructure(GridDataSetT* output, BlockInformationT& blockInformation)
{
  const ExtentType& ghostThickness = blockInformation.ExtentGhostThickness;
  ExtentType outputExtent = blockInformation.Extent;
  // We update the extent of the current output and add ghost layers.
  outputExtent[0] -= ghostThickness[0];
  outputExtent[1] += ghostThickness[1];
  outputExtent[2] -= ghostThickness[2];
  outputExtent[3] += ghostThickness[3];
  outputExtent[4] -= ghostThickness[4];
  outputExtent[5] += ghostThickness[5];
  output->SetExtent(outputExtent.data());

  UpdateOutputGridPoints(output, blockInformation);
}

//----------------------------------------------------------------------------
/**
 * Function computing the link map and allocating ghosts for grids.
 * See `ComputeLinkMapAndAllocateGhosts`.
 */
template <class GridDataSetT>
LinkMap ComputeGridLinkMapAndAllocateGhosts(const diy::Master& master,
  std::vector<GridDataSetT*>& inputs, std::vector<GridDataSetT*>& outputs, int outputGhostLevels)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockStructureType = typename BlockType::BlockStructureType;

  LinkMap linkMap(inputs.size());

  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    // Getting block structures sent by other blocks
    BlockType* block = master.block<BlockType>(localId);
    BlockMapType<BlockStructureType>& blockStructures = block->BlockStructures;

    auto& input = inputs[localId];
    const ExtentType& localExtent = block->Information.Extent;

    // If I am myself empty, I get rid of everything and skip.
    if (localExtent[0] > localExtent[1] || localExtent[2] > localExtent[3] ||
      localExtent[4] > localExtent[5])
    {
      blockStructures.clear();
      continue;
    }

    auto& localLinks = linkMap[localId];

    BlockStructureType localBlockStructure(input, block->Information);

    for (auto it = blockStructures.begin(); it != blockStructures.end();)
    {
      BlockStructureType& blockStructure = it->second;

      // We synchronize extents, i.e. we shift the extent of current block neighbor
      // so it is described relative to current block.
      ExtentType shiftedExtent;
      if (!SynchronizeGridExtents(localBlockStructure, blockStructure, shiftedExtent))
      {
        // We end up here if extents cannot be fitted together
        it = blockStructures.erase(it);
        continue;
      }

      unsigned char& adjacencyMask = blockStructure.AdjacencyMask;
      unsigned char overlapMask;

      // We compute the adjacency mask and the extent.
      ComputeAdjacencyAndOverlapMasks(localExtent, shiftedExtent, adjacencyMask, overlapMask);

      ExtentType& neighborExtentWithNewGhosts = blockStructure.ExtentWithNewGhosts;
      neighborExtentWithNewGhosts = blockStructure.Extent;

      // We compute the adjacency mask and the extent.
      // We update our neighbor's block extent with ghost layers given spatial adjacency.
      LinkGrid(blockStructures, it, block->Information, localLinks, adjacencyMask, overlapMask,
        outputGhostLevels);
    }

    auto& output = outputs[localId];
    UpdateOutputGridStructure(output, block->Information);

    // Now that output is allocated and spatially defined, we clone the input into the output.
    CloneGrid(input, output);
  }
  return linkMap;
}

//----------------------------------------------------------------------------
/**
 * Given 2 input extents `localExtent` and `extent`, this function returns the list of ids in `grid`
 * such that the cells lie in the intersection of the 2 input extents.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeGridInterfaceCellIds(
  const ExtentType& localExtent, const ExtentType& extent, GridDataSetT* grid)
{
  int imin, imax, jmin, jmax, kmin, kmax;
  // We shift imax, jmax and kmax in case of degenerate dimension.
  imin = std::max(extent[0], localExtent[0]);
  imax = std::min(extent[1], localExtent[1]) + (localExtent[0] == localExtent[1]);
  jmin = std::max(extent[2], localExtent[2]);
  jmax = std::min(extent[3], localExtent[3]) + (localExtent[2] == localExtent[3]);
  kmin = std::max(extent[4], localExtent[4]);
  kmax = std::min(extent[5], localExtent[5]) + (localExtent[4] == localExtent[5]);

  const int* gridExtent = grid->GetExtent();

  vtkNew<vtkIdList> ids;
  ids->SetNumberOfIds((imax - imin) * (jmax - jmin) * (kmax - kmin));
  vtkIdType count = 0;
  for (int k = kmin; k < kmax; ++k)
  {
    for (int j = jmin; j < jmax; ++j)
    {
      for (int i = imin; i < imax; ++i, ++count)
      {
        int ijk[3] = { i, j, k };
        ids->SetId(count, vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk));
      }
    }
  }
  return ids;
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in input `grid` of the cells such that `grid`'s extent overlaps the
 * block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT, class BlockT>
vtkSmartPointer<vtkIdList> ComputeInputGridInterfaceCellIds(
  const BlockT* block, int gid, GridDataSetT* grid)
{
  typename BlockT::BlockStructureType blockStructure = block->BlockStructures.at(gid);
  const ExtentType& extent = blockStructure.ExtentWithNewGhosts;
  const ExtentType& localExtent = block->Information.Extent;

  return ComputeGridInterfaceCellIds(localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in output `grid` of the cells such that `grid`'s extent overlaps
 * the block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT, class BlockT>
vtkSmartPointer<vtkIdList> ComputeOutputGridInterfaceCellIds(
  const BlockT* block, int gid, GridDataSetT* grid)
{
  typename BlockT::BlockStructureType blockStructure = block->BlockStructures.at(gid);
  const ExtentType& extent = blockStructure.Extent;
  int* gridExtent = grid->GetExtent();
  ExtentType localExtent{
    gridExtent[0], gridExtent[1], gridExtent[2], gridExtent[3], gridExtent[4], gridExtent[5] };

  return ComputeGridInterfaceCellIds(localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * Given 2 input extents `localExtent` and `extent`, this function returns the list of ids in `grid`
 * such that the points lie in the intersection of the 2 input extents.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeGridInterfacePointIds(unsigned char adjacencyMask,
  const ExtentType& localExtent, const ExtentType& extent, GridDataSetT* grid)
{
  int imin, imax, jmin, jmax, kmin, kmax;
  imin = std::max(extent[0], localExtent[0]);
  imax = std::min(extent[1], localExtent[1]);
  jmin = std::max(extent[2], localExtent[2]);
  jmax = std::min(extent[3], localExtent[3]);
  kmin = std::max(extent[4], localExtent[4]);
  kmax = std::min(extent[5], localExtent[5]);

  // We give ownership of the non ghost version of a point to the most right / back / top grid.
  // We do that by removing the most right / back / top layer of points of the intersection between
  // the 2 input extents.
  if (adjacencyMask & Adjacency::Right)
  {
    --imax;
  }
  if (adjacencyMask & Adjacency::Back)
  {
    --jmax;
  }
  if (adjacencyMask & Adjacency::Top)
  {
    --kmax;
  }

  const int* gridExtent = grid->GetExtent();

  vtkNew<vtkIdList> ids;
  ids->SetNumberOfIds((imax - imin + 1) * (jmax - jmin + 1) * (kmax - kmin + 1));
  vtkIdType count = 0;
  for (int k = kmin; k <= kmax; ++k)
  {
    for (int j = jmin; j <= jmax; ++j)
    {
      for (int i = imin; i <= imax; ++i, ++count)
      {
        int ijk[3] = { i, j, k };
        ids->SetId(count, vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk));
      }
    }
  }
  return ids;
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in input `grid` of the pointss such that `grid`'s extent overlaps
 * the block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT, class BlockT>
vtkSmartPointer<vtkIdList> ComputeInputGridInterfacePointIds(
  const BlockT* block, int gid, GridDataSetT* grid)
{
  typename BlockT::BlockStructureType blockStructure = block->BlockStructures.at(gid);
  const unsigned char& adjacencyMask = blockStructure.AdjacencyMask;
  const ExtentType& extent = blockStructure.ExtentWithNewGhosts;
  const ExtentType& localExtent = block->Information.Extent;

  return ComputeGridInterfacePointIds(adjacencyMask, localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in output `grid` of the points such that `grid`'s extent overlaps
 * the block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT, class BlockT>
vtkSmartPointer<vtkIdList> ComputeOutputGridInterfacePointIds(
  const BlockT* block, int gid, GridDataSetT* grid)
{
  typename BlockT::BlockStructureType blockStructure = block->BlockStructures.at(gid);
  const unsigned char& adjacencyMask = blockStructure.AdjacencyMask;
  const ExtentType& extent = blockStructure.Extent;
  int* gridExtent = grid->GetExtent();
  ExtentType localExtent
    { gridExtent[0], gridExtent[1], gridExtent[2], gridExtent[3], gridExtent[4], gridExtent[5] };

  // We do a bit shift on adjacencyMask to have the same adjacency mask as in the Input version of
  // this function. It produces an axial symmetry on each dimension having an adjacency.
  return ComputeGridInterfacePointIds(adjacencyMask << 1, localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * This function fills hidden ghosts in allocated ghost layers for grid data sets.
 * This step is essential to perform before filling duplicate because there might be junctions with
 * allocated ghosts but no grid to get data from. This can happen when adjacent faces are of
 * different size.
 */
template <class GridDataSetT>
void FillGridHiddenGhosts(const diy::Master& master, std::vector<GridDataSetT*>& outputs,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    auto& output = outputs[localId];

    ExtentType localExtent;
    output->GetExtent(localExtent.data());

    vtkSmartPointer<vtkUnsignedCharArray>& ghostCellArray = ghostCellArrays[localId];
    vtkSmartPointer<vtkUnsignedCharArray>& ghostPointArray = ghostPointArrays[localId];

    BlockType* block = master.block<BlockType>(localId);
    const ExtentType& localExtentWithNoGhosts = block->Information.Extent;

    int isDimensionDegenerate[3] = { localExtent[0] == localExtent[1],
      localExtent[2] == localExtent[3], localExtent[4] == localExtent[5] };

    // We are carefull and take into account when dimensions are degenerate:
    // we do not want to fill a degenerate dimension with ghosts.
    //
    // On each dimension, we have to fill each end of each segment on points and cells.
    // This is repeated for each dimension.
    if (!isDimensionDegenerate[0])
    {
      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtent[0],
        localExtentWithNoGhosts[0], localExtent[2], localExtent[3] + isDimensionDegenerate[1],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtentWithNoGhosts[1],
        localExtent[1], localExtent[2], localExtent[3] + isDimensionDegenerate[1], localExtent[4],
        localExtent[5] + isDimensionDegenerate[2],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtent[0],
        localExtentWithNoGhosts[0] - 1, localExtent[2], localExtent[3], localExtent[4],
        localExtent[5], vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtentWithNoGhosts[1] + 1,
        localExtent[1], localExtent[2], localExtent[3], localExtent[4], localExtent[5],
        vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);
    }
    if (!isDimensionDegenerate[1])
    {
      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2], localExtentWithNoGhosts[2],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtentWithNoGhosts[3], localExtent[3],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtent[0], localExtent[1],
        localExtent[2], localExtentWithNoGhosts[2] - 1, localExtent[4], localExtent[5],
        vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtent[0], localExtent[1],
        localExtentWithNoGhosts[3] + 1, localExtent[3], localExtent[4], localExtent[5],
        vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);
    }
    if (!isDimensionDegenerate[2])
    {
      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2],
        localExtent[3] + isDimensionDegenerate[1], localExtent[4], localExtentWithNoGhosts[4],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridCellArray(ghostCellArray.GetPointer(), output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2],
        localExtent[3] + isDimensionDegenerate[1], localExtentWithNoGhosts[5], localExtent[5],
        vtkDataSetAttributes::CellGhostTypes::HIDDENCELL);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtent[0], localExtent[1],
        localExtent[2], localExtent[3], localExtent[4], localExtentWithNoGhosts[4] - 1,
        vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);

      FillGridPointArray(ghostPointArray.GetPointer(), output, localExtent[0], localExtent[1],
        localExtent[2], localExtent[3], localExtentWithNoGhosts[5] + 1, localExtent[5],
        vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT);
    }
  }
}
} // anonymous namespace

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::GridBlockStructure::GridBlockStructure(const int* extent)
  : Extent{ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] }
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(const int extent[6],
  const double origin[3], const double spacing[3], const double orientationQuaternion[4])
  : GridBlockStructure(extent)
  , Origin{ origin[0], origin[1], origin[2] }
  , Spacing{ spacing[0], spacing[1], spacing[2] }
  , OrientationQuaternion{ orientationQuaternion[0], orientationQuaternion[1],
      orientationQuaternion[2], orientationQuaternion[3] }
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(const int extent[6],
  const double origin[3], const double spacing[3], vtkMatrix3x3* directionMatrix)
  : GridBlockStructure(extent)
  , Origin{ origin[0], origin[1], origin[2] }
  , Spacing{ spacing[0], spacing[1], spacing[2] }
{
  vtkMath::Matrix3x3ToQuaternion(directionMatrix->GetData(), OrientationQuaternion.GetData());
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(
  vtkImageData* image, const GridInformation& information)
  : ImageDataBlockStructure(information.Extent.data(), image->GetOrigin(), image->GetSpacing(),
      image->GetDirectionMatrix())
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::RectilinearGridBlockStructure::RectilinearGridBlockStructure(
  const int extent[6], vtkDataArray* xCoordinates, vtkDataArray* yCoordinates,
  vtkDataArray* zCoordinates)
  : GridBlockStructure(extent)
  , XCoordinates(vtkSmartPointer<vtkDataArray>::Take(xCoordinates))
  , YCoordinates(vtkSmartPointer<vtkDataArray>::Take(yCoordinates))
  , ZCoordinates(vtkSmartPointer<vtkDataArray>::Take(zCoordinates))
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::RectilinearGridBlockStructure::RectilinearGridBlockStructure(
  vtkRectilinearGrid* vtkNotUsed(grid), const RectilinearGridInformation& information)
  : GridBlockStructure(information.Extent.data())
  , XCoordinates(vtkSmartPointer<vtkDataArray>::Take(information.XCoordinates.GetPointer()))
  , YCoordinates(vtkSmartPointer<vtkDataArray>::Take(information.YCoordinates.GetPointer()))
  , ZCoordinates(vtkSmartPointer<vtkDataArray>::Take(information.ZCoordinates.GetPointer()))
{
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::ExchangeBlockStructures(diy::Master& master,
  const vtkDIYExplicitAssigner& assigner, std::vector<vtkImageData*>& inputs, int inputGhostLevels)
{
  using BlockType = ImageDataBlock;
  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    BlockType* block = master.block<BlockType>(localId);
    block->Information.Extent = PeelOffGhostLayers(inputs[localId], inputGhostLevels);
  }

  // Share Block Structures of everyone
  diy::all_to_all(
    master, assigner, [&master, &inputs](BlockType* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      int localId = master.lid(myBlockId);
      auto& input = inputs[localId];
      if (srp.round() == 0)
      {
        const ExtentType& extent = block->Information.Extent;
        double* origin = input->GetOrigin();
        double* spacing = input->GetSpacing();
        QuaternionType q;
        vtkMath::Matrix3x3ToQuaternion(input->GetDirectionMatrix()->GetData(), q.GetData());
        double* qBuffer = q.GetData();
        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue(blockId, origin, 3);
            srp.enqueue(blockId, spacing, 3);
            srp.enqueue(blockId, qBuffer, 4);
            srp.enqueue(blockId, extent.data(), 6);
          }
        }
      }
      else
      {
        int extent[6];
        double origin[3];
        double spacing[3];
        double q[4];
        for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.dequeue(blockId, origin, 3);
            srp.dequeue(blockId, spacing, 3);
            srp.dequeue(blockId, q, 4);
            srp.dequeue(blockId, extent, 6);

            block->BlockStructures.emplace(
              blockId.gid, ImageDataBlockStructure(extent, origin, spacing, q));
          }
        }
      }
    });
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::ExchangeBlockStructures(diy::Master& master,
  const vtkDIYExplicitAssigner& assigner, std::vector<vtkRectilinearGrid*>& inputs,
  int inputGhostLevels)
{
  using BlockType = RectilinearGridBlock;
  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    vtkRectilinearGrid* input = inputs[localId];
    int* inputExtent = input->GetExtent();
    if (!IsExtentValid(inputExtent))
    {
      continue;
    }
    BlockType* block = master.block<BlockType>(localId);
    auto& info = block->Information;
    ExtentType& extent = info.Extent;
    extent = PeelOffGhostLayers(input, inputGhostLevels);

    vtkDataArray* inputXCoordinates = input->GetXCoordinates();
    vtkDataArray* inputYCoordinates = input->GetYCoordinates();
    vtkDataArray* inputZCoordinates = input->GetZCoordinates();

    info.XCoordinates = inputXCoordinates->NewInstance();
    info.YCoordinates = inputYCoordinates->NewInstance();
    info.ZCoordinates = inputZCoordinates->NewInstance();

    info.XCoordinates->InsertTuples(
      0, extent[1] - extent[0] + 1, extent[0] - inputExtent[0], inputXCoordinates);

    info.YCoordinates->InsertTuples(
      0, extent[3] - extent[2] + 1, extent[2] - inputExtent[2], inputYCoordinates);
    info.ZCoordinates->InsertTuples(
      0, extent[5] - extent[4] + 1, extent[4] - inputExtent[4], inputZCoordinates);
  }

  // Share Block Structures of everyone
  diy::all_to_all(
    master, assigner, [](BlockType* block, const diy::ReduceProxy& srp) {
      int myBlockId = srp.gid();
      if (srp.round() == 0)
      {
        auto& info = block->Information;
        const ExtentType& extent = info.Extent;
        vtkDataArray* xCoordinates = info.XCoordinates;
        vtkDataArray* yCoordinates = info.YCoordinates;
        vtkDataArray* zCoordinates = info.ZCoordinates;
        for (int i = 0; i < srp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = srp.out_link().target(i);
          if (blockId.gid != myBlockId)
          {
            srp.enqueue(blockId, extent.data(), 6);
            srp.enqueue<vtkDataArray*>(blockId, xCoordinates);
            srp.enqueue<vtkDataArray*>(blockId, yCoordinates);
            srp.enqueue<vtkDataArray*>(blockId, zCoordinates);
          }
        }
      }
      else
      {
        int extent[6];
        for (int i = 0; i < static_cast<int>(srp.in_link().size()); ++i)
        {
          const diy::BlockID& blockId = srp.in_link().target(i);
          if (blockId.gid != myBlockId)
          {
            vtkDataArray* xCoordinates = nullptr;
            vtkDataArray* yCoordinates = nullptr;
            vtkDataArray* zCoordinates = nullptr;

            srp.dequeue(blockId, extent, 6);
            srp.dequeue<vtkDataArray*>(blockId, xCoordinates);
            srp.dequeue<vtkDataArray*>(blockId, yCoordinates);
            srp.dequeue<vtkDataArray*>(blockId, zCoordinates);

            block->BlockStructures.emplace(blockId.gid,
              RectilinearGridBlockStructure(extent, xCoordinates, yCoordinates, zCoordinates));
          }
        }
      }
    });
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::LinkMap vtkDIYGhostUtilities::ComputeLinkMapAndAllocateGhosts(
  const diy::Master& master, std::vector<vtkImageData*>& inputs,
  std::vector<vtkImageData*>& outputs, int outputGhostLevels)
{
  return ComputeGridLinkMapAndAllocateGhosts(master, inputs, outputs, outputGhostLevels);
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::LinkMap vtkDIYGhostUtilities::ComputeLinkMapAndAllocateGhosts(
  const diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs,
  std::vector<vtkRectilinearGrid*>& outputs, int outputGhostLevels)
{
  return ComputeGridLinkMapAndAllocateGhosts(master, inputs, outputs, outputGhostLevels);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::FillGhostArrays(const diy::Master& master,
  std::vector<vtkImageData*>& outputs,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays)
{
  FillGridHiddenGhosts(master, outputs, ghostCellArrays, ghostPointArrays);
  FillDuplicateGhosts(master, outputs, ghostCellArrays, ghostPointArrays);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::FillGhostArrays(const diy::Master& master,
  std::vector<vtkRectilinearGrid*>& outputs,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostCellArrays,
  std::vector<vtkSmartPointer<vtkUnsignedCharArray>>& ghostPointArrays)
{
  FillGridHiddenGhosts(master, outputs, ghostCellArrays, ghostPointArrays);
  FillDuplicateGhosts(master, outputs, ghostCellArrays, ghostPointArrays);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeInputInterfaceCellIds(
  const ImageDataBlock* block, int gid, vtkImageData* input)
{
  return ComputeInputGridInterfaceCellIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeInputInterfaceCellIds(
  const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input)
{
  return ComputeInputGridInterfaceCellIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeOutputInterfaceCellIds(
  const ImageDataBlock* block, int gid, vtkImageData* input)
{
  return ComputeOutputGridInterfaceCellIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeOutputInterfaceCellIds(
  const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input)
{
  return ComputeOutputGridInterfaceCellIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeInputInterfacePointIds(
  const ImageDataBlock* block, int gid, vtkImageData* input)
{
  return ComputeInputGridInterfacePointIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeInputInterfacePointIds(
  const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input)
{
  return ComputeInputGridInterfacePointIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeOutputInterfacePointIds(
  const ImageDataBlock* block, int gid, vtkImageData* input)
{
  return ComputeOutputGridInterfacePointIds(block, gid, input);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> vtkDIYGhostUtilities::ComputeOutputInterfacePointIds(
  const RectilinearGridBlock* block, int gid, vtkRectilinearGrid* input)
{
  return ComputeOutputGridInterfacePointIds(block, gid, input);
}
