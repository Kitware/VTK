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

#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTimeStamp.h"

vtkStandardNewMacro(vtkExtractCells);

#include <algorithm>
#include <numeric>
#include <vector>

namespace {
struct FastPointMap
{
  using ConstIteratorType = const vtkIdType*;

  vtkNew<vtkIdList> Map;
  vtkIdType LastInput;
  vtkIdType LastOutput;

  ConstIteratorType CBegin() const
  {
    return this->Map->GetPointer(0);
  }

  ConstIteratorType CEnd() const
  {
    return this->Map->GetPointer(this->Map->GetNumberOfIds());
  }

  vtkIdType* Reset(vtkIdType numValues)
  {
    this->LastInput = -1;
    this->LastOutput = -1;
    this->Map->SetNumberOfIds(numValues);
    return this->Map->GetPointer(0);
  }

  // Map inputId to the new PointId. If inputId is invalid, return -1.
  vtkIdType LookUp(vtkIdType inputId)
  {
    vtkIdType outputId = -1;
    ConstIteratorType first;
    ConstIteratorType last;

    if (this->LastOutput >= 0)
    {
      // Here's the optimization: since the point ids are usually requested
      // with some locality, we can reduce the search range by caching the
      // results of the last lookup. This reduces the number of lookups and
      // improves CPU cache behavior.

      // Offset is the distance (in input space) between the last lookup and
      // the current id. Since the point map is sorted and unique, this is the
      // maximum distance that the current ID can be from the previous one.
      vtkIdType offset = inputId - this->LastInput;

      // Our search range is from the last output location
      first = this->CBegin() + this->LastOutput;
      last = first + offset;

      // Ensure these are correctly ordered (offset may be < 0):
      if (last < first)
      {
        std::swap(first, last);
      }

      // Adjust last to be past-the-end:
      ++last;

      // Clamp to map bounds:
      first = std::max(first, this->CBegin());
      last = std::min(last, this->CEnd());
    }
    else
    { // First run, use full range:
      first = this->CBegin();
      last = this->CEnd();
    }

    outputId = this->BinaryFind(first, last, inputId);
    if (outputId >= 0)
    {
      this->LastInput = inputId;
      this->LastOutput = outputId;
    }

    return outputId;
  }

private:
  // Modified version of std::lower_bound that returns as soon as a value is
  // found (rather than finding the beginning of a sequence). Returns the
  // position in the list, or -1 if not found.
  vtkIdType BinaryFind(ConstIteratorType first, ConstIteratorType last,
                       vtkIdType val) const
  {
    vtkIdType len = last - first;

    while (len > 0)
    {
      // Select median
      vtkIdType half = len / 2;
      ConstIteratorType middle = first + half;

      const vtkIdType &mVal = *middle;
      if (mVal < val)
      { // This soup is too cold.
        first = middle;
        ++first;
        len = len - half - 1;
      }
      else if (val < mVal)
      { // This soup is too hot!
        len = half;
      }
      else
      { // This soup is juuuust right.
        return middle - this->Map->GetPointer(0);
      }
    }

    return -1;
  }
};
} // end anon namespace

class vtkExtractCellsSTLCloak
{
public:
  std::vector<vtkIdType> CellIds;
  vtkTimeStamp ModifiedTime;
  vtkTimeStamp SortTime;
  FastPointMap PointMap;

  void Modified()
  {
    this->ModifiedTime.Modified();
  }

  inline bool IsPrepared() const
  {
    return this->ModifiedTime.GetMTime() < this->SortTime.GetMTime();
  }

  void Prepare()
  {
    if (!this->IsPrepared())
    {
      std::sort(this->CellIds.begin(), this->CellIds.end());
      auto last = std::unique(this->CellIds.begin(), this->CellIds.end());
      this->CellIds.resize(std::distance(this->CellIds.begin(), last));
      this->SortTime.Modified();
    }
  }
};

//----------------------------------------------------------------------------
vtkExtractCells::vtkExtractCells()
{
  this->SubSetUGridCellArraySize = 0;
  this->InputIsUgrid = 0;
  this->CellList = new vtkExtractCellsSTLCloak;
}

//----------------------------------------------------------------------------
vtkExtractCells::~vtkExtractCells()
{
  delete this->CellList;
}

//----------------------------------------------------------------------------
void vtkExtractCells::SetCellList(vtkIdList *l)
{
  delete this->CellList;
  this->CellList = new vtkExtractCellsSTLCloak;

  if (l != nullptr)
  {
    this->AddCellList(l);
  }
}

