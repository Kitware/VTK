// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStaticCellLinksTemplate.h"

#ifndef vtkStaticCellLinksTemplate_txx
#define vtkStaticCellLinksTemplate_txx

#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkUnstructuredGrid.h"
#include <array>
#include <atomic>

#include <type_traits>

//----------------------------------------------------------------------------
// Note: this class is a faster, threaded version of vtkCellLinks. It uses
// vtkSMPTools and std::atomic.

//----------------------------------------------------------------------------
// Default constructor. BuildLinks() does most of the work.
VTK_ABI_NAMESPACE_BEGIN
template <typename TIds>
vtkStaticCellLinksTemplate<TIds>::vtkStaticCellLinksTemplate()
  : LinksSize(0)
  , NumPts(0)
  , NumCells(0)
  , LinkSharedPtr(nullptr)
  , Links(nullptr)
  , OffsetsSharedPtr(nullptr)
  , Offsets(nullptr)
{
  if (std::is_same<unsigned short, TIds>::value)
  {
    this->Type = vtkAbstractCellLinks::STATIC_CELL_LINKS_USHORT;
  }

  else if (std::is_same<unsigned int, TIds>::value)
  {
    this->Type = vtkAbstractCellLinks::STATIC_CELL_LINKS_UINT;
  }

  else if (std::is_same<vtkIdType, TIds>::value)
  {
    this->Type = vtkAbstractCellLinks::STATIC_CELL_LINKS_IDTYPE;
  }

  else
  {
    this->Type = vtkAbstractCellLinks::STATIC_CELL_LINKS_SPECIALIZED;
  }

  this->SequentialProcessing = false;
}

//----------------------------------------------------------------------------
template <typename TIds>
vtkStaticCellLinksTemplate<TIds>::~vtkStaticCellLinksTemplate()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
// Clean up any previously allocated memory
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::Initialize()
{
  if (this->Links)
  {
    // this->LinkSharedPtr will be reset by the destructor
    this->Links = nullptr;
  }
  if (this->Offsets)
  {
    // this->OffsetsSharedPtr will be reset by the destructor
    this->Offsets = nullptr;
  }
}

