//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/CellLocatorTwoLevel.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleTransform.h>

#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

using namespace viskores::internal::cl_uniform_bins;

namespace
{

struct BinsBBox
{
  DimVec3 Min;
  DimVec3 Max;

  VISKORES_EXEC
  bool Empty() const
  {
    return (this->Max[0] < this->Min[0]) || (this->Max[1] < this->Min[1]) ||
      (this->Max[2] < this->Min[2]);
  }
};

VISKORES_EXEC_CONT static DimVec3 ComputeGridDimension(viskores::Id numberOfCells,
                                                       const FloatVec3& size,
                                                       viskores::FloatDefault density)
{
  viskores::FloatDefault nsides = 0.0f;
  viskores::FloatDefault volume = 1.0f;
  viskores::FloatDefault maxside = viskores::Max(size[0], viskores::Max(size[1], size[2]));
  for (int i = 0; i < 3; ++i)
  {
    if (size[i] / maxside >= 1e-4f)
    {
      nsides += 1.0f;
      volume *= size[i];
    }
  }

  auto r = viskores::Pow((static_cast<viskores::FloatDefault>(numberOfCells) / (volume * density)),
                         1.0f / nsides);
  return viskores::Max(DimVec3(1),
                       DimVec3(static_cast<DimensionType>(size[0] * r),
                               static_cast<DimensionType>(size[1] * r),
                               static_cast<DimensionType>(size[2] * r)));
}

VISKORES_EXEC static BinsBBox ComputeIntersectingBins(const Bounds cellBounds, const Grid& grid)
{
  auto minb = static_cast<DimVec3>((cellBounds.Min - grid.Origin) / grid.BinSize);
  auto maxb = static_cast<DimVec3>((cellBounds.Max - grid.Origin) / grid.BinSize);

  return { viskores::Max(DimVec3(0), minb), viskores::Min(grid.Dimensions - DimVec3(1), maxb) };
}

VISKORES_EXEC static viskores::Id GetNumberOfBins(const BinsBBox& binsBBox)
{
  return binsBBox.Empty()
    ? 0
    : ((binsBBox.Max[0] - binsBBox.Min[0] + 1) * (binsBBox.Max[1] - binsBBox.Min[1] + 1) *
       (binsBBox.Max[2] - binsBBox.Min[2] + 1));
}

class BBoxIterator
{
public:
  VISKORES_EXEC_CONT BBoxIterator(const BinsBBox& bbox, const DimVec3& dim)
    : BBox(bbox)
    , Dim(dim)
    , StepY(dim[0] - (bbox.Max[0] - bbox.Min[0] + 1))
    , StepZ((dim[0] * dim[1]) - ((bbox.Max[1] - bbox.Min[1] + 1) * dim[0]))
  {
    this->Init();
  }

  VISKORES_EXEC_CONT void Init()
  {
    this->Idx = this->BBox.Min;
    this->FlatIdx = ComputeFlatIndex(this->Idx, this->Dim);
    this->DoneFlag = this->BBox.Empty();
  }

  VISKORES_EXEC_CONT bool Done() const { return this->DoneFlag; }

  VISKORES_EXEC_CONT void Next()
  {
    if (!this->DoneFlag)
    {
      ++this->Idx[0];
      this->FlatIdx += 1;
      if (this->Idx[0] > this->BBox.Max[0])
      {
        this->Idx[0] = this->BBox.Min[0];
        ++this->Idx[1];
        this->FlatIdx += this->StepY;
        if (this->Idx[1] > this->BBox.Max[1])
        {
          this->Idx[1] = this->BBox.Min[1];
          ++this->Idx[2];
          this->FlatIdx += this->StepZ;
          if (this->Idx[2] > this->BBox.Max[2])
          {
            this->DoneFlag = true;
          }
        }
      }
    }
  }

  VISKORES_EXEC_CONT const DimVec3& GetIdx() const { return this->Idx; }

  VISKORES_EXEC_CONT viskores::Id GetFlatIdx() const { return this->FlatIdx; }

private:
  const BinsBBox BBox;
  const DimVec3 Dim;
  const viskores::Id StepY, StepZ;

