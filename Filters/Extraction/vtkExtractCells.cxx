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
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkExtractCells);

#include <set>
#include <algorithm>

class vtkExtractCellsSTLCloak
{
public:
  std::set<vtkIdType> IdTypeSet;
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

  if (l != NULL)
  {
    this->AddCellList(l);
  }
}

//----------------------------------------------------------------------------
void vtkExtractCells::AddCellList(vtkIdList *l)
{
  if (l == NULL)
  {
    return;
  }

  vtkIdType ncells = l->GetNumberOfIds();

  if (ncells == 0)
  {
    return;
  }

  for (int i=0; i<ncells; i++)
  {
    this->CellList->IdTypeSet.insert(l->GetId(i));
  }

  this->Modified();

  return;
}

//----------------------------------------------------------------------------
void vtkExtractCells::AddCellRange(vtkIdType from, vtkIdType to)
{
  if (to < from) return;

  for (vtkIdType id=from; id <= to; id++)
  {
    this->CellList->IdTypeSet.insert(id);
  }

  this->Modified();

  return;
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

  this->InputIsUgrid =
    ((vtkUnstructuredGrid::SafeDownCast(input)) != NULL);

  vtkIdType numCellsInput = input->GetNumberOfCells();
  vtkIdType numCells = static_cast<vtkIdType>(this->CellList->IdTypeSet.size());

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

  vtkIdList *ptIdMap = reMapPointIds(input);

  vtkIdType numPoints = ptIdMap->GetNumberOfIds();

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

  for (vtkIdType newId =0; newId<numPoints; newId++)
  {
    vtkIdType oldId = ptIdMap->GetId(newId);

    pts->SetPoint(newId, input->GetPoint(oldId));

    newPD->CopyData(PD, oldId, newId);
  }

  output->SetPoints(pts);
  pts->Delete();

  if (this->InputIsUgrid)
  {
    this->CopyCellsUnstructuredGrid(ptIdMap, input, output);
  }
  else
  {
    this->CopyCellsDataSet(ptIdMap, input, output);
  }

  ptIdMap->Delete();

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

  return;
}

//----------------------------------------------------------------------------
vtkIdType vtkExtractCells::findInSortedList(vtkIdList *idList, vtkIdType id)
{
  vtkIdType numids = idList->GetNumberOfIds();

  if (numids < 8) return idList->IsId(id);

  vtkIdType L, R, M;
  L=0;
  R=numids-1;

  vtkIdType *ids = idList->GetPointer(0);

  vtkIdType loc = -1;

  while (R > L)
  {
    if (R == L+1)
    {
      if (ids[R] == id)
      {
        loc = R;
      }
      else if (ids[L] == id)
      {
        loc = L;
      }
      break;
    }

    M = (R + L) / 2;

    if (ids[M] > id)
    {
      R = M;
      continue;
    }
    else if (ids[M] < id)
    {
      L = M;
      continue;
    }
    else
    {
      loc = M;
      break;
    }
  }
  return loc;
}

//----------------------------------------------------------------------------
vtkIdList *vtkExtractCells::reMapPointIds(vtkDataSet *grid)
{
  vtkIdType totalPoints = grid->GetNumberOfPoints();

  char *temp = new char [totalPoints];

  if (!temp)
  {
    vtkErrorMacro(<< "vtkExtractCells::reMapPointIds memory allocation");
    return NULL;
  }
  memset(temp, 0, totalPoints);

  int numberOfIds = 0;
  int i;
  vtkIdType id;
  vtkIdList *ptIds = vtkIdList::New();
  std::set<vtkIdType>::iterator cellPtr;

  if (!this->InputIsUgrid)
  {
    for (cellPtr = this->CellList->IdTypeSet.begin();
         cellPtr != this->CellList->IdTypeSet.end();
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

    for (cellPtr = this->CellList->IdTypeSet.begin();
         cellPtr != this->CellList->IdTypeSet.end();
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

  ptIds->SetNumberOfIds(numberOfIds);
  vtkIdType next=0;

  for (id=0; id<totalPoints; id++)
  {
    if (temp[id]) ptIds->SetId(next++, id);
  }

  delete [] temp;

  return ptIds;
}

//----------------------------------------------------------------------------
void vtkExtractCells::CopyCellsDataSet(vtkIdList *ptMap, vtkDataSet *input,
                                       vtkUnstructuredGrid *output)
{
  output->Allocate(static_cast<vtkIdType>(this->CellList->IdTypeSet.size()));

  vtkCellData *oldCD = input->GetCellData();
  vtkCellData *newCD = output->GetCellData();

  // We only create vtkOriginalCellIds for the output data set if it does not
  // exist in the input data set.  If it is in the input data set then we
  // let CopyData() take care of copying it over.
  vtkIdTypeArray *origMap = 0;
  if(oldCD->GetArray("vtkOriginalCellIds") == 0)
  {
    origMap = vtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("vtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  vtkIdList *cellPoints = vtkIdList::New();

  std::set<vtkIdType>::iterator cellPtr;

  for (cellPtr = this->CellList->IdTypeSet.begin();
       cellPtr != this->CellList->IdTypeSet.end();
       ++cellPtr)
  {
    vtkIdType cellId = *cellPtr;

    input->GetCellPoints(cellId, cellPoints);

    for (int i=0; i < cellPoints->GetNumberOfIds(); i++)
    {
      vtkIdType oldId = cellPoints->GetId(i);

      vtkIdType newId = vtkExtractCells::findInSortedList(ptMap, oldId);

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

  return;
}

//----------------------------------------------------------------------------
void vtkExtractCells::CopyCellsUnstructuredGrid(vtkIdList *ptMap,
                                                vtkDataSet *input,
                                                vtkUnstructuredGrid *output)
{
  vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(input);
  if (ugrid == NULL)
  {
    this->CopyCellsDataSet(ptMap, input, output);
    return;
  }

  vtkCellData *oldCD = input->GetCellData();
  vtkCellData *newCD = output->GetCellData();

  // We only create vtkOriginalCellIds for the output data set if it does not
  // exist in the input data set.  If it is in the input data set then we
  // let CopyData() take care of copying it over.
  vtkIdTypeArray *origMap = 0;
  if(oldCD->GetArray("vtkOriginalCellIds") == 0)
  {
    origMap = vtkIdTypeArray::New();
    origMap->SetNumberOfComponents(1);
    origMap->SetName("vtkOriginalCellIds");
    newCD->AddArray(origMap);
    origMap->Delete();
  }

  vtkIdType numCells = static_cast<vtkIdType>(this->CellList->IdTypeSet.size());

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

  std::set<vtkIdType>::iterator cellPtr;                           // input
  vtkIdType *cells = ugrid->GetCells()->GetPointer();
  vtkIdType maxid = ugrid->GetCellLocationsArray()->GetMaxId();
  vtkIdType *locs = ugrid->GetCellLocationsArray()->GetPointer(0);
  vtkUnsignedCharArray *types = ugrid->GetCellTypesArray();

  for (cellPtr = this->CellList->IdTypeSet.begin();
       cellPtr != this->CellList->IdTypeSet.end();
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
      vtkIdType newId = vtkExtractCells::findInSortedList(ptMap, oldId);

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

  return;
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

