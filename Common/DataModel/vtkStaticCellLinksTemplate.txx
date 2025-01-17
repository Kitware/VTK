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

#include <atomic>
#include <type_traits>
#include <vector>

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

  // Traverse data to determine number of uses of each point. Also count the
  // number of links to allocate.
  std::vector<TIds> counts(static_cast<size_t>(this->NumPts));

  vtkNew<vtkIdList> cellPts;
  this->LinksSize = 0;
  for (vtkIdType cellId = 0; cellId < this->NumCells; cellId++)
  {
    ds->GetCellPoints(cellId, cellPts);
    vtkIdType npts = cellPts->GetNumberOfIds();
    for (vtkIdType j = 0; j < npts; j++)
    {
      counts[cellPts->GetId(j)]++;
      this->LinksSize++;
    }
  }

  // Perform prefix sum to determine offsets
  this->OffsetsSharedPtr.reset(new TIds[this->NumPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  this->Offsets[0] = 0;
  for (vtkIdType ptId = 1; ptId < this->NumPts; ++ptId)
  {
    const TIds& nCells = counts[ptId - 1];
    this->Offsets[ptId] = this->Offsets[ptId - 1] + nCells;
  }
  this->Offsets[this->NumPts] = this->LinksSize;

  // Allocate links array, Extra one allocated to simplify later pointer manipulation
  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  this->Links[this->LinksSize] = this->NumPts;

  // Now build the links. The summation from the prefix sum indicates where
  // the cells are to be inserted. Each time a cell is inserted, the offset
  // is decremented. In the end, the offset array is also constructed as it
  // points to the beginning of each cell run.
  for (vtkIdType cellId = 0; cellId < this->NumCells; ++cellId)
  {
    ds->GetCellPoints(cellId, cellPts);
    vtkIdType npts = cellPts->GetNumberOfIds();
    for (vtkIdType j = 0; j < npts; ++j)
    {
      vtkIdType ptId = cellPts->GetId(j);
      const TIds offset = this->Offsets[ptId + 1] - counts[ptId]--;
      this->Links[offset] = cellId;
    }
  }
  this->Offsets[this->NumPts] = this->LinksSize;
}
VTK_ABI_NAMESPACE_END

namespace vtkSCLT_detail
{
VTK_ABI_NAMESPACE_BEGIN

struct CountPoints
{
  template <typename CellStateT, typename TIds>
  void operator()(
    CellStateT& state, std::atomic<TIds>* counts, vtkIdType beginCellId, vtkIdType endCellId)
  {
    using ValueType = typename CellStateT::ValueType;
    const vtkIdType connBeginId = state.GetBeginOffset(beginCellId);
    const vtkIdType connEndId = state.GetEndOffset(endCellId - 1);
    auto connRange = vtk::DataArrayValueRange<1>(state.GetConnectivity(), connBeginId, connEndId);

    // Count number of point uses
    for (const ValueType ptId : connRange)
    {
      // memory_order_relaxed is safe here, since we're not using the atomics for synchronization.
      counts[ptId].fetch_add(1, std::memory_order_relaxed);
    }
  }
};

struct BuildLinks
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
        offset = offsets[ptId + 1] - counts[ptId].fetch_sub(1, std::memory_order_relaxed);
        links[offset] = idOffset + cellId;
      }
    }
  }
};

VTK_ABI_NAMESPACE_END
} // end namespace vtkSCLT_detail

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
    this->CellArray->Visit(vtkSCLT_detail::CountPoints{}, this->Counts, cellId, endCellId);
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
    this->CellArray->Visit(vtkSCLT_detail::BuildLinks{}, this->Offsets, this->Counts, this->Links,
      cellId, endCellId, this->IdOffset);
  }
};

template <typename TIds>
struct SortLinks
{
  const TIds* Offsets;
  TIds* Links;

  SortLinks(const TIds* offsets, TIds* links)
    : Offsets(offsets)
    , Links(links)
  {
  }

