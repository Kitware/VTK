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
#include "vtkStaticPointLocator.h"
#include "vtkStructuredGrid.h"
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
using ImageDataBlock = vtkDIYGhostUtilities::ImageDataBlock;
using RectilinearGridBlock = vtkDIYGhostUtilities::RectilinearGridBlock;
using StructuredGridBlock = vtkDIYGhostUtilities::StructuredGridBlock;

using ImageDataBlockStructure = ImageDataBlock::BlockStructureType;
using RectilinearGridBlockStructure = RectilinearGridBlock::BlockStructureType;
using StructuredGridBlockStructure = StructuredGridBlock::BlockStructureType;

using ImageDataInformation = ImageDataBlock::InformationType;
using RectilinearGridInformation = RectilinearGridBlock::InformationType;
using StructuredGridInformation = StructuredGridBlock::InformationType;
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
void FillCellArrayForStructuredData(ArrayT* array, GridDataSetT* grid, int imin, int imax, int jmin, int jmax,
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
void FillPointArrayForStructuredData(ArrayT* array, GridDataSetT* grid, int imin, int imax, int jmin, int jmax,
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
template <class GridDataSetT>
ExtentType PeelOffGhostLayers(GridDataSetT* grid)
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

  int ijkmin[3] = { gridExtent[0], gridExtent[2], gridExtent[4] };
  // We use `std::max` here to work for grids of dimension 2 and 1.
  // This gives "thickness" to the degenerate dimension
  int ijkmax[3] = { std::max(gridExtent[1], gridExtent[0] + 1), std::max(gridExtent[3], gridExtent[2] + 1),
    std::max(gridExtent[5], gridExtent[4] + 1) };

  // We lock degenerate dimensions
  bool lock[3] = { gridExtent[0] == gridExtent[1], gridExtent[2] == gridExtent[3],
    gridExtent[4] == gridExtent[5] };

  {
    // Strategy:
    // We create a cursor `ijk` that is at the bottom left front corner of the grid.
    // From there, we iterate each cursor dimension until the targetted brick is not a duplicate ghost.
    // When this happens, we stop the loop, and look in each non degenerate dimension
    // if consecutive shift backs land on a ghost or not. If it lands on a ghost, then the
    // corresponding dimension needs to be peeled up to the current position of the cursor.
    // If not, it doesn't.
    int ijk[3] = { ijkmin[0], ijkmin[1], ijkmin[2] };

    while (ijk[0] < ijkmax[0] && ijk[1] < ijkmax[1] && ijk[2] < ijkmax[2] &&
        (ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk))
          & vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL))
    {
      for (int dim = 0; dim < 3; ++dim)
      {
        if (!lock[dim])
        {
          ++ijk[dim];
        }
      }
    }

    for (int dim = 0; dim < 3; ++dim)
    {
      if (!lock[dim] && ijk[dim] != ijkmin[dim])
      {
        int tmp = ijk[dim];
        for (--ijk[dim]; ijk[dim] >= ijkmin[dim] &&
            !(ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk))
              & vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL); --ijk[dim])
        {
        }
        extent[2 * dim] = ijk[dim] + 1;
        ijk[dim] = tmp;
      }
      else
      {
        extent[2 * dim] = gridExtent[2 * dim];
      }
    }
  }

  {
    // Same pipeline than previous block, but starting from the top back right corner.
    int ijk[3] = { ijkmax[0] - 1, ijkmax[1] - 1, ijkmax[2] - 1 };

    while (ijk[0] >= ijkmin[0] && ijk[1] >= ijkmin[1] && ijk[2] >= ijkmin[2] &&
        (ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk))
          & vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL))
    {
      for (int dim = 0; dim < 3; ++dim)
      {
        if (!lock[dim])
        {
          --ijk[dim];
        }
      }
    }

    for (int dim = 0; dim < 3; ++dim)
    {
      if (!lock[dim] && ijk[dim] != ijkmax[dim])
      {
        int tmp = ijk[dim];
        for (++ijk[dim]; ijk[dim] < ijkmax[dim] &&
            !(ghosts->GetValue(vtkStructuredData::ComputeCellIdForExtent(gridExtent, ijk))
              & vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL); ++ijk[dim])
        {
        }
        extent[2 * dim + 1] = ijk[dim];
        ijk[dim] = tmp;
      }
      else
      {
        extent[2 * dim + 1] = gridExtent[2 * dim + 1];
      }
    }
  }
  
  return extent;
}

//----------------------------------------------------------------------------
void AddGhostLayerOfGridPoints(int vtkNotUsed(extentIdx), ImageDataInformation& vtkNotUsed(information),
  const ImageDataBlockStructure& vtkNotUsed(blockStructure))
{
  // Do nothing for image data. Points are all implicit.
}

//----------------------------------------------------------------------------
void AddGhostLayerOfGridPoints(int extentIdx, RectilinearGridInformation& blockInformation,
  const RectilinearGridBlockStructure& blockStructure)
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
    if (!(extentIdx % 2))
    {
      vtkSmartPointer<vtkDataArray> tmp = vtkSmartPointer<vtkDataArray>::Take(coords->NewInstance());
      tmp->InsertTuples(0, layerThickness - coordinateGhosts->GetNumberOfTuples(),
          coords->GetNumberOfTuples() - layerThickness - 1, coords);
      tmp->InsertTuples(tmp->GetNumberOfTuples(),
          coordinateGhosts->GetNumberOfTuples(), 0, coordinateGhosts);
      std::swap(tmp, coordinateGhosts);
    }
    else
    {
      coordinateGhosts->InsertTuples(coordinateGhosts->GetNumberOfTuples(), layerThickness
          - coordinateGhosts->GetNumberOfTuples(), 1, coords);
    }
  }
}

