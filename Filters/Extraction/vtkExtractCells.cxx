/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCells.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkExtractCells.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkTimeStamp.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

namespace
{

struct ExtractedCellsT
{
  vtkSmartPointer<vtkCellArray> Connectivity;
  vtkSmartPointer<vtkUnsignedCharArray> CellTypes;
  vtkSmartPointer<vtkIdTypeArray> Faces;
  vtkSmartPointer<vtkIdTypeArray> FaceLocations;
};

//=============================================================================
/*
 * These work types help us reuse the same code for extracting elements
 * when a smaller subset is being extracted or everything is being extracted
 * with ease.
 */
struct AllElementsWork
{
  vtkIdType NumberOfPoints;
  vtkIdType NumberOfCells;

  // PointWork API
  inline vtkIdType GetNumberOfPoints() const { return this->NumberOfPoints; }
  inline vtkIdType GetPointId(vtkIdType index) const { return index; }

  // CellWork API
  inline vtkIdType GetNumberOfCells() const { return this->NumberOfCells; }
  inline vtkIdType GetCellId(vtkIdType index) const { return index; }
  inline void MapPointIds(vtkIdList*) const {};
};

struct SubsetCellsWork
{
  const std::vector<vtkIdType>::const_iterator Begin;
  const std::vector<vtkIdType>::const_iterator End;
  const std::vector<vtkIdType>& PointMap;
  vtkIdType NumberOfCells;

