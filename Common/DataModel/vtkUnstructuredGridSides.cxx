// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#define VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATING
#include "vtkUnstructuredGridSides.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCellType.h"
#include "vtkCellTypeSides.h"
#include "vtkCellTypeUtilities.h"
#include "vtkConstantArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocal.h"
#include "vtkSMPTools.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
// The sides of the cells: a side is a face, an edge or a vertex of a cell, given by its points,
// and by the cell, local index and dimension that are written back if it is kept. The sides are
// scattered by their key, their smallest point id, then the sides of every key are matched, see
// SideMatcher. BuildSides() reads the cell arrays of the input in a single dispatch, see
// BuildSidesWorker. The structs that read the settings are nested in vtkUnstructuredGridSides, so
// that they read its members directly, the others are in anonymous namespaces. Every struct and
// function is defined before its first use.
namespace // anonymous
{
using SideFlags = vtkCellGridSidesQuery::SideFlags;
using SummaryStrategy = vtkCellGridSidesQuery::SummaryStrategy;

//------------------------------------------------------------------------------
// The cell types of a vtkUnstructuredGrid: an array of unsigned char, or a single type, see
// InputCells.
struct CellTypes
{
  const unsigned char* Values = nullptr; // nullptr for cells of a single type
  unsigned char Constant = 0;

  unsigned char GetCellType(vtkIdType cellId) const
  {
    return this->Values ? this->Values[cellId] : this->Constant;
  }
};

//------------------------------------------------------------------------------
// The state of a cell, see InputCells::ClassifyCells():
// - Absent: the cell takes no part, its sides do not cancel the sides of its neighbors, which are
//   then boundaries: the clipped cells (see vtkUnstructuredGridSides::SetCellClipping(),
//   SetPointClipping() and SetExtentClipping()), and the masked ghost cells, see
//   vtkUnstructuredGridSides::SetMaskedGhostTypes();
// - Kept: the other cells, e.g., the duplicate ghost cells without RemoveGhostInterfaces, which are
//   ordinary cells, as in vtkGeometryFilter (see the geometryFilterGhosts test);
// - Covering: the duplicate ghost cells with RemoveGhostInterfaces, whose sides are found, to
//   cancel the sides of the other cells, so that the interfaces between processes are not
//   boundaries, but are not kept, see SideMatcher::PopulateSides(), and which are not preserved.
enum CellState : unsigned char
{
  Absent,
  Kept,
  Covering
};
} // anonymous namespace

//------------------------------------------------------------------------------
// The cells of the input of a vtkUnstructuredGridSides, read by the steps of BuildSidesWorker, with
// the settings of the vtkUnstructuredGridSides, read directly:
// - the cell arrays of the input, read directly through iterators of the offsets and the
//   connectivity, taken once, which are raw pointers for vtkAOSDataArrayTemplate arrays;
// - the cell types of the input, an array, shared or copied, or a single type;
// - the states of the cells, see ClassifyCells();
// - the dimension of every distinct cell type, and the dimension of all the sides if they have a
//   single one, otherwise the dimensions of the sides are stored, set by BuildSidesWorker.
template <typename TId, typename TSideId>
template <class OffsetsT, class ConnectivityT>
struct vtkUnstructuredGridSides<TId, TSideId>::InputCells
{
  using OffsetsIterator =
    typename vtk::detail::SelectValueRange<OffsetsT*, 1>::type::const_iterator;
  using PointsIterator =
    typename vtk::detail::SelectValueRange<ConnectivityT*, 1>::type::const_iterator;

  const vtkUnstructuredGridSides* Self;
  vtkUnstructuredGrid* Input;
  OffsetsIterator Offsets;
  PointsIterator Connectivity;
  CellTypes Types;
  vtkNew<vtkAOSDataArrayTemplate<unsigned char>> CellTypeArray;
  std::unique_ptr<CellState[]> CellStates;       // nullptr if the cells are all kept
  std::array<std::int8_t, 256> TypeDimensions{}; // of the distinct cell types
  int SingleSideDimension = -1;                  // -1 if the sides have several dimensions
  bool HasDimensions = false;                    // whether the dimensions of the sides are stored
  bool MatchesSidePoints = false;                // see vtkUnstructuredGridSides::MatchSidePoints

  InputCells(const vtkUnstructuredGridSides* self, OffsetsT* offsets, ConnectivityT* connectivity)
    : Self(self)
    , Input(self->DataSet)
    , Offsets(vtk::DataArrayValueRange<1>(offsets).cbegin())
    , Connectivity(vtk::DataArrayValueRange<1>(connectivity).cbegin())
  {
    // The cell types: a single type, otherwise an array of unsigned char, shared if it is one, or
    // copied.
    vtkDataArray* types = this->Input->GetCellTypes();
    if (auto* constantTypes = vtkConstantArray<unsigned char>::FastDownCast(types))
    {
      this->Types.Constant = constantTypes->GetConstantValue();
    }
    else
    {
      if (auto* aosTypes = vtkAOSDataArrayTemplate<unsigned char>::FastDownCast(types))
      {
        this->CellTypeArray->ShallowCopy(aosTypes);
      }
      else
      {
        this->CellTypeArray->DeepCopy(types);
      }
      this->Types.Values = this->CellTypeArray->GetPointer(0);
    }
  }

  // The npts points of the cell cellId, in the connectivity.
  PointsIterator GetCellPoints(vtkIdType cellId, vtkIdType& npts) const
  {
    const auto begin = static_cast<vtkIdType>(this->Offsets[cellId]);
    npts = static_cast<vtkIdType>(this->Offsets[cellId + 1]) - begin;
    return this->Connectivity + begin;
  }

  unsigned char GetCellType(vtkIdType cellId) const { return this->Types.GetCellType(cellId); }

  // Classify the cells of the input, into CellStates, in 2 passes: the clipped points, by their ids
  // or by the extent, then the cells, by their ids, their points and their ghost type. Without
  // clipping or ghost cells, the cells are all kept, and have no states.
  void ClassifyCells()
  {
    const vtkUnstructuredGridSides* self = this->Self;
    vtkUnstructuredGrid* input = this->Input;
    vtkUnsignedCharArray* ghostArray = input->GetCellGhostArray();
    const unsigned char* ghosts = ghostArray ? ghostArray->GetPointer(0) : nullptr;
    if (!self->PointClipping && !self->CellClipping && !self->ExtentClipping && !ghosts)
    {
      return;
    }

    // Pass 1: the clipped points.
    std::unique_ptr<bool[]> clippedPoints;
    if (self->PointClipping || self->ExtentClipping)
    {
      const vtkIdType numPts = input->GetNumberOfPoints();
      clippedPoints.reset(new bool[numPts]);
      vtkPoints* points = input->GetPoints();
      const double* extent = self->Extent;
      vtkSMPTools::For(0, numPts,
        [&](vtkIdType begin, vtkIdType end)
        {
          double x[3];
          for (vtkIdType ptId = begin; ptId < end; ++ptId)
          {
            bool clipped =
              self->PointClipping && (ptId < self->PointMinimum || ptId > self->PointMaximum);
            if (!clipped && self->ExtentClipping)
            {
              points->GetPoint(ptId, x);
              clipped = x[0] < extent[0] || x[0] > extent[1] || x[1] < extent[2] ||
                x[1] > extent[3] || x[2] < extent[4] || x[2] > extent[5];
            }
            clippedPoints[ptId] = clipped;
          }
        });
    }

    // Pass 2: the cells.
    const vtkIdType numCells = input->GetNumberOfCells();
    this->CellStates.reset(new CellState[numCells]);
    CellState* cellStates = this->CellStates.get();
    vtkSMPTools::For(0, numCells,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType cellId = begin; cellId < end; ++cellId)
        {
          const unsigned char ghost = ghosts ? ghosts[cellId] : 0;
          bool absent = (ghost & self->MaskedGhostTypes) ||
            (self->CellClipping && (cellId < self->CellMinimum || cellId > self->CellMaximum));
          if (!absent && clippedPoints)
          {
            vtkIdType npts = 0;
            const auto pts = this->GetCellPoints(cellId, npts);
            for (vtkIdType i = 0; i < npts && !absent; ++i)
            {
              absent = clippedPoints[static_cast<vtkIdType>(pts[i])];
            }
          }
          if (absent)
          {
            cellStates[cellId] = Absent;
          }
          else if ((ghost & vtkDataSetAttributes::DUPLICATECELL) && self->RemoveGhostInterfaces)
          {
            cellStates[cellId] = Covering;
          }
          else
          {
            cellStates[cellId] = Kept;
          }
        }
      });
  }

  // Whether the cell is preserved, see PreservedCells, and whether its sides are found, given its
  // state, see CellState: the absent cells take no part, and the covering cells are not
  // preserved. The empty cells have no sides, see
  // vtkCellTypeSides::FixedSizeCellSides<VTK_EMPTY_CELL>, and are not preserved.
  VTK_ALWAYS_INLINE void ClassifyCell(vtkIdType cellId, unsigned char cellType, int cellDim,
    bool& preserveCell, bool& findSides) const
  {
    const CellState cellState = this->CellStates ? this->CellStates[cellId] : Kept;
    preserveCell = cellState == Kept && cellDim < 3 && this->Self->PreserveRenderableInputs &&
      cellType != VTK_EMPTY_CELL;
    findSides = cellState != Absent && !(cellDim < 3 && this->Self->OmitSidesForRenderableInputs);
  }

  // Whether only the faces of the 3D cells are found, with the dimension of their sides known at
  // compile time, see SideMapper::FindsSides().
  bool IsFacesOnly() const { return this->SingleSideDimension == 2 && !this->HasDimensions; }
};