//----------------------------------------------------------------------------
void AddGhostLayerOfGridPoints(int vtkNotUsed(extentIdx),
    StructuredGridInformation& vtkNotUsed(blockInformation),
    const StructuredGridBlockStructure& vtkNotUsed(blockStructure))
{
  // Do nothing, we only have grid interfaces at this point. We will allocate the points
  // after the accumulated extent is computed.
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
template <class BlockT>
void AddGhostLayerToGrid(int idx, int outputGhostLevels,
  typename BlockT::BlockStructureType& blockStructure,
  typename BlockT::InformationType& blockInformation)
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
template <class BlockT, class IteratorT>
void LinkGrid(BlockMapType<typename BlockT::BlockStructureType>& blockStructures, IteratorT& it,
  typename BlockT::InformationType& blockInformation, Links& localLinks,
  unsigned char adjacencyMask, unsigned char overlapMask, int outputGhostLevels, int dim)
{
  int gid = it->first;
  auto& blockStructure = it->second;

  // Here we look at adjacency where faces overlap
  //   ______
  //  /__/__/|
  // |  |  | |
  // |__|__|/
  //
  if ((((dim == 3 && overlapMask == Overlap::YZ) || (dim == 2 && overlapMask & Overlap::YZ)
          || (dim == 1 && !overlapMask)) &&
        (adjacencyMask & (Adjacency::Left | Adjacency::Right))) ||
    ((((dim == 3 && overlapMask == Overlap::XZ) || (dim == 2 && overlapMask & Overlap::XZ))) &&
     (adjacencyMask & (Adjacency::Front | Adjacency::Back))) ||
    ((((dim == 3 && overlapMask == Overlap::XY) || (dim == 2 && overlapMask & Overlap::XY))) &&
     (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))))
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
        // Blocks are not connected, we can erase current block
        it = blockStructures.erase(it);
        if (dim != 1)
        {
          vtkLog(ERROR, "Wrong adjacency mask for 1D grid inputs");
        }
        return;
    }

    AddGhostLayerToGrid<BlockT>(idx, outputGhostLevels, blockStructure, blockInformation);
  }
  // Here we look at ajacency where edges overlaps but no face overlap occurs
  //   ___
  //  /__/|
  // |  | |__
  // |__|/__/|
  //    |  | |
  //    |__|/
  //
  else if ((((dim == 3 && overlapMask == Overlap::X) || (dim == 2 && !overlapMask)) &&
        (adjacencyMask & (Adjacency::Front | Adjacency::Back)) &&
             (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))) ||
    (((dim == 3 && overlapMask == Overlap::Y) || (dim == 2 && !overlapMask)) &&
     (adjacencyMask & (Adjacency::Left | Adjacency::Right)) &&
      (adjacencyMask & (Adjacency::Bottom | Adjacency::Top))) ||
    (((dim == 3 && overlapMask == Overlap::Z) || (dim == 2 && !overlapMask)) &&
      (adjacencyMask & (Adjacency::Left | Adjacency::Right)) &&
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
        // Blocks are not connected, we can erase current block
        it = blockStructures.erase(it);
        if (dim != 2)
        {
          vtkLog(ERROR, "Wrong adjacency mask for 2D grid inputs");
        }
        return;
    }

    AddGhostLayerToGrid<BlockT>(idx1, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid<BlockT>(idx2, outputGhostLevels, blockStructure, blockInformation);
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
      default:
        // Blocks are not connected, we can erase current block
        it = blockStructures.erase(it);
        if (dim != 3)
        {
          vtkLog(ERROR, "Wrong adjacency mask for 3D grid inputs");
        }
        return;
    }

    AddGhostLayerToGrid<BlockT>(idx1, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid<BlockT>(idx2, outputGhostLevels, blockStructure, blockInformation);
    AddGhostLayerToGrid<BlockT>(idx3, outputGhostLevels, blockStructure, blockInformation);
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
  int localDim = localBlockStructure.DataDimension;

  const ExtentType& extent = blockStructure.Extent;
  const QuaternionType& q = blockStructure.OrientationQuaternion;
  const VectorType& spacing = blockStructure.Spacing;
  int dim = blockStructure.DataDimension;

  // We skip if dimension, spacing or quaternions don't match
  // spacing == localSpacing <=> dot(spacing, localSpacing) == norm(localSpacing)^2
  // q == localQ <=> dot(q, localQ) == 1 (both are unitary quaternions)
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5] ||
    dim != localDim ||
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
  const ExtentType& extent = blockStructure.Extent;
  if (localBlockStructure.DataDimension != blockStructure.DataDimension ||
      extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
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

//============================================================================
struct StructuredGridFittingWorker
{
  /**
   * Constructor storing the 6 faces of the neighboring block.
   */
  StructuredGridFittingWorker(const vtkSmartPointer<vtkPoints> points[6],
      vtkNew<vtkStaticPointLocator> locator[6],
      const ExtentType& extent, StructuredGridBlockStructure::Grid2D& grid)
    : Points{ points[0]->GetData(), points[1]->GetData(), points[2]->GetData(), points[3]->GetData(),
        points[4]->GetData(), points[5]->GetData() }
    , Locator{ locator[0], locator[1], locator[2], locator[3], locator[4], locator[5] }
    , Grid(grid)
  {
    // We compute the extent of each external face of the neighbor block.
    for (int i = 0; i < 6; ++i)
    {
      ExtentType& e = this->Extent[i];
      e[i] = extent[i];
      e[i % 2 ? i - 1 : i + 1] = extent[i];
      for (int j = 0; j < 6; ++j)
      {
        if (i / 2 != j / 2)
        {
          e[j] = extent[j];
        }
      }
    }
  }

  /**
   * This method determines the local points (points from on external face of local block) are
   * connected the the points of one of the 6 faces of the block's neighbor.
   * The main subroutine `GridsFit` is asymmetrical: it needs to be called twice, first by querying
   * from the local block, finally by querying from the neighbor's block.
   *
   * If grids are connected, the overlapping extent is extracted in the form of a 2D grid.
   *
   * This method determines if grids are connected regardless of the orientation of their extent.
   * This means that given a direct frame (i, j, k) spanning the first grid, i can be mangled with
   * any dimension of the other grid. To simplify mpi communication, the convention is express the
   * indexing of the neighboring block relative to the one of the local block. For instance, if we
   * find that (i, -j) of the first grid connect with (j, k) of the second, we will multiply the
   * second dimension by -1 so that the local grid is spanned by (i, j), and the second by (j, -k).
   */
  template<class ArrayT>
  void operator()(ArrayT* localPoints)
  {
    for (int sideId = 0; sideId < 6; ++sideId)
    {
      ArrayT* points = vtkArrayDownCast<ArrayT>(this->Points[sideId]);
      if (this->GridsFit(localPoints, this->LocalExtent, this->LocalExtentIndex,
            points, this->Locator[sideId], this->Extent[sideId], sideId))
      {
        this->Connected = true;
      }
      else if (this->GridsFit(points, this->Extent[sideId], sideId,
            localPoints, this->LocalLocator, this->LocalExtent, this->LocalExtentIndex))
      {
        this->Connected = true;
        std::swap(this->Grid, this->LocalGrid);
      }
      else
      {
        continue;
      }

      // Now, we flip the grids so the local grid uses canonical coordinates (x increasing, y
      // increasing)
      if (this->LocalGrid.StartX > this->LocalGrid.EndX)
      {
        std::swap(this->LocalGrid.StartX, this->LocalGrid.EndX);
        this->LocalGrid.XOrientation *= -1;
        std::swap(this->Grid.StartX, this->Grid.EndX);
        this->Grid.XOrientation *= -1;
      }
      if (this->LocalGrid.StartY > this->LocalGrid.EndY)
      {
        std::swap(this->LocalGrid.StartY, this->LocalGrid.EndY);
        this->LocalGrid.YOrientation *= -1;
        std::swap(this->Grid.StartY, this->Grid.EndY);
        this->Grid.YOrientation *= -1;
      }

      // We have a 2D grid, we succeeded for sure
      if ((this->Grid.EndX - this->Grid.StartX) && (this->Grid.EndY - this->Grid.StartY))
      {
        this->BestConnectionFound = true;
        return;
      }
    }
  }

  /**
   * This looks if the 4 corners of the grid composed of points from `queryPoints` are points of the
   * second grid.
   * queryExtentId and extentId are parameters that tell on which face of the block the grids lie.
   * For each corners part of the neighboring grids, a subroutine is called to see if grids fit
   * perfectly. One match is not a sufficient condition for us to stop checking if grids fit.
   * Indeed, one can catch an edge on one face, while an entire face fits elsewhere, so this method
   * might be called even if a match has been found.
   */
  template<class ArrayT>
  bool GridsFit(ArrayT* queryPoints, const ExtentType& queryExtent, int queryExtentId,
      ArrayT* points, vtkAbstractPointLocator* locator, const ExtentType& extent, int extentId)
  {
    using ValueType = typename ArrayT::ValueType;
    constexpr bool IsInteger = std::numeric_limits<ValueType>::is_integer;

    bool retVal = false;

    int queryXDim = (queryExtentId + 2) % 6;
    queryXDim -= queryXDim % 2;
    int queryYDim = (queryExtentId + 4) % 6;
    queryYDim -= queryYDim % 2;
    int queryijk[3];
    queryijk[queryExtentId / 2] = queryExtent[queryExtentId];

    int xCorners[2] = { queryExtent[queryXDim], queryExtent[queryXDim + 1] };
    int yCorners[2] = { queryExtent[queryYDim], queryExtent[queryYDim + 1] };
    constexpr int sweepDirection[2] = { 1, -1 };

    for (int xCornerId = 0; xCornerId < 2; ++xCornerId)
    {
      queryijk[queryXDim / 2] = xCorners[xCornerId];
      for (int yCornerId = 0; yCornerId < 2; ++yCornerId)
      {
        queryijk[queryYDim / 2] = yCorners[yCornerId];
        vtkIdType queryPointId = vtkStructuredData::ComputePointIdForExtent(
            queryExtent.data(), queryijk);
        ValueType queryPoint[3];
        queryPoints->GetTypedTuple(queryPointId, queryPoint);
        double tmp[3] = { static_cast<double>(queryPoint[0]), static_cast<double>(queryPoint[1]),
          static_cast<double>(queryPoint[2]) };
        vtkIdType pointId = locator->FindClosestPoint(tmp);
        ValueType point[3];
        points->GetTypedTuple(pointId, point);

        if (Comparator<IsInteger>::Equals(point[0], queryPoint[0]) &&
            Comparator<IsInteger>::Equals(point[1], queryPoint[1]) &&
            Comparator<IsInteger>::Equals(point[2], queryPoint[2]))
        {
          if (this->SweepGrids(queryPoints, queryExtentId, queryExtent, queryXDim,
                xCorners[xCornerId], xCorners[(xCornerId + 1) % 2], sweepDirection[xCornerId],
                queryYDim, yCorners[yCornerId], yCorners[(yCornerId + 1) % 2],
                sweepDirection[yCornerId], points, pointId, extentId, extent))
          {
            retVal = true;
          }
        }
      }
    }
    return retVal;
  }

  /**
   * This method is called when one corner of the querying grid exists inside the other grid.
   * Both grids are swept in all directions. If points match until one corner is reached, then grids
   * are connected. If grids are connected, if the grid overlapping is larger than any previous
   * computed one, its extents and the id of the face are saved.
   */
  template<class ArrayT>
  bool SweepGrids(ArrayT* queryPoints, int queryExtentId, const ExtentType& queryExtent,
      int queryXDim, int queryXBegin, int queryXEnd, int directionX, int queryYDim, int queryYBegin,
      int queryYEnd, int directionY, ArrayT* points, int pointId, int extentId, const ExtentType& extent)
  {
    using ValueType = typename ArrayT::ValueType;
    constexpr bool IsInteger = std::numeric_limits<ValueType>::is_integer;
    constexpr int sweepDirection[2] = { 1, -1 };

    bool retVal = false;

    int queryijk[3], ijk[3];
    queryijk[queryExtentId / 2] = queryExtent[queryExtentId];
    vtkStructuredData::ComputePointStructuredCoordsForExtent(pointId, extent.data(), ijk);

    int xdim = ((extentId + 2) % 6);
    xdim -= xdim % 2;
    int ydim = ((extentId + 4) % 6);
    ydim -= ydim % 2;

    int xCorners[2] = { extent[xdim], extent[xdim + 1] };
    int yCorners[2] = { extent[ydim], extent[ydim + 1] };

    int xBegin = ijk[xdim / 2];
    int yBegin = ijk[ydim / 2];

    for (int xCornerId = 0; xCornerId < 2; ++xCornerId)
    {
      for (int yCornerId = 0; yCornerId < 2; ++yCornerId)
      {
        bool gridsAreFitting = true;
        int queryX, queryY = queryYBegin, x, y = yBegin;

        for (queryX = queryXBegin, x = xBegin;
            gridsAreFitting && queryX != queryXEnd + directionX &&
            x != xCorners[(xCornerId + 1) % 2] + sweepDirection[xCornerId];
            queryX += directionX, x += sweepDirection[xCornerId])
        {
          queryijk[queryXDim / 2] = queryX;
          ijk[xdim / 2] = x;

          for (queryY = queryYBegin, y = yBegin;
              gridsAreFitting && queryY != queryYEnd + directionY &&
              y != yCorners[(yCornerId + 1) % 2] + sweepDirection[yCornerId];
              queryY += directionY, y += sweepDirection[yCornerId])
          {
            queryijk[queryYDim / 2] = queryY;
            ijk[ydim / 2] = y;

            vtkIdType queryPointId = vtkStructuredData::ComputePointIdForExtent(
                queryExtent.data(), queryijk);
            vtkIdType id = vtkStructuredData::ComputePointIdForExtent(extent.data(), ijk);

            ValueType queryPoint[3];
            queryPoints->GetTypedTuple(queryPointId, queryPoint);
            ValueType point[3];
            points->GetTypedTuple(id, point);

            if (!Comparator<IsInteger>::Equals(point[0], queryPoint[0]) ||
                !Comparator<IsInteger>::Equals(point[1], queryPoint[1]) ||
                !Comparator<IsInteger>::Equals(point[2], queryPoint[2]))
            {
              gridsAreFitting = false;
            }
          }
        }
        queryX -= directionX;
        queryY -= directionY;
        x -= sweepDirection[xCornerId];
        y -= sweepDirection[yCornerId];
        // We save grid characteristics if the new grid is larger than the previous selected one.
        // This can happen when an edge is caught, but a whole face should be caught instead
        if (gridsAreFitting &&
            (std::abs(this->LocalGrid.EndX - this->LocalGrid.StartX) <= std::abs(queryX - queryXBegin) ||
             std::abs(this->LocalGrid.EndY - this->LocalGrid.StartY) <= std::abs(queryY - queryYBegin)))
        {
          this->LocalGrid.StartX = queryXBegin;
          this->LocalGrid.StartY = queryYBegin;
          this->LocalGrid.EndX = queryX;
          this->LocalGrid.EndY = queryY;
          this->LocalGrid.XOrientation = directionX;
          this->LocalGrid.YOrientation = directionY;
          this->LocalGrid.ExtentId = queryExtentId;

          this->Grid.StartX = xBegin;
          this->Grid.StartY = yBegin;
          this->Grid.EndX = x;
          this->Grid.EndY = y;
          this->Grid.XOrientation = sweepDirection[xCornerId];
          this->Grid.YOrientation = sweepDirection[yCornerId];
          this->Grid.ExtentId = queryExtentId;

          retVal = true;
        }
      }
    }
    return retVal;
  }

  vtkDataArray* Points[6];
  vtkStaticPointLocator* Locator[6];
  int LocalExtentIndex;
  ExtentType LocalExtent;
  ExtentType Extent[6];
  vtkStaticPointLocator* LocalLocator;
  bool Connected = false;
  bool BestConnectionFound = false;
  StructuredGridBlockStructure::Grid2D& Grid;
  StructuredGridBlockStructure::Grid2D LocalGrid;
};

//----------------------------------------------------------------------------
/**
 * Function to be overloaded for each supported input grid data sets.
 * This function will return true if 2 input block structures are adjacent, false otherwise.
 */
bool SynchronizeGridExtents(StructuredGridBlockStructure& localBlockStructure,
    StructuredGridBlockStructure& blockStructure, ExtentType& shiftedExtent)
{
  const ExtentType& extent = blockStructure.Extent;
  shiftedExtent = extent;
  
  if (localBlockStructure.DataDimension != blockStructure.DataDimension ||
      extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
  {
    return false;
  }
  const ExtentType& localExtent = localBlockStructure.Extent;
  const vtkSmartPointer<vtkPoints>* localPoints = localBlockStructure.OuterPointLayers;
  const vtkSmartPointer<vtkPoints>* points = blockStructure.OuterPointLayers;

  // This grid will be set by the structured grid fitting worker if the 2 blocks are connected.
  StructuredGridBlockStructure::Grid2D& gridInterface = blockStructure.GridInterface;

  // We need locators to query points inside grids.
  // Locators need `vtkDataSet`, so we create a `vtkPointSet` with the points of each face of the
  // neighboring block.
  vtkNew<vtkStaticPointLocator> locator[6];
  for (int id = 0; id < 6; ++id)
  {
    vtkNew<vtkPointSet> ps;
    ps->SetPoints(points[id]);
    locator[id]->SetDataSet(ps);
    locator[id]->BuildLocator();
  }

  using Dispatcher = vtkArrayDispatch::Dispatch;
  StructuredGridFittingWorker worker(points, locator, extent, gridInterface);

  // We look for a connection until either we tried them all, or if we found the best connection,
  // i.e. a full 2D grid has been found.
  // We iterate on each face of the local block.
  for (worker.LocalExtentIndex = 0;
      !worker.BestConnectionFound && worker.LocalExtentIndex < 6; ++worker.LocalExtentIndex)
  {
    vtkNew<vtkStaticPointLocator> localLocator;
    vtkNew<vtkPointSet> ps;

    ps->SetPoints(localPoints[worker.LocalExtentIndex]);
    localLocator->SetDataSet(ps);
    localLocator->BuildLocator();

    worker.LocalLocator = localLocator;
    worker.LocalExtent = localExtent;
    worker.LocalExtent[worker.LocalExtentIndex + (worker.LocalExtentIndex % 2 ? -1 : 1)] =
      localExtent[worker.LocalExtentIndex];

    Dispatcher::Execute(localPoints[worker.LocalExtentIndex]->GetData(), worker);
  }

  if (!worker.Connected)
  {
    return false;
  }
  
  const auto& localGrid = worker.LocalGrid;
  int xdim = (localGrid.ExtentId + 2) % 6;
  xdim -= xdim % 2;
  int ydim = (localGrid.ExtentId + 4) % 6;
  ydim -= ydim % 2;

  // We match extents to local extents.
  // We know the intersection already, so we ca just use the local grid interface extent.
  shiftedExtent[xdim] = localGrid.StartX;
  shiftedExtent[xdim + 1] = localGrid.EndX;
  shiftedExtent[ydim] = localGrid.StartY;
  shiftedExtent[ydim + 1] = localGrid.EndY;

  const auto& grid = worker.Grid;
  // Concerning the dimension orthogonal to the interface grid, we just copy the corresponding value
  // from the local extent, and we add the "depth" of the neighbor grid by looking at what is in
  // `extent`.
  int oppositeExtentId = grid.ExtentId + (grid.ExtentId % 2 ? -1 : 1);
  int deltaExtent = (localGrid.ExtentId % 2 ? -1 : 1) * std::abs(extent[grid.ExtentId] - extent[oppositeExtentId]);
  shiftedExtent[localGrid.ExtentId + (localGrid.ExtentId % 2 ? -1 : 1)] = shiftedExtent[localGrid.ExtentId] + deltaExtent;
  shiftedExtent[localGrid.ExtentId] = localExtent[localGrid.ExtentId];

  return true;
}

//----------------------------------------------------------------------------
template <class GridDataSetT>
LinkMap ComputeLinkMapForStructuredData(const diy::Master& master,
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

    auto& output = outputs[localId];
    int dim = output->GetDataDimension();

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
      LinkGrid<BlockType>(blockStructures, it, block->Information, localLinks,
        adjacencyMask, overlapMask, outputGhostLevels, dim);
    }
  }

  return linkMap;
}

