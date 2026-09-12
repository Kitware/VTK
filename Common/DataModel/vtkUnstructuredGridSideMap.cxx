// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUnstructuredGridSideMap.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypeSides.h"
#include "vtkCellTypeUtilities.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTypeInt32Array.h"
#include "vtkTypeInt64Array.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridSides.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
// Whether the edge localSide of a 2D cell of this type runs against its boundary loop. The edges
// of a pixel and of a higher-order quadrilateral follow its parametric axes, so that their edges 2
// and 3 run against it, and are reversed so that two neighbors traverse their edge opposite ways.
bool IsAxisOrderedEdge(int cellType, vtkIdType localSide)
{
  switch (cellType)
  {
    case VTK_PIXEL:
    case VTK_LAGRANGE_QUADRILATERAL:
    case VTK_BEZIER_QUADRILATERAL:
      return localSide == 2 || localSide == 3;
    default:
      return false;
  }
}

// Reverse an edge given by its two corners, then its nodes in order from the first to the second.
template <typename Iterator>
void ReverseEdge(Iterator first, Iterator last)
{
  if (last - first < 2)
  {
    return;
  }
  std::swap(first[0], first[1]);
  std::reverse(first + 2, last);
}

// How many sides a cell has. For a 2D cell the sides are its edges.
vtkIdType CellSideCount(
  vtkUnstructuredGrid* grid, vtkIdType cellId, int cellType, vtkGenericCell* cell)
{
  const int sideDimension =
    vtkCellTypeSides::GetSideDimension(static_cast<unsigned char>(cellType));
  if (VTK_UNLIKELY(
        sideDimension < 1 || (cellType == VTK_POLYHEDRON && !grid->GetPolyhedronCellArray())))
  {
    return 0;
  }
  return vtkCellTypeSides::GetCellNumberOfSides(
    grid, cellId, static_cast<unsigned char>(cellType), sideDimension, cell);
}

// Whether two traversals of the same side run opposite ways round. This is
// what the orientation convention asserts, and what makes one stored value
// serve both cells.
//
// Only the corners take part. A quadratic side carries mid-edge and centre
// nodes after them, which are part of the side but not of the loop, and which
// the two cells list in their own orders.
bool AreOppositeTraversals(
  const std::vector<vtkIdType>& a, int cornersA, const std::vector<vtkIdType>& b, int cornersB)
{
  if (VTK_UNLIKELY(a.size() != b.size() || cornersA != cornersB || cornersA < 2))
  {
    return false;
  }
  const std::size_t k = static_cast<std::size_t>(cornersA);
  if (VTK_UNLIKELY(k > a.size()))
  {
    return false;
  }
  if (k == 2)
  {
    return a[0] == b[1] && a[1] == b[0];
  }
  // Line the two up on their smallest corner, and walk one forward and the
  // other backward from it: they are opposite traversals if every corner
  // agrees.
  const std::size_t m = std::min_element(a.begin(), a.begin() + k) - a.begin();
  const std::size_t n = std::min_element(b.begin(), b.begin() + k) - b.begin();
  for (std::size_t i = 1; i < k; ++i)
  {
    if (VTK_UNLIKELY(a[(m + i) % k] != b[(n + k - i) % k]))
    {
      return false;
    }
  }
  return true;
}

// The points of half-side (cellId, localSide), in that cell's outward winding,
// read with vtkCellTypeSides into `buffer` or `sidePointIds`, with `ids`
// pointing at them, and how many of them are corners; 0 if there is no such
// side. `cellPointIds` and `cell` are scratch, e.g., one per thread, and
// `sidePointIds` is distinct from `cellPointIds`.
vtkIdType ReadHalfSide(vtkUnstructuredGrid* grid, vtkIdType cellId, int cellType,
  vtkIdType localSide, vtkIdList* cellPointIds, vtkIdList* sidePointIds, vtkGenericCell* cell,
  vtkIdType* buffer, const vtkIdType*& ids, vtkIdType& numberOfCorners)
{
  if (VTK_UNLIKELY(localSide < 0 || localSide >= CellSideCount(grid, cellId, cellType, cell)))
  {
    return 0;
  }
  vtkIdType npts;
  const vtkIdType* pts;
  grid->GetCellPoints(cellId, npts, pts, cellPointIds);
  const auto type = static_cast<unsigned char>(cellType);
  const int sideDimension = vtkCellTypeSides::GetSideDimension(type);
  vtkIdType numberOfSidePoints = 0;
  vtkCellTypeSides::DispatchCellSides(type,
    [&](auto sides) VTK_ALWAYS_INLINE_LAMBDA
    {
      numberOfSidePoints = sides.GetSidePoints(grid, cellId, type, sideDimension,
        static_cast<int>(localSide), npts, pts, buffer, sidePointIds, ids, numberOfCorners, cell);
    });
  return std::max<vtkIdType>(numberOfSidePoints, 0);
}