namespace // anonymous
{
//------------------------------------------------------------------------------
// The bit of SideFlags of the sides of dimension sideDim of the cells of dimension cellDim, 0 if
// there is none.
constexpr unsigned int GetSideFlag(int cellDim, int sideDim)
{
  if (sideDim >= cellDim)
  {
    return 0U;
  }
  switch (cellDim)
  {
    case 1:
      return SideFlags::VerticesOfEdges;
    case 2:
      return sideDim == 0 ? SideFlags::VerticesOfSurfaces : SideFlags::EdgesOfSurfaces;
    case 3:
      if (sideDim == 0)
      {
        return SideFlags::VerticesOfVolumes;
      }
      return sideDim == 1 ? SideFlags::EdgesOfVolumes : SideFlags::SurfacesOfVolumes;
    default:
      return 0U;
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
// The preserved cells (PreserveRenderableInputs), in the order of their ids: they are
// counted per chunk of cells, the counts are scanned, then they are written after the found
// sides, with a local index of -1. They are not scattered, since they are not matched, so that
// they are as cheap as a copy, and their dimension, the one of their cell type, is not stored.
template <typename TId, typename TSideId>
struct vtkUnstructuredGridSides<TId, TSideId>::PreservedCells
{
  static constexpr vtkIdType ChunkSize = 65536;

  bool AllPreserved = false; // whether all the cells are preserved, without chunks
  vtkIdType NumberOfPreservedCells = 0;
  vtkIdType NumberOfChunks = 0;
  std::unique_ptr<vtkIdType[]> ChunkOffsets; // of the preserved cells of the chunks, scanned

  template <class InputCellsT>
  static bool IsPreserved(const InputCellsT& cells, vtkIdType cellId, unsigned char cellType)
  {
    bool preserveCell, findSides;
    cells.ClassifyCell(cellId, cellType, cells.TypeDimensions[cellType], preserveCell, findSides);
    return preserveCell;
  }

  // Count the preserved cells, all the cells if allPreserved, e.g., without masks and with cells of
  // dimension 2 or less only. Returns their number.
  template <class InputCellsT>
  vtkIdType Count(const InputCellsT& cells, bool allPreserved)
  {
    this->AllPreserved = allPreserved;
    const vtkIdType numCells = cells.Input->GetNumberOfCells();
    if (allPreserved)
    {
      return this->NumberOfPreservedCells = numCells;
    }
    this->NumberOfChunks = (numCells + ChunkSize - 1) / ChunkSize;
    this->ChunkOffsets.reset(new vtkIdType[this->NumberOfChunks + 1]);
    vtkSMPTools::For(0, this->NumberOfChunks,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType chunk = begin; chunk < end; ++chunk)
        {
          const vtkIdType last = std::min((chunk + 1) * ChunkSize, numCells);
          vtkIdType count = 0;
          for (vtkIdType cellId = chunk * ChunkSize; cellId < last; ++cellId)
          {
            count += IsPreserved(cells, cellId, cells.GetCellType(cellId)) ? 1 : 0;
          }
          this->ChunkOffsets[chunk] = count;
        }
      });
    this->ChunkOffsets[this->NumberOfChunks] = 0;
    return this->NumberOfPreservedCells = vtkSMPTools::ExclusiveScan(this->ChunkOffsets.get(),
             this->ChunkOffsets.get() + this->NumberOfChunks + 1, static_cast<vtkIdType>(0));
  }

  // Write the counted cells, to NumberOfPreservedCells values of cellIds and sideIds.
  template <class InputCellsT>
  void Write(const InputCellsT& cells, TId* cellIds, TSideId* sideIds) const
  {
    const vtkIdType numCells = cells.Input->GetNumberOfCells();
    if (this->AllPreserved)
    {
      vtkSMPTools::For(0, numCells,
        [&](vtkIdType begin, vtkIdType end)
        {
          for (vtkIdType cellId = begin; cellId < end; ++cellId)
          {
            cellIds[cellId] = static_cast<TId>(cellId);
            sideIds[cellId] = static_cast<TSideId>(-1);
          }
        });
      return;
    }
    vtkSMPTools::For(0, this->NumberOfChunks,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType chunk = begin; chunk < end; ++chunk)
        {
          const vtkIdType last = std::min((chunk + 1) * ChunkSize, numCells);
          vtkIdType next = this->ChunkOffsets[chunk];
          for (vtkIdType cellId = chunk * ChunkSize; cellId < last; ++cellId)
          {
            if (IsPreserved(cells, cellId, cells.GetCellType(cellId)))
            {
              cellIds[next] = static_cast<TId>(cellId);
              sideIds[next] = static_cast<TSideId>(-1);
              ++next;
            }
          }
        }
      });
  }
};

namespace // anonymous
{
//------------------------------------------------------------------------------
// The scratch objects of a thread for the sides of its cells, given to the classes of sides of
// vtkCellTypeSides.h: a cell and a list, for the cells whose sides are not read from static tables
// and for the faces of the polyhedra. As the LocalDataType of vtkGeometryFilter, a copy allocates
// its own objects, so that every thread has its own when vtkSMPThreadLocal copies its exemplar.
struct CellSidesLocalData
{
  vtkSmartPointer<vtkGenericCell> Cell;
  vtkSmartPointer<vtkIdList> SidePts;

  CellSidesLocalData() { this->Allocate(); }

  CellSidesLocalData(const CellSidesLocalData&)
  {
    // These are here to have a different allocation for each thread.
    this->Allocate();
  }

  // The list is written for every side of the cells that use it, e.g., the polyhedra: its first
  // allocation, of 2 KiB, keeps it apart from the memory of the other threads.
  void Allocate()
  {
    this->Cell.TakeReference(vtkGenericCell::New());
    this->SidePts.TakeReference(vtkIdList::New());
    this->SidePts->Reserve(256);
  }

  void Swap(CellSidesLocalData& other)
  {
    using std::swap;
    swap(this->Cell, other.Cell);
    swap(this->SidePts, other.SidePts);
  }

  CellSidesLocalData& operator=(const CellSidesLocalData& other)
  {
    if (this != &other)
    {
      CellSidesLocalData tmp(other);
      this->Swap(tmp);
    }
    return *this;
  }
};

//------------------------------------------------------------------------------
// The output of the scattered sides, as a structure of arrays, since the scatter is bound by
// memory: their cell, local index and dimension (if HasDimensions) at the same positions, then
// the cell and local index of the NumberOfPreservedCells preserved cells, see PreservedCells. As
// in the ExternalFaces worklet of Viskores, the sides are scattered as their cell and their index
// in it, see the Output concept of vtkKeys.h, from which their local index and dimension are
// located, see SetElement(). The cells are scattered in order, so that the cell type, and the
// offsets of the sides that use the number of points, are read from the cache.
template <typename TId, typename TSideId, class InputCellsT>
struct SideOutput
{
  std::unique_ptr<TId[]>& CellIds;
  std::unique_ptr<TSideId[]>& SideIds;
  std::unique_ptr<std::uint8_t[]>& Dimensions;
  bool HasDimensions;
  vtkIdType NumberOfPreservedCells;
  const InputCellsT& Cells;
  unsigned int Flags; // The SideFlags of the sides to find

  mutable vtkSMPThreadLocal<CellSidesLocalData> TLScratch;

  void SetNumberOfElements(vtkIdType numberOfSides)
  {
    // Default-initialized, so that they are not filled before being scattered.
    this->CellIds.reset(new TId[numberOfSides + this->NumberOfPreservedCells]);
    this->SideIds.reset(new TSideId[numberOfSides + this->NumberOfPreservedCells]);
    this->Dimensions.reset(this->HasDimensions ? new std::uint8_t[numberOfSides] : nullptr);
  }

  // The side k of the cell cellId, its sides of all the found dimensions by increasing dimension,
  // which is its local index with a single dimension. With several, its local index and dimension
  // are located by subtracting the number of sides of every found dimension until k is one of
  // them, npts being read only by the sides that use it, as in SideMapper::Count().
  void SetElement(vtkIdType position, TId cellId, vtkIdType k) const
  {
    this->CellIds[position] = cellId;
    if (!this->HasDimensions)
    {
      this->SideIds[position] = static_cast<TSideId>(k);
      return;
    }
    const unsigned char cellType = this->Cells.GetCellType(cellId);
    vtkCellTypeSides::DispatchCellSides(cellType,
      [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
      {
        vtkIdType npts = 0;
        CellSidesLocalData& scratch = this->TLScratch.Local();
        if constexpr (decltype(sides)::UsesNumberOfPoints)
        {
          this->Cells.GetCellPoints(cellId, npts);
        }
        const int cellDim = sides.GetDimension(cellType);
        for (int sideDim = 0; sideDim < cellDim; ++sideDim)
        {
          if ((this->Flags & GetSideFlag(cellDim, sideDim)) != 0U)
          {
            const int numSides = sides.GetNumberOfSides(
              this->Cells.Input, cellId, cellType, sideDim, npts, scratch.Cell);
            if (k < numSides)
            {
              this->SideIds[position] = static_cast<TSideId>(k);
              this->Dimensions[position] = static_cast<std::uint8_t>(sideDim);
              return;
            }
            k -= numSides;
          }
        }
        assert(false && "the side must be in its cell");
      });
  }
};
} // anonymous namespace

//------------------------------------------------------------------------------
// The sides of the cells, the InputMapper of vtkKeys.h, whose records are the cells,
// found in 3 passes over the cells, as the faces of DP-Hash-Count:
// 1. CountElements() of every cell, scanned by vtk::keys::ScatterDense() into the offsets of the
//    elements of the cells;
// 2. WriteKeys() of the elements of every cell, computing the points of every side once;
// 3. the scatter of the elements, as their cell and index, into a SideOutput, which locates them
//    in their cell with several dimensions.
// The elements of a cell are its sides by increasing dimension, keyed by their smallest point id;
// the preserved cells are not scattered, see PreservedCells. The work of a cell is done by an
// inlined lambda for the sides of its class, see vtkCellTypeSides::DispatchCellSides().
template <typename TId, typename TSideId>
template <class InputCellsT>
struct vtkUnstructuredGridSides<TId, TSideId>::SideMapper
{
  using KeyType = TId;
  using ElementIdType = vtkIdType;

  const InputCellsT& Cells;
  unsigned int Flags; // The SideFlags of the sides to find
  // Whether a cell can be masked, otherwise its elements are all its sides.
  bool HasCellOptions;
  // Whether only the faces of the 3D cells are found, see FindsSides().
  bool FacesOnly;