//----------------------------------------------------------------------------
void vtkExtractCells::AddCellList(vtkIdList *l)
{
  const vtkIdType inputSize = l ? l->GetNumberOfIds() : 0;
  if (inputSize == 0)
  {
    return;
  }

  const vtkIdType *inputBegin = l->GetPointer(0);
  const vtkIdType *inputEnd = inputBegin + inputSize;

  const std::size_t oldSize = this->CellList->CellIds.size();
  const std::size_t newSize = oldSize + static_cast<std::size_t>(inputSize);
  this->CellList->CellIds.resize(newSize);

  auto outputBegin = this->CellList->CellIds.begin();
  std::advance(outputBegin, oldSize);

  std::copy(inputBegin, inputEnd, outputBegin);

  this->CellList->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractCells::AddCellRange(vtkIdType from, vtkIdType to)
{
  if (to < from)
  {
    return;
  }

  const vtkIdType inputSize = to - from + 1; // +1 to include 'to'
  const std::size_t oldSize = this->CellList->CellIds.size();
  const std::size_t newSize = oldSize + static_cast<std::size_t>(inputSize);

  this->CellList->CellIds.resize(newSize);

  auto outputBegin = this->CellList->CellIds.begin();
  auto outputEnd = outputBegin;
  std::advance(outputBegin, oldSize);
  std::advance(outputEnd, newSize);

  std::iota(outputBegin, outputEnd, from);

  this->CellList->Modified();
}

//----------------------------------------------------------------------------
vtkMTimeType vtkExtractCells::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  mTime = std::max(mTime, this->CellList->ModifiedTime.GetMTime());
  mTime = std::max(mTime, this->CellList->SortTime.GetMTime());
  return mTime;
}

//----------------------------------------------------------------------------
int vtkExtractCells::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Sort/uniquify the cell ids if needed.
  this->CellList->Prepare();

  this->InputIsUgrid =
    ((vtkUnstructuredGrid::SafeDownCast(input)) != nullptr);

  vtkIdType numCellsInput = input->GetNumberOfCells();
  vtkIdType numCells = static_cast<vtkIdType>(this->CellList->CellIds.size());

  if (numCells == numCellsInput)
  {
    #if 0
    this->Copy(input, output);

    return;
   #else
    // The Copy method seems to have a bug, causing codes using ExtractCells to die
    #endif
  }

  vtkPointData *PD = input->GetPointData();
  vtkCellData *CD = input->GetCellData();

  if (numCells == 0)
  {
    // set up a ugrid with same data arrays as input, but
    // no points, cells or data.

    output->Allocate(1);

    output->GetPointData()->CopyGlobalIdsOn();
    output->GetPointData()->CopyAllocate(PD, VTK_CELL_SIZE);
    output->GetCellData()->CopyGlobalIdsOn();
    output->GetCellData()->CopyAllocate(CD, 1);

    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(0);

    output->SetPoints(pts);

    pts->Delete();

    return 1;
  }

  vtkPointData *newPD = output->GetPointData();
  vtkCellData *newCD  = output->GetCellData();

  vtkIdType numPoints = reMapPointIds(input);

  newPD->CopyGlobalIdsOn();
  newPD->CopyAllocate(PD, numPoints);

  newCD->CopyGlobalIdsOn();
  newCD->CopyAllocate(CD, numCells);

  vtkPoints *pts = vtkPoints::New();
  if(vtkPointSet* inputPS = vtkPointSet::SafeDownCast(input))
  {
    // preserve input datatype
    pts->SetDataType(inputPS->GetPoints()->GetDataType());
  }
  pts->SetNumberOfPoints(numPoints);

  // Copy points and point data:
  vtkPointSet *pointSet;
  if ((pointSet = vtkPointSet::SafeDownCast(input)))
  { // Optimize when a vtkPoints object exists in the input:
    vtkNew<vtkIdList> dstIds; // contiguous range [0, numPoints)
    dstIds->SetNumberOfIds(numPoints);
    std::iota(dstIds->GetPointer(0), dstIds->GetPointer(numPoints), 0);

    pts->InsertPoints(dstIds, this->CellList->PointMap.Map, pointSet->GetPoints());
    newPD->CopyData(PD, this->CellList->PointMap.Map, dstIds);
  }
  else
  { // Slow path if we have to query the dataset:
    for (vtkIdType newId = 0; newId < numPoints; ++newId)
    {
      vtkIdType oldId = this->CellList->PointMap.Map->GetId(newId);
      pts->SetPoint(newId, input->GetPoint(oldId));
      newPD->CopyData(PD, oldId, newId);
    }
  }

  output->SetPoints(pts);
  pts->Delete();

  if (this->InputIsUgrid)
  {
    this->CopyCellsUnstructuredGrid(input, output);
  }
  else
  {
    this->CopyCellsDataSet(input, output);
  }

  this->CellList->PointMap.Reset(0);
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractCells::Copy(vtkDataSet *input, vtkUnstructuredGrid *output)
{
  if (this->InputIsUgrid)
  {
    output->DeepCopy(vtkUnstructuredGrid::SafeDownCast(input));
    return;
  }

  vtkIdType numCells = input->GetNumberOfCells();

  vtkPointData *PD = input->GetPointData();
  vtkCellData *CD = input->GetCellData();

  vtkPointData *newPD = output->GetPointData();
  vtkCellData *newCD  = output->GetCellData();

  vtkIdType numPoints = input->GetNumberOfPoints();

  output->Allocate(numCells);

  newPD->CopyAllocate(PD, numPoints);

  newCD->CopyAllocate(CD, numCells);

  vtkPoints *pts = vtkPoints::New();
  pts->SetNumberOfPoints(numPoints);

  for (vtkIdType i=0; i<numPoints; i++)
  {
    pts->SetPoint(i, input->GetPoint(i));
  }
  newPD->DeepCopy(PD);

  output->SetPoints(pts);

  pts->Delete();

  vtkIdList *cellPoints = vtkIdList::New();

  for (vtkIdType cellId=0; cellId < numCells; cellId++)
  {
    input->GetCellPoints(cellId, cellPoints);

    output->InsertNextCell(input->GetCellType(cellId), cellPoints);
  }
  newCD->DeepCopy(CD);

  cellPoints->Delete();

  output->Squeeze();
}