//----------------------------------------------------------------------------
/**
 * Given 2 input extents `localExtent` and `extent`, this function returns the list of ids in `grid`
 * such that the cells lie in the intersection of the 2 input extents.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeInterfaceCellIdsForStructuredData(
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
  int ijk[3];
  for (int k = kmin; k < kmax; ++k)
  {
    ijk[2] = k;
    for (int j = jmin; j < jmax; ++j)
    {
      ijk[1] = j;
      for (int i = imin; i < imax; ++i, ++count)
      {
        ijk[0] = i;
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
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeInputInterfaceCellIdsForStructuredData(
  const typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType* block,
  int gid, GridDataSetT* grid)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockStructureType = typename BlockType::BlockStructureType;

  const BlockStructureType& blockStructure = block->BlockStructures.at(gid);
  const ExtentType& extent = blockStructure.ExtentWithNewGhosts;
  const ExtentType& localExtent = block->Information.Extent;

  return ComputeInterfaceCellIdsForStructuredData(localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in output `grid` of the cells such that `grid`'s extent overlaps
 * the block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeOutputInterfaceCellIdsForStructuredData(
  const typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType* block,
  int gid, GridDataSetT* grid)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockStructureType = typename BlockType::BlockStructureType;

  const BlockStructureType& blockStructure = block->BlockStructures.at(gid);
  const ExtentType& extent = blockStructure.Extent;
  int* gridExtent = grid->GetExtent();
  ExtentType localExtent{
    gridExtent[0], gridExtent[1], gridExtent[2], gridExtent[3], gridExtent[4], gridExtent[5] };

  return ComputeInterfaceCellIdsForStructuredData(localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * Given 2 input extents `localExtent` and `extent`, this function returns the list of ids in `grid`
 * such that the points lie in the intersection of the 2 input extents.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeInterfacePointIdsForStructuredData(unsigned char adjacencyMask,
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
  int ijk[3];
  for (int k = kmin; k <= kmax; ++k)
  {
    ijk[2] = k;
    for (int j = jmin; j <= jmax; ++j)
    {
      ijk[1] = j;
      for (int i = imin; i <= imax; ++i, ++count)
      {
        ijk[0] = i;
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
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeInputInterfacePointIdsForStructuredData(
  const typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType* block,
  int gid, GridDataSetT* grid)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockStructureType = typename BlockType::BlockStructureType;

  const BlockStructureType& blockStructure = block->BlockStructures.at(gid);
  const unsigned char& adjacencyMask = blockStructure.AdjacencyMask;
  const ExtentType& extent = blockStructure.ExtentWithNewGhosts;
  const ExtentType& localExtent = block->Information.Extent;

  return ComputeInterfacePointIdsForStructuredData(adjacencyMask, localExtent, extent, grid);
}

//----------------------------------------------------------------------------
/**
 * This function returns the ids in output `grid` of the points such that `grid`'s extent overlaps
 * the block of global id gid's extent when ghosts are added.
 */