  mutable vtkSMPThreadLocal<CellSidesLocalData> TLScratch;

  SideMapper(const InputCellsT& cells)
    : Cells(cells)
    , Flags(static_cast<unsigned int>(cells.Self->OutputDimensionControl))
    , HasCellOptions(cells.CellStates || cells.Self->OmitSidesForRenderableInputs)
    , FacesOnly(cells.IsFacesOnly())
  {
  }

  // Whether the sides of dimension sideDim of the cells of dimension cellDim are found: with
  // Faces, the faces of the 3D cells only, known at compile time.
  template <bool Faces>
  VTK_ALWAYS_INLINE bool FindsSides(int cellDim, int sideDim) const
  {
    if constexpr (Faces)
    {
      return cellDim == 3 && sideDim == 2;
    }
    else
    {
      return (this->Flags & GetSideFlag(cellDim, sideDim)) != 0U;
    }
  }

  // Whether the sides of the cell are found, see InputCells::ClassifyCell().
  VTK_ALWAYS_INLINE bool FindsCellSides(vtkIdType cellId, unsigned char cellType, int cellDim) const
  {
    if (!this->HasCellOptions)
    {
      // The empty cells have no sides, see vtkCellTypeSides::FixedSizeCellSides<VTK_EMPTY_CELL>.
      return true;
    }
    bool preserveCell, findSides;
    this->Cells.ClassifyCell(cellId, cellType, cellDim, preserveCell, findSides);
    return findSides;
  }

  // The records of the InputMapper concept of vtkKeys.h: the cells.
  TId GetNumberOfRecords() const { return static_cast<TId>(this->Cells.Input->GetNumberOfCells()); }

  // Pass 1: the number of elements of the cells [begin, end), at counts[cellId - begin], see the
  // InputMapper concept of vtkKeys.h.
  template <bool Faces, typename CountType>
  void Count(vtkIdType begin, vtkIdType end, CountType* counts) const
  {
    CellSidesLocalData& scratch = this->TLScratch.Local();
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      const unsigned char cellType = this->Cells.GetCellType(cellId);
      TId count = 0;
      vtkCellTypeSides::DispatchCellSides(cellType,
        [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
        {
          const int cellDim = sides.GetDimension(cellType);
          if (!this->FindsCellSides(cellId, cellType, cellDim))
          {
            return;
          }
          vtkIdType npts = 0;
          if constexpr (decltype(sides)::UsesNumberOfPoints)
          {
            this->Cells.GetCellPoints(cellId, npts);
          }
          for (int sideDim = 0; sideDim < cellDim; ++sideDim)
          {
            if (this->FindsSides<Faces>(cellDim, sideDim))
            {
              const int numSides = sides.GetNumberOfSides(
                this->Cells.Input, cellId, cellType, sideDim, npts, scratch.Cell);
              assert(numSides - 1 <= std::numeric_limits<TSideId>::max() &&
                "the local indices of the sides must fit in TSideId");
              count += static_cast<TId>(numSides);
            }
          }
        });
      counts[cellId - begin] = static_cast<CountType>(count);
    }
  }

  // Pass 1 of the InputMapper concept, with the faces only known at compile time.
  template <typename CountType>
  void CountElements(TId beginCellId, TId endCellId, CountType* counts) const
  {
    if (this->FacesOnly)
    {
      this->template Count<true>(beginCellId, endCellId, counts);
    }
    else
    {
      this->template Count<false>(beginCellId, endCellId, counts);
    }
  }

  // Pass 2: the keys of the elements of the cells [begin, end).
  template <bool Faces, typename ElementIdType>
  void Write(vtkIdType begin, vtkIdType end, const ElementIdType* offsets, TId* keys) const
  {
    CellSidesLocalData& scratch = this->TLScratch.Local();
    vtkIdType ptIds[vtkCellTypeSides::MaximumCellSidePoints];
    for (vtkIdType cellId = begin; cellId < end; ++cellId)
    {
      vtkIdType next = offsets[cellId];
      if (next == offsets[cellId + 1])
      {
        continue;
      }
      const unsigned char cellType = this->Cells.GetCellType(cellId);
      vtkCellTypeSides::DispatchCellSides(cellType,
        [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
        {
          const int cellDim = sides.GetDimension(cellType);
          if (!this->FindsCellSides(cellId, cellType, cellDim))
          {
            return;
          }
          vtkIdType npts = 0;
          const auto pts = this->Cells.GetCellPoints(cellId, npts);
          for (int sideDim = 0; sideDim < cellDim; ++sideDim)
          {
            if (!this->FindsSides<Faces>(cellDim, sideDim))
            {
              continue;
            }
            const int numSides = sides.GetNumberOfSides(
              this->Cells.Input, cellId, cellType, sideDim, npts, scratch.Cell);
            for (int s = 0; s < numSides; ++s)
            {
              const vtkIdType* ids = nullptr;
              vtkIdType numCorners = 0;
              sides.GetSidePoints(this->Cells.Input, cellId, cellType, sideDim, s, npts, pts, ptIds,
                scratch.SidePts, ids, numCorners, scratch.Cell);
              // The key of a side is the smallest id of its corners, by which it is matched. A
              // side without points is skipped when the sides are matched.
              keys[next++] =
                numCorners > 0 ? static_cast<TId>(*std::min_element(ids, ids + numCorners)) : 0;
            }
          }
        });
    }
  }

  // Pass 2 of the InputMapper concept, with the faces only known at compile time.
  template <typename ElementIdType>
  void WriteKeys(TId beginCellId, TId endCellId, const ElementIdType* offsets, TId* keys) const
  {
    if (this->FacesOnly)
    {
      this->template Write<true>(beginCellId, endCellId, offsets, keys);
    }
    else
    {
      this->template Write<false>(beginCellId, endCellId, offsets, keys);
    }
  }
};

namespace // anonymous
{
//------------------------------------------------------------------------------
// Put the sides of every key back in the order of their cells, as vtkStaticCellLinksTemplate does,
// see vtk::keys::ForEachKeyRange(): the threads of vtk::keys::ScatterDense() fill a key from its
// front concurrently, so that the sides of a cell are in the order of their local index, but those
// of the cells of different threads can interleave. A stable sort by cell id, of the keys that are
// not sorted only, makes the scattered, reduced and grouped sides, see MatchingFunctor, the same
// from one run to the next: an insertion sort for the keys of few sides, and for the others, e.g.,
// the faces around a point of many cells, a sort of their positions, since the insertion sort is
// quadratic when their cells interleave.
struct SortSidesByCell
{
  // The largest number of sides of a key sorted by insertion.
  static constexpr int MaximumInsertionSortSize = 32;

  template <class OutputT, typename KeyIdType, typename ElementIdType>
  void operator()(
    OutputT& output, KeyIdType beginKeyId, KeyIdType endKeyId, const ElementIdType* offsets) const
  {
    auto* cellIds = output.CellIds.get();
    auto* sideIds = output.SideIds.get();
    std::uint8_t* dimensions = output.HasDimensions ? output.Dimensions.get() : nullptr;
    // The scratch arrays of the keys sorted by their positions, kept from key to key.
    std::vector<ElementIdType> order;
    std::vector<std::remove_pointer_t<decltype(cellIds)>> cellIdsCopy;
    std::vector<std::remove_pointer_t<decltype(sideIds)>> sideIdsCopy;
    std::vector<std::uint8_t> dimensionsCopy;
    for (KeyIdType keyId = beginKeyId; keyId < endKeyId; ++keyId)
    {
      const ElementIdType start = offsets[keyId];
      const ElementIdType stop = offsets[keyId + 1];
      if (std::is_sorted(cellIds + start, cellIds + stop))
      {
        continue;
      }
      if (stop - start > MaximumInsertionSortSize)
      {
        // The positions of the sides by cell id, then by position, as the insertion sort orders
        // them, applied to the arrays of the sides.
        order.resize(static_cast<std::size_t>(stop - start));
        std::iota(order.begin(), order.end(), start);
        std::sort(order.begin(), order.end(), [cellIds](ElementIdType a, ElementIdType b)
          { return cellIds[a] < cellIds[b] || (cellIds[a] == cellIds[b] && a < b); });
        SortSidesByCell::Permute(cellIds, start, order, cellIdsCopy);
        SortSidesByCell::Permute(sideIds, start, order, sideIdsCopy);
        if (dimensions)
        {
          SortSidesByCell::Permute(dimensions, start, order, dimensionsCopy);
        }
        continue;
      }
      for (ElementIdType i = start + 1; i < stop; ++i)
      {
        const auto cellId = cellIds[i];
        const auto sideId = sideIds[i];
        const std::uint8_t dimension = dimensions ? dimensions[i] : 0;
        ElementIdType j = i;
        for (; j > start && cellIds[j - 1] > cellId; --j)
        {
          cellIds[j] = cellIds[j - 1];
          sideIds[j] = sideIds[j - 1];
          if (dimensions)
          {
            dimensions[j] = dimensions[j - 1];
          }
        }
        cellIds[j] = cellId;
        sideIds[j] = sideId;
        if (dimensions)
        {
          dimensions[j] = dimension;
        }
      }
    }
  }

  // Reorder the values of a key, from start, as its sorted positions `order`, through `copy`.
  template <typename T, typename ElementIdType>
  static void Permute(
    T* values, ElementIdType start, const std::vector<ElementIdType>& order, std::vector<T>& copy)
  {
    copy.assign(values + start, values + start + static_cast<ElementIdType>(order.size()));
    for (std::size_t k = 0; k < order.size(); ++k)
    {
      values[start + static_cast<ElementIdType>(k)] = copy[order[k] - start];
    }
  }
};

//------------------------------------------------------------------------------
/**
 * An allocator of blocks aligned to, and padded to a multiple of, 128 bytes, the cache line of the
 * Apple M processors and the pair of lines that x86 processors prefetch: the buffers that a thread
 * writes for every side then share no cache line with those of the other threads, which made the
 * matching twice as slow with 16 threads.
 */
template <typename T>
struct ThreadBufferAllocator
{
  using value_type = T;
  static constexpr std::size_t Alignment = 128;