// The same into `points`, with the edges that run against the boundary loop
// reversed, see IsAxisOrderedEdge().
bool HalfSidePoints(vtkUnstructuredGrid* grid, vtkIdType cellId, int cellType, vtkIdType localSide,
  std::vector<vtkIdType>& points, vtkIdList* cellPointIds, vtkIdList* sidePointIds,
  vtkGenericCell* cell, int* cornerCount = nullptr)
{
  // Only the sides read from static tables go in the buffer. The faces of a
  // polyhedron and the sides of the higher-order cells, of any size, go in
  // sidePointIds.
  vtkIdType buffer[vtkCellTypeSides::MaximumCellSidePoints];
  const vtkIdType* ids = nullptr;
  vtkIdType numberOfCorners = 0;
  const vtkIdType n = ReadHalfSide(grid, cellId, cellType, localSide, cellPointIds, sidePointIds,
    cell, buffer, ids, numberOfCorners);
  points.assign(ids, ids + n);
  if (n == 0)
  {
    return false;
  }
  if (IsAxisOrderedEdge(cellType, localSide))
  {
    ReverseEdge(points.begin(), points.end());
  }
  if (cornerCount)
  {
    *cornerCount = static_cast<int>(numberOfCorners);
  }
  return true;
}

// The modification time of what the sides are built from: the cells, their types, the sides of
// the polyhedra and the orders of the higher-order cells. The points are not part of it, since
// moving them changes no side.
vtkMTimeType GetTopologyMTime(vtkUnstructuredGrid* grid)
{
  vtkMTimeType time = 0;
  if (vtkCellArray* cells = grid->GetCells())
  {
    time = std::max(time, cells->GetMTime());
  }
  if (vtkDataArray* types = grid->GetCellTypes())
  {
    time = std::max(time, types->GetMTime());
  }
  if (vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray())
  {
    time = std::max(time, polyhedra->GetMTime());
  }
  vtkDataArray* degrees =
    grid->GetCellData() ? grid->GetCellData()->GetHigherOrderDegrees() : nullptr;
  if (degrees)
  {
    time = std::max(time, degrees->GetMTime());
  }
  return time;
}

} // anonymous namespace

//------------------------------------------------------------------------------
// IdColumn: the narrowest integer type that holds the values, read through a
// raw pointer resolved once.
//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::IdColumn::GetNumberOfValues() const
{
  return this->Array ? this->Array->GetNumberOfTuples() : 0;
}