template <class GridDataSetT>
vtkSmartPointer<vtkIdList> ComputeOutputInterfacePointIdsForStructuredData(
  const typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType* block,
  int gid, GridDataSetT* grid)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockStructureType = typename BlockType::BlockStructureType;

  const BlockStructureType& blockStructure = block->BlockStructures.at(gid);
  const unsigned char& adjacencyMask = blockStructure.AdjacencyMask;
  const ExtentType& extent = blockStructure.Extent;
  int* gridExtent = grid->GetExtent();
  ExtentType localExtent
    { gridExtent[0], gridExtent[1], gridExtent[2], gridExtent[3], gridExtent[4], gridExtent[5] };

  // We do a bit shift on adjacencyMask to have the same adjacency mask as in the Input version of
  // this function. It produces an axial symmetry on each dimension having an adjacency.
  return ComputeInterfacePointIdsForStructuredData(adjacencyMask << 1, localExtent, extent, grid);
}

//----------------------------------------------------------------------------
void UpdateOutputGridPoints(
  vtkImageData* vtkNotUsed(output), ImageDataInformation& vtkNotUsed(blockInformation))
{
  // Points are implicit in an vtkImageData. We do nothing.
}

//----------------------------------------------------------------------------
void AppendGhostPointsForRectilinearGrid(vtkSmartPointer<vtkDataArray>& coordinates,
    vtkSmartPointer<vtkDataArray>& preCoordinates, vtkSmartPointer<vtkDataArray>& postCoordinates)
{
  if (preCoordinates)
  {
    std::swap(preCoordinates, coordinates);
    coordinates->InsertTuples(coordinates->GetNumberOfTuples(),
        preCoordinates->GetNumberOfTuples(), 0, preCoordinates.GetPointer());
  }
  if (postCoordinates)
  {
    coordinates->InsertTuples(coordinates->GetNumberOfTuples(),
        postCoordinates->GetNumberOfTuples(), 0, postCoordinates.GetPointer());
  }
}

//----------------------------------------------------------------------------
void UpdateOutputGridPoints(
  vtkRectilinearGrid* output, RectilinearGridInformation& blockInformation)
{
  auto& coordinateGhosts = blockInformation.CoordinateGhosts;

  vtkSmartPointer<vtkDataArray> xCoordinates = blockInformation.XCoordinates;
  vtkSmartPointer<vtkDataArray>& preXCoordinates = coordinateGhosts[0];
  AppendGhostPointsForRectilinearGrid(xCoordinates, preXCoordinates, coordinateGhosts[1]);
  output->SetXCoordinates(xCoordinates);

  vtkSmartPointer<vtkDataArray>& yCoordinates = blockInformation.YCoordinates;
  vtkSmartPointer<vtkDataArray>& preYCoordinates = coordinateGhosts[2];
  AppendGhostPointsForRectilinearGrid(yCoordinates, preYCoordinates, coordinateGhosts[3]);
  output->SetYCoordinates(yCoordinates);

  vtkSmartPointer<vtkDataArray>& zCoordinates = blockInformation.ZCoordinates;
  vtkSmartPointer<vtkDataArray>& preZCoordinates = coordinateGhosts[4];
  AppendGhostPointsForRectilinearGrid(zCoordinates, preZCoordinates, coordinateGhosts[5]);
  output->SetZCoordinates(zCoordinates);
}

//----------------------------------------------------------------------------
void UpdateOutputGridPoints(vtkStructuredGrid* output,
    StructuredGridInformation& blockInformation)
{
  // We create a new instance because at this point input and output share the same point arrays.
  // This is done in vtkStructuredGrid::CopyStructure.
  vtkNew<vtkPoints> points;
  vtkPoints* inputPoints = blockInformation.InputPoints;
  const ExtentType& inputExtent = blockInformation.Extent;
  const int* extent = output->GetExtent();

  points->SetNumberOfPoints((extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) *
      (extent[5] - extent[4] + 1));

  int ijk[3];

  for (int k = inputExtent[4]; k <= inputExtent[5]; ++k)
  {
    ijk[2] = k;
    for (int j = inputExtent[2]; j <= inputExtent[3]; ++j)
    {
      ijk[1] = j;
      for (int i = inputExtent[0]; i <= inputExtent[1]; ++i)
      {
        ijk[0] = i;
        double* point = inputPoints->GetPoint(
            vtkStructuredData::ComputePointIdForExtent(inputExtent.data(), ijk));
        points->SetPoint(vtkStructuredData::ComputePointIdForExtent(extent, ijk), point);
      }
    }
  }
  output->SetPoints(points);
}