  ThreadBufferAllocator() = default;
  template <typename U>
  ThreadBufferAllocator(const ThreadBufferAllocator<U>&) noexcept
  {
  }

  // A block of the alignment, whose array new and delete expressions call the aligned operator new
  // and delete, without naming std::align_val_t, which some compilers find twice, e.g., icpx with
  // libstdc++ 8.
  struct alignas(Alignment) Block
  {
    unsigned char Bytes[Alignment];
  };

  static T* allocate(std::size_t n)
  {
    return reinterpret_cast<T*>(new Block[(n * sizeof(T) + Alignment - 1) / Alignment]);
  }

  static void deallocate(T* p, std::size_t) noexcept { delete[] reinterpret_cast<Block*>(p); }

  // Default-initialize the values that resize() adds, instead of zeroing them, since they are
  // written right after, see SideMatcher::WritePoints().
  template <typename U>
  void construct(U* p) noexcept(std::is_nothrow_default_constructible_v<U>)
  {
    ::new (static_cast<void*>(p)) U;
  }

  template <typename U, typename... Args>
  void construct(U* p, Args&&... args)
  {
    ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
  }

  template <typename U>
  bool operator==(const ThreadBufferAllocator<U>&) const noexcept
  {
    return true;
  }

  template <typename U>
  bool operator!=(const ThreadBufferAllocator<U>&) const noexcept
  {
    return false;
  }
};

template <typename T>
using ThreadBuffer = std::vector<T, ThreadBufferAllocator<T>>;

//------------------------------------------------------------------------------
/**
 * The sides of a key, matched by their points: two sides are equal if they have the same points in
 * the same cyclic order, in either direction. The points of a side are rotated to start with its
 * smallest point, the key, or written in their canonical form if it is repeated, see WritePoints(),
 * in a buffer shared by the sides of the key, which are searched linearly, since a key has few
 * sides. The second and the last points of a side are kept with it, so that most sides are told
 * apart without reading their points. Every side is inserted in an entry, its own or the one of
 * the side it matches, so that the sides can also be grouped by entry, see PopulateGroups(). The
 * arrays are reused from key to key.
 */
template <typename TId, typename TSideId>
class SideMatcher
{
public:
  /**
   * Whether the sides are inserted with both rules, see InsertStaticSide(): with Boundary and the
   * sides of several dimensions, see MatchingFunctor. Off by default.
   */
  void SetMixedRules(bool mixedRules) { this->MixedRules = mixedRules; }

  /**
   * Insert a side without corners, which the tables of the sides never give, in an entry of its
   * own, and return it, so that it is a group alone for PopulateGroups(): it matches no side.
   */
  std::uint32_t InsertUnmatchedSide(TId cellId, TSideId sideId, std::uint8_t dimension)
  {
    this->Sides.emplace_back(static_cast<std::uint32_t>(this->Points.size()), 0, 0, TId(0), TId(0),
      cellId, sideId, dimension, static_cast<std::uint8_t>(0));
    return static_cast<std::uint32_t>(this->Sides.size() - 1);
  }

  /**
   * Insert a side of numPts points with a rule, Winding or AnyOccurrence (see SummaryStrategy, and
   * MatchingFunctor for Boundary). With Winding, an equal side already inserted is removed, so
   * that the sides used an even number of times cancel. With AnyOccurrence, one of the equal sides
   * is kept. With both rules, see SetMixedRules(), an equal side is never removed: its flags are
   * updated, so that whether it is kept does not depend on the order of the sides, see
   * IsKeptBits. One of the equal sides is kept, preferring one that is not of a covering cell, see
   * CellState. The numPts points of the side are its corners, followed by its numOthers other
   * points, e.g., its mid-edge points, which equal sides have too, in any order, or none when they
   * are matched by their corners only, see vtkUnstructuredGridSides::SetMatchSidePoints(). N is
   * the number of corners when it is known at compile time, 0 otherwise. Return the entry of the
   * side, see PopulateGroups(): the one of the side it matches, or its own, which is meaningful
   * with AnyOccurrence only, since Winding removes the entries of the sides that cancel.
   */
  template <int N>
  VTK_ALWAYS_INLINE std::uint32_t InsertStaticSide(TId cellId, TSideId sideId,
    std::uint8_t dimension, int numPts, int numOthers, const vtkIdType* ids, bool isCovering,
    SummaryStrategy rule)
  {
    assert(rule != SummaryStrategy::Boundary &&
      "Boundary is mapped to a rule per side, see MatchingFunctor");
    const int n = SideMatcher::GetNumberOfPoints<N>(numPts);
    // The points of the side, added after the points of the inserted sides, and removed if an
    // equal side is already inserted.
    assert(n <= VTK_UNSIGNED_SHORT_MAX && numOthers <= VTK_UNSIGNED_SHORT_MAX &&
      "the numbers of points of a side must fit in SideEntry");
    const auto offset = static_cast<std::uint32_t>(this->Points.size());
    this->WritePoints(n, ids);
    if (numOthers > 0)
    {
      this->WriteOtherPoints(numOthers, ids + n);
    }
    const TId* points = this->Points.data() + offset;
    const TId second = SideMatcher::GetSecond(n, points);
    for (SideEntry& side : this->Sides)
    {
      // An equal side has the same second point, forward, or the same last point, backward, and
      // the same other points, which rarely differ.
      if (side.NumberOfPoints == n && (side.Second == second || side.Last == second) &&
        side.NumberOfOthers == numOthers &&
        SideMatcher::Equal<N>(this->Points.data() + side.Offset, points, n) &&
        std::equal(points + n, points + n + numOthers, this->Points.data() + side.Offset + n))
      {
        const auto entry = static_cast<std::uint32_t>(&side - this->Sides.data());
        if (rule == SummaryStrategy::Winding && !this->MixedRules)
        {
          // The sides used an even number of times cancel.
          side = this->Sides.back();
          this->Sides.pop_back();
        }
        else
        {
          // With both rules, the side is kept until all the sides of the key are inserted.
          if (rule == SummaryStrategy::Winding)
          {
            side.Flags ^= IsOddBit;
          }
          else
          {
            side.Flags |= IsAnyOccurrenceBit;
          }
          if ((side.Flags & IsCoveringBit) && !isCovering)
          {
            side.CellId = cellId;
            side.SideId = sideId;
            side.Flags &= static_cast<std::uint8_t>(~IsCoveringBit);
          }
        }
        this->Points.resize(offset);
        return entry;
      }
    }
    return this->Append(n, numOthers, offset, cellId, sideId, dimension, isCovering, rule);
  }

  /**
   * Same as InsertStaticSide(), for a side whose number of points is not known at compile time: the
   * sides of up to 12 points, e.g., the faces of polyhedra, are inserted with their size known at
   * compile time. Not inlined, since it is called for every class of cells, see
   * vtkCellTypeSides::DispatchCellSides().
   */
  VTK_NEVER_INLINE std::uint32_t InsertDynamicSide(TId cellId, TSideId sideId,
    std::uint8_t dimension, int numPts, int numOthers, const vtkIdType* ids, bool isCovering,
    SummaryStrategy rule)
  {
    switch (numPts)
    {
#define vtkUnstructuredGridSidesInsertCase(N)                                                      \
  case N:                                                                                          \
    return this->InsertStaticSide<N>(                                                              \
      cellId, sideId, dimension, N, numOthers, ids, isCovering, rule);
      vtkUnstructuredGridSidesInsertCase(1);
      vtkUnstructuredGridSidesInsertCase(2);
      vtkUnstructuredGridSidesInsertCase(3);
      vtkUnstructuredGridSidesInsertCase(4);
      vtkUnstructuredGridSidesInsertCase(5);
      vtkUnstructuredGridSidesInsertCase(6);
      vtkUnstructuredGridSidesInsertCase(7);
      vtkUnstructuredGridSidesInsertCase(8);
      vtkUnstructuredGridSidesInsertCase(9);
      vtkUnstructuredGridSidesInsertCase(10);
      vtkUnstructuredGridSidesInsertCase(11);
      vtkUnstructuredGridSidesInsertCase(12);
#undef vtkUnstructuredGridSidesInsertCase
      default:
        return this->InsertStaticSide<0>(
          cellId, sideId, dimension, numPts, numOthers, ids, isCovering, rule);
    }
  }

  /**
   * Record the entry of the side inserted last, for PopulateGroups(): one per side of the key, in
   * the order of the sides.
   */
  void Record(std::uint32_t entry) { this->Entries.emplace_back(entry); }

  /**
   * Write the kept sides that are not of covering cells at `cellIds`, `sideIds` and `dimensions`
   * (if not nullptr), and return their number: the sides with one of the IsKeptBits.
   */
  vtkIdType PopulateSides(TId* cellIds, TSideId* sideIds, std::uint8_t* dimensions) const
  {
    vtkIdType count = 0;
    for (const SideEntry& side : this->Sides)
    {
      if (!(side.Flags & IsCoveringBit) && (side.Flags & IsKeptBits))
      {
        cellIds[count] = side.CellId;
        sideIds[count] = side.SideId;
        if (dimensions)
        {
          dimensions[count] = side.Dimension;
        }
        ++count;
      }
    }
    return count;
  }