  DimVec3 Idx;
  viskores::Id FlatIdx;
  bool DoneFlag;
};

class CountBinsL1 : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint coords, FieldOutCell bincount);
  using ExecutionSignature = void(_2, _3);

  CountBinsL1(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType>
  VISKORES_EXEC void operator()(const PointsVecType& points, viskores::Id& numBins) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);
    numBins = GetNumberOfBins(binsBBox);
  }

private:
  Grid L1Grid;
};

class FindBinsL1 : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInCell offsets,
                                WholeArrayOut binIds);
  using ExecutionSignature = void(_2, _3, _4);

  FindBinsL1(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType, typename BinIdsPortalType>
  VISKORES_EXEC void operator()(const PointsVecType& points,
                                viskores::Id offset,
                                BinIdsPortalType& binIds) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      binIds.Set(offset, i.GetFlatIdx());
      ++offset;
    }
  }

private:
  Grid L1Grid;
};

class GenerateBinsL1 : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn binIds, FieldIn cellCounts, WholeArrayOut dimensions);
  using ExecutionSignature = void(_1, _2, _3);

  using InputDomain = _1;

  GenerateBinsL1(FloatVec3 size, viskores::FloatDefault density)
    : Size(size)
    , Density(density)
  {
  }

  template <typename OutputDimensionsPortal>
  VISKORES_EXEC void operator()(viskores::Id binId,
                                viskores::Id numCells,
                                OutputDimensionsPortal& dimensions) const
  {
    dimensions.Set(binId, ComputeGridDimension(numCells, this->Size, this->Density));
  }

private:
  FloatVec3 Size;
  viskores::FloatDefault Density;
};

class CountBinsL2 : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                WholeArrayIn binDimensions,
                                FieldOutCell bincount);
  using ExecutionSignature = void(_2, _3, _4);

  CountBinsL2(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType, typename BinDimensionsPortalType>
  VISKORES_EXEC void operator()(const PointsVecType& points,
                                const BinDimensionsPortalType& binDimensions,
                                viskores::Id& numBins) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    numBins = 0;
    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      Grid leaf = ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
      auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
      numBins += GetNumberOfBins(binsBBoxL2);
    }
  }

private:
  Grid L1Grid;
};

class FindBinsL2 : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                WholeArrayIn binDimensions,
                                WholeArrayIn binStarts,
                                FieldInCell offsets,
                                WholeArrayOut binIds,
                                WholeArrayOut cellIds);
  using ExecutionSignature = void(InputIndex, _2, _3, _4, _5, _6, _7);

  FindBinsL2(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType,
            typename BinDimensionsPortalType,
            typename BinStartsPortalType,
            typename BinIdsPortalType,
            typename CellIdsPortalType>
  VISKORES_EXEC void operator()(viskores::Id cellId,
                                const PointsVecType& points,
                                const BinDimensionsPortalType& binDimensions,
                                const BinStartsPortalType& binStarts,
                                viskores::Id offset,
                                BinIdsPortalType binIds,
                                CellIdsPortalType cellIds) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      Grid leaf = ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
      auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
      viskores::Id leafStart = binStarts.Get(i.GetFlatIdx());

      for (BBoxIterator j(binsBBoxL2, leaf.Dimensions); !j.Done(); j.Next())
      {
        binIds.Set(offset, leafStart + j.GetFlatIdx());
        cellIds.Set(offset, cellId);
        ++offset;
      }
    }
  }

private:
  Grid L1Grid;
};

class GenerateBinsL2 : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn binIds,
                                FieldIn startsIn,
                                FieldIn countsIn,
                                WholeArrayOut startsOut,
                                WholeArrayOut countsOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  using InputDomain = _1;

  template <typename CellStartsPortalType, typename CellCountsPortalType>
  VISKORES_EXEC void operator()(viskores::Id binIndex,
                                viskores::Id start,
                                viskores::Id count,
                                CellStartsPortalType& cellStarts,
                                CellCountsPortalType& cellCounts) const
  {
    cellStarts.Set(binIndex, start);
    cellCounts.Set(binIndex, count);
  }
};