//----------------------------------------------------------------------------
template <class GridDataSetT>
void UpdateOutputGridStructure(GridDataSetT* output,
    typename DataSetTypeToBlockTypeConverter<GridDataSetT>
    ::BlockType::InformationType& blockInformation)
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
 * Clone a `grid` into a `clone`. clone should have wider extents than grid.
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

  const int* cloneExtent = clone->GetExtent();
  const int* gridExtent = grid->GetExtent();

  // We use `std::max` here to work for grids of dimension 2 and 1.
  // This gives "thickness" to the degenerate dimension
  int imin = gridExtent[0];
  int imax = std::max(gridExtent[1], gridExtent[0] + 1);
  int jmin = gridExtent[2];
  int jmax = std::max(gridExtent[3], gridExtent[2] + 1);
  int kmin = gridExtent[4];
  int kmax = std::max(gridExtent[5], gridExtent[4] + 1);

  int ijk[3];

  for (ijk[2] = kmin; ijk[2] < kmax; ++ijk[2])
  {
    for (ijk[1] = jmin; ijk[1] < jmax; ++ijk[1])
    {
      for (ijk[0] = imin; ijk[0] < imax; ++ijk[0])
      {
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

  imax = gridExtent[1];
  jmax = gridExtent[3];
  kmax = gridExtent[5];

  for (ijk[2] = kmin; ijk[2] <= kmax; ++ijk[2])
  {
    for (ijk[1] = jmin; ijk[1] <= jmax; ++ijk[1])
    {
      for (ijk[0] = imin; ijk[0] <= imax; ++ijk[0])
      {
        clonePointData->SetTuple(vtkStructuredData::ComputePointIdForExtent(cloneExtent, ijk),
          vtkStructuredData::ComputePointIdForExtent(gridExtent, ijk), gridPointData);
      }
    }
  }

  clone->GetFieldData()->ShallowCopy(grid->GetFieldData());
}

//----------------------------------------------------------------------------
void EnqueuePointDataForStructuredData(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkDataSet* input, vtkIdList* pointIds)
{
  vtkNew<vtkPointData> pointData;
  vtkPointData* inputPointData = input->GetPointData();
  pointData->CopyStructure(inputPointData);
  inputPointData->GetField(pointIds, pointData);

  cp.enqueue<vtkFieldData*>(blockId, pointData);
}

//----------------------------------------------------------------------------
void EnqueueCellDataForStructuredData(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkDataSet* input, vtkIdList* pointIds)
{
  vtkNew<vtkCellData> cellData;
  vtkCellData* inputCellData = input->GetCellData();
  cellData->CopyStructure(inputCellData);
  inputCellData->GetField(pointIds, cellData);

  cp.enqueue<vtkFieldData*>(blockId, cellData);
}


//----------------------------------------------------------------------------
void EnqueuePointsForStructuredData(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkPointSet* input, vtkIdList* pointIds)
{
  vtkDataArray* inputPoints = input->GetPoints()->GetData();
  vtkSmartPointer<vtkDataArray> points(
    vtkSmartPointer<vtkDataArray>::Take(inputPoints->NewInstance()));
  points->SetNumberOfComponents(3);
  points->SetNumberOfTuples(pointIds->GetNumberOfIds());
  inputPoints->GetTuples(pointIds, points);

  cp.enqueue<vtkDataArray*>(blockId, points);
}

//----------------------------------------------------------------------------
template<class BlockStructureT>
void DequeueCellData(const diy::Master::ProxyWithLink& cp, int gid,
    BlockStructureT& blockStructure)
{
  vtkFieldData* cellData = nullptr;
  cp.dequeue<vtkFieldData*>(gid, cellData);
  blockStructure.GhostCellData = vtkSmartPointer<vtkFieldData>::Take(cellData);
}

//----------------------------------------------------------------------------
template<class BlockStructureT>
void DequeuePointData(const diy::Master::ProxyWithLink& cp, int gid,
    BlockStructureT& blockStructure)
{
  vtkFieldData* pointData = nullptr;
  cp.dequeue<vtkFieldData*>(gid, pointData);
  blockStructure.GhostPointData = vtkSmartPointer<vtkFieldData>::Take(pointData);
}

//----------------------------------------------------------------------------
template<class BlockStructureT>
void DequeuePoints(const diy::Master::ProxyWithLink& cp, int gid,
    BlockStructureT& blockStructure)
{
  vtkDataArray* points = nullptr;
  cp.dequeue<vtkDataArray*>(gid, points);
  if (points)
  {
    blockStructure.GhostPoints->SetData(points);
    points->FastDelete();
  }
}

//----------------------------------------------------------------------------
template<class GridDataSetT>
void DeepCopyInputsAndAllocateGhostsForStructuredData(const diy::Master& master,
    std::vector<GridDataSetT*>& inputs, std::vector<GridDataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  using BlockInformationType = typename BlockType::InformationType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    GridDataSetT* input = inputs[localId];
    GridDataSetT* output = outputs[localId];

    BlockInformationType& info = master.block<BlockType>(localId)->Information;
    UpdateOutputGridStructure(output, info);

    CloneGrid(input, output);
  }
}

//----------------------------------------------------------------------------
/**
 * This function fills hidden ghosts in allocated ghost layers for grid data sets.
 * This step is essential to perform before filling duplicate because there might be junctions with
 * allocated ghosts but no grid to get data from. This can happen when adjacent faces are of
 * different size.
 */
template <class GridDataSetT>
void FillHiddenGhostsForStructuredData(const diy::Master& master, std::vector<GridDataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<GridDataSetT>::BlockType;
  static constexpr unsigned char CELL_GHOST_VALUE =
    vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL
    | vtkDataSetAttributes::CellGhostTypes::HIDDENCELL;
  static constexpr unsigned char POINT_GHOST_VALUE =
    vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT
    | vtkDataSetAttributes::PointGhostTypes::HIDDENPOINT;
  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    auto& output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    vtkUnsignedCharArray* ghostCellArray = block->GhostCellArray;
    vtkUnsignedCharArray* ghostPointArray = block->GhostPointArray;

    ExtentType localExtent;
    output->GetExtent(localExtent.data());

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
      FillCellArrayForStructuredData(ghostCellArray, output, localExtent[0],
        localExtentWithNoGhosts[0], localExtent[2], localExtent[3] + isDimensionDegenerate[1],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2], CELL_GHOST_VALUE);

      FillCellArrayForStructuredData(ghostCellArray, output, localExtentWithNoGhosts[1],
        localExtent[1], localExtent[2], localExtent[3] + isDimensionDegenerate[1], localExtent[4],
        localExtent[5] + isDimensionDegenerate[2], CELL_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtent[0],
        localExtentWithNoGhosts[0] - 1, localExtent[2], localExtent[3], localExtent[4],
        localExtent[5], POINT_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtentWithNoGhosts[1] + 1,
        localExtent[1], localExtent[2], localExtent[3], localExtent[4], localExtent[5],
        POINT_GHOST_VALUE);
    }
    if (!isDimensionDegenerate[1])
    {
      FillCellArrayForStructuredData(ghostCellArray, output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2], localExtentWithNoGhosts[2],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2], CELL_GHOST_VALUE);

      FillCellArrayForStructuredData(ghostCellArray, output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtentWithNoGhosts[3], localExtent[3],
        localExtent[4], localExtent[5] + isDimensionDegenerate[2], CELL_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtent[0], localExtent[1],
        localExtent[2], localExtentWithNoGhosts[2] - 1, localExtent[4], localExtent[5],
        POINT_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtent[0], localExtent[1],
        localExtentWithNoGhosts[3] + 1, localExtent[3], localExtent[4], localExtent[5],
        POINT_GHOST_VALUE);
    }
    if (!isDimensionDegenerate[2])
    {
      FillCellArrayForStructuredData(ghostCellArray, output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2],
        localExtent[3] + isDimensionDegenerate[1], localExtent[4], localExtentWithNoGhosts[4],
        CELL_GHOST_VALUE);

      FillCellArrayForStructuredData(ghostCellArray, output, localExtent[0],
        localExtent[1] + isDimensionDegenerate[0], localExtent[2],
        localExtent[3] + isDimensionDegenerate[1], localExtentWithNoGhosts[5], localExtent[5],
        CELL_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtent[0], localExtent[1],
        localExtent[2], localExtent[3], localExtent[4], localExtentWithNoGhosts[4] - 1,
        POINT_GHOST_VALUE);

      FillPointArrayForStructuredData(ghostPointArray, output, localExtent[0], localExtent[1],
        localExtent[2], localExtent[3], localExtentWithNoGhosts[5] + 1, localExtent[5],
        POINT_GHOST_VALUE);
    }
  }
}