  inline vtkIdType GetNumberOfCells() const { return this->NumberOfCells; }
  inline vtkIdType GetCellId(vtkIdType index) const { return *std::next(this->Begin, index); }
  inline vtkIdType MapPointId(vtkIdType id) const
  {
    assert(id >= 0 && id < static_cast<vtkIdType>(this->PointMap.size()));
    return this->PointMap[id];
  }
  inline void MapPointIds(vtkIdList* ids) const
  {
    for (vtkIdType ptid = 0, max = ids->GetNumberOfIds(); ptid < max; ++ptid)
    {
      ids->SetId(ptid, this->MapPointId(ids->GetId(ptid)));
    }
  };
};

struct SubsetPointsWork
{
  const vtkSmartPointer<vtkIdList>& PointIdsToExtract;
  inline vtkIdType GetNumberOfPoints() const { return this->PointIdsToExtract->GetNumberOfIds(); }
  inline vtkIdType GetPointId(vtkIdType index) const
  {
    return this->PointIdsToExtract->GetId(index);
  }
};
//=============================================================================

//------------------------------------------------------------------------------
/* This function returns a new vtkPoints extracted from the `input`.
 * The points to extract are identified by the `PointWork`:
 *  `PointWork::GetNumberOfPoints`: total number of points to extract
 *  `PointWork::GetPointId(idx)`: original pt id for extracted point at the `idx`
 */
template <typename PointWorkT>
static vtkSmartPointer<vtkPoints> DoExtractPoints(vtkDataSet* input, const PointWorkT& work)
{
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(work.GetNumberOfPoints());
  auto array = vtkDoubleArray::SafeDownCast(pts->GetData());

  vtkSMPTools::For(
    0, work.GetNumberOfPoints(), [&work, &array, &input](vtkIdType first, vtkIdType last) {
      double coords[3];
      for (vtkIdType cc = first; cc < last; ++cc)
      {
        input->GetPoint(work.GetPointId(cc), coords);
        array->SetTypedTuple(cc, coords);
      }
    });
  return pts;
}

//------------------------------------------------------------------------------
/**
 * Adds `vtkOriginalCellIds` array, if not already present in `outCD`.
 * `CellWorkT::GetNumberOfCells`: total number of cells being extracted.
 * `CellWorkT::GetCellId(idx)`: original cell id for extracted cell at index `idx`.
 */
template <typename CellWorkT>
static void AddOriginalCellIds(vtkCellData* outCD, const CellWorkT& work)
{
  // add vtkOriginalCellIds array, if needed.
  if (outCD->GetArray("vtkOriginalCellIds") == nullptr)
  {
    const auto numCells = work.GetNumberOfCells();
    vtkNew<vtkIdTypeArray> ids;
    ids->SetName("vtkOriginalCellIds");
    ids->SetNumberOfTuples(numCells);
    vtkSMPTools::For(0, numCells, [&ids, &work](vtkIdType start, vtkIdType end) {
      for (vtkIdType cc = start; cc < end; ++cc)
      {
        ids->SetTypedComponent(cc, 0, work.GetCellId(cc));
      }
    });
    outCD->AddArray(ids);
  }
}

//------------------------------------------------------------------------------
/* Extracts cells identified by `work` from the input.
 * Returns ExtractedCellsT with connectivity and cell-types array set.
 */
template <typename CellWorkT>
static ExtractedCellsT DoExtractCells(vtkDataSet* input, const CellWorkT& work)
{
  const auto numCells = work.GetNumberOfCells();

  ExtractedCellsT result;
  result.Connectivity.TakeReference(vtkCellArray::New());
  result.Connectivity->AllocateEstimate(numCells, input->GetMaxCellSize());
  result.CellTypes.TakeReference(vtkUnsignedCharArray::New());
  result.CellTypes->Allocate(numCells);

  vtkNew<vtkIdList> ptIds;
  for (vtkIdType cc = 0; cc < numCells; ++cc)
  {
    const auto in_cellid = work.GetCellId(cc);
    input->GetCellPoints(in_cellid, ptIds);
    work.MapPointIds(ptIds);
    result.Connectivity->InsertNextCell(ptIds);
    result.CellTypes->InsertNextValue(input->GetCellType(in_cellid));
  }
  result.Connectivity->Squeeze();
  result.CellTypes->Squeeze();
  return result;
}

//------------------------------------------------------------------------------
/**
 * Extract polyhedral cell-face information form input. Adds `Faces` and
 * `FaceLocations` to `result`.
 */
template <typename CellWorkT>
static void DoExtractPolyhedralFaces(
  ExtractedCellsT& result, vtkUnstructuredGrid* input, const CellWorkT& work)
{
  const auto numCells = work.GetNumberOfCells();
  auto inFaceLocations = input->GetFaceLocations();
  auto inFaces = input->GetFaces();

  result.FaceLocations.TakeReference(vtkIdTypeArray::New());
  result.FaceLocations->SetNumberOfTuples(numCells);

  vtkIdType outFacesSize = 0;
  for (vtkIdType cc = 0; cc < numCells; ++cc)
  {
    const auto loc = inFaceLocations->GetValue(work.GetCellId(cc));
    if (loc == -1)
    {
      // not a polyhedral cell
      result.FaceLocations->SetTypedComponent(cc, 0, -1);
    }
    else
    {
      result.FaceLocations->SetTypedComponent(cc, 0, outFacesSize);
      vtkIdType* pfaces_start = inFaces->GetPointer(loc);
      vtkIdType* pfaces = pfaces_start;
      const auto nfaces = (*pfaces++);
      for (vtkIdType face = 0; face < nfaces; ++face)
      {
        const auto npts = (*pfaces++);
        pfaces += npts;
      }
      outFacesSize += static_cast<vtkIdType>(std::distance(pfaces_start, pfaces));
    }
  }

  // Now copy polyhedron Faces.
  result.Faces.TakeReference(vtkIdTypeArray::New());
  result.Faces->SetNumberOfTuples(outFacesSize);

  vtkSMPTools::For(0, numCells, [&](vtkIdType start, vtkIdType end) {
    for (vtkIdType cc = start; cc < end; ++cc)
    {
      const auto inLoc = inFaceLocations->GetValue(work.GetCellId(cc));
      if (inLoc == -1)
      {
        continue;
      }
      const auto outLoc = result.FaceLocations->GetValue(cc);

      auto iptr = inFaces->GetPointer(inLoc);
      auto optr = result.Faces->GetPointer(outLoc);
      const auto nfaces = *iptr++;
      *optr++ = nfaces;
      for (vtkIdType face = 0; face < nfaces; ++face)
      {
        const auto npts = (*iptr++);
        *optr++ = npts;
        std::transform(
          iptr, iptr + npts, optr, [&work](vtkIdType id) { return work.MapPointId(id); });
        optr += npts;
        iptr += npts;
      }
    }
  });
}

//------------------------------------------------------------------------------
template <typename IteratorType>
static std::vector<vtkIdType> FlagChosenPoints(
  vtkDataSet* input, const IteratorType& start, const IteratorType& end)
{
  std::vector<vtkIdType> chosen_points(input->GetNumberOfPoints(), 0);
  const vtkIdType num_cells = static_cast<vtkIdType>(std::distance(start, end));

  vtkSMPThreadLocalObject<vtkIdList> ptIds;

  // make input API threadsafe by calling it once in a single thread.
  input->GetCellType(0);
  input->GetCellPoints(0, ptIds.Local());

  // flag each point used by all of the selected cells.
  vtkSMPTools::For(0, num_cells, [&](vtkIdType first, vtkIdType last) {
    auto& lptIds = ptIds.Local();
    auto celliditer = std::next(start, first);
    for (vtkIdType cc = first; cc < last; ++cc, ++celliditer)
    {
      const auto id = *(celliditer);
      input->GetCellPoints(id, lptIds);
      for (vtkIdType i = 0, max = lptIds->GetNumberOfIds(); i < max; ++i)
      {
        chosen_points[lptIds->GetId(i)] = 1;
      }
    }
  });
  return chosen_points;
}

//------------------------------------------------------------------------------
// Faster overload for vtkUnstructuredGrid
template <typename IteratorType>
static std::vector<vtkIdType> FlagChosenPoints(
  vtkUnstructuredGrid* input, const IteratorType& start, const IteratorType& end)
{
  std::vector<vtkIdType> chosen_points(input->GetNumberOfPoints(), 0);
  const vtkIdType num_cells = static_cast<vtkIdType>(std::distance(start, end));
  auto cellArray = input->GetCells();

  // flag each point used by all of the selected cells.
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> storage;

  vtkSMPTools::For(0, num_cells, [&](vtkIdType first, vtkIdType last) {
    auto celliditer = std::next(start, first);
    auto& caIter = storage.Local();
    if (caIter.GetPointer() == nullptr)
    {
      caIter.TakeReference(cellArray->NewIterator());
    }
    vtkIdType npts;
    const vtkIdType* ptids;
    for (vtkIdType cc = first; cc < last; ++cc, ++celliditer)
    {
      const auto id = *(celliditer);
      caIter->GetCellAtId(id, npts, ptids);
      for (vtkIdType i = 0; i < npts; ++i)
      {
        chosen_points[ptids[i]] = 1;
      }
    }
  });
  return chosen_points;
}

//------------------------------------------------------------------------------
template <typename IteratorType>
static std::vector<vtkIdType> GeneratePointMap(
  vtkDataSet* input, const IteratorType& start, const IteratorType& end, vtkIdType& outNumPoints)
{
  auto ugInput = vtkUnstructuredGrid::SafeDownCast(input);
  std::vector<vtkIdType> chosen_points = ugInput != nullptr ? FlagChosenPoints(ugInput, start, end)
                                                            : FlagChosenPoints(input, start, end);
  // convert flags to  map where index is old id, value is new id and -1 means
  // the point is to be discarded.
  vtkIdType nextid = 0;
  for (auto& pt : chosen_points)
  {
    pt = pt ? nextid++ : -1;
  }
  outNumPoints = nextid;
  return chosen_points;
}

//------------------------------------------------------------------------------
template <typename CellWorkT>
static void CopyCellData(vtkCellData* input, vtkCellData* output, const CellWorkT& work)
{
  const auto numValues = work.GetNumberOfCells();
  output->CopyAllocate(input, numValues);

  vtkNew<vtkIdList> srcIds;
  srcIds->SetNumberOfIds(numValues);

  vtkIdType next = 0;
  std::generate_n(
    srcIds->GetPointer(0), numValues, [&work, &next]() { return work.GetCellId(next++); });

  vtkNew<vtkIdList> dstIds;
  dstIds->SetNumberOfIds(numValues);
  std::iota(dstIds->GetPointer(0), dstIds->GetPointer(numValues), 0);

  output->CopyData(input, srcIds, dstIds);
}

//------------------------------------------------------------------------------
static void CopyPointData(vtkPointData* inPD, vtkPointData* outPD, vtkIdList* srcIds)
{
  const auto numValues = srcIds->GetNumberOfIds();
  outPD->CopyAllocate(inPD, numValues);
  vtkNew<vtkIdList> dstIds;
  dstIds->SetNumberOfIds(numValues);
  std::iota(dstIds->GetPointer(0), dstIds->GetPointer(numValues), 0);
  outPD->CopyData(inPD, srcIds, dstIds);
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkIdList> ConvertToPointIdsToExtract(
  const std::vector<vtkIdType>& pointMap, const vtkIdType numValues)
{
  vtkNew<vtkIdList> srcIds;
  srcIds->Allocate(numValues);
  for (vtkIdType cc = 0; cc < static_cast<vtkIdType>(pointMap.size()); ++cc)
  {
    if (pointMap[cc] != -1)
    {
      srcIds->InsertNextId(cc);
    }
  }
  srcIds->Squeeze();
  assert(numValues == srcIds->GetNumberOfIds());
  return srcIds;
}

} // end anonymous namespace

//=============================================================================
class vtkExtractCellsSTLCloak
{
  vtkTimeStamp SortTime;

public:
  std::vector<vtkIdType> CellIds;
  std::pair<typename std::vector<vtkIdType>::const_iterator,
    typename std::vector<vtkIdType>::const_iterator>
    CellIdsRange;