struct DimensionsToCount
{
  VISKORES_EXEC viskores::Id operator()(const DimVec3& dim) const
  {
    return dim[0] * dim[1] * dim[2];
  }
};

} // anonymous namespace

namespace viskores
{
namespace cont
{

//----------------------------------------------------------------------------
/// Builds the cell locator lookup structure
///
VISKORES_CONT void CellLocatorTwoLevel::Build()
{
  VISKORES_LOG_SCOPE(viskores::cont::LogLevel::Perf, "CellLocatorTwoLevel::Build");

  viskores::cont::Invoker invoke;

  auto cellset = this->GetCellSet();
  const auto& coords = this->GetCoordinates();

  // 1: Compute the top level grid
  auto bounds = coords.GetBounds();
  FloatVec3 bmin(static_cast<viskores::FloatDefault>(bounds.X.Min),
                 static_cast<viskores::FloatDefault>(bounds.Y.Min),
                 static_cast<viskores::FloatDefault>(bounds.Z.Min));
  FloatVec3 bmax(static_cast<viskores::FloatDefault>(bounds.X.Max),
                 static_cast<viskores::FloatDefault>(bounds.Y.Max),
                 static_cast<viskores::FloatDefault>(bounds.Z.Max));
  auto size = bmax - bmin;
  auto fudge = viskores::Max(FloatVec3(1e-6f), size * 1e-4f);
  size += 2.0f * fudge;

  this->TopLevel.Dimensions =
    ComputeGridDimension(cellset.GetNumberOfCells(), size, this->DensityL1);
  this->TopLevel.Origin = bmin - fudge;
  this->TopLevel.BinSize = size / static_cast<FloatVec3>(this->TopLevel.Dimensions);

  // 2: For each cell, find the number of top level bins they intersect
  viskores::cont::ArrayHandle<viskores::Id> binCounts;
  CountBinsL1 countL1(this->TopLevel);
  invoke(countL1, cellset, coords, binCounts);

  // 3: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
  viskores::Id numPairsL1 = viskores::cont::Algorithm::ScanExclusive(binCounts, binCounts);

  // 4: For each cell find the top level bins that intersect it
  viskores::cont::ArrayHandle<viskores::Id> binIds;
  binIds.Allocate(numPairsL1);
  FindBinsL1 findL1(this->TopLevel);
  invoke(findL1, cellset, coords, binCounts, binIds);
  binCounts.ReleaseResources();

  // 5: From above, find the number of cells that intersect each top level bin
  viskores::cont::Algorithm::Sort(binIds);
  viskores::cont::ArrayHandle<viskores::Id> bins;
  viskores::cont::ArrayHandle<viskores::Id> cellsPerBin;
  viskores::cont::Algorithm::ReduceByKey(
    binIds,
    viskores::cont::make_ArrayHandleConstant(viskores::Id(1), numPairsL1),
    bins,
    cellsPerBin,
    viskores::Sum());
  binIds.ReleaseResources();

  // 6: Compute level-2 dimensions
  viskores::Id numberOfBins =
    this->TopLevel.Dimensions[0] * this->TopLevel.Dimensions[1] * this->TopLevel.Dimensions[2];
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleConstant(DimVec3(0), numberOfBins),
                            this->LeafDimensions);
  GenerateBinsL1 generateL1(this->TopLevel.BinSize, this->DensityL2);
  invoke(generateL1, bins, cellsPerBin, this->LeafDimensions);
  bins.ReleaseResources();
  cellsPerBin.ReleaseResources();

  // 7: Compute number of level-2 bins
  viskores::Id numberOfLeaves = viskores::cont::Algorithm::ScanExclusive(
    viskores::cont::make_ArrayHandleTransform(this->LeafDimensions, DimensionsToCount()),
    this->LeafStartIndex);


  // 8: For each cell, find the number of l2 bins they intersect
  CountBinsL2 countL2(this->TopLevel);
  invoke(countL2, cellset, coords, this->LeafDimensions, binCounts);