//----------------------------------------------------------------------------
vtkIdType vtkExtractCells::reMapPointIds(vtkDataSet *grid)
{
  vtkIdType totalPoints = grid->GetNumberOfPoints();

  char *temp = new char [totalPoints];

  if (!temp)
  {
    vtkErrorMacro(<< "vtkExtractCells::reMapPointIds memory allocation");
    return 0;
  }
  memset(temp, 0, totalPoints);

  int numberOfIds = 0;
  int i;
  vtkIdType id;
  vtkIdList *ptIds = vtkIdList::New();
  std::vector<vtkIdType>::const_iterator cellPtr;

  if (!this->InputIsUgrid)
  {
    for (cellPtr = this->CellList->CellIds.cbegin();
         cellPtr != this->CellList->CellIds.cend();
         ++cellPtr)
    {
      grid->GetCellPoints(*cellPtr, ptIds);

      vtkIdType nIds = ptIds->GetNumberOfIds();

      vtkIdType *ptId = ptIds->GetPointer(0);

      for (i=0; i<nIds; i++)
      {
        id = *ptId++;

        if (temp[id] == 0)
        {
          numberOfIds++;
          temp[id] = 1;
        }
      }
    }
  }
  else
  {
    vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(grid);

    this->SubSetUGridCellArraySize = 0;

    vtkIdType *cellArray = ugrid->GetCells()->GetPointer();
    vtkIdType *locs = ugrid->GetCellLocationsArray()->GetPointer(0);

    this->SubSetUGridCellArraySize = 0;
    vtkIdType maxid = ugrid->GetCellLocationsArray()->GetMaxId();

    for (cellPtr = this->CellList->CellIds.cbegin();
         cellPtr != this->CellList->CellIds.cend();
         ++cellPtr)
    {
      if (*cellPtr > maxid) continue;

      vtkIdType loc = locs[*cellPtr];

      vtkIdType nIds = cellArray[loc++];

      this->SubSetUGridCellArraySize += (1 + nIds);

      for (i=0; i<nIds; i++)
      {
        id = cellArray[loc++];

        if (temp[id] == 0)
        {
          numberOfIds++;
          temp[id] = 1;
        }
      }
    }
  }
  ptIds->Delete();
  ptIds = nullptr;

  vtkIdType *pointMap = this->CellList->PointMap.Reset(numberOfIds);

  for (id=0; id<totalPoints; id++)
  {
    if (temp[id])
    {
      (*pointMap++) = id;
    }
  }

  delete [] temp;

  return numberOfIds;
}