//----------------------------------------------------------------------------
template<class BlockT>
void FillReceivedGhostPointDataForStructuredData(BlockT* block, int gid, vtkDataSet* output,
    vtkIdList* pointIds)
{
  vtkUnsignedCharArray* ghostPointArray = block->GhostPointArray;

  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
  {
    vtkIdType pointId = pointIds->GetId(i);
    ghostPointArray->SetValue(pointId, vtkDataSetAttributes::PointGhostTypes::DUPLICATEPOINT);
  }

  vtkFieldData* pointData = block->BlockStructures.at(gid).GhostPointData;

  if (!pointData)
  {
    return;
  }

  vtkPointData* outputPointData = output->GetPointData();

  vtkNew<vtkIdList> sourcePointIds;
  sourcePointIds->SetNumberOfIds(pointData->GetNumberOfTuples());
  std::iota(sourcePointIds->begin(), sourcePointIds->end(), 0);

  for (int arrayId = 0; arrayId < pointData->GetNumberOfArrays(); ++arrayId)
  {
    vtkAbstractArray* array = pointData->GetArray(arrayId);
    if (strcmp(array->GetName(), vtkDataSetAttributes::GhostArrayName()))
    {
      vtkAbstractArray* outputArray = outputPointData->GetAbstractArray(array->GetName());
      if (outputArray)
      {
        outputArray->InsertTuples(pointIds, sourcePointIds, array);
      }
    }
  }
}

//----------------------------------------------------------------------------
template<class BlockT>
void FillReceivedGhostCellDataForStructuredData(BlockT* block, int gid, vtkDataSet* output,
    vtkIdList* cellIds)
{
  vtkUnsignedCharArray* ghostCellArray = block->GhostCellArray;

  for (vtkIdType i = 0; i < cellIds->GetNumberOfIds(); ++i)
  {
    vtkIdType cellId = cellIds->GetId(i);
    ghostCellArray->SetValue(cellId, vtkDataSetAttributes::CellGhostTypes::DUPLICATECELL);
  }

  vtkFieldData* cellData = block->BlockStructures.at(gid).GhostCellData;

  if (!cellData)
  {
    return;
  }

  vtkCellData* outputCellData = output->GetCellData();

  vtkNew<vtkIdList> sourceCellIds;
  sourceCellIds->SetNumberOfIds(cellData->GetNumberOfTuples());
  std::iota(sourceCellIds->begin(), sourceCellIds->end(), 0);

  for (int arrayId = 0; arrayId < cellData->GetNumberOfArrays(); ++arrayId)
  {
    vtkAbstractArray* array = cellData->GetArray(arrayId);
    if (strcmp(array->GetName(), vtkDataSetAttributes::GhostArrayName()))
    {
      vtkAbstractArray* outputArray = outputCellData->GetAbstractArray(array->GetName());
      if (outputArray)
      {
        outputArray->InsertTuples(cellIds, sourceCellIds, array);
      }
    }
  }
}

//----------------------------------------------------------------------------
template<class BlockT>
void FillReceivedGhostPointsForStructuredData(BlockT* block, int gid, vtkPointSet* output,
    vtkIdList* pointIds)
{
  vtkPoints* points = block->BlockStructures.at(gid).GhostPoints;
  if (!points)
  {
    return;
  }

  vtkPoints* outputPoints = output->GetPoints();

  for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i)
  {
    vtkIdType pointId = pointIds->GetId(i);
    outputPoints->SetPoint(pointId, points->GetPoint(i));
  }
}

//----------------------------------------------------------------------------
void FillReceivedGhosts(ImageDataBlock* block, int gid, vtkImageData* output)
{
  vtkSmartPointer<vtkIdList> pointIds =
    ComputeOutputInterfacePointIdsForStructuredData(block, gid, output);
  FillReceivedGhostPointDataForStructuredData(block, gid, output, pointIds);

  vtkSmartPointer<vtkIdList> cellIds =
    ComputeOutputInterfaceCellIdsForStructuredData(block, gid, output);
  FillReceivedGhostCellDataForStructuredData(block, gid, output, cellIds);
}

//----------------------------------------------------------------------------
void FillReceivedGhosts(RectilinearGridBlock* block, int gid, vtkRectilinearGrid* output)
{
  vtkSmartPointer<vtkIdList> pointIds =
    ComputeOutputInterfacePointIdsForStructuredData(block, gid, output);
  FillReceivedGhostPointDataForStructuredData(block, gid, output, pointIds);

  vtkSmartPointer<vtkIdList> cellIds =
    ComputeOutputInterfaceCellIdsForStructuredData(block, gid, output);
  FillReceivedGhostCellDataForStructuredData(block, gid, output, cellIds);
}

//----------------------------------------------------------------------------
void FillReceivedGhosts(StructuredGridBlock* block, int gid, vtkStructuredGrid* output)
{
  vtkSmartPointer<vtkIdList> pointIds =
    ComputeOutputInterfacePointIdsForStructuredData(block, gid, output);
  FillReceivedGhostPointDataForStructuredData(block, gid, output, pointIds);
  FillReceivedGhostPointsForStructuredData(block, gid, output, pointIds);

  vtkSmartPointer<vtkIdList> cellIds =
    ComputeOutputInterfaceCellIdsForStructuredData(block, gid, output);
  FillReceivedGhostCellDataForStructuredData(block, gid, output, cellIds);
}



//----------------------------------------------------------------------------
template <class DataSetT>
void FillReceivedGhosts(const diy::Master& master, std::vector<DataSetT*>& outputs)
{
  using BlockType = typename DataSetTypeToBlockTypeConverter<DataSetT>::BlockType;

  for (int localId = 0; localId < static_cast<int>(outputs.size()); ++localId)
  {
    DataSetT* output = outputs[localId];
    BlockType* block = master.block<BlockType>(localId);

    for (auto& item : block->BlockStructures)
    {
      FillReceivedGhosts(block, item.first, output);
    }
  }
}