  // 9: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
  viskores::Id numPairsL2 = viskores::cont::Algorithm::ScanExclusive(binCounts, binCounts);

  // 10: For each cell, find the l2 bins they intersect
  binIds.Allocate(numPairsL2);
  this->CellIds.Allocate(numPairsL2);
  FindBinsL2 findL2(this->TopLevel);
  invoke(findL2,
         cellset,
         coords,
         this->LeafDimensions,
         this->LeafStartIndex,
         binCounts,
         binIds,
         this->CellIds);
  binCounts.ReleaseResources();

  // 11: From above, find the cells that each l2 bin intersects
  viskores::cont::Algorithm::SortByKey(binIds, this->CellIds);
  viskores::cont::Algorithm::ReduceByKey(
    binIds,
    viskores::cont::make_ArrayHandleConstant(viskores::Id(1), numPairsL2),
    bins,
    cellsPerBin,
    viskores::Sum());
  binIds.ReleaseResources();

  // 12: Generate the leaf bin arrays
  viskores::cont::ArrayHandle<viskores::Id> cellsStart;
  viskores::cont::Algorithm::ScanExclusive(cellsPerBin, cellsStart);

  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, numberOfLeaves),
                            this->CellStartIndex);
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Id>(0, numberOfLeaves),
                            this->CellCount);
  invoke(GenerateBinsL2{}, bins, cellsStart, cellsPerBin, this->CellStartIndex, this->CellCount);
}

//----------------------------------------------------------------------------
struct CellLocatorTwoLevel::MakeExecObject
{
  template <typename CellSetType>
  VISKORES_CONT void operator()(const CellSetType& cellSet,
                                viskores::cont::DeviceAdapterId device,
                                viskores::cont::Token& token,
                                const CellLocatorTwoLevel& self,
                                ExecObjType& execObject) const
  {
    using CellStructuredType = CellSetContToExec<CellSetType>;
    execObject = viskores::exec::CellLocatorTwoLevel<CellStructuredType>(self.TopLevel,
                                                                         self.LeafDimensions,
                                                                         self.LeafStartIndex,
                                                                         self.CellStartIndex,
                                                                         self.CellCount,
                                                                         self.CellIds,
                                                                         cellSet,
                                                                         self.GetCoordinates(),
                                                                         device,
                                                                         token);
  }
};

CellLocatorTwoLevel::ExecObjType CellLocatorTwoLevel::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  this->Update();
  ExecObjType execObject;
  viskores::cont::CastAndCall(
    this->GetCellSet(), MakeExecObject{}, device, token, *this, execObject);
  return execObject;
}

//----------------------------------------------------------------------------
void CellLocatorTwoLevel::PrintSummary(std::ostream& out) const
{
  out << "DensityL1: " << this->DensityL1 << "\n";
  out << "DensityL2: " << this->DensityL2 << "\n";
  out << "Input CellSet: \n";
  this->GetCellSet().PrintSummary(out);
  out << "Input Coordinates: \n";
  this->GetCoordinates().PrintSummary(out);
  out << "LookupStructure:\n";
  out << "  TopLevelGrid\n";
  out << "    Dimensions: " << this->TopLevel.Dimensions << "\n";
  out << "    Origin: " << this->TopLevel.Origin << "\n";
  out << "    BinSize: " << this->TopLevel.BinSize << "\n";
  out << "  LeafDimensions:\n";
  viskores::cont::printSummary_ArrayHandle(this->LeafDimensions, out);
  out << "  LeafStartIndex:\n";
  viskores::cont::printSummary_ArrayHandle(this->LeafStartIndex, out);
  out << "  CellStartIndex:\n";
  viskores::cont::printSummary_ArrayHandle(this->CellStartIndex, out);
  out << "  CellCount:\n";
  viskores::cont::printSummary_ArrayHandle(this->CellCount, out);
  out << "  CellIds:\n";
  viskores::cont::printSummary_ArrayHandle(this->CellIds, out);
}
}
} // viskores::cont