  vtkIdType Prepare(vtkIdType numInputCells, vtkExtractCells* self)
  {
    assert(numInputCells > 0);

    if (self->GetAssumeSortedAndUniqueIds() == false && (self->GetMTime() > this->SortTime))
    {
      vtkSMPTools::Sort(this->CellIds.begin(), this->CellIds.end());
      auto last = std::unique(this->CellIds.begin(), this->CellIds.end());
      this->CellIds.erase(last, this->CellIds.end());
      this->SortTime.Modified();
    }

    this->CellIdsRange =
      std::make_pair(std::lower_bound(this->CellIds.begin(), this->CellIds.end(), 0),
        std::upper_bound(this->CellIds.begin(), this->CellIds.end(), (numInputCells - 1)));
    return static_cast<vtkIdType>(
      std::distance(this->CellIdsRange.first, this->CellIdsRange.second));
  }
};

vtkStandardNewMacro(vtkExtractCells);
//------------------------------------------------------------------------------
vtkExtractCells::vtkExtractCells()
{
  this->CellList = new vtkExtractCellsSTLCloak;
}

//------------------------------------------------------------------------------
vtkExtractCells::~vtkExtractCells()
{
  delete this->CellList;
}

//------------------------------------------------------------------------------
void vtkExtractCells::SetCellList(vtkIdList* l)
{
  delete this->CellList;
  this->CellList = new vtkExtractCellsSTLCloak;
  if (l != nullptr)
  {
    this->AddCellList(l);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExtractCells::AddCellList(vtkIdList* l)
{
  const vtkIdType inputSize = l ? l->GetNumberOfIds() : 0;
  if (inputSize == 0)
  {
    return;
  }

  auto& cellIds = this->CellList->CellIds;
  const vtkIdType* inputBegin = l->GetPointer(0);
  const vtkIdType* inputEnd = inputBegin + inputSize;
  std::copy(inputBegin, inputEnd, std::back_inserter(cellIds));
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExtractCells::SetCellIds(const vtkIdType* ptr, vtkIdType numValues)
{
  delete this->CellList;
  this->CellList = new vtkExtractCellsSTLCloak;
  if (ptr != nullptr && numValues > 0)
  {
    this->AddCellIds(ptr, numValues);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExtractCells::AddCellIds(const vtkIdType* ptr, vtkIdType numValues)
{
  auto& cellIds = this->CellList->CellIds;
  std::copy(ptr, ptr + numValues, std::back_inserter(cellIds));
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkExtractCells::AddCellRange(vtkIdType from, vtkIdType to)
{
  if (to < from || to < 0)
  {
    vtkWarningMacro("Bad cell range: (" << to << "," << from << ")");
    return;
  }

  // This range specification is inconsistent with C++. Left for backward
  // compatibility reasons.  Add 1 to `to` to make it consistent.
  ++to;

  auto& cellIds = this->CellList->CellIds;
  std::generate_n(std::back_inserter(cellIds), (to - from), [&from]() { return from++; });
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkExtractCells::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the input and output
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid* output = vtkUnstructuredGrid::GetData(outputVector);

  auto inPD = input->GetPointData();
  auto inCD = input->GetCellData();
  auto outPD = output->GetPointData();
  auto outCD = output->GetCellData();

  // copy all arrays, including global ids etc.
  outPD->CopyAllOn();
  outCD->CopyAllOn();

  const vtkIdType numCellsInput = input->GetNumberOfCells();
  const vtkIdType numCells =
    this->ExtractAllCells ? numCellsInput : this->CellList->Prepare(numCellsInput, this);

  // Handle simple cases, first.
  // Check if no cells are to be extracted
  if (numCells == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.
    output->Allocate(1);
    outPD->CopyAllocate(inPD, 1);
    outCD->CopyAllocate(inCD, 1);
    vtkNew<vtkPoints> pts;
    pts->SetNumberOfPoints(0);
    output->SetPoints(pts);
    return 1;
  }
  else if (numCellsInput == numCells)
  {
    // Check if all cells are to be extracted.
    // `Copy` will ShallowCopy input if input is vtkUnstructuredGrid, else
    // convert it to an unstructured grid.
    return this->Copy(input, output) ? 1 : 0;
  }

  // Build point map for selected cells.
  const auto& cellids_range = this->CellList->CellIdsRange;
  assert(cellids_range.first != cellids_range.second);

  vtkIdType numPoints;
  const auto pointMap =
    ::GeneratePointMap(input, cellids_range.first, cellids_range.second, numPoints);
  auto chosenPtIds = ::ConvertToPointIdsToExtract(pointMap, numPoints);
  this->UpdateProgress(0.25);

  const SubsetCellsWork work{ cellids_range.first, cellids_range.second, pointMap, numCells };

  // Copy cell and point data first, since that's easy enough.
  ::CopyCellData(inCD, outCD, work);
  ::AddOriginalCellIds(outCD, work);
  ::CopyPointData(inPD, outPD, chosenPtIds);
  this->UpdateProgress(0.5);

  // Get new points
  auto pts = ::DoExtractPoints(input, SubsetPointsWork{ chosenPtIds });
  output->SetPoints(pts);
  this->UpdateProgress(0.75);

  // Extract cells
  auto cells = ::DoExtractCells(input, work);
  this->UpdateProgress(0.85);

  // Handle polyhedral cells
  auto inputUG = vtkUnstructuredGrid::SafeDownCast(input);
  if (inputUG && inputUG->GetFaces() && inputUG->GetFaceLocations() &&
    inputUG->GetFaceLocations()->GetRange(0)[1] != -1)
  {
    ::DoExtractPolyhedralFaces(cells, inputUG, work);
  }
  output->SetCells(cells.CellTypes, cells.Connectivity, cells.FaceLocations, cells.Faces);
  return 1;
}

//------------------------------------------------------------------------------
bool vtkExtractCells::Copy(vtkDataSet* input, vtkUnstructuredGrid* output)
{
  if (vtkUnstructuredGrid::SafeDownCast(input))
  {
    output->ShallowCopy(input);
    return true;
  }

  if (vtkPointSet::SafeDownCast(input))
  {
    // pass points along.
    output->vtkPointSet::ShallowCopy(input);
  }
  else
  {
    // copy points manually.
    const vtkIdType numPoints = input->GetNumberOfPoints();
    auto pts = ::DoExtractPoints(input, AllElementsWork{ numPoints, 0 });
    output->SetPoints(pts);
  }

  const auto numCells = input->GetNumberOfCells();
  auto cells = ::DoExtractCells(input, AllElementsWork{ 0, numCells });
  output->SetCells(cells.CellTypes, cells.Connectivity, nullptr, nullptr);

  // copy cell/point arrays.
  output->GetPointData()->ShallowCopy(input->GetPointData());
  output->GetCellData()->ShallowCopy(input->GetCellData());
  ::AddOriginalCellIds(output->GetCellData(), AllElementsWork{ 0, numCells });
  return true;
}

//------------------------------------------------------------------------------
int vtkExtractCells::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkExtractCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ExtractAllCells: " << this->ExtractAllCells << endl;
  os << indent << "AssumeSortedAndUniqueIds: " << this->AssumeSortedAndUniqueIds << endl;
}