//------------------------------------------------------------------------------
unsigned long vtkUnstructuredGridSideMap::IdColumn::GetActualMemorySize() const
{
  return this->Array ? this->Array->GetActualMemorySize() : 0;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::IdColumn::Clear()
{
  this->Array = nullptr;
  this->Tiny = nullptr;
  this->Narrow = nullptr;
  this->Wide = nullptr;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::IdColumn::Bind()
{
  this->Tiny = nullptr;
  this->Narrow = nullptr;
  this->Wide = nullptr;
  if (!this->Array || this->Array->GetNumberOfTuples() == 0 ||
    this->Array->GetNumberOfComponents() != 1)
  {
    return;
  }
  vtkDataArray* array = this->Array;
  if (auto* tiny = vtkArrayDownCast<vtkAOSDataArrayTemplate<unsigned char>>(array))
  {
    this->Tiny = tiny->GetPointer(0);
  }
  else if (auto* narrow = vtkArrayDownCast<vtkAOSDataArrayTemplate<vtkTypeInt32>>(array))
  {
    this->Narrow = narrow->GetPointer(0);
  }
  else if (auto* wide = vtkArrayDownCast<vtkAOSDataArrayTemplate<vtkTypeInt64>>(array))
  {
    this->Wide = wide->GetPointer(0);
  }
}

namespace
{
bool FitsInInt32(vtkIdType lo, vtkIdType hi)
{
#ifdef VTK_USE_64BIT_IDS
  return lo >= static_cast<vtkIdType>(std::numeric_limits<vtkTypeInt32>::min()) &&
    hi <= static_cast<vtkIdType>(std::numeric_limits<vtkTypeInt32>::max());
#else
  (void)lo;
  (void)hi;
  return true;
#endif
}

// The sides of the grid, from the groups of equal sides of
// vtkUnstructuredGridSides::BuildGroupedSides(): every group with a half-side
// is a side, numbered in the order of the keys and of their groups, and its
// half-sides, in the order of their cells, are its entries in the inverse.
// halfSideOf(cellId, localSide) is -1 for a side that is not a half-side here.
// Returns the number of sides.
template <typename TId, typename HalfSideOf>
vtkIdType MatchHalfSides(vtkUnstructuredGrid* grid, HalfSideOf&& halfSideOf,
  std::vector<vtkIdType>& halfSideToSide, std::vector<vtkIdType>& sideOffsets,
  std::vector<vtkIdType>& sideCells, std::vector<vtkIdType>& sideLocalSides)
{
  using Sides = vtkUnstructuredGridSides<TId, vtkTypeInt32>;
  vtkNew<Sides> sides;
  sides->SetDataSet(grid);
  sides->SetOutputDimensionControl(
    Sides::SideFlags::SurfacesOfVolumes | Sides::SideFlags::EdgesOfSurfaces);
  sides->SetMaskedGhostTypes(0);
  sides->SetRemoveGhostInterfaces(false);
  sides->BuildGroupedSides();
  const vtkIdType numberOfKeys = sides->GetNumberOfKeys() - 1;
  sideOffsets.assign(1, 0);
  sideCells.clear();
  sideLocalSides.clear();
  if (numberOfKeys <= 0)
  {
    return 0;
  }

  // Visit the half-sides of a key, group by group, with whether each one is
  // the first half-side of its group.
  auto forEachHalfSide = [&](vtkIdType keyId, auto&& visit)
  {
    const TId key = static_cast<TId>(keyId);
    const TId* cellIds = sides->GetCellIdsOfKey(key);
    const vtkTypeInt32* sideIds = sides->GetSideIdsOfKey(key);
    const vtkIdType keyOffset = sides->GetSideOffsetOfKey(key);
    for (vtkIdType g = sides->GetGroupOffsetOfKey(key); g < sides->GetGroupOffsetOfKey(key + 1);
         ++g)
    {
      const vtkIdType first = sides->GetSideOffsetOfGroup(g) - keyOffset;
      const vtkIdType last = first + sides->GetNumberOfSidesOfGroup(g);
      bool startsSide = true;
      for (vtkIdType k = first; k < last; ++k)
      {
        const vtkIdType halfSide = halfSideOf(cellIds[k], sideIds[k]);
        if (halfSide >= 0)
        {
          visit(halfSide, cellIds[k], sideIds[k], startsSide);
          startsSide = false;
        }
      }
    }
  };

  std::vector<vtkIdType> keySides(numberOfKeys + 1, 0);
  std::vector<vtkIdType> keyEntries(numberOfKeys + 1, 0);
  vtkSMPTools::For(0, numberOfKeys,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType keyId = begin; keyId < end; ++keyId)
      {
        forEachHalfSide(keyId,
          [&](vtkIdType, TId, vtkTypeInt32, bool startsSide)
          {
            keySides[keyId] += startsSide ? 1 : 0;
            ++keyEntries[keyId];
          });
      }
    });
  const vtkIdType numberOfSides =
    vtkSMPTools::ExclusiveScan(keySides.begin(), keySides.end(), vtkIdType(0));
  const vtkIdType numberOfEntries =
    vtkSMPTools::ExclusiveScan(keyEntries.begin(), keyEntries.end(), vtkIdType(0));

  sideOffsets.resize(numberOfSides + 1);
  sideCells.resize(numberOfEntries);
  sideLocalSides.resize(numberOfEntries);
  vtkSMPTools::For(0, numberOfKeys,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType keyId = begin; keyId < end; ++keyId)
      {
        vtkIdType sideId = keySides[keyId] - 1;
        vtkIdType entry = keyEntries[keyId];
        forEachHalfSide(keyId,
          [&](vtkIdType halfSide, TId cellId, vtkTypeInt32 localSide, bool startsSide)
          {
            if (startsSide)
            {
              sideOffsets[++sideId] = entry;
            }
            halfSideToSide[halfSide] = sideId;
            sideCells[entry] = cellId;
            sideLocalSides[entry] = localSide;
            ++entry;
          });
      }
    });
  sideOffsets[numberOfSides] = numberOfEntries;
  return numberOfSides;
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::IdColumn::Pack(const std::vector<vtkIdType>& values)
{
  const vtkIdType n = static_cast<vtkIdType>(values.size());

  vtkIdType lo = 0;
  vtkIdType hi = 0;
  for (vtkIdType value : values)
  {
    lo = std::min(lo, value);
    hi = std::max(hi, value);
  }

  // A local side index is single digits and a side id fits in 32 bits on any
  // mesh short of two billion sides, so most columns land well short of
  // vtkIdType. Note the unsigned tier is only available when nothing is
  // negative: HalfSideToSide uses -1 for a half-side whose cell type this does
  // not understand.
  if (lo >= 0 && hi <= static_cast<vtkIdType>(std::numeric_limits<unsigned char>::max()))
  {
    auto array = vtkSmartPointer<vtkUnsignedCharArray>::New();
    array->SetNumberOfValues(n);
    unsigned char* out = array->GetPointer(0);
    vtkSMPTools::For(0, n,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          out[i] = static_cast<unsigned char>(values[i]);
        }
      });
    this->Array = array;
  }
  else if (FitsInInt32(lo, hi))
  {
    auto array = vtkSmartPointer<vtkTypeInt32Array>::New();
    array->SetNumberOfValues(n);
    vtkTypeInt32* out = array->GetPointer(0);
    vtkSMPTools::For(0, n,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          out[i] = static_cast<vtkTypeInt32>(values[i]);
        }
      });
    this->Array = array;
  }
  else
  {
    auto array = vtkSmartPointer<vtkTypeInt64Array>::New();
    array->SetNumberOfValues(n);
    vtkTypeInt64* out = array->GetPointer(0);
    vtkSMPTools::For(0, n,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          out[i] = static_cast<vtkTypeInt64>(values[i]);
        }
      });
    this->Array = array;
  }
  this->Bind();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::IdColumn::Adopt(vtkDataArray* array)
{
  this->Array = array;
  this->Bind();
  if (array && array->GetNumberOfTuples() > 0 && this->Empty())
  {
    // Not a layout that can be read through a raw pointer. Take a copy in a
    // type that can, rather than pay a virtual call on every access for the
    // life of the structure.
    std::vector<vtkIdType> values(array->GetNumberOfTuples());
    vtkSMPTools::For(0, array->GetNumberOfTuples(),
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType i = begin; i < end; ++i)
        {
          values[i] = static_cast<vtkIdType>(array->GetComponent(i, 0));
        }
      });
    this->Pack(values);
  }
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkUnstructuredGridSideMap);

//------------------------------------------------------------------------------
vtkUnstructuredGridSideMap::vtkUnstructuredGridSideMap() = default;

//------------------------------------------------------------------------------
vtkUnstructuredGridSideMap::~vtkUnstructuredGridSideMap()
{
  this->SetDataSet(nullptr);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::ClearStructure()
{
  this->CellSideOffsets.Clear();
  this->HalfSideToSide.Clear();
  this->SideOffsets.Clear();
  this->SideCells.Clear();
  this->SideLocalSides.Clear();
  this->NumberOfSides = 0;
  this->NumberOfHalfSides = 0;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::SetDataSet(vtkUnstructuredGrid* dataSet)
{
  if (this->DataSet == dataSet)
  {
    return;
  }
  vtkUnstructuredGrid* previous = this->DataSet;
  this->DataSet = dataSet;
  if (dataSet)
  {
    dataSet->Register(this);
  }
  if (previous)
  {
    previous->UnRegister(this);
  }
  this->ClearStructure();
  this->Authored = false;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkUnstructuredGrid* vtkUnstructuredGridSideMap::GetDataSet()
{
  return this->DataSet;
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridSideMap::IsMapBuilt()
{
  if (!this->DataSet || this->SideOffsets.Empty())
  {
    return false;
  }
  return this->BuildTime > GetTopologyMTime(this->DataSet);
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridSideMap::HasSideData()
{
  vtkFieldData* sideData =
    this->DataSet ? this->DataSet->GetAttributesAsFieldData(vtkDataObject::SIDE) : nullptr;
  return sideData && sideData->GetNumberOfArrays() > 0;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::SetSideIds(vtkDataArray* halfSideToSide, vtkIdType numberOfSides)
{
  if (!this->DataSet)
  {
    vtkErrorMacro("SetSideIds() needs a grid. Call SetDataSet() first.");
    return;
  }
  if (!this->BuildCellSideOffsets())
  {
    return;
  }
  if (!halfSideToSide || halfSideToSide->GetNumberOfTuples() != this->NumberOfHalfSides)
  {
    vtkErrorMacro(
      "SetSideIds() wants one side id per half-side, " << this->NumberOfHalfSides << " of them.");
    return;
  }
  // The first half-side whose id is out of range, if any.
  std::atomic<vtkIdType> firstBad(this->NumberOfHalfSides);
  vtkSMPTools::For(0, this->NumberOfHalfSides,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType i = begin; i < end; ++i)
      {
        const vtkIdType sideId = static_cast<vtkIdType>(halfSideToSide->GetComponent(i, 0));
        if (VTK_UNLIKELY(sideId < 0 || sideId >= numberOfSides))
        {
          vtkIdType current = firstBad.load(std::memory_order_relaxed);
          while (i < current && !firstBad.compare_exchange_weak(current, i))
          {
          }
          return;
        }
      }
    });
  if (firstBad < this->NumberOfHalfSides)
  {
    const vtkIdType i = firstBad;
    vtkErrorMacro("Side id " << halfSideToSide->GetComponent(i, 0) << " at half-side " << i
                             << " is outside [0, " << numberOfSides << ").");
    return;
  }

  this->HalfSideToSide.Adopt(halfSideToSide);
  this->NumberOfSides = numberOfSides;
  this->Authored = true;
  this->BuildInverse();
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::BuildMap()
{
  if (!this->DataSet || this->IsMapBuilt())
  {
    return;
  }

  // The topology changed under a numbering that data is indexed by. The
  // numbering cannot survive, so neither can the data.
  if (!this->SideOffsets.Empty() && this->HasSideData())
  {
    vtkWarningMacro("The topology changed while side data was attached. The side numbering "
                    "cannot be carried across a topology change, so the side arrays have "
                    "been dropped.");
    this->DataSet->GetAttributesAsFieldData(vtkDataObject::SIDE)->Initialize();
  }

  if (!this->BuildCellSideOffsets())
  {
    return;
  }

  if (this->Authored)
  {
    // An authored numbering is only as good as the mesh it was authored
    // against, and the topology changed since it was supplied, even if the
    // number of half-sides did not.
    vtkWarningMacro("The topology changed since the side numbering was supplied. "
                    "Falling back to a derived numbering.");
    this->Authored = false;
    this->HalfSideToSide.Clear();
  }

  this->DeriveSideIds();
  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
bool vtkUnstructuredGridSideMap::BuildCellSideOffsets()
{
  vtkUnstructuredGrid* grid = this->DataSet;
  const vtkIdType numberOfCells = grid->GetNumberOfCells();

  std::vector<vtkIdType> offsets(numberOfCells + 1, 0);
  vtkSMPThreadLocalObject<vtkGenericCell> cells;
  std::atomic<bool> sawUnsupported(false);

  vtkSMPTools::For(0, numberOfCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType cellId = begin; cellId < end; ++cellId)
      {
        const int cellType = grid->GetCellType(cellId);
        const vtkIdType count = CellSideCount(grid, cellId, cellType, cells.Local());
        // The 0D and 1D cells have no sides to give; a 2D or 3D cell without
        // any is of a type vtkCellTypeSides does not know.
        if (VTK_UNLIKELY(count == 0) &&
          vtkCellTypeUtilities::GetDimension(static_cast<unsigned char>(cellType)) >= 2)
        {
          sawUnsupported.store(true, std::memory_order_relaxed);
        }
        offsets[cellId] = count;
      }
    });
  const vtkIdType total = vtkSMPTools::ExclusiveScan(offsets.begin(), offsets.end(), vtkIdType(0));

  this->NumberOfHalfSides = total;
  this->CellSideOffsets.Pack(offsets);

  if (sawUnsupported)
  {
    vtkWarningMacro("2D or 3D cells of an unsupported type, e.g., higher-order pyramids, "
                    "contributed no sides.");
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::DeriveSideIds()
{
  vtkUnstructuredGrid* grid = this->DataSet;
  const vtkIdType numberOfCells = grid->GetNumberOfCells();
  const vtkIdType numberOfPoints = grid->GetNumberOfPoints();

  std::vector<vtkIdType> halfSideToSide(this->NumberOfHalfSides, -1);
  std::vector<vtkIdType> sideOffsets(1, 0);
  std::vector<vtkIdType> sideCells;
  std::vector<vtkIdType> sideLocalSides;
  this->NumberOfSides = 0;
  if (this->NumberOfHalfSides > 0 && numberOfPoints > 0)
  {
    auto halfSideOf = [this](vtkIdType cellId, vtkIdType localSide) -> vtkIdType
    {
      const vtkIdType from = this->CellSideOffsets[cellId];
      return localSide >= 0 && localSide < this->CellSideOffsets[cellId + 1] - from
        ? from + localSide
        : -1;
    };
    this->NumberOfSides = FitsInInt32(0, std::max(numberOfPoints, numberOfCells))
      ? MatchHalfSides<vtkTypeInt32>(
          grid, halfSideOf, halfSideToSide, sideOffsets, sideCells, sideLocalSides)
      : MatchHalfSides<vtkTypeInt64>(
          grid, halfSideOf, halfSideToSide, sideOffsets, sideCells, sideLocalSides);
  }
  this->HalfSideToSide.Pack(halfSideToSide);
  this->SideOffsets.Pack(sideOffsets);
  this->SideCells.Pack(sideCells);
  this->SideLocalSides.Pack(sideLocalSides);
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::BuildInverse()
{
  const vtkIdType numberOfCells = this->DataSet->GetNumberOfCells();
  const vtkIdType numberOfHalfSides = this->NumberOfHalfSides;
  const vtkIdType numberOfSides = this->NumberOfSides;

  // A counting sort of the half-sides by side, as the grouping of
  // DeriveSideIds(): counted, scanned, placed by decrementing the count of their
  // side, then sorted within every side.
  std::vector<vtkIdType> sideOffsets(numberOfSides + 1, 0);
  auto counts = std::make_unique<std::atomic<vtkIdType>[]>(numberOfSides);
  vtkSMPTools::For(0, numberOfHalfSides,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType halfSide = begin; halfSide < end; ++halfSide)
      {
        const vtkIdType sideId = this->HalfSideToSide[halfSide];
        if (sideId >= 0)
        {
          counts[sideId].fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  vtkSMPTools::For(0, numberOfSides,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType sideId = begin; sideId < end; ++sideId)
      {
        sideOffsets[sideId] = counts[sideId].load(std::memory_order_relaxed);
      }
    });
  const vtkIdType total =
    vtkSMPTools::ExclusiveScan(sideOffsets.begin(), sideOffsets.end(), vtkIdType(0));

  // The (cell, local side) of every half-side, whose order is the order of the
  // half-sides.
  std::vector<std::pair<vtkIdType, vtkIdType>> entries(total);
  vtkSMPTools::For(0, numberOfCells,
    [&](vtkIdType beginCell, vtkIdType endCell)
    {
      for (vtkIdType cellId = beginCell; cellId < endCell; ++cellId)
      {
        const vtkIdType begin = this->CellSideOffsets[cellId];
        const vtkIdType count = this->CellSideOffsets[cellId + 1] - begin;
        for (vtkIdType localSide = 0; localSide < count; ++localSide)
        {
          const vtkIdType sideId = this->HalfSideToSide[begin + localSide];
          if (sideId >= 0)
          {
            const vtkIdType slot =
              sideOffsets[sideId] + counts[sideId].fetch_sub(1, std::memory_order_relaxed) - 1;
            entries[slot] = { cellId, localSide };
          }
        }
      }
    });

  // Each side's first entry is its lowest half-side, so the representative is
  // well defined whether the numbering was derived or supplied. The entries of
  // a side are distinct (cell, local side) pairs, so std::sort gives the same
  // order as std::stable_sort, whatever order the scatter above left them in.
  std::vector<vtkIdType> sideCells(total, -1);
  std::vector<vtkIdType> sideLocalSides(total, -1);
  vtkSMPTools::For(0, numberOfSides,
    [&](vtkIdType begin, vtkIdType end)
    {
      for (vtkIdType sideId = begin; sideId < end; ++sideId)
      {
        std::sort(entries.begin() + sideOffsets[sideId], entries.begin() + sideOffsets[sideId + 1]);
        for (vtkIdType i = sideOffsets[sideId]; i < sideOffsets[sideId + 1]; ++i)
        {
          sideCells[i] = entries[i].first;
          sideLocalSides[i] = entries[i].second;
        }
      }
    });

  this->SideOffsets.Pack(sideOffsets);
  this->SideCells.Pack(sideCells);
  this->SideLocalSides.Pack(sideLocalSides);
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::GetNumberOfSides()
{
  this->BuildMap();
  return this->NumberOfSides;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::GetNumberOfHalfSides()
{
  this->BuildMap();
  return this->NumberOfHalfSides;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::GetNumberOfCellSides(vtkIdType cellId)
{
  this->BuildMap();
  if (VTK_UNLIKELY(this->CellSideOffsets.Empty() || cellId < 0 ||
        cellId + 1 >= this->CellSideOffsets.GetNumberOfValues()))
  {
    return 0;
  }
  return this->CellSideOffsets[cellId + 1] - this->CellSideOffsets[cellId];
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::GetHalfSideId(vtkIdType cellId, vtkIdType localSide)
{
  this->BuildMap();
  if (VTK_UNLIKELY(this->CellSideOffsets.Empty() || cellId < 0 ||
        cellId + 1 >= this->CellSideOffsets.GetNumberOfValues()))
  {
    return -1;
  }
  const vtkIdType begin = this->CellSideOffsets[cellId];
  if (VTK_UNLIKELY(localSide < 0 || begin + localSide >= this->CellSideOffsets[cellId + 1]))
  {
    return -1;
  }
  return begin + localSide;
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::GetCellSide(vtkIdType cellId, vtkIdType localSide)
{
  const vtkIdType halfSide = this->GetHalfSideId(cellId, localSide);
  return VTK_UNLIKELY(halfSide < 0) ? -1 : this->HalfSideToSide[halfSide];
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::GetCellSides(vtkIdType cellId, vtkIdList* sideIds)
{
  sideIds->Reset();
  const vtkIdType count = this->GetNumberOfCellSides(cellId);
  if (VTK_UNLIKELY(count == 0))
  {
    return;
  }
  sideIds->SetNumberOfIds(count);
  const vtkIdType begin = this->CellSideOffsets[cellId];
  for (vtkIdType localSide = 0; localSide < count; ++localSide)
  {
    sideIds->SetId(localSide, this->HalfSideToSide[begin + localSide]);
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::GetSidePoints(
  vtkIdType sideId, vtkIdList* ptIds, vtkIdList* tempCellPointIds, vtkGenericCell* tempCell)
{
  ptIds->Reset();
  this->BuildMap();
  if (VTK_UNLIKELY(this->SideOffsets.Empty() || sideId < 0 || sideId >= this->NumberOfSides))
  {
    return;
  }
  const vtkIdType representative = this->SideOffsets[sideId];
  if (VTK_UNLIKELY(representative >= this->SideOffsets[sideId + 1]))
  {
    return;
  }
  const vtkIdType cellId = this->SideCells[representative];
  const vtkIdType localSide = this->SideLocalSides[representative];
  const int cellType = this->DataSet->GetCellType(cellId);
  vtkIdType buffer[vtkCellTypeSides::MaximumCellSidePoints];
  const vtkIdType* ids = nullptr;
  vtkIdType numberOfCorners = 0;
  const vtkIdType n = ReadHalfSide(this->DataSet, cellId, cellType, localSide, tempCellPointIds,
    ptIds, tempCell, buffer, ids, numberOfCorners);
  if (ids != ptIds->GetPointer(0))
  {
    ptIds->SetNumberOfIds(n);
    std::copy(ids, ids + n, ptIds->GetPointer(0));
  }
  if (IsAxisOrderedEdge(cellType, localSide))
  {
    ReverseEdge(ptIds->begin(), ptIds->end());
  }
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::GetSideCells(vtkIdType sideId, vtkIdList* cellIds)
{
  cellIds->Reset();
  this->BuildMap();
  if (VTK_UNLIKELY(this->SideOffsets.Empty() || sideId < 0 || sideId >= this->NumberOfSides))
  {
    return;
  }
  const vtkIdType begin = this->SideOffsets[sideId];
  const vtkIdType end = this->SideOffsets[sideId + 1];
  cellIds->SetNumberOfIds(end - begin);
  for (vtkIdType i = begin; i < end; ++i)
  {
    cellIds->SetId(i - begin, this->SideCells[i]);
  }
}

//------------------------------------------------------------------------------
int vtkUnstructuredGridSideMap::GetSideOrientation(vtkIdType cellId, vtkIdType localSide)
{
  const vtkIdType sideId = this->GetCellSide(cellId, localSide);
  if (VTK_UNLIKELY(sideId < 0))
  {
    return 0;
  }
  const vtkIdType representative = this->SideOffsets[sideId];
  return (this->SideCells[representative] == cellId &&
           this->SideLocalSides[representative] == localSide)
    ? 1
    : -1;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::GetCellNeighbors(vtkIdType cellId, vtkIdList* neighborCellIds)
{
  neighborCellIds->Reset();
  const vtkIdType count = this->GetNumberOfCellSides(cellId);
  if (VTK_UNLIKELY(count == 0))
  {
    return;
  }
  const vtkIdType begin = this->CellSideOffsets[cellId];
  for (vtkIdType localSide = 0; localSide < count; ++localSide)
  {
    const vtkIdType sideId = this->HalfSideToSide[begin + localSide];
    if (VTK_UNLIKELY(sideId < 0))
    {
      continue;
    }
    for (vtkIdType i = this->SideOffsets[sideId]; i < this->SideOffsets[sideId + 1]; ++i)
    {
      const vtkIdType other = this->SideCells[i];
      if (other != cellId)
      {
        neighborCellIds->InsertUniqueId(other);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkIdType vtkUnstructuredGridSideMap::CheckWindingConsistency(vtkIdList* offendingSides)
{
  if (offendingSides)
  {
    offendingSides->Reset();
  }
  this->BuildMap();
  if (this->SideOffsets.Empty())
  {
    return 0;
  }

  // Each side is independent, so the comparisons spread. A side is marked 1
  // when its cells disagree, and an exclusive scan of the marks gives each
  // offender its position in the report, in side order.
  std::vector<vtkIdType> inconsistentSide(this->NumberOfSides + 1, 0);
  vtkSMPThreadLocalObject<vtkIdList> cellPointIds;
  vtkSMPThreadLocalObject<vtkIdList> sidePointIds;
  vtkSMPThreadLocalObject<vtkGenericCell> cells;

  vtkSMPTools::For(0, this->NumberOfSides,
    [&](vtkIdType begin, vtkIdType end)
    {
      vtkIdList* local = cellPointIds.Local();
      vtkIdList* side = sidePointIds.Local();
      std::vector<vtkIdType> first;
      std::vector<vtkIdType> other;
      int firstCorners = 0;
      int otherCorners = 0;
      for (vtkIdType sideId = begin; sideId < end; ++sideId)
      {
        const vtkIdType from = this->SideOffsets[sideId];
        const vtkIdType to = this->SideOffsets[sideId + 1];
        if (to - from != 2)
        {
          // A boundary side has nobody to disagree with, and a non-manifold
          // side has no two-sided convention to check in the first place.
          continue;
        }
        const vtkIdType cellA = this->SideCells[from];
        const vtkIdType cellB = this->SideCells[from + 1];
        if (!HalfSidePoints(this->DataSet, cellA, this->DataSet->GetCellType(cellA),
              this->SideLocalSides[from], first, local, side, cells.Local(), &firstCorners) ||
          !HalfSidePoints(this->DataSet, cellB, this->DataSet->GetCellType(cellB),
            this->SideLocalSides[from + 1], other, local, side, cells.Local(), &otherCorners))
        {
          continue;
        }
        if (VTK_UNLIKELY(!AreOppositeTraversals(first, firstCorners, other, otherCorners)))
        {
          inconsistentSide[sideId] = 1;
        }
      }
    });

  const vtkIdType inconsistent =
    vtkSMPTools::ExclusiveScan(inconsistentSide.begin(), inconsistentSide.end(), vtkIdType(0));
  if (offendingSides && inconsistent > 0)
  {
    offendingSides->SetNumberOfIds(inconsistent);
    vtkSMPTools::For(0, this->NumberOfSides,
      [&](vtkIdType begin, vtkIdType end)
      {
        for (vtkIdType sideId = begin; sideId < end; ++sideId)
        {
          if (inconsistentSide[sideId + 1] != inconsistentSide[sideId])
          {
            offendingSides->SetId(inconsistentSide[sideId], sideId);
          }
        }
      });
  }

  if (inconsistent > 0)
  {
    vtkWarningMacro(<< inconsistent << " of " << this->NumberOfSides
                    << " sides are traversed the same way by both of their cells. "
                       "GetSideOrientation() still reports which cell represents a side, but "
                       "the sign no longer means outward, so a signed quantity read through it "
                       "will not cancel between neighbours.");
  }
  return inconsistent;
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::Initialize()
{
  if (this->HasSideData())
  {
    vtkWarningMacro("Refusing to drop the side map while side data is attached: "
                    "the data is indexed by this numbering and a rebuilt one would be a "
                    "different permutation. Remove the side arrays first.");
    return;
  }
  this->ClearStructure();
  this->Authored = false;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::ShallowCopy(vtkUnstructuredGridSideMap* other)
{
  if (!other || other == this)
  {
    return;
  }
  const IdColumn* from[] = { &other->CellSideOffsets, &other->HalfSideToSide, &other->SideOffsets,
    &other->SideCells, &other->SideLocalSides };
  IdColumn* to[] = { &this->CellSideOffsets, &this->HalfSideToSide, &this->SideOffsets,
    &this->SideCells, &this->SideLocalSides };
  for (int i = 0; i < 5; ++i)
  {
    to[i]->Array = from[i]->Array;
    to[i]->Bind();
  }

  this->NumberOfSides = other->NumberOfSides;
  this->NumberOfHalfSides = other->NumberOfHalfSides;
  this->Authored = other->Authored;
  this->BuildTime.Modified();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::DeepCopy(vtkUnstructuredGridSideMap* other)
{
  if (!other || other == this)
  {
    return;
  }
  const IdColumn* from[] = { &other->CellSideOffsets, &other->HalfSideToSide, &other->SideOffsets,
    &other->SideCells, &other->SideLocalSides };
  IdColumn* to[] = { &this->CellSideOffsets, &this->HalfSideToSide, &this->SideOffsets,
    &this->SideCells, &this->SideLocalSides };
  for (int i = 0; i < 5; ++i)
  {
    if (!from[i]->Array)
    {
      to[i]->Clear();
      continue;
    }
    to[i]->Array = vtkSmartPointer<vtkDataArray>::Take(from[i]->Array->NewInstance());
    to[i]->Array->DeepCopy(from[i]->Array);
    to[i]->Bind();
  }

  this->NumberOfSides = other->NumberOfSides;
  this->NumberOfHalfSides = other->NumberOfHalfSides;
  this->Authored = other->Authored;
  this->BuildTime.Modified();
  this->Modified();
}

//------------------------------------------------------------------------------
vtkDataArray* vtkUnstructuredGridSideMap::GetCellSideOffsets()
{
  this->BuildMap();
  return this->CellSideOffsets.Array;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkUnstructuredGridSideMap::GetHalfSideToSide()
{
  this->BuildMap();
  return this->HalfSideToSide.Array;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkUnstructuredGridSideMap::GetSideOffsets()
{
  this->BuildMap();
  return this->SideOffsets.Array;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkUnstructuredGridSideMap::GetSideCellsArray()
{
  this->BuildMap();
  return this->SideCells.Array;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkUnstructuredGridSideMap::GetSideLocalSidesArray()
{
  this->BuildMap();
  return this->SideLocalSides.Array;
}

//------------------------------------------------------------------------------
unsigned long vtkUnstructuredGridSideMap::GetActualMemorySize()
{
  return this->CellSideOffsets.GetActualMemorySize() + this->HalfSideToSide.GetActualMemorySize() +
    this->SideOffsets.GetActualMemorySize() + this->SideCells.GetActualMemorySize() +
    this->SideLocalSides.GetActualMemorySize();
}

//------------------------------------------------------------------------------
void vtkUnstructuredGridSideMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DataSet: " << this->DataSet << "\n";
  os << indent << "Authored: " << (this->Authored ? "yes" : "no") << "\n";
  os << indent << "NumberOfSides: " << this->NumberOfSides << "\n";
  os << indent << "NumberOfHalfSides: " << this->NumberOfHalfSides << "\n";
}

VTK_ABI_NAMESPACE_END