  /**
   * Write the recorded sides, see Record(), whose cells, local indices and dimensions (if not
   * nullptr) are at `cellIds`, `sideIds` and `dimensions` in the order of their insertion, in the
   * order of their entries, which is the order of their first sides, the sides of an entry in the
   * order of their insertion, and flag the first side of every entry in `groupStarts`, as a group
   * functor of vtk::keys::Group(). With AnyOccurrence only, which removes no entry.
   */
  void PopulateGroups(
    TId* cellIds, TSideId* sideIds, std::uint8_t* dimensions, std::uint8_t* groupStarts)
  {
    const std::size_t numSides = this->Entries.size();
    const std::uint32_t* entries = this->Entries.data();
    if (std::is_sorted(entries, entries + numSides))
    {
      // The sides of every entry are contiguous already, e.g., if they are all distinct.
      for (std::size_t i = 0; i < numSides; ++i)
      {
        groupStarts[i] = static_cast<std::uint8_t>(i == 0 || entries[i] != entries[i - 1]);
      }
      return;
    }
    // A stable counting sort of the sides by entry, through a copy of them.
    const std::size_t numEntries = this->Sides.size();
    this->EntryOffsets.assign(numEntries + 1, 0);
    std::uint32_t* entryOffsets = this->EntryOffsets.data();
    for (std::size_t i = 0; i < numSides; ++i)
    {
      ++entryOffsets[entries[i] + 1];
    }
    std::fill_n(groupStarts, numSides, std::uint8_t(0));
    for (std::size_t entry = 0; entry < numEntries; ++entry)
    {
      groupStarts[entryOffsets[entry]] = 1;
      entryOffsets[entry + 1] += entryOffsets[entry];
    }
    this->Copies.resize(numSides);
    for (std::size_t i = 0; i < numSides; ++i)
    {
      this->Copies[i] = { cellIds[i], sideIds[i], dimensions ? dimensions[i] : std::uint8_t(0) };
    }
    for (std::size_t i = 0; i < numSides; ++i)
    {
      const std::uint32_t position = entryOffsets[entries[i]]++;
      const GroupedSide& side = this->Copies[i];
      cellIds[position] = side.CellId;
      sideIds[position] = side.SideId;
      if (dimensions)
      {
        dimensions[position] = side.Dimension;
      }
    }
  }

  /**
   * Remove the inserted sides, keeping the memory of the arrays for the next key.
   */
  void Reset()
  {
    this->Sides.clear();
    this->Points.clear();
    this->Entries.clear();
  }

private:
  // The flags of an inserted side, see SideEntry: whether its cell is covering, see CellState,
  // whether it is inserted an odd number of times with Winding, and whether one of its equal sides
  // is inserted with AnyOccurrence. It is kept with one of the IsKeptBits, see PopulateSides(): an
  // AnyOccurrence side keeps it whatever the number of its Winding sides.
  static constexpr std::uint8_t IsCoveringBit = 1 << 0;
  static constexpr std::uint8_t IsOddBit = 1 << 1;
  static constexpr std::uint8_t IsAnyOccurrenceBit = 1 << 2;
  static constexpr std::uint8_t IsKeptBits = IsOddBit | IsAnyOccurrenceBit;

  // An inserted side: its corners and other points, see InsertStaticSide(), the cell, local index
  // and dimension written back if it is kept, and its flags.
  struct SideEntry
  {
    SideEntry(std::uint32_t offset, int numberOfPoints, int numberOfOthers, TId second, TId last,
      TId cellId, TSideId sideId, std::uint8_t dimension, std::uint8_t flags)
      : Offset(offset)
      , NumberOfPoints(static_cast<std::uint16_t>(numberOfPoints))
      , NumberOfOthers(static_cast<std::uint16_t>(numberOfOthers))
      , Second(second)
      , Last(last)
      , CellId(cellId)
      , SideId(sideId)
      , Dimension(dimension)
      , Flags(flags)
    {
    }

    std::uint32_t Offset;         // of its points in Points, its corners then its other points
    std::uint16_t NumberOfPoints; // of its corners
    std::uint16_t NumberOfOthers;
    TId Second;
    TId Last;
    TId CellId;
    TSideId SideId;
    std::uint8_t Dimension;
    std::uint8_t Flags;
  };

  // The number of points of a side: N if it is known at compile time, numPts otherwise.
  template <int N>
  static VTK_ALWAYS_INLINE int GetNumberOfPoints(int numPts)
  {
    if constexpr (N > 0)
    {
      return N;
    }
    else
    {
      return numPts;
    }
  }

  // Compare the sequences of the n points `ids` of a side that start at the points a and b, in the
  // directions da and db (1 forward, n - 1 backward): negative, zero or positive, as their first
  // different points.
  static int CompareSequences(const vtkIdType* ids, int n, int a, int da, int b, int db)
  {
    for (int i = 1; i < n; ++i)
    {
      // The next points of the sequences, cyclically, as (a + da) % n without a division: a step
      // of n - 1 is a step back, and a + da < 2 n, so that one subtraction wraps it.
      a = a + da < n ? a + da : a + da - n;
      b = b + db < n ? b + db : b + db - n;
      if (ids[a] != ids[b])
      {
        return ids[a] < ids[b] ? -1 : 1;
      }
    }
    return 0;
  }

  // Add the n points `ids` of a side whose smallest point is repeated, from its first occurrence
  // `first`, to `points`, in their canonical form: the smallest of the sequences of its points
  // that start with its smallest point, in either direction. Two such sides with the same points in
  // the same cyclic order, in either direction, have the same canonical form, which a rotation to
  // one of the occurrences of their smallest point does not give, e.g., the degenerate faces of a
  // collapsed hexahedron. Not inlined, since it is only needed for degenerate sides.
  static VTK_NEVER_INLINE void WriteCanonicalPoints(
    const vtkIdType* ids, int n, int first, ThreadBuffer<TId>& points)
  {
    int start = first;
    int step = SideMatcher::CompareSequences(ids, n, first, 1, first, n - 1) <= 0 ? 1 : n - 1;
    for (int i = first + 1; i < n; ++i)
    {
      if (ids[i] != ids[first])
      {
        continue;
      }
      if (SideMatcher::CompareSequences(ids, n, i, 1, start, step) < 0)
      {
        start = i;
        step = 1;
      }
      if (SideMatcher::CompareSequences(ids, n, i, n - 1, start, step) < 0)
      {
        start = i;
        step = n - 1;
      }
    }
    const std::size_t offset = points.size();
    points.resize(offset + n);
    TId* out = points.data() + offset;
    for (int i = 0, j = start; i < n; ++i)
    {
      out[i] = static_cast<TId>(ids[j]);
      // The next point, cyclically, as (j + step) % n without a division: a step of n - 1 is a
      // step back, and j + step < 2 n, so that one subtraction wraps it.
      j = j + step < n ? j + step : j + step - n;
    }
  }

  // Add the n points of a side after the points of the inserted sides, rotated to start with its
  // smallest point, or in their canonical form if it is repeated, see WriteCanonicalPoints(),
  // without inserting the side, see Append().
  VTK_ALWAYS_INLINE void WritePoints(int n, const vtkIdType* ids)
  {
    // The first smallest point, and whether it is not repeated.
    int first = 0;
    bool notRepeated = true;
    for (int i = 1; i < n; ++i)
    {
      if (ids[i] < ids[first])
      {
        first = i;
        notRepeated = true;
      }
      else if (VTK_UNLIKELY(ids[i] == ids[first])) // this only happens on degenerate cells.
      {
        notRepeated = false;
      }
    }
    if (VTK_LIKELY(notRepeated))
    {
      // One growth for the n points, which are then written, see ThreadBufferAllocator.
      const std::size_t offset = this->Points.size();
      this->Points.resize(offset + n);
      TId* out = this->Points.data() + offset;
      for (int i = 0, j = first; i < n; ++i)
      {
        out[i] = static_cast<TId>(ids[j]);
        // The next point, cyclically: the first one after the last one, as (j + 1) % n without a
        // division.
        j = j + 1 < n ? j + 1 : 0;
      }
    }
    else
    {
      SideMatcher::WriteCanonicalPoints(ids, n, first, this->Points);
    }
  }

  // Add the m other points `ids` of a side after its corners, see WritePoints(), sorted, so that
  // they are compared in any order. Not inlined, since only the nonlinear sides have them.
  VTK_NEVER_INLINE void WriteOtherPoints(int m, const vtkIdType* ids)
  {
    const auto start = static_cast<std::ptrdiff_t>(this->Points.size());
    for (int i = 0; i < m; ++i)
    {
      this->Points.emplace_back(static_cast<TId>(ids[i]));
    }
    std::sort(this->Points.begin() + start, this->Points.end());
  }

  // The second point of a side of n points, its only point if it is a vertex.
  static VTK_ALWAYS_INLINE TId GetSecond(int n, const TId* points)
  {
    return n > 1 ? points[1] : points[0];
  }

  // Whether two sides of n points, starting with the same point, see WritePoints(), whose second
  // points are equal or the last point of the first one is the second point of the other, see
  // InsertStaticSide(), have the same points in the same cyclic order, in either direction:
  // forward if their second points are equal, otherwise backward. Both are tried only for a
  // degenerate side, whose second and last points are equal, e.g., of points 0 1 2 3 1.
  template <int N>
  static VTK_ALWAYS_INLINE bool Equal(const TId* a, const TId* b, int n)
  {
    if constexpr (N == 1)
    {
      return true;
    }
    else if constexpr (N == 2)
    {
      return a[1] == b[1];
    }
    else
    {
      n = SideMatcher::GetNumberOfPoints<N>(n);
      if (n < 3)
      {
        return n < 2 || a[1] == b[1];
      }
      assert((a[1] == b[1] || a[n - 1] == b[1]) && "see InsertStaticSide()");
      if (a[1] == b[1])
      {
        int i = 2;
        while (i < n && a[i] == b[i])
        {
          ++i;
        }
        if (i == n)
        {
          return true;
        }
        // Backward too, only if the second and the last points are equal: a degenerate side.
        if (VTK_LIKELY(a[n - 1] != b[1]))
        {
          return false;
        }
      }
      for (int i = 2; i < n; ++i)
      {
        if (a[n - i] != b[i])
        {
          return false;
        }
      }
      return true;
    }
  }

  // Insert the side of n corners and m other points, added by WritePoints() and WriteOtherPoints()
  // at the offset `offset` of the points, in an entry of its own, which is returned.
  VTK_ALWAYS_INLINE std::uint32_t Append(int n, int m, std::uint32_t offset, TId cellId,
    TSideId sideId, std::uint8_t dimension, bool isCovering, SummaryStrategy rule)
  {
    const TId* points = this->Points.data() + offset;
    const auto flags = static_cast<std::uint8_t>((isCovering ? IsCoveringBit : 0) |
      (rule == SummaryStrategy::Winding ? IsOddBit : IsAnyOccurrenceBit));
    this->Sides.emplace_back(offset, n, m, SideMatcher::GetSecond(n, points), points[n - 1], cellId,
      sideId, dimension, flags);
    return static_cast<std::uint32_t>(this->Sides.size() - 1);
  }