//----------------------------------------------------------------------------
// Build the link list array for any dataset type. Specialized methods are
// used for dataset types that use vtkCellArrays to represent cells.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkDataSet* ds)
{
  this->Initialize();
  // Use a fast path if polydata or unstructured grid
  if (ds->GetDataObjectType() == VTK_POLY_DATA)
  {
    return this->BuildLinks(static_cast<vtkPolyData*>(ds));
  }

  else if (ds->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
  {
    return this->BuildLinks(static_cast<vtkUnstructuredGrid*>(ds));
  }

  else if (ds->GetDataObjectType() == VTK_EXPLICIT_STRUCTURED_GRID)
  {
    return this->BuildLinks(static_cast<vtkExplicitStructuredGrid*>(ds));
  }

  // Any other type of dataset. Generally this is not called as datasets have
  // their own, more efficient ways of getting similar information.
  // Make sure that we clear out previous allocation.
  this->NumCells = ds->GetNumberOfCells();
  this->NumPts = ds->GetNumberOfPoints();

  vtkIdType npts, ptId;
  vtkIdType cellId, j;
  vtkIdList* cellPts = vtkIdList::New();

  // Traverse data to determine number of uses of each point. Also count the
  // number of links to allocate.
  this->OffsetsSharedPtr.reset(new TIds[this->NumPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  vtkSMPTools::Fill(this->Offsets, this->Offsets + this->NumPts + 1, 0);

  for (this->LinksSize = 0, cellId = 0; cellId < this->NumCells; cellId++)
  {
    ds->GetCellPoints(cellId, cellPts);
    npts = cellPts->GetNumberOfIds();
    for (j = 0; j < npts; j++)
    {
      this->Offsets[cellPts->GetId(j)]++;
      this->LinksSize++;
    }
  }

  // Allocate space for links. Perform prefix sum.
  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  this->Links[this->LinksSize] = this->NumPts;

  for (ptId = 0; ptId < this->NumPts; ++ptId)
  {
    npts = this->Offsets[ptId + 1];
    this->Offsets[ptId + 1] = this->Offsets[ptId] + npts;
  }

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for (cellId = 0; cellId < this->NumCells; ++cellId)
  {
    ds->GetCellPoints(cellId, cellPts);
    npts = cellPts->GetNumberOfIds();
    for (j = 0; j < npts; ++j)
    {
      ptId = cellPts->GetId(j);
      this->Offsets[ptId]--;
      this->Links[this->Offsets[ptId]] = cellId;
    }
  }
  this->Offsets[this->NumPts] = this->LinksSize;

  cellPts->Delete();
}
VTK_ABI_NAMESPACE_END

namespace vtkSCLT_detail
{
VTK_ABI_NAMESPACE_BEGIN

struct CountPoints
{
  template <typename CellStateT, typename TIds>
  void operator()(CellStateT& state, TIds* linkOffsets)
  {
    using ValueType = typename CellStateT::ValueType;
    const auto cellConnectivity = vtk::DataArrayValueRange<1>(state.GetConnectivity());

    // Count number of point uses
    for (const ValueType ptId : cellConnectivity)
    {
      ++linkOffsets[ptId];
    }
  }
};

struct CountPointsThreaded
{
  template <typename CellStateT, typename TIds>
  void operator()(
    CellStateT& state, std::atomic<TIds>* linkOffsets, vtkIdType beginCellId, vtkIdType endCellId)
  {
    using ValueType = typename CellStateT::ValueType;
    const vtkIdType connBeginId = state.GetBeginOffset(beginCellId);
    const vtkIdType connEndId = state.GetEndOffset(endCellId - 1);
    auto connRange = vtk::DataArrayValueRange<1>(state.GetConnectivity(), connBeginId, connEndId);

    // Count number of point uses
    for (const ValueType ptId : connRange)
    {
      // memory_order_relaxed is safe here, since we're not using the atomics for synchronization.
      linkOffsets[ptId].fetch_add(1, std::memory_order_relaxed);
    }
  }
};

// Serial version:
struct BuildLinks
{
  template <typename CellStateT, typename TIds>
  void operator()(CellStateT& state, TIds* linkOffsets, TIds* links, vtkIdType idOffset = 0)
  {
    using ValueType = typename CellStateT::ValueType;

    const vtkIdType numCells = state.GetNumberOfCells();

    const auto cellConnectivity = vtk::DataArrayValueRange<1>(state.GetConnectivity());
    const auto cellOffsets = vtk::DataArrayValueRange<1>(state.GetOffsets());
    // Now build the links. The summation from the prefix sum indicates where
    // the cells are to be inserted. Each time a cell is inserted, the offset
    // is decremented. In the end, the offset array is also constructed as it
    // points to the beginning of each cell run.
    ValueType ptIdOffset;
    size_t ptId;
    for (vtkIdType cellId = 0; cellId < numCells; ++cellId)
    {
      for (ptIdOffset = cellOffsets[cellId]; ptIdOffset < cellOffsets[cellId + 1]; ++ptIdOffset)
      {
        ptId = static_cast<size_t>(cellConnectivity[ptIdOffset]);
        --linkOffsets[ptId];
        links[linkOffsets[ptId]] = static_cast<TIds>(idOffset + cellId);
      }
    }
  }
};

// Parallel version:
struct BuildLinksThreaded
{
  template <typename CellStateT, typename TIds>
  void operator()(CellStateT& state, const TIds* offsets, std::atomic<TIds>* counts, TIds* links,
    vtkIdType beginCellId, vtkIdType endCellId, const TIds idOffset = 0)
  {
    using ValueType = typename CellStateT::ValueType;

    const auto cellConnectivity = vtk::DataArrayValueRange<1>(state.GetConnectivity());
    const auto cellOffsets = vtk::DataArrayValueRange<1>(state.GetOffsets());
    // Now build the links. The summation from the prefix sum indicates where
    // the cells are to be inserted. Each time a cell is inserted, the offset
    // is decremented. In the end, the offset array is also constructed as it
    // points to the beginning of each cell run.
    ValueType ptIdOffset;
    size_t ptId;
    TIds offset;
    for (vtkIdType cellId = beginCellId; cellId < endCellId; ++cellId)
    {
      for (ptIdOffset = cellOffsets[cellId]; ptIdOffset < cellOffsets[cellId + 1]; ++ptIdOffset)
      {
        ptId = static_cast<size_t>(cellConnectivity[ptIdOffset]);
        // memory_order_relaxed is safe here, since we're not using the atomics for synchronization.
        offset = offsets[ptId] + counts[ptId].fetch_sub(1, std::memory_order_relaxed) - 1;
        links[offset] = idOffset + cellId;
      }
    }
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace vtkSCLT_detail

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
// Build the link list array for unstructured grids. Note this is a serial
// implementation: while there is another method (threaded) that is usually
// much faster, in certain pathological situations the serial version can be
// faster.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::SerialBuildLinksFromMultipleArrays(
  vtkIdType numPts, vtkIdType numCells, const std::vector<vtkCellArray*> cellArrays)
{
  // Basic information about the grid
  this->NumPts = numPts;
  this->NumCells = numCells;

  // compute links size
  this->LinksSize = 0;
  for (const vtkCellArray* cellArray : cellArrays)
  {
    this->LinksSize += cellArray->GetNumberOfConnectivityIds();
  }
  // compute offsets of number of cells
  std::vector<vtkIdType> offsets(cellArrays.size(), 0);
  for (size_t i = 1; i < cellArrays.size(); ++i)
  {
    offsets[i] = cellArrays[i - 1]->GetNumberOfCells() + offsets[i - 1];
  }

  // Extra one allocated to simplify later pointer manipulation
  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  this->Links[this->LinksSize] = this->NumPts;
  this->OffsetsSharedPtr.reset(new TIds[this->NumPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  vtkSMPTools::Fill(this->Offsets, this->Offsets + this->NumPts + 1, 0);

  // Count how many cells each point appears in:
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    cellArrays[i]->Visit(vtkSCLT_detail::CountPoints{}, this->Offsets);
  }

  // Perform prefix sum (inclusive scan)
  for (vtkIdType ptId = 0; ptId < this->NumPts; ++ptId)
  {
    const vtkIdType npts = this->Offsets[ptId + 1];
    this->Offsets[ptId + 1] = this->Offsets[ptId] + npts;
  }

  // Construct the links table and finalize the offsets:
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    cellArrays[i]->Visit(vtkSCLT_detail::BuildLinks{}, this->Offsets, this->Links, offsets[i]);
  }

  this->Offsets[numPts] = this->LinksSize;
}
VTK_ABI_NAMESPACE_END

//----------------------------------------------------------------------------
// Threaded implementation of BuildLinks() using vtkSMPTools and std::atomic.

namespace
{ // anonymous

template <typename TIds>
struct CountUses
{
  vtkCellArray* CellArray;
  std::atomic<TIds>* Counts;

  CountUses(vtkCellArray* cellArray, std::atomic<TIds>* counts)
    : CellArray(cellArray)
    , Counts(counts)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    this->CellArray->Visit(vtkSCLT_detail::CountPointsThreaded{}, this->Counts, cellId, endCellId);
  }
};

template <typename TIds>
struct InsertLinks
{
  vtkCellArray* CellArray;
  std::atomic<TIds>* Counts;
  const TIds* Offsets;
  TIds* Links;
  TIds IdOffset;

  InsertLinks(vtkCellArray* cellArray, std::atomic<TIds>* counts, const TIds* offsets, TIds* links,
    TIds idOffset)
    : CellArray(cellArray)
    , Counts(counts)
    , Offsets(offsets)
    , Links(links)
    , IdOffset(idOffset)
  {
  }

  void operator()(vtkIdType cellId, vtkIdType endCellId)
  {
    this->CellArray->Visit(vtkSCLT_detail::BuildLinksThreaded{}, this->Offsets, this->Counts,
      this->Links, cellId, endCellId, this->IdOffset);
  }
};

} // anonymous

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
// Build the link list array for unstructured grids. Note this is a threaded
// implementation: it uses SMPTools and atomics to prevent race situations.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::ThreadedBuildLinksFromMultipleArrays(
  vtkIdType numPts, vtkIdType numCells, const std::vector<vtkCellArray*> cellArrays)
{
  // Basic information about the grid
  this->NumPts = numPts;
  this->NumCells = numCells;

  // compute links size
  this->LinksSize = 0;
  for (const vtkCellArray* cellArray : cellArrays)
  {
    this->LinksSize += cellArray->GetNumberOfConnectivityIds();
  }
  // compute offsets of number of cells
  std::vector<vtkIdType> offsets(cellArrays.size(), 0);
  for (size_t i = 1; i < cellArrays.size(); ++i)
  {
    offsets[i] = cellArrays[i - 1]->GetNumberOfCells() + offsets[i - 1];
  }

  // Extra one allocated to simplify later pointer manipulation
  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  this->Links[this->LinksSize] = this->NumPts;

  // Create an array of atomics with initial count=0. This will keep
  // track of point uses. Count them in parallel.
  std::atomic<TIds>* counts = new std::atomic<TIds>[numPts]();
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    CountUses<TIds> count(cellArrays[i], counts);
    vtkSMPTools::For(0, cellArrays[i]->GetNumberOfCells(), count);
  }

  // Perform prefix sum to determine offsets
  vtkIdType ptId, npts;
  this->OffsetsSharedPtr.reset(new TIds[numPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  this->Offsets[0] = 0;
  for (ptId = 1; ptId < numPts; ++ptId)
  {
    npts = counts[ptId - 1].load(std::memory_order_relaxed);
    this->Offsets[ptId] = this->Offsets[ptId - 1] + npts;
  }
  this->Offsets[numPts] = this->LinksSize;

  // Now insert cell ids into cell links.
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    InsertLinks<TIds> insertLinks(cellArrays[i], counts, this->Offsets, this->Links, offsets[i]);
    vtkSMPTools::For(0, cellArrays[i]->GetNumberOfCells(), insertLinks);
  }

  // Clean up
  delete[] counts;
}

//----------------------------------------------------------------------------
// Build the link list array for unstructured grids
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkUnstructuredGrid* ugrid)
{
  // Basic information about the grid
  vtkIdType numPts = ugrid->GetNumberOfPoints();
  vtkIdType numCells = ugrid->GetNumberOfCells();

  // We're going to get into the guts of the class
  vtkCellArray* cellArray = ugrid->GetCells();

  // Use serial or threaded implementations
  if (!this->SequentialProcessing)
  {
    this->ThreadedBuildLinks(numPts, numCells, cellArray);
  }
  else
  {
    this->SerialBuildLinks(numPts, numCells, cellArray);
  }
}

//----------------------------------------------------------------------------
// Build the link list array for unstructured grids
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkExplicitStructuredGrid* esgrid)
{
  // Basic information about the grid
  vtkIdType numPts = esgrid->GetNumberOfPoints();
  vtkIdType numCells = esgrid->GetNumberOfCells();

  // We're going to get into the guts of the class
  vtkCellArray* cellArray = esgrid->GetCells();

  // Use serial or threaded implementations
  if (!this->SequentialProcessing)
  {
    this->ThreadedBuildLinks(numPts, numCells, cellArray);
  }
  else
  {
    this->SerialBuildLinks(numPts, numCells, cellArray);
  }
}

//----------------------------------------------------------------------------
// Build the link list array for poly data. This is more complex because there
// are potentially four different cell arrays to contend with.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkPolyData* pd)
{
  // Basic information about the grid
  vtkIdType numPts = pd->GetNumberOfPoints();
  vtkIdType numCells = pd->GetNumberOfCells();

  std::vector<vtkCellArray*> cellArrays = { pd->GetVerts(), pd->GetLines(), pd->GetPolys(),
    pd->GetStrips() };
  // Remove any null cell arrays
  cellArrays.erase(std::remove(cellArrays.begin(), cellArrays.end(), nullptr), cellArrays.end());
  if (!this->SequentialProcessing)
  {
    this->ThreadedBuildLinksFromMultipleArrays(numPts, numCells, cellArrays);
  }
  else
  {
    this->SerialBuildLinksFromMultipleArrays(numPts, numCells, cellArrays);
  }
}

//----------------------------------------------------------------------------
// Indicate whether the point ids provided form part of at least one cell.
template <typename TIds>
template <typename TGivenIds>
bool vtkStaticCellLinksTemplate<TIds>::MatchesCell(TGivenIds npts, const TGivenIds* pts)
{
  // Find the shortest cell links list.
  int minList = 0;
  vtkIdType minNumCells = VTK_INT_MAX;
  TIds numCells;
  for (auto i = 0; i < npts; ++i)
  {
    numCells = this->GetNcells(pts[i]);
    if (numCells < minNumCells)
    {
      minList = i;
      minNumCells = numCells;
    }
  }

  // Process the cells in the shortest list
  auto shortCells = this->GetCells(pts[minList]);
  for (auto j = 0; j < minNumCells; ++j)
  {
    bool foundCell = true;
    auto cellId = shortCells[j];
    // Loop over all cell lists looking for this cellId
    for (auto i = 0; i < npts && foundCell; ++i)
    {
      if (i != minList)
      {
        numCells = this->GetNcells(pts[i]);
        auto linkedCells = this->GetCells(pts[i]);
        vtkIdType k;
        for (k = 0; k < numCells; ++k)
        {
          if (linkedCells[k] == cellId)
          {
            break; // we matched cell
          }
        }
        foundCell = (k >= numCells ? false : foundCell);
      } // search for cell in each list
    }   // for all cell lists

    if (foundCell)
    {
      return true;
    }
  } // for all cells in the shortest list

  return false;
}

//----------------------------------------------------------------------------
// Given some point ids, return the cells that use these points in the
// provided id list.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::GetCells(
  vtkIdType npts, const vtkIdType* pts, vtkIdList* cells)
{
  // Initialize to no uses.
  cells->Reset();

  // Find the shortest cell links list.
  int minList = 0;
  vtkIdType minNumCells = VTK_INT_MAX;
  TIds numCells;
  for (auto i = 0; i < npts; ++i)
  {
    numCells = this->GetNcells(pts[i]);
    if (numCells < minNumCells)
    {
      minList = i;
      minNumCells = numCells;
    }
  }

  // Process the cells in the shortest list
  auto shortCells = this->GetCells(pts[minList]);
  for (auto j = 0; j < minNumCells; ++j)
  {
    bool foundCell = true;
    auto cellId = shortCells[j];
    // Loop over all cell lists looking for this cellId
    for (auto i = 0; i < npts && foundCell; ++i)
    {
      if (i != minList)
      {
        numCells = this->GetNcells(pts[i]);
        auto linkedCells = this->GetCells(pts[i]);
        vtkIdType k;
        for (k = 0; k < numCells; ++k)
        {
          if (linkedCells[k] == cellId)
          {
            break; // we matched cell
          }
        }
        foundCell = (k >= numCells ? false : foundCell);
      } // search for cell in each list
    }   // for all cell lists

    if (foundCell)
    {
      cells->InsertNextId(cellId);
    }
  } // for all cells in the shortest list
}