  void operator()(vtkIdType beginPointId, vtkIdType endPointId)
  {
    for (vtkIdType pointId = beginPointId; pointId < endPointId; ++pointId)
    {
      // check if the links are sorted, because that's the most common case
      const bool isSorted = std::is_sorted(
        this->Links + this->Offsets[pointId], this->Links + this->Offsets[pointId + 1]);
      // if the links are not sorted, we need to sort them
      if (!isSorted)
      {
        std::sort(this->Links + this->Offsets[pointId], this->Links + this->Offsets[pointId + 1]);
      }
    }
  }
};

} // anonymous

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
// Build the link list array for unstructured grids. Note this is a threaded
// implementation: it uses SMPTools and atomics to prevent race situations.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinksFromMultipleArrays(
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

  // Create an array of atomics with initial count=0. This will keep
  // track of point uses. Count them in parallel.
  std::atomic<TIds>* counts = new std::atomic<TIds>[numPts]();
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    CountUses<TIds> count(cellArrays[i], counts);
    vtkSMPTools::For(0, cellArrays[i]->GetNumberOfCells(), count);
  }

  // Perform prefix sum to determine offsets
  this->OffsetsSharedPtr.reset(new TIds[numPts + 1], std::default_delete<TIds[]>());
  this->Offsets = this->OffsetsSharedPtr.get();
  this->Offsets[0] = 0;
  for (vtkIdType ptId = 1; ptId < numPts; ++ptId)
  {
    TIds nCells = counts[ptId - 1].load(std::memory_order_relaxed);
    this->Offsets[ptId] = this->Offsets[ptId - 1] + nCells;
  }
  this->Offsets[numPts] = this->LinksSize;

  // Allocate links array, Extra one allocated to simplify later pointer manipulation
  this->LinkSharedPtr.reset(new TIds[this->LinksSize + 1], std::default_delete<TIds[]>());
  this->Links = this->LinkSharedPtr.get();
  this->Links[this->LinksSize] = this->NumPts;

  // Now insert cell ids into cell links.
  for (size_t i = 0; i < cellArrays.size(); ++i)
  {
    InsertLinks<TIds> insertLinks(cellArrays[i], counts, this->Offsets, this->Links, offsets[i]);
    vtkSMPTools::For(0, cellArrays[i]->GetNumberOfCells(), insertLinks);
  }

  // Clean up
  delete[] counts;

  // Sort the cell links of each point (if needed) to ensure deterministic order
  SortLinks<TIds> sortLinks(this->Offsets, this->Links);
  vtkSMPTools::For(0, numPts, sortLinks);
}

//----------------------------------------------------------------------------
// Build the link list array for unstructured grids.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkUnstructuredGrid* ugrid)
{
  // Build links
  this->BuildLinks(ugrid->GetNumberOfPoints(), ugrid->GetNumberOfCells(), ugrid->GetCells());
}

//----------------------------------------------------------------------------
// Build the link list array for explicit structured grids.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkExplicitStructuredGrid* esgrid)
{
  // Build links
  this->BuildLinks(esgrid->GetNumberOfPoints(), esgrid->GetNumberOfCells(), esgrid->GetCells());
}

//----------------------------------------------------------------------------
// Build the link list array for poly data. This is more complex because there
// are potentially four different cell arrays to contend with.
template <typename TIds>
void vtkStaticCellLinksTemplate<TIds>::BuildLinks(vtkPolyData* pd)
{
  // Get cell arrays
  std::vector<vtkCellArray*> cellArrays = { pd->GetVerts(), pd->GetLines(), pd->GetPolys(),
    pd->GetStrips() };
  // Remove any null cell arrays
  cellArrays.erase(std::remove(cellArrays.begin(), cellArrays.end(), nullptr), cellArrays.end());
  // Build links
  this->BuildLinksFromMultipleArrays(pd->GetNumberOfPoints(), pd->GetNumberOfCells(), cellArrays);
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