  // A side copied by PopulateGroups().
  struct GroupedSide
  {
    TId CellId;
    TSideId SideId;
    std::uint8_t Dimension;
  };

  // The inserted sides, and their points, which keep their memory from key to key, in buffers of
  // their own, see ThreadBufferAllocator, with the recorded entries of the sides, and the offsets
  // of the entries and the copies of the sides of PopulateGroups().
  ThreadBuffer<SideEntry> Sides;
  ThreadBuffer<TId> Points;
  ThreadBuffer<std::uint32_t> Entries;
  ThreadBuffer<std::uint32_t> EntryOffsets;
  ThreadBuffer<GroupedSide> Copies;
  bool MixedRules = false;
};

//------------------------------------------------------------------------------
// The scratch data of a thread to match the sides of the keys: its matcher, and the scratch
// objects for the sides of its cells, see CellSidesLocalData.
template <typename TId, typename TSideId>
struct MatchingLocalData
{
  SideMatcher<TId, TSideId> Matcher;
  CellSidesLocalData Scratch;
};
} // anonymous namespace

//------------------------------------------------------------------------------
// The reduce functor matching the sides of a key pairwise, and keeping them depending on the
// strategy, which is also the group functor grouping the equal sides of a key, with AnyOccurrence.
template <typename TId, typename TSideId>
template <class InputCellsT, bool Faces>
struct vtkUnstructuredGridSides<TId, TSideId>::MatchingFunctor
{
  // The rule of a side, Winding or AnyOccurrence: the strategy itself, or with Boundary, Winding
  // for the sides of dimension d - 1 of the cells of dimension d, AnyOccurrence otherwise. Only
  // Boundary with sides of several dimensions needs the cell dimension of a side, and mixes the
  // rules, see SideMatcher::SetMixedRules().
  bool AlwaysWinding;
  bool NeverWinding;

  const InputCellsT& Cells;
  // See SideMapper.
  int SingleSideDimension;

  mutable vtkSMPThreadLocal<MatchingLocalData<TId, TSideId>> LocalData;

  MatchingFunctor(const InputCellsT& cells, SummaryStrategy strategy)
    : AlwaysWinding(strategy == SummaryStrategy::Winding ||
        (strategy == SummaryStrategy::Boundary &&
          (cells.Self->OutputDimensionControl & ~SideFlags::NextLowestDimension) == 0))
    , NeverWinding(strategy == SummaryStrategy::AnyOccurrence ||
        (strategy == SummaryStrategy::Boundary &&
          (cells.Self->OutputDimensionControl & SideFlags::NextLowestDimension) == 0))
    , Cells(cells)
    , SingleSideDimension(cells.SingleSideDimension)
  {
  }

  // Insert the side sideId of dimension sideDim of the cell cellId, of the sides of its class, in
  // the matcher of the thread, with the rule of the side, and return its entry, see SideMatcher. A
  // side without corners is dropped, or, if Grouped, which keeps all the sides, a group alone.
  template <bool Grouped, class Sides>
  VTK_ALWAYS_INLINE std::uint32_t InsertSide(Sides sides,
    MatchingLocalData<TId, TSideId>& localData, TId cellId, TSideId sideId, int sideDim,
    unsigned char cellType, vtkIdType npts, typename InputCellsT::PointsIterator pts,
    vtkIdType* ptIds, bool isCovering) const
  {
    // The faces of the 3D cells are their sides of the next lowest dimension, so that the rule of
    // the faces only is the same for all of them.
    SummaryStrategy rule =
      this->AlwaysWinding ? SummaryStrategy::Winding : SummaryStrategy::AnyOccurrence;
    if constexpr (!Faces)
    {
      if (!this->AlwaysWinding && !this->NeverWinding)
      {
        rule = sideDim + 1 == sides.GetDimension(cellType) ? SummaryStrategy::Winding
                                                           : SummaryStrategy::AnyOccurrence;
      }
    }
    // The sides are matched by their corners, which are their first points, then by their other
    // points, e.g., the mid-edge points of the faces of the quadratic cells.
    const vtkIdType* ids = nullptr;
    vtkIdType numCorners = 0;
    const vtkIdType numSidePts = sides.GetSidePoints(this->Cells.Input, cellId, cellType, sideDim,
      sideId, npts, pts, ptIds, localData.Scratch.SidePts, ids, numCorners, localData.Scratch.Cell);
    auto& matcher = localData.Matcher;
    const auto dimension = static_cast<std::uint8_t>(sideDim);
    if (numCorners <= 0)
    {
      if constexpr (Grouped)
      {
        return matcher.InsertUnmatchedSide(cellId, sideId, dimension);
      }
      else
      {
        return 0;
      }
    }
    const auto n = static_cast<int>(numCorners);
    // The number of the other points of a nonlinear side, if the sides are also matched by them,
    // see vtkUnstructuredGridSides::SetMatchSidePoints(), otherwise 0, the same for all of them.
    // The points of a linear side are its corners.
    int numOthers = 0;
    if constexpr (std::is_same_v<Sides, vtkCellTypeSides::NonLinearCellSides>)
    {
      if (this->Cells.MatchesSidePoints)
      {
        numOthers = static_cast<int>(numSidePts - numCorners);
      }
    }
    // The faces with the most and the fewest corners of the cells with static tables are
    // inserted with their number of corners known at compile time.
    if constexpr (Sides::MaximumFaceCorners > 0)
    {
      if (sideDim == 2 && n == Sides::MaximumFaceCorners)
      {
        return matcher.template InsertStaticSide<Sides::MaximumFaceCorners>(
          cellId, sideId, dimension, n, numOthers, ids, isCovering, rule);
      }
      if constexpr (Sides::MinimumFaceCorners != Sides::MaximumFaceCorners)
      {
        if (sideDim == 2 && n == Sides::MinimumFaceCorners)
        {
          return matcher.template InsertStaticSide<Sides::MinimumFaceCorners>(
            cellId, sideId, dimension, n, numOthers, ids, isCovering, rule);
        }
      }
    }
    return matcher.InsertDynamicSide(
      cellId, sideId, dimension, n, numOthers, ids, isCovering, rule);
  }