//----------------------------------------------------------------------------
// Satisfy vtkAbstractCellLinks API
template <typename TIds>
unsigned long vtkStaticCellLinksTemplate<TIds>::GetActualMemorySize()
{
  unsigned long total = 0;
  if (this->Links != nullptr)
  {
    total = static_cast<unsigned long>((this->LinksSize + 1) * sizeof(TIds));
    total += static_cast<unsigned long>((this->NumPts + 1) * sizeof(TIds));
  }
  return total;
}

//----------------------------------------------------------------------------
// Satisfy vtkAbstractCellLinks API
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::DeepCopy(vtkStaticCellLinksTemplate* links)
{
  if (!links)
  {
    return;
  }
  this->LinksSize = links->LinksSize;
  this->NumPts = links->NumPts;
  this->NumCells = links->NumCells;

  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  vtkSMPTools::For(0, this->LinksSize + 1,
    [&](vtkIdType beginLink, vtkIdType endLink)
    { std::copy(links->Links + beginLink, links->Links + endLink, this->Links + beginLink); });
  this->OffsetsSharedPtr.reset(new TIds[this->NumPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  vtkSMPTools::For(0, this->NumPts + 1,
    [&](vtkIdType beginPoint, vtkIdType endPoint) {
      std::copy(links->Offsets + beginPoint, links->Offsets + endPoint, this->Offsets + beginPoint);
    });
}

//----------------------------------------------------------------------------
// Satisfy vtkAbstractCellLinks API
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::ShallowCopy(vtkStaticCellLinksTemplate* links)
{
  if (!links)
  {
    return;
  }
  this->LinksSize = links->LinksSize;
  this->NumPts = links->NumPts;
  this->NumCells = links->NumCells;

  this->LinkSharedPtr = links->LinkSharedPtr;
  this->Links = this->LinkSharedPtr.get();
  this->OffsetsSharedPtr = links->OffsetsSharedPtr;
  this->Offsets = this->OffsetsSharedPtr.get();
}

//----------------------------------------------------------------------------
// Support the vtkAbstractCellLinks API
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::SelectCells(
  vtkIdType minMaxDegree[2], unsigned char* cellSelection)
{
  std::fill_n(cellSelection, this->NumCells, 0);
  vtkSMPTools::For(0, this->NumPts,
    [this, minMaxDegree, cellSelection](vtkIdType ptId, vtkIdType endPtId)
    {
      for (; ptId < endPtId; ++ptId)
      {
        vtkIdType degree = this->Offsets[ptId + 1] - this->Offsets[ptId];
        if (degree >= minMaxDegree[0] && degree < minMaxDegree[1])
        {
          TIds* cells = this->GetCells(ptId);
          for (auto i = 0; i < degree; ++i)
          {
            cellSelection[cells[i]] = 1;
          }
        }
      } // for all points in this batch
    }); // end lambda
}

VTK_ABI_NAMESPACE_END
#endif