//----------------------------------------------------------------------------
void vtkExtractCells::CopyCellsDataSet(vtkDataSet *input,
                                       vtkUnstructuredGrid *output)
{
  output->Allocate(static_cast<vtkIdType>(this->CellList->CellIds.size()));

  vtkCellData *oldCD = input->GetCellData();
  vtkCellData *newCD = output->GetCellData();

  // We only create vtkOriginalCellIds for the output data set if it does not
  // exist in the input data set.  If it is in the input data set then we
  // let CopyData() take care of copying it over.
  vtkIdTypeArray *origMap = nullptr;
  if(oldCD->GetArray("vtkOriginalCellIds") == nullptr)
  {
    origMap = vtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("vtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  vtkIdList *cellPoints = vtkIdList::New();

  std::vector<vtkIdType>::const_iterator cellPtr;

  for (cellPtr = this->CellList->CellIds.cbegin();
       cellPtr != this->CellList->CellIds.cend();
       ++cellPtr)
  {
    vtkIdType cellId = *cellPtr;

    input->GetCellPoints(cellId, cellPoints);

    for (int i=0; i < cellPoints->GetNumberOfIds(); i++)
    {
      vtkIdType oldId = cellPoints->GetId(i);

      vtkIdType newId = this->CellList->PointMap.LookUp(oldId);
      assert("Old id exists in map." && newId >= 0);

      cellPoints->SetId(i, newId);
    }
    vtkIdType newId = output->InsertNextCell(input->GetCellType(cellId), cellPoints);

    newCD->CopyData(oldCD, cellId, newId);
    if(origMap)
    {
      origMap->InsertNextValue(cellId);
    }
  }

  cellPoints->Delete();
}

//----------------------------------------------------------------------------
void vtkExtractCells::CopyCellsUnstructuredGrid(vtkDataSet *input,
                                                vtkUnstructuredGrid *output)
{
  vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (ugrid == nullptr)
  {
    this->CopyCellsDataSet(input, output);
    return;
  }

  vtkCellData *oldCD = input->GetCellData();
  vtkCellData *newCD = output->GetCellData();

  // We only create vtkOriginalCellIds for the output data set if it does not
  // exist in the input data set.  If it is in the input data set then we
  // let CopyData() take care of copying it over.
  vtkIdTypeArray *origMap = nullptr;
  if(oldCD->GetArray("vtkOriginalCellIds") == nullptr)
  {
    origMap = vtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("vtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  vtkIdType numCells = static_cast<vtkIdType>(this->CellList->CellIds.size());

  vtkCellArray *cellArray = vtkCellArray::New();                 // output
  vtkIdTypeArray *newcells = vtkIdTypeArray::New();
  newcells->SetNumberOfValues(this->SubSetUGridCellArraySize);
  cellArray->SetCells(numCells, newcells);
  vtkIdType cellArrayIdx = 0;

  vtkIdTypeArray *locationArray = vtkIdTypeArray::New();
  locationArray->SetNumberOfValues(numCells);

  vtkUnsignedCharArray *typeArray = vtkUnsignedCharArray::New();
  typeArray->SetNumberOfValues(numCells);

  vtkIdType nextCellId = 0;

  std::vector<vtkIdType>::const_iterator cellPtr; // input
  vtkIdType *cells = ugrid->GetCells()->GetPointer();
  vtkIdType maxid = ugrid->GetCellLocationsArray()->GetMaxId();
  vtkIdType *locs = ugrid->GetCellLocationsArray()->GetPointer(0);
  vtkUnsignedCharArray *types = ugrid->GetCellTypesArray();

  for (cellPtr = this->CellList->CellIds.cbegin();
       cellPtr != this->CellList->CellIds.cend();
       ++cellPtr)
  {
    if (*cellPtr > maxid) continue;

    vtkIdType oldCellId = *cellPtr;

    vtkIdType loc = locs[oldCellId];
    int size = static_cast<int>(cells[loc]);
    vtkIdType *pts = cells + loc + 1;
    unsigned char type = types->GetValue(oldCellId);

    locationArray->SetValue(nextCellId, cellArrayIdx);
    typeArray->SetValue(nextCellId, type);

    newcells->SetValue(cellArrayIdx++, size);

    for (int i=0; i<size; i++)
    {
      vtkIdType oldId = *pts++;
      vtkIdType newId = this->CellList->PointMap.LookUp(oldId);
      assert("Old id exists in map." && newId >= 0);

      newcells->SetValue(cellArrayIdx++, newId);
    }

    newCD->CopyData(oldCD, oldCellId, nextCellId);
    if(origMap)
    {
      origMap->InsertNextValue(oldCellId);
    }
    nextCellId++;
  }

  output->SetCells(typeArray, locationArray, cellArray);

  typeArray->Delete();
  locationArray->Delete();
  newcells->Delete();
  cellArray->Delete();
}

//----------------------------------------------------------------------------
int vtkExtractCells::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkExtractCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