  // Match the sides of the keys [beginKeyId, endKeyId) of the output, and write the kept sides of
  // every key at its front, and their number to counts[keyId], or, if Grouped, all its sides by
  // group, flagging the first side of every group in groupStarts. The scratch data of the thread is
  // looked up once per range.
  template <bool Grouped, typename ElementIdType, typename ResultT>
  VTK_ALWAYS_INLINE void Match(SideOutput<TId, TSideId, InputCellsT>& output, TId beginKeyId,
    TId endKeyId, const ElementIdType* offsets, ResultT* results) const
  {
    TId* cellIds = output.CellIds.get();
    TSideId* sideIds = output.SideIds.get();
    std::uint8_t* dimensions = output.Dimensions.get();
    auto& localData = this->LocalData.Local();
    auto& matcher = localData.Matcher;
    matcher.SetMixedRules(!this->AlwaysWinding && !this->NeverWinding);
    vtkIdType ptIds[vtkCellTypeSides::MaximumCellSidePoints];
    const CellState* cellStates = this->Cells.CellStates.get();
    // The faces of the 3D cells only have the dimension 2, which is not stored.
    int singleSideDimension = 2;
    bool hasDimensions = false;
    if constexpr (!Faces)
    {
      singleSideDimension = this->SingleSideDimension;
      hasDimensions = output.HasDimensions;
    }
    for (TId keyId = beginKeyId; keyId < endKeyId; ++keyId)
    {
      const ElementIdType start = offsets[keyId];
      const ElementIdType stop = offsets[keyId + 1];
      if (start == stop)
      {
        if constexpr (!Grouped)
        {
          results[keyId] = 0;
        }
        continue;
      }
      for (ElementIdType i = start; i < stop; ++i)
      {
        const TId cellId = cellIds[i];
        const TSideId sideId = sideIds[i];
        const int sideDim = hasDimensions ? dimensions[i] : singleSideDimension;
        const unsigned char cellType = this->Cells.GetCellType(cellId);
        vtkIdType npts = 0;
        const auto pts = this->Cells.GetCellPoints(cellId, npts);
        // Of the masked cells, only the covering ones reach this point, see
        // InputCells::ClassifyCell().
        const bool isCovering = cellStates && cellStates[cellId] == Covering;
        std::uint32_t entry = 0;
        vtkCellTypeSides::DispatchCellSides(cellType,
          [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
          {
            entry = this->template InsertSide<Grouped>(
              sides, localData, cellId, sideId, sideDim, cellType, npts, pts, ptIds, isCovering);
          });
        if constexpr (Grouped)
        {
          matcher.Record(entry);
        }
      }
      // Write the kept sides of the key at the front of its sides, or all of them by group, and
      // reset the matcher.
      std::uint8_t* keyDimensions = hasDimensions ? dimensions + start : nullptr;
      if constexpr (Grouped)
      {
        matcher.PopulateGroups(cellIds + start, sideIds + start, keyDimensions, results + start);
      }
      else
      {
        results[keyId] = static_cast<ElementIdType>(
          matcher.PopulateSides(cellIds + start, sideIds + start, keyDimensions));
      }
      matcher.Reset();
    }
  }

  // The reduce functor, see vtk::keys::Reduce().
  template <typename ElementIdType>
  void operator()(SideOutput<TId, TSideId, InputCellsT>& output, TId beginKeyId, TId endKeyId,
    const ElementIdType* offsets, ElementIdType* counts) const
  {
    this->template Match<false>(output, beginKeyId, endKeyId, offsets, counts);
  }

  // The group functor, see vtk::keys::Group().
  template <typename ElementIdType>
  void operator()(SideOutput<TId, TSideId, InputCellsT>& output, TId beginKeyId, TId endKeyId,
    const ElementIdType* offsets, std::uint8_t* groupStarts) const
  {
    this->template Match<true>(output, beginKeyId, endKeyId, offsets, groupStarts);
  }
};

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
TId vtkUnstructuredGridSides<TId, TSideId>::GetNumberOfKeys() const
{
  // The keys of the sides, their smallest point ids, then the key id of the preserved cells. No key
  // is made if no side is found, e.g., if all the cells are masked or omit their sides.
  return this->ScatteredKeys.GetNumberOfKeys() + 1;
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetStorageOffsetOfKey(TId keyId) const
{
  assert(keyId >= 0 && keyId < this->GetNumberOfKeys() && "the key id must exist");
  return keyId < this->ScatteredKeys.GetNumberOfKeys() ? this->ScatteredKeys.GetOffset(keyId)
                                                       : this->ScatteredKeys.GetNumberOfElements();
}

//------------------------------------------------------------------------------
// Find the sides of the cells of the input, in the order of vtkUnstructuredGridSides::BuildSides(),
// whose cell arrays are dispatched once: their offsets and connectivity, read through InputCells.
template <typename TId, typename TSideId>
struct vtkUnstructuredGridSides<TId, TSideId>::BuildSidesWorker
  : public vtkCellArray::DispatchUtilities
{
  // Match the sides of every key, and keep them at the front of the sides of their key, given the
  // strategy, or group all of them, with AnyOccurrence, which removes no side.
  template <bool Faces, class InputCellsT>
  static void Match(const InputCellsT& cells, vtkUnstructuredGridSides* self,
    SideOutput<TId, TSideId, InputCellsT>& output, BuildMode mode)
  {
    if (mode == BuildMode::Grouped)
    {
      MatchingFunctor<InputCellsT, Faces> functor(cells, SummaryStrategy::AnyOccurrence);
      self->Groups = vtk::keys::Group(self->ScatteredKeys, output, functor);
    }
    else
    {
      MatchingFunctor<InputCellsT, Faces> functor(cells, self->Strategy);
      self->ReducedKeys = vtk::keys::Reduce(self->ScatteredKeys, output, functor);
      self->BuiltKeys = &self->ReducedKeys;
    }
  }

  template <class OffsetsT, class ConnectivityT>
  void operator()(OffsetsT* offsets, ConnectivityT* connectivity, vtkUnstructuredGridSides* self,
    BuildMode mode) const
  {
    using InputCellsT = InputCells<OffsetsT, ConnectivityT>;
    InputCellsT cells(self, offsets, connectivity);
    const auto flags = static_cast<unsigned int>(self->OutputDimensionControl);

    // The dimension of every distinct cell type, whether some cells can be preserved, and which
    // dimensions the sides to find have, given by the flags and the distinct cell types.
    std::array<bool, 3> hasSideDimension{};
    bool hasRenderableCells = false;
    bool hasOnlyRenderableCells = true;
    bool hasNonLinearCells = false;
    vtkUnsignedCharArray* distinctTypes = cells.Input->GetDistinctCellTypesArray();
    for (vtkIdType t = 0; t < distinctTypes->GetNumberOfValues(); ++t)
    {
      const unsigned char cellType = distinctTypes->GetValue(t);
      const int cellDim = vtkCellTypeUtilities::GetDimension(cellType);
      const bool isRenderable = cellDim < 3 && cellType != VTK_EMPTY_CELL;
      cells.TypeDimensions[cellType] = static_cast<std::int8_t>(cellDim);
      hasRenderableCells |= isRenderable;
      hasOnlyRenderableCells &= isRenderable;
      hasNonLinearCells |= !vtkCellTypeUtilities::IsLinear(cellType);
      for (int sideDim = 0; sideDim < cellDim; ++sideDim)
      {
        hasSideDimension[sideDim] |= (flags & GetSideFlag(cellDim, sideDim)) != 0U;
      }
    }
    // The dimensions of the sides are stored only if they have several ones.
    int numSideDimensions = 0;
    int sideDimension = 0;
    for (int sideDim = 0; sideDim < 3; ++sideDim)
    {
      if (hasSideDimension[sideDim])
      {
        ++numSideDimensions;
        sideDimension = sideDim;
      }
    }
    cells.HasDimensions = numSideDimensions > 1;
    cells.SingleSideDimension = cells.HasDimensions ? -1 : sideDimension;
    self->SideDimension = static_cast<std::uint8_t>(sideDimension);
    // The points of a linear side are its corners, which are matched: the other points are compared
    // only with nonlinear cells.
    cells.MatchesSidePoints = self->MatchSidePoints && hasNonLinearCells;
    const auto numPoints = static_cast<TId>(cells.Input->GetNumberOfPoints());

    // The states of the cells, with clipping or ghost cells.
    cells.ClassifyCells();

    PreservedCells preserved;
    if (self->PreserveRenderableInputs && hasRenderableCells)
    {
      const bool allPreserved = hasOnlyRenderableCells && !cells.CellStates;
      self->NumberOfPreservedCells = preserved.Count(cells, allPreserved);
    }
    // The sides are scattered to their arrays, with room for the preserved cells after them.
    const bool findSides = numSideDimensions > 0 && numPoints > 0;
    SideOutput<TId, TSideId, InputCellsT> output{
      self->CellIds,
      self->SideIds,
      self->Dimensions,
      cells.HasDimensions,
      self->NumberOfPreservedCells,
      cells,
      flags,
      {},
    };
    if (findSides)
    {
      // Scatter the sides by their key, their smallest point id. Their keys are only kept until
      // they are scattered.
      const SideMapper<InputCellsT> mapper(cells);
      self->ScatteredKeys =
        vtk::keys::ScatterDense(mapper, static_cast<TId>(0), numPoints - 1, output);
      SortSidesByCell sortSides;
      vtk::keys::ForEachKeyRange(self->ScatteredKeys, output, sortSides);
    }
    else
    {
      output.SetNumberOfElements(0);
    }
    // The preserved cells, in the last key id, after the found sides, see GetNumberOfKeys().
    if (self->NumberOfPreservedCells > 0)
    {
      const vtkIdType offset = self->GetStorageOffsetOfKey(self->GetNumberOfKeys() - 1);
      preserved.Write(cells, self->CellIds.get() + offset, self->SideIds.get() + offset);
    }
    if (!findSides || mode == BuildMode::Scattered)
    {
      return;
    }
    // Match the sides of every key, and reduce or group them: the faces of the 3D cells only with
    // the dimension of their sides known at compile time, and a single rule, the other sides
    // generically.
    if (cells.IsFacesOnly())
    {
      Match<true>(cells, self, output, mode);
    }
    else
    {
      Match<false>(cells, self, output, mode);
    }
  }
};

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkUnstructuredGridSides<TId, TSideId>* vtkUnstructuredGridSides<TId, TSideId>::New()
{
  VTK_STANDARD_NEW_BODY(vtkUnstructuredGridSides);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkUnstructuredGridSides<TId, TSideId>::vtkUnstructuredGridSides() = default;

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkUnstructuredGridSides<TId, TSideId>::~vtkUnstructuredGridSides()
{
  this->SetDataSet(nullptr);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::SetDataSet(vtkUnstructuredGrid* dataSet)
{
  vtkSetObjectBodyMacro(DataSet, vtkUnstructuredGrid, dataSet);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::BuildScatteredSides()
{
  this->BuildSides(BuildMode::Scattered);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::BuildReducedSides()
{
  this->BuildSides(BuildMode::Reduced);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::BuildGroupedSides()
{
  this->BuildSides(BuildMode::Grouped);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::BuildSides(BuildMode mode)
{
  static_assert(std::is_same_v<TId, vtkTypeInt32> || std::is_same_v<TId, vtkTypeInt64>,
    "TId must be vtkTypeInt32 or vtkTypeInt64");
  static_assert(std::is_same_v<TSideId, vtkTypeInt8> || std::is_same_v<TSideId, vtkTypeInt32>,
    "TSideId must be vtkTypeInt8 or vtkTypeInt32");
  // The sides are built again if this object or the dataset changed since they were built, or to
  // build them in another way. The build time is 0 if they were not built, see Reset().
  const vtkMTimeType dataSetTime = this->DataSet ? this->DataSet->GetMTime() : 0;
  if (this->Mode == mode && this->BuildTime > this->GetMTime() && this->BuildTime > dataSetTime)
  {
    return;
  }
  this->Reset();
  this->Mode = mode;
  this->BuildTime.Modified();
  vtkUnstructuredGrid* input = this->DataSet;
  if (!input || input->GetNumberOfCells() == 0)
  {
    return;
  }
  // The sides are found in a single dispatch on the cell arrays of the input, read directly.
  input->GetCells()->Dispatch(BuildSidesWorker(), this, mode);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::Reset()
{
  this->ScatteredKeys.Reset();
  this->ReducedKeys.Reset();
  this->Groups.Groups.Reset();
  this->Groups.Keys.Reset();
  this->BuiltKeys = &this->ScatteredKeys;
  this->Mode = BuildMode::None;
  this->BuildTime = vtkTimeStamp();
  this->CellIds.reset();
  this->SideIds.reset();
  this->Dimensions.reset();
  this->SideDimension = 0;
  this->NumberOfPreservedCells = 0;
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetNumberOfSides() const
{
  return this->BuiltKeys->GetNumberOfElements() + this->NumberOfPreservedCells;
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetNumberOfSidesOfKey(TId keyId) const
{
  assert(keyId >= 0 && keyId < this->GetNumberOfKeys() && "the key id must exist");
  if (VTK_LIKELY(keyId < this->ScatteredKeys.GetNumberOfKeys()))
  {
    return this->BuiltKeys->GetNumberOfElementsOfKey(keyId);
  }
  else
  {
    // The last key id, of the preserved cells.
    return this->NumberOfPreservedCells;
  }
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetSideOffsetOfKey(TId keyId) const
{
  assert(keyId >= 0 && keyId <= this->GetNumberOfKeys() && "the key id must exist, or be the end");
  if (VTK_LIKELY(keyId < this->ScatteredKeys.GetNumberOfKeys()))
  {
    return this->BuiltKeys->GetOffset(keyId);
  }
  else
  {
    const vtkIdType numSidesOfKeys = this->BuiltKeys->GetNumberOfElements();
    return keyId < this->GetNumberOfKeys() ? numSidesOfKeys
                                           : numSidesOfKeys + this->NumberOfPreservedCells;
  }
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
const TId* vtkUnstructuredGridSides<TId, TSideId>::GetCellIdsOfKey(TId keyId) const
{
  return this->CellIds.get() + this->GetStorageOffsetOfKey(keyId);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
const TSideId* vtkUnstructuredGridSides<TId, TSideId>::GetSideIdsOfKey(TId keyId) const
{
  return this->SideIds.get() + this->GetStorageOffsetOfKey(keyId);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
TId vtkUnstructuredGridSides<TId, TSideId>::GetCellIdOfSide(TId keyId, vtkIdType k) const
{
  assert(k >= 0 && k < this->GetNumberOfSidesOfKey(keyId) && "the side must exist");
  return this->GetCellIdsOfKey(keyId)[k];
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
TSideId vtkUnstructuredGridSides<TId, TSideId>::GetSideIdOfSide(TId keyId, vtkIdType k) const
{
  assert(k >= 0 && k < this->GetNumberOfSidesOfKey(keyId) && "the side must exist");
  return this->GetSideIdsOfKey(keyId)[k];
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
int vtkUnstructuredGridSides<TId, TSideId>::GetDimensionOfSide(TId keyId, vtkIdType k) const
{
  assert(k >= 0 && k < this->GetNumberOfSidesOfKey(keyId) && "the side must exist");
  if (VTK_LIKELY(keyId < this->ScatteredKeys.GetNumberOfKeys()))
  {
    return this->Dimensions ? this->Dimensions[this->GetStorageOffsetOfKey(keyId) + k]
                            : this->SideDimension;
  }
  else
  {
    // A preserved cell has the dimension of its type.
    return vtkCellTypeUtilities::GetDimension(
      static_cast<unsigned char>(this->DataSet->GetCellType(this->GetCellIdOfSide(keyId, k))));
  }
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
typename vtkUnstructuredGridSides<TId, TSideId>::SideInformation
vtkUnstructuredGridSides<TId, TSideId>::GetSideInformation(
  TId keyId, vtkIdType k, vtkGenericCell* tempCell) const
{
  if (VTK_UNLIKELY(!this->DataSet))
  {
    return SideInformation{ -1, VTK_EMPTY_CELL, -1, VTK_EMPTY_CELL, 0, 0 };
  }
  assert(k >= 0 && k < this->GetNumberOfSidesOfKey(keyId) && "the side must exist");
  // The position of the side in the arrays, whose key offset is computed once.
  const vtkIdType position = this->GetStorageOffsetOfKey(keyId) + k;
  SideInformation side{};
  side.CellId = this->CellIds[position];
  side.CellType = static_cast<unsigned char>(this->DataSet->GetCellType(side.CellId));
  side.SideId = static_cast<int>(this->SideIds[position]);
  if (VTK_LIKELY(side.SideId >= 0))
  {
    // A found side, not a preserved cell. The dimension first, which gives the type and the size.
    side.SideDimension = this->Dimensions ? this->Dimensions[position] : this->SideDimension;
    side.SideType =
      vtkCellTypeSides::GetCellSideType(side.CellType, side.SideDimension, side.SideId);
    side.SideSize = vtkCellTypeSides::GetCellSideSize(
      this->DataSet, side.CellId, side.CellType, side.SideDimension, side.SideId, tempCell);
  }
  else
  {
    // A preserved cell.
    side.SideType = side.CellType;
    side.SideDimension = vtkCellTypeUtilities::GetDimension(side.CellType);
    side.SideSize = static_cast<int>(this->DataSet->GetCellSize(side.CellId));
  }
  return side;
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::GetSidePoints(TId keyId, vtkIdType k,
  vtkIdList* sidePtIds, unsigned char& cellType, vtkIdList* tempCellPointIds,
  vtkGenericCell* tempCell) const
{
  sidePtIds->Reset();
  if (VTK_UNLIKELY(!this->DataSet))
  {
    cellType = VTK_EMPTY_CELL;
    return;
  }
  assert(k >= 0 && k < this->GetNumberOfSidesOfKey(keyId) && "the side must exist");
  // The position of the side in the arrays, whose key offset is computed once.
  const vtkIdType position = this->GetStorageOffsetOfKey(keyId) + k;
  const vtkIdType cellId = this->CellIds[position];
  const int sideId = static_cast<int>(this->SideIds[position]);
  cellType = static_cast<unsigned char>(this->DataSet->GetCellType(cellId));
  vtkIdType npts = 0;
  const vtkIdType* pts = nullptr;
  this->DataSet->GetCellPoints(cellId, npts, pts, tempCellPointIds);
  if (VTK_LIKELY(sideId >= 0))
  {
    // A found side, not a preserved cell.
    const int sideDim = this->Dimensions ? this->Dimensions[position] : this->SideDimension;
    vtkCellTypeSides::GetCellSidePoints(
      this->DataSet, cellId, cellType, sideDim, sideId, npts, pts, sidePtIds, tempCell);
  }
  else
  {
    // A preserved cell: its points.
    sidePtIds->SetNumberOfIds(npts);
    std::copy_n(pts, npts, sidePtIds->GetPointer(0));
  }
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetNumberOfGroups() const
{
  return this->Groups.Groups.GetNumberOfGroups();
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetGroupOffsetOfKey(TId keyId) const
{
  assert(keyId >= 0 && keyId <= this->GetNumberOfKeys() && "the key id must exist, or be the end");
  // The groups of the keys of the sides, which are none if the sides are not grouped, then the
  // preserved cells, which are in no group.
  return keyId < this->Groups.Keys.GetNumberOfKeys() ? this->Groups.Keys.GetOffset(keyId)
                                                     : this->GetNumberOfGroups();
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetNumberOfSidesOfGroup(vtkIdType group) const
{
  assert(group >= 0 && group < this->GetNumberOfGroups() && "the group must exist");
  return this->Groups.Groups.GetGroupSize(group);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
vtkIdType vtkUnstructuredGridSides<TId, TSideId>::GetSideOffsetOfGroup(vtkIdType group) const
{
  assert(group >= 0 && group <= this->GetNumberOfGroups() && "the group must exist, or be the end");
  // The grouped sides keep all the sides, so that their positions in the arrays, without gaps
  // between the keys, are their positions among all the sides.
  return this->Groups.Groups.GetGroupOffset(group);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
const TId* vtkUnstructuredGridSides<TId, TSideId>::GetCellIdsOfGroup(vtkIdType group) const
{
  assert(group >= 0 && group < this->GetNumberOfGroups() && "the group must exist");
  return this->CellIds.get() + this->GetSideOffsetOfGroup(group);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
const TSideId* vtkUnstructuredGridSides<TId, TSideId>::GetSideIdsOfGroup(vtkIdType group) const
{
  assert(group >= 0 && group < this->GetNumberOfGroups() && "the group must exist");
  return this->SideIds.get() + this->GetSideOffsetOfGroup(group);
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
int vtkUnstructuredGridSides<TId, TSideId>::GetDimensionOfGroup(vtkIdType group) const
{
  assert(group >= 0 && group < this->GetNumberOfGroups() && "the group must exist");
  // The dimension of its first side.
  return this->Dimensions ? this->Dimensions[this->GetSideOffsetOfGroup(group)]
                          : this->SideDimension;
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}

//------------------------------------------------------------------------------
template <typename TId, typename TSideId>
void vtkUnstructuredGridSides<TId, TSideId>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataSet: " << this->DataSet << "\n";
  os << indent << "OutputDimensionControl: " << this->OutputDimensionControl << "\n";
  os << indent << "Strategy: " << this->Strategy << "\n";
  os << indent << "PreserveRenderableInputs: " << this->PreserveRenderableInputs << "\n";
  os << indent << "OmitSidesForRenderableInputs: " << this->OmitSidesForRenderableInputs << "\n";
  os << indent << "PointClipping: " << this->PointClipping << "\n";
  os << indent << "CellClipping: " << this->CellClipping << "\n";
  os << indent << "ExtentClipping: " << this->ExtentClipping << "\n";
  os << indent << "PointMinimum: " << this->PointMinimum << "\n";
  os << indent << "PointMaximum: " << this->PointMaximum << "\n";
  os << indent << "CellMinimum: " << this->CellMinimum << "\n";
  os << indent << "CellMaximum: " << this->CellMaximum << "\n";
  os << indent << "Extent: (" << this->Extent[0] << ", " << this->Extent[1] << ", "
     << this->Extent[2] << ", " << this->Extent[3] << ", " << this->Extent[4] << ", "
     << this->Extent[5] << ")\n";
  os << indent << "RemoveGhostInterfaces: " << this->RemoveGhostInterfaces << "\n";
  os << indent << "MaskedGhostTypes: " << static_cast<int>(this->MaskedGhostTypes) << "\n";
  os << indent << "MatchSidePoints: " << this->MatchSidePoints << "\n";
  os << indent << "BuildMode: " << static_cast<int>(this->Mode) << "\n";
  os << indent << "NumberOfSides: " << this->GetNumberOfSides() << "\n";
  os << indent << "NumberOfKeys: " << this->GetNumberOfKeys() << "\n";
  os << indent << "NumberOfGroups: " << this->GetNumberOfGroups() << "\n";
  os << indent << "BuildTime: " << this->BuildTime.GetMTime() << "\n";
}

VTK_ABI_NAMESPACE_END

VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATE(vtkTypeInt32, vtkTypeInt8)
VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATE(vtkTypeInt32, vtkTypeInt32)
VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATE(vtkTypeInt64, vtkTypeInt8)
VTK_UNSTRUCTURED_GRID_SIDES_INSTANTIATE(vtkTypeInt64, vtkTypeInt32)