//----------------------------------------------------------------------------
void CopyOuterLayerGridPoints(vtkStructuredGrid* input, vtkSmartPointer<vtkPoints>& outputPoints,
    ExtentType extent, int i)
{
  int j = (i + 2) % 6;
  j -= j % 2;
  int k = (i + 4) % 6;
  k -= k % 2;

  vtkPoints* inputPoints = input->GetPoints();
  int* inputExtent = input->GetExtent();

  outputPoints = vtkSmartPointer<vtkPoints>::New();
  outputPoints->SetDataType(inputPoints->GetDataType());
  outputPoints->SetNumberOfPoints(
      (extent[j + 1] - extent[j] + 1) * (extent[k + 1] - extent[k] + 1));

  // We collapse one dimension
  extent[i + (i % 2 ? -1 : 1)] = extent[i];

  int ijk[3];
  ijk[i / 2] = extent[i];
  for (int y = extent[k]; y <= extent[k + 1]; ++y)
  {
    ijk[k / 2] = y;
    for (int x = extent[j]; x <= extent[j + 1]; ++x)
    {
      ijk[j / 2] = x;
      outputPoints->SetPoint(vtkStructuredData::ComputePointIdForExtent(extent.data(), ijk),
          inputPoints->GetPoint(vtkStructuredData::ComputePointIdForExtent(inputExtent, ijk)));
    }
  }
}
} // anonymous namespace

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::GridBlockStructure::GridBlockStructure(const int* extent, int dim)
  : Extent{ extent[0], extent[1], extent[2], extent[3], extent[4], extent[5] }
  , DataDimension(dim)
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(const int extent[6],
  int dim, const double origin[3], const double spacing[3], const double orientationQuaternion[4])
  : GridBlockStructure(extent, dim)
  , Origin{ origin[0], origin[1], origin[2] }
  , Spacing{ spacing[0], spacing[1], spacing[2] }
  , OrientationQuaternion{ orientationQuaternion[0], orientationQuaternion[1],
      orientationQuaternion[2], orientationQuaternion[3] }
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(const int extent[6],
  int dim, const double origin[3], const double spacing[3], vtkMatrix3x3* directionMatrix)
  : GridBlockStructure(extent, dim)
  , Origin{ origin[0], origin[1], origin[2] }
  , Spacing{ spacing[0], spacing[1], spacing[2] }
{
  vtkMath::Matrix3x3ToQuaternion(directionMatrix->GetData(), OrientationQuaternion.GetData());
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::ImageDataBlockStructure::ImageDataBlockStructure(
  vtkImageData* image, const ImageDataInformation& information)
  : ImageDataBlockStructure(information.Extent.data(), image->GetDataDimension(),
      image->GetOrigin(), image->GetSpacing(), image->GetDirectionMatrix())
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::RectilinearGridBlockStructure::RectilinearGridBlockStructure(
  const int extent[6], int dim, vtkDataArray* xCoordinates, vtkDataArray* yCoordinates,
  vtkDataArray* zCoordinates)
  : GridBlockStructure(extent, dim)
  , XCoordinates(vtkSmartPointer<vtkDataArray>::Take(xCoordinates))
  , YCoordinates(vtkSmartPointer<vtkDataArray>::Take(yCoordinates))
  , ZCoordinates(vtkSmartPointer<vtkDataArray>::Take(zCoordinates))
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::RectilinearGridBlockStructure::RectilinearGridBlockStructure(
  vtkRectilinearGrid* grid, const RectilinearGridInformation& information)
  : GridBlockStructure(information.Extent.data(), grid->GetDataDimension())
  , XCoordinates(information.XCoordinates)
  , YCoordinates(information.YCoordinates)
  , ZCoordinates(information.ZCoordinates)
{
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::StructuredGridBlockStructure::StructuredGridBlockStructure(
    const int extent[6], int dim, vtkDataArray* points[6])
  : GridBlockStructure(extent, dim)
{
  for (int i = 0; i < 6; ++i)
  {
    this->OuterPointLayers[i] = vtkSmartPointer<vtkPoints>::New();
    this->OuterPointLayers[i]->SetData(points[i]);
    points[i]->FastDelete();
  }
}

//----------------------------------------------------------------------------
vtkDIYGhostUtilities::StructuredGridBlockStructure::StructuredGridBlockStructure(
    vtkStructuredGrid* grid, const StructuredGridInformation& info)
  : GridBlockStructure(info.Extent.data(), grid->GetDataDimension())
  , OuterPointLayers{ info.OuterPointLayers[0].Points, info.OuterPointLayers[1].Points,
      info.OuterPointLayers[2].Points, info.OuterPointLayers[3].Points,
      info.OuterPointLayers[4].Points, info.OuterPointLayers[5].Points }
{
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::SetupBlockSelfInformation(diy::Master& vtkNotUsed(master),
    std::vector<vtkImageData*>& vtkNotUsed(inputs))
{
  // Do nothing, there is no extra information needed from input for vtkImageData.
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::SetupBlockSelfInformation(diy::Master& vtkNotUsed(master),
    std::vector<vtkRectilinearGrid*>& vtkNotUsed(inputs))
{
  // Do nothing, there is no extra information needed from input for vtkRectilinearGrid.
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::SetupBlockSelfInformation(diy::Master& master,
    std::vector<vtkStructuredGrid*>& inputs)
{
  using BlockType = StructuredGridBlock;
  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    vtkStructuredGrid* input = inputs[localId];
    BlockType* block = master.block<BlockType>(localId);
    typename BlockType::InformationType& information = block->Information;
    information.InputPoints = input->GetPoints();
  }
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::ExchangeBlockStructures(diy::Master& master,
  std::vector<vtkImageData*>& inputs)
{
  using BlockType = ImageDataBlock;

  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    BlockType* block = master.block<BlockType>(localId);
    block->Information.Extent = PeelOffGhostLayers(inputs[localId]);
  }

  master.foreach ([&master, &inputs](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    int myBlockId = cp.gid();
    int localId = master.lid(myBlockId);
    auto& input = inputs[localId];

    const ExtentType& extent = block->Information.Extent;
    double* origin = input->GetOrigin();
    double* spacing = input->GetSpacing();
    int dimension = input->GetDataDimension();
    QuaternionType q;
    vtkMath::Matrix3x3ToQuaternion(input->GetDirectionMatrix()->GetData(), q.GetData());
    double* qBuffer = q.GetData();
    for (int id = 0; id < static_cast<int>(cp.link()->size()); ++id)
    {
      const diy::BlockID& blockId = cp.link()->target(id);
      cp.enqueue(blockId, &dimension, 1);
      cp.enqueue(blockId, origin, 3);
      cp.enqueue(blockId, spacing, 3);
      cp.enqueue(blockId, qBuffer, 4);
      cp.enqueue(blockId, extent.data(), 6);
    }
  });

  master.exchange();

  master.foreach ([](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    std::vector<int> incoming;
    cp.incoming(incoming);

    int dimension;
    int extent[6];
    double origin[3];
    double spacing[3];
    double q[4];

    for (const int& gid : incoming)
    {
      // we need this extra check because incoming is not empty when using only one block
      if (!cp.incoming(gid).empty())
      {
        cp.dequeue(gid, &dimension, 1);
        cp.dequeue(gid, origin, 3);
        cp.dequeue(gid, spacing, 3);
        cp.dequeue(gid, q, 4);
        cp.dequeue(gid, extent, 6);

        block->BlockStructures.emplace(gid,
            ImageDataBlockStructure(extent, dimension, origin, spacing, q));
      }
    }
  });
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::ExchangeBlockStructures(diy::Master& master,
  std::vector<vtkRectilinearGrid*>& inputs)
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
    extent = PeelOffGhostLayers(input);

    vtkDataArray* inputXCoordinates = input->GetXCoordinates();
    vtkDataArray* inputYCoordinates = input->GetYCoordinates();
    vtkDataArray* inputZCoordinates = input->GetZCoordinates();

    info.XCoordinates.TakeReference(inputXCoordinates->NewInstance());
    info.YCoordinates.TakeReference(inputYCoordinates->NewInstance());
    info.ZCoordinates.TakeReference(inputZCoordinates->NewInstance());

    info.XCoordinates->InsertTuples(
      0, extent[1] - extent[0] + 1, extent[0] - inputExtent[0], inputXCoordinates);

    info.YCoordinates->InsertTuples(
      0, extent[3] - extent[2] + 1, extent[2] - inputExtent[2], inputYCoordinates);
    info.ZCoordinates->InsertTuples(
      0, extent[5] - extent[4] + 1, extent[4] - inputExtent[4], inputZCoordinates);
  }

  master.foreach ([&master, &inputs](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    int myBlockId = cp.gid();
    int localId = master.lid(myBlockId);
    auto& input = inputs[localId];

    auto& info = block->Information;
    int dimension = input->GetDataDimension();
    const ExtentType& extent = info.Extent;
    vtkDataArray* xCoordinates = info.XCoordinates;
    vtkDataArray* yCoordinates = info.YCoordinates;
    vtkDataArray* zCoordinates = info.ZCoordinates;

    for (int id = 0; id < static_cast<int>(cp.link()->size()); ++id)
    {
      const diy::BlockID& blockId = cp.link()->target(id);
      cp.enqueue(blockId, &dimension, 1);
      cp.enqueue(blockId, extent.data(), 6);
      cp.enqueue<vtkDataArray*>(blockId, xCoordinates);
      cp.enqueue<vtkDataArray*>(blockId, yCoordinates);
      cp.enqueue<vtkDataArray*>(blockId, zCoordinates);
    }
  });

  master.exchange();

  master.foreach ([](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    std::vector<int> incoming;
    cp.incoming(incoming);

    int dimension;
    int extent[6];

    for (const int& gid : incoming)
    {
      // we need this extra check because incoming is not empty when using only one block
      if (!cp.incoming(gid).empty())
      {
        vtkDataArray* xCoordinates = nullptr;
        vtkDataArray* yCoordinates = nullptr;
        vtkDataArray* zCoordinates = nullptr;

        cp.dequeue(gid, &dimension, 1);
        cp.dequeue(gid, extent, 6);
        cp.dequeue(gid, xCoordinates);
        cp.dequeue(gid, yCoordinates);
        cp.dequeue(gid, zCoordinates);

        block->BlockStructures.emplace(gid,
            RectilinearGridBlockStructure(extent, dimension, xCoordinates, yCoordinates,
            zCoordinates));
      }
    }
  });
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::ExchangeBlockStructures(diy::Master& master,
    std::vector<vtkStructuredGrid*>& inputs)
{
  using BlockType = StructuredGridBlock;

  // In addition to the extent, we need to share the points lying on the 6 external faces of each
  // structured grid. These points will be used to determine if structured grids are connected or
  // not.

  for (int localId = 0; localId < static_cast<int>(inputs.size()); ++localId)
  {
    vtkStructuredGrid* input = inputs[localId];
    int* inputExtent = input->GetExtent();
    if (!IsExtentValid(inputExtent))
    {
      continue;
    }
    BlockType* block = master.block<BlockType>(localId);
    StructuredGridInformation& info = block->Information;
    ExtentType& extent = info.Extent;
    extent = PeelOffGhostLayers(input);

    for (int i = 0; i < 6; ++i)
    {
      CopyOuterLayerGridPoints(input, info.OuterPointLayers[i].Points, extent, i);
    }
  }

  master.foreach ([&master, &inputs](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    int myBlockId = cp.gid();
    int localId = master.lid(myBlockId);
    auto& input = inputs[localId];

    auto& info = block->Information;
    int dimension = input->GetDataDimension();
    const ExtentType& extent = info.Extent;

    for (int id = 0; id < static_cast<int>(cp.link()->size()); ++id)
    {
      const diy::BlockID& blockId = cp.link()->target(id);
      cp.enqueue(blockId, &dimension, 1);
      cp.enqueue(blockId, extent.data(), 6);
      for (int extentId = 0; extentId < 6; ++extentId)
      {
        cp.enqueue<vtkDataArray*>(blockId, info.OuterPointLayers[extentId].Points->GetData());
      }
    }
  });

  master.exchange();

  master.foreach ([](BlockType* block, const diy::Master::ProxyWithLink& cp) {
    std::vector<int> incoming;
    cp.incoming(incoming);

    int dimension;
    int extent[6];

    for (const int& gid : incoming)
    {
      // we need this extra check because incoming is not empty when using only one block
      if (!cp.incoming(gid).empty())
      {
        vtkDataArray* points[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

        cp.dequeue(gid, &dimension, 1);
        cp.dequeue(gid, extent, 6);
        for (int extentId = 0; extentId < 6; ++extentId)
        {
          vtkDataArray* tmp = points[extentId];
          cp.dequeue<vtkDataArray*>(gid, tmp);
          points[extentId] = tmp;
        }

        block->BlockStructures.emplace(gid,
            StructuredGridBlockStructure(extent, dimension, points));
      }
    }
  });
}

//----------------------------------------------------------------------------
LinkMap vtkDIYGhostUtilities::ComputeLinkMap(
  const diy::Master& master, std::vector<vtkImageData*>& inputs,
  std::vector<vtkImageData*>& outputs, int outputGhostLevels)
{
  return ComputeLinkMapForStructuredData(master, inputs, outputs, outputGhostLevels);
}

//----------------------------------------------------------------------------
LinkMap vtkDIYGhostUtilities::ComputeLinkMap(
  const diy::Master& master, std::vector<vtkRectilinearGrid*>& inputs,
  std::vector<vtkRectilinearGrid*>& outputs, int outputGhostLevels)
{
  return ComputeLinkMapForStructuredData(master, inputs, outputs, outputGhostLevels);
}

//----------------------------------------------------------------------------
LinkMap vtkDIYGhostUtilities::ComputeLinkMap(
    const diy::Master& master, std::vector<vtkStructuredGrid*>& inputs,
    std::vector<vtkStructuredGrid*>& outputs, int outputGhostLevels)
{
  return ComputeLinkMapForStructuredData(master, inputs, outputs,
      outputGhostLevels);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DeepCopyInputsAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkImageData*>& inputs, std::vector<vtkImageData*>& outputs)
{
  DeepCopyInputsAndAllocateGhostsForStructuredData(master, inputs, outputs);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DeepCopyInputsAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkRectilinearGrid*>& inputs, std::vector<vtkRectilinearGrid*>& outputs)
{
  DeepCopyInputsAndAllocateGhostsForStructuredData(master, inputs, outputs);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DeepCopyInputsAndAllocateGhosts(const diy::Master& master,
    std::vector<vtkStructuredGrid*>& inputs, std::vector<vtkStructuredGrid*>& outputs)
{
  DeepCopyInputsAndAllocateGhostsForStructuredData(master, inputs, outputs);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::EnqueueGhosts(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkImageData* input, ImageDataBlock* block)
{
  vtkSmartPointer<vtkIdList> cellIds = ComputeInputInterfaceCellIdsForStructuredData(
      block, blockId.gid, input);
  EnqueueCellDataForStructuredData(cp, blockId, input, cellIds);

  vtkSmartPointer<vtkIdList> pointIds = ComputeInputInterfacePointIdsForStructuredData(
      block, blockId.gid, input);
  EnqueuePointDataForStructuredData(cp, blockId, input, pointIds);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::EnqueueGhosts(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkRectilinearGrid* input, RectilinearGridBlock* block)
{
  vtkSmartPointer<vtkIdList> cellIds = ComputeInputInterfaceCellIdsForStructuredData(
      block, blockId.gid, input);
  EnqueueCellDataForStructuredData(cp, blockId, input, cellIds);

  vtkSmartPointer<vtkIdList> pointIds = ComputeInputInterfacePointIdsForStructuredData(
      block, blockId.gid, input);
  EnqueuePointDataForStructuredData(cp, blockId, input, pointIds);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::EnqueueGhosts(const diy::Master::ProxyWithLink& cp,
    const diy::BlockID& blockId, vtkStructuredGrid* input, StructuredGridBlock* block)
{
  vtkSmartPointer<vtkIdList> cellIds = ComputeInputInterfaceCellIdsForStructuredData(
      block, blockId.gid, input);
  EnqueueCellDataForStructuredData(cp, blockId, input, cellIds);

  vtkSmartPointer<vtkIdList> pointIds = ComputeInputInterfacePointIdsForStructuredData(
      block, blockId.gid, input);
  EnqueuePointDataForStructuredData(cp, blockId, input, pointIds);
  EnqueuePointsForStructuredData(cp, blockId, input, pointIds);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DequeueGhosts(const diy::Master::ProxyWithLink& cp, int gid,
    ImageDataBlockStructure& blockStructure)
{
  DequeueCellData(cp, gid, blockStructure);
  DequeuePointData(cp, gid, blockStructure);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DequeueGhosts(const diy::Master::ProxyWithLink& cp, int gid,
    RectilinearGridBlockStructure& blockStructure)
{
  DequeueCellData(cp, gid, blockStructure);
  DequeuePointData(cp, gid, blockStructure);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::DequeueGhosts(const diy::Master::ProxyWithLink& cp, int gid,
    StructuredGridBlockStructure& blockStructure)
{
  DequeueCellData(cp, gid, blockStructure);
  DequeuePointData(cp, gid, blockStructure);
  DequeuePoints(cp, gid, blockStructure);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::FillGhostArrays(const diy::Master& master,
  std::vector<vtkImageData*>& outputs)
{
  FillHiddenGhostsForStructuredData(master, outputs);
  FillReceivedGhosts(master, outputs);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::FillGhostArrays(const diy::Master& master,
  std::vector<vtkRectilinearGrid*>& outputs)
{
  FillHiddenGhostsForStructuredData(master, outputs);
  FillReceivedGhosts(master, outputs);
}

//----------------------------------------------------------------------------
void vtkDIYGhostUtilities::FillGhostArrays(const diy::Master& master,
    std::vector<vtkStructuredGrid*>& outputs)
{
  FillHiddenGhostsForStructuredData(master, outputs);
  FillReceivedGhosts(master, outputs);
}
