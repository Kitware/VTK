/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeCells.cxx

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

#include "vtkMergeCells.h"

#include "vtkUnstructuredGrid.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkCharArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkIdTypeArray.h"
#include "vtkDataArray.h"
#include "vtkMergePoints.h"
#include "vtkKdTree.h"
#include <cstdlib>
#include <map>
#include <algorithm>

vtkStandardNewMacro(vtkMergeCells);

vtkCxxSetObjectMacro(vtkMergeCells, UnstructuredGrid, vtkUnstructuredGrid);

class vtkMergeCellsSTLCloak
{
public:
  std::map<vtkIdType, vtkIdType> IdTypeMap;
};

vtkMergeCells::vtkMergeCells()
{
  this->TotalNumberOfDataSets = 0;
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfPoints = 0;

  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  this->PointMergeTolerance = 10e-4;
  this->MergeDuplicatePoints = 1;

  this->InputIsUGrid = 0;
  this->InputIsPointSet = 0;

  this->ptList = NULL;
  this->cellList = NULL;

  this->UnstructuredGrid = NULL;

  this->GlobalIdMap = new vtkMergeCellsSTLCloak;
  this->GlobalCellIdMap = new vtkMergeCellsSTLCloak;

  this->UseGlobalIds = 0;
  this->UseGlobalCellIds = 0;

  this->nextGrid = 0;
}

vtkMergeCells::~vtkMergeCells()
{
  this->FreeLists();

  delete this->GlobalIdMap;
  delete this->GlobalCellIdMap;

  this->SetUnstructuredGrid(0);
}

void vtkMergeCells::FreeLists()
{
  delete this->ptList;
  this->ptList = NULL;

  delete this->cellList;
  this->cellList = NULL;
}


int vtkMergeCells::MergeDataSet(vtkDataSet *set)
{
  vtkIdType newPtId, oldPtId, newCellId;
  vtkIdType *idMap;

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (!ugrid)
  {
    vtkErrorMacro(<< "SetUnstructuredGrid first");
    return -1;
  }

  if (this->TotalNumberOfDataSets <= 0)
  {
    // TotalNumberOfCells and TotalNumberOfPoints may both be zero
    // if all data sets to be merged are empty

    vtkErrorMacro(<<
     "Must SetTotalNumberOfCells, SetTotalNumberOfPoints and SetTotalNumberOfDataSets (upper bounds at least)"
     " before starting to MergeDataSets");

    return -1;
  }

  vtkPointData *pointArrays = set->GetPointData();
  vtkCellData *cellArrays   = set->GetCellData();

  // Since vtkMergeCells is to be used only on distributed vtkDataSets,
  // each DataSet should have the same field arrays.  However I've been
  // told that the field arrays may get rearranged in the process of
  // Marshalling/UnMarshalling.  So we use a
  // vtkDataSetAttributes::FieldList to ensure the field arrays are
  // merged in the right order.

  if (ugrid->GetNumberOfCells() == 0)
  {
    vtkPointSet *check1 = vtkPointSet::SafeDownCast(set);

    if (check1)
    {
      this->InputIsPointSet = 1;
      vtkUnstructuredGrid *check2 = vtkUnstructuredGrid::SafeDownCast(set);
      this->InputIsUGrid = (check2 != NULL);
    }

    this->StartUGrid(set);
  }
  else
  {
    this->ptList->IntersectFieldList(pointArrays);
    this->cellList->IntersectFieldList(cellArrays);
  }

  vtkIdType numPoints = set->GetNumberOfPoints();
  vtkIdType numCells  = set->GetNumberOfCells();

  if (numCells == 0)
  {
    return 0;
  }

  if (this->MergeDuplicatePoints)
  {
    if (this->UseGlobalIds)   // faster by far
    {
      // Note:  It has been observed that an input dataset may
      // have an invalid global ID array.  Using the array to
      // merge points results in bad geometry.  It may be
      // worthwhile to do a quick sanity check when merging
      // points.  Downside is that will slow down this filter.

      idMap = this->MapPointsToIdsUsingGlobalIds(set);
    }
    else
    {
      idMap = this->MapPointsToIdsUsingLocator(set);
    }
  }
  else
  {
    idMap = NULL;
  }

  vtkIdType nextPt = (vtkIdType)this->NumberOfPoints;

  vtkPoints *pts = ugrid->GetPoints();

  for (oldPtId=0; oldPtId < numPoints; oldPtId++)
  {
    if (idMap)
    {
      newPtId = idMap[oldPtId];
    }
    else
    {
      newPtId = nextPt;
    }

    if (newPtId == nextPt)
    {
      pts->SetPoint(nextPt, set->GetPoint(oldPtId));

      ugrid->GetPointData()->CopyData(*this->ptList,
                           pointArrays, this->nextGrid, oldPtId, nextPt);

      nextPt++;
    }
  }

  pts->Modified();   // so that subsequent GetBounds will be correct

  if (this->InputIsUGrid)
  {
    newCellId = this->AddNewCellsUnstructuredGrid(set, idMap);
  }
  else
  {
    newCellId = this->AddNewCellsDataSet(set, idMap);
  }

  delete [] idMap;
  idMap = 0;

  this->NumberOfPoints = nextPt;
  this->NumberOfCells = newCellId;

  this->nextGrid++;

  return 0;
}
vtkIdType vtkMergeCells::AddNewCellsDataSet(vtkDataSet *set, vtkIdType *idMap)
{
  vtkIdType oldCellId, id, newPtId, newCellId = 0, oldPtId;

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;
  vtkCellData *cellArrays = set->GetCellData();
  vtkIdType numCells      = set->GetNumberOfCells();

  vtkIdList *cellPoints = vtkIdList::New();
  cellPoints->Allocate(VTK_CELL_SIZE);

  vtkIdType nextCellId = 0;

  int duplicateCellTest = 0;

  if (this->UseGlobalCellIds)
  {
    int success = this->GlobalCellIdAccessStart(set);

    if (success)
    {
      nextCellId = this->GlobalCellIdMap->IdTypeMap.size();
      duplicateCellTest = 1;
    }
  }

  for (oldCellId=0; oldCellId < numCells; oldCellId++)
  {
    if (duplicateCellTest)
    {
      vtkIdType globalId = this->GlobalCellIdAccessGetId(oldCellId);

      std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =

        this->GlobalCellIdMap->IdTypeMap.insert(
           std::map<vtkIdType, vtkIdType>::value_type(globalId, nextCellId));

      if (inserted.second)
      {
        nextCellId++;
      }
      else
      {
        continue;  // skip it, we already have this cell
      }
    }

    set->GetCellPoints(oldCellId, cellPoints);

    for (id=0; id < cellPoints->GetNumberOfIds(); id++)
    {
      oldPtId = cellPoints->GetId(id);

      if (idMap)
      {
        newPtId = idMap[oldPtId];
      }
      else
      {
        newPtId = this->NumberOfPoints + oldPtId;
      }
      cellPoints->SetId(id, newPtId);
    }

    newCellId =
      (vtkIdType)ugrid->InsertNextCell(set->GetCellType(oldCellId), cellPoints);

    ugrid->GetCellData()->CopyData(*(this->cellList), cellArrays,
                                   this->nextGrid, oldCellId, newCellId);
  }

  cellPoints->Delete();

  return newCellId;
}
vtkIdType vtkMergeCells::AddNewCellsUnstructuredGrid(vtkDataSet *set,
                                                     vtkIdType *idMap)
{
  vtkIdType id;

  char firstSet = 0;

  if (this->nextGrid == 0) firstSet = 1;

  vtkUnstructuredGrid *newUgrid = vtkUnstructuredGrid::SafeDownCast(set);
  vtkUnstructuredGrid *Ugrid = this->UnstructuredGrid;

  // connectivity information for the new data set

  vtkCellArray *newCellArray = newUgrid->GetCells();
  vtkIdType *newCells = newCellArray->GetPointer();
  vtkIdType *newLocs = newUgrid->GetCellLocationsArray()->GetPointer(0);
  unsigned char *newTypes = newUgrid->GetCellTypesArray()->GetPointer(0);

  int newNumCells = newUgrid->GetNumberOfCells();
  int newNumConnections = newCellArray->GetData()->GetNumberOfTuples();

  // If we are checking for duplicate cells, create a list now of
  // any cells in the new data set that we already have.

  vtkIdList *duplicateCellIds = NULL;
  int numDuplicateCells = 0;
  int numDuplicateConnections = 0;

  if (this->UseGlobalCellIds)
  {
    int success = this->GlobalCellIdAccessStart(set);

    if (success)
    {
      vtkIdType nextLocalId = this->GlobalCellIdMap->IdTypeMap.size();

      duplicateCellIds = vtkIdList::New();

      for (id = 0; id < newNumCells; id++)
      {
        vtkIdType globalId = this->GlobalCellIdAccessGetId(id);

        std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =

        this->GlobalCellIdMap->IdTypeMap.insert(
            std::map<vtkIdType, vtkIdType>::value_type(globalId, nextLocalId));

        if (inserted.second)
        {
          nextLocalId++;
        }
        else
        {
          duplicateCellIds->InsertNextId(id);
          numDuplicateCells++;

          int npoints = newCells[newLocs[id]];

          numDuplicateConnections += (npoints + 1);
        }
      }

      if (numDuplicateCells == 0)
      {
        duplicateCellIds->Delete();
        duplicateCellIds = NULL;
      }
    }
  }

  // connectivity for the merged ugrid so far

  vtkCellArray *cellArray = NULL;
  vtkIdType *cells = NULL;
  vtkIdType *locs = NULL;
  unsigned char *types = NULL;

  int numCells = 0;
  int numConnections = 0;

  if (!firstSet)
  {
    cellArray  = Ugrid->GetCells();
    cells = cellArray->GetPointer();
    locs = Ugrid->GetCellLocationsArray()->GetPointer(0);
    types = Ugrid->GetCellTypesArray()->GetPointer(0);;

    numCells         = Ugrid->GetNumberOfCells();
    numConnections   =  cellArray->GetData()->GetNumberOfTuples();
  }

  //  New output grid: merging of existing and incoming grids

  //           CELL ARRAY

  int totalNumCells = numCells + newNumCells - numDuplicateCells;
  int totalNumConnections =
      numConnections + newNumConnections - numDuplicateConnections;

  vtkIdTypeArray *mergedcells = vtkIdTypeArray::New();
  mergedcells->SetNumberOfValues(totalNumConnections);

  if (!firstSet)
  {
    vtkIdType *idptr = mergedcells->GetPointer(0);
    memcpy(idptr, cells, sizeof(vtkIdType) * numConnections);
  }

  vtkCellArray *finalCellArray = vtkCellArray::New();
  finalCellArray->SetCells(totalNumCells, mergedcells);

  //           LOCATION ARRAY

  vtkIdTypeArray *locationArray = vtkIdTypeArray::New();
  locationArray->SetNumberOfValues(totalNumCells);

  vtkIdType *iptr = locationArray->GetPointer(0);  // new output dataset

  if (!firstSet)
  {
    memcpy(iptr, locs, numCells * sizeof(vtkIdType));   // existing set
  }

  //           TYPE ARRAY

  vtkUnsignedCharArray *typeArray = vtkUnsignedCharArray::New();
  typeArray->SetNumberOfValues(totalNumCells);

  unsigned char *cptr = typeArray->GetPointer(0);

  if (!firstSet)
  {
    memcpy(cptr, types, numCells * sizeof(unsigned char));
  }

  // set up new cell data

  vtkIdType finalCellId = numCells;
  vtkIdType nextCellArrayIndex = static_cast<vtkIdType>(numConnections);
  vtkCellData *cellArrays = set->GetCellData();

  vtkIdType oldPtId, finalPtId;

  int nextDuplicateCellId = 0;

  for (vtkIdType oldCellId=0; oldCellId < newNumCells; oldCellId++)
  {
    vtkIdType size = *newCells++;

    if (duplicateCellIds)
    {
      vtkIdType skipId = duplicateCellIds->GetId(nextDuplicateCellId);

      if (skipId == oldCellId)
      {
        newCells += size;
        nextDuplicateCellId++;
        continue;
      }
    }

    locationArray->SetValue(finalCellId, nextCellArrayIndex);

    typeArray->SetValue(finalCellId, newTypes[oldCellId]);

    mergedcells->SetValue(nextCellArrayIndex++, size);

    for (id=0; id < size; id++)
    {
      oldPtId = *newCells++;

      if (idMap)
      {
        finalPtId = idMap[oldPtId];
      }
      else
      {
        finalPtId = this->NumberOfPoints + oldPtId;
      }

      mergedcells->SetValue(nextCellArrayIndex++, finalPtId);
    }

    Ugrid->GetCellData()->CopyData(*(this->cellList), cellArrays,
                                   this->nextGrid, oldCellId, finalCellId);

    finalCellId++;
  }

  Ugrid->SetCells(typeArray, locationArray, finalCellArray);

  mergedcells->Delete();
  typeArray->Delete();
  locationArray->Delete();
  finalCellArray->Delete();

  if (duplicateCellIds)
  {
    duplicateCellIds->Delete();
  }

  return finalCellId;
}

void vtkMergeCells::StartUGrid(vtkDataSet *set)
{
  vtkPointData *PD = set->GetPointData();
  vtkCellData *CD = set->GetCellData();

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (!this->InputIsUGrid)
  {
    ugrid->Allocate(this->TotalNumberOfCells);
  }

  vtkPoints *pts = vtkPoints::New();

  // If the input has a vtkPoints object, we'll make the merged output
  // grid have a vtkPoints object of the same data type.  Otherwise,
  // the merged output grid will have the default of points of type float.

  if (this->InputIsPointSet)
  {
    vtkPointSet *ps = vtkPointSet::SafeDownCast(set);
    pts->SetDataType(ps->GetPoints()->GetDataType());
  }

  pts->SetNumberOfPoints(this->TotalNumberOfPoints);  // allocate for upper bound

  ugrid->SetPoints(pts);

  pts->Delete();

  // Order of field arrays may get changed when data sets are
  // marshalled/sent/unmarshalled.  So we need to re-index the
  // field arrays before copying them using a FieldList

  this->ptList   = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);
  this->cellList = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);

  this->ptList->InitializeFieldList(PD);
  this->cellList->InitializeFieldList(CD);

  if (this->UseGlobalIds)
  {
    ugrid->GetPointData()->CopyGlobalIdsOn();
  }
  ugrid->GetPointData()->CopyAllocate(*ptList, this->TotalNumberOfPoints);
  if (this->UseGlobalCellIds)
  {
    ugrid->GetCellData()->CopyGlobalIdsOn();
  }
  ugrid->GetCellData()->CopyAllocate(*cellList, this->TotalNumberOfCells);

  return;
}

void vtkMergeCells::Finish()
{
  this->FreeLists();

  vtkUnstructuredGrid *ugrid = this->UnstructuredGrid;

  if (this->NumberOfPoints < this->TotalNumberOfPoints)
  {
    // if we don't do this, ugrid->GetNumberOfPoints() gives
    //   the wrong value

    ugrid->GetPoints()->GetData()->Resize(this->NumberOfPoints);
  }

  ugrid->Squeeze();

  return;
}

//  Use an array of global node ids to map all points to
// their new Ids in the merged grid.

vtkIdType *vtkMergeCells::MapPointsToIdsUsingGlobalIds(vtkDataSet *set)
{
  int success = this->GlobalNodeIdAccessStart(set);

  if (!success)
  {
    vtkErrorMacro("global id array is not available");
    return NULL;
  }

  vtkIdType npoints = set->GetNumberOfPoints();

  vtkIdType *idMap = new vtkIdType [npoints];

  vtkIdType nextNewLocalId = this->GlobalIdMap->IdTypeMap.size();

  // map global point Ids to Ids in the new data set

  for (vtkIdType oldId=0; oldId<npoints; oldId++)
  {
    vtkIdType globalId = this->GlobalNodeIdAccessGetId(oldId);

    std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =

      this->GlobalIdMap->IdTypeMap.insert(
         std::map<vtkIdType, vtkIdType>::value_type(globalId, nextNewLocalId));

    if (inserted.second)
    {
      // this is a new global node Id

      idMap[oldId] = nextNewLocalId;

      nextNewLocalId++;
    }
    else
    {
      // a repeat, it was not inserted

      idMap[oldId] = inserted.first->second;
    }
  }

  return idMap;
}

// Use a spatial locator to filter out duplicate points and map
// the new Ids to their Ids in the merged grid.

vtkIdType *vtkMergeCells::MapPointsToIdsUsingLocator(vtkDataSet *set)
{
  vtkIdType ptId;

  vtkUnstructuredGrid *grid = this->UnstructuredGrid;
  vtkPoints *points0 = grid->GetPoints();
  vtkIdType npoints0 = (vtkIdType)this->NumberOfPoints;

  vtkPointSet *ps = vtkPointSet::SafeDownCast(set);
  vtkPoints *points1;
  vtkIdType npoints1 = set->GetNumberOfPoints();

  if (ps)
  {
    points1 = ps->GetPoints();
  }
  else
  {
    points1 = vtkPoints::New();
    points1->SetNumberOfPoints(npoints1);

    for (ptId=0; ptId<npoints1; ptId++)
    {
      points1->SetPoint(ptId, set->GetPoint(ptId));
    }
  }

  vtkIdType *idMap = new vtkIdType [npoints1];

  vtkIdType nextNewLocalId = npoints0;

  if (this->PointMergeTolerance == 0.0)
  {
    // testing shows vtkMergePoints is fastest when tolerance is 0

    vtkMergePoints *locator = vtkMergePoints::New();

    vtkPoints *ptarray = vtkPoints::New();

    double bounds[6];

    set->GetBounds(bounds);

    if (npoints0 > 0)
    {
      double tmpbounds[6];

      // Prior to MapPointsToIdsUsingLocator(), points0->SetNumberOfPoints()
      // has been called to set the number of points to the upper bound on the
      // points TO BE merged and now points0->GetNumberOfPoints() does not
      // refer to the number of the points merged so far. Thus we need to
      // temporarily set the number to the latter such that grid->GetBounds()
      // is able to return the correct bounding information. This is a fix to
      // bug #0009626.

      points0->GetData()->SetNumberOfTuples( npoints0 );
      grid->GetBounds( tmpbounds ); // safe to call GetBounds() for real info
      points0->GetData()->SetNumberOfTuples( this->TotalNumberOfPoints );

      bounds[0] = ((tmpbounds[0] < bounds[0]) ? tmpbounds[0] : bounds[0]);
      bounds[2] = ((tmpbounds[2] < bounds[2]) ? tmpbounds[2] : bounds[2]);
      bounds[4] = ((tmpbounds[4] < bounds[4]) ? tmpbounds[4] : bounds[4]);

      bounds[1] = ((tmpbounds[1] > bounds[1]) ? tmpbounds[1] : bounds[1]);
      bounds[3] = ((tmpbounds[3] > bounds[3]) ? tmpbounds[3] : bounds[3]);
      bounds[5] = ((tmpbounds[5] > bounds[5]) ? tmpbounds[5] : bounds[5]);
    }

    locator->InitPointInsertion(ptarray, bounds);

    vtkIdType newId;
    double x[3];

    for (ptId = 0; ptId < npoints0; ptId++)
    {
      // We already know there are no duplicates in this array.
      // Just add them to the locator's point array.

      points0->GetPoint(ptId, x);
      locator->InsertUniquePoint(x, newId);
    }
    for (ptId = 0; ptId < npoints1; ptId++)
    {
      points1->GetPoint(ptId, x);
      locator->InsertUniquePoint(x, newId);

      idMap[ptId] = newId;
    }

    locator->Delete();
    ptarray->Delete();
  }
  else
  {
    // testing shows vtkKdTree is fastest when tolerance is > 0

    vtkKdTree *kd = vtkKdTree::New();

    vtkPoints *ptArrays[2];
    int numArrays;

    if (npoints0 > 0)
    {
      // points0->GetNumberOfPoints() is equal to the upper bound
      // on the points in the final merged grid.  We need to temporarily
      // set it to the number of points added to the merged grid so far.

      points0->GetData()->SetNumberOfTuples(npoints0);

      ptArrays[0] = points0;
      ptArrays[1] = points1;
      numArrays = 2;
    }
    else
    {
      ptArrays[0] = points1;
      numArrays = 1;
    }

    kd->BuildLocatorFromPoints(ptArrays, numArrays);

    vtkIdTypeArray *pointToEquivClassMap =
      kd->BuildMapForDuplicatePoints(this->PointMergeTolerance);

    kd->Delete();

    if (npoints0 > 0)
    {
      points0->GetData()->SetNumberOfTuples(this->TotalNumberOfPoints);
    }

    // The map we get back isn't quite what we need.  The range of
    // the map is a subset of original point IDs which each
    // represent an equivalence class of duplicate points.  But the
    // point chosen to represent the class could be any one of the
    // equivalent points.  We need to create a map that uses IDs
    // of points in the points0 array as the representative, and
    // then new logical contiguous point IDs
    // (npoints0, npoints0+1, ..., numUniquePoints-1) for the
    // points in the new set that are not duplicates of points
    // in the points0 array.

    std::map<vtkIdType, vtkIdType> newIdMap;

    if (npoints0 > 0)   // these were already a unique set
    {
      for (ptId = 0; ptId < npoints0 ; ptId++)
      {
        vtkIdType EqClassRep = pointToEquivClassMap->GetValue(ptId);

        if (EqClassRep != ptId)
        {
          newIdMap.insert(std::map<vtkIdType, vtkIdType>::value_type(EqClassRep, ptId));
        }
      }
    }
    for (ptId = 0; ptId < npoints1; ptId++)
    {
      vtkIdType EqClassRep = pointToEquivClassMap->GetValue(ptId + npoints0);

      if (EqClassRep < npoints0){
        idMap[ptId] = EqClassRep;   // a duplicate of a point in the first set
        continue;
      }

      std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =

        newIdMap.insert(
          std::map<vtkIdType, vtkIdType>::value_type(EqClassRep, nextNewLocalId));

      bool newEqClassRep = inserted.second;
      vtkIdType existingMappedId = inserted.first->second;

      if (newEqClassRep)
      {
        idMap[ptId] = nextNewLocalId;   // here's a new unique point

        nextNewLocalId++;
      }
      else
      {
        idMap[ptId] = existingMappedId;  // a duplicate of a point in the new set
      }
    }

    pointToEquivClassMap->Delete();
    newIdMap.clear();
  }

  if (!ps)
  {
    points1->Delete();
  }

  return idMap;
}
//-------------------------------------------------------------------------
// Help with the complex business of efficient access to the node ID arrays.
// The array was given to us by the user, and we don't know the data type or
// size.
//-------------------------------------------------------------------------

vtkIdType vtkMergeCells::GlobalCellIdAccessGetId(vtkIdType idx)
{
  if(this->GlobalCellIdArray)
  {
    switch (this->GlobalCellIdArrayType)
    {
      vtkTemplateMacro(
        VTK_TT* ids = static_cast<VTK_TT*>(this->GlobalCellIdArray);
        return static_cast<vtkIdType>(ids[idx])
        );
    }
  }
  return 0;
}
int vtkMergeCells::GlobalCellIdAccessStart(vtkDataSet *set)
{
  if(this->UseGlobalCellIds)
  {
    vtkDataArray* da = set->GetCellData()->GetGlobalIds();
    if (da)
    {
      this->GlobalCellIdArray = da->GetVoidPointer(0);
      this->GlobalCellIdArrayType = da->GetDataType();
      return 1;
    }
  }

  this->GlobalCellIdArray = 0;
  return 0;
}

vtkIdType vtkMergeCells::GlobalNodeIdAccessGetId(vtkIdType idx)
{
  if(this->GlobalIdArray)
  {
    switch (this->GlobalIdArrayType)
    {
      vtkTemplateMacro(
        VTK_TT* ids = static_cast<VTK_TT*>(this->GlobalIdArray);
        return static_cast<vtkIdType>(ids[idx])
        );
    }
  }
  return 0;
}

int vtkMergeCells::GlobalNodeIdAccessStart(vtkDataSet *set)
{
  if(this->UseGlobalIds)
  {
    vtkDataArray* da = set->GetPointData()->GetGlobalIds();
    if (da)
    {
      this->GlobalIdArray = da->GetVoidPointer(0);
      this->GlobalIdArrayType = da->GetDataType();
      return 1;
    }
  }

  this->GlobalIdArray = 0;
  return 0;
}


void vtkMergeCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TotalNumberOfDataSets: " << this->TotalNumberOfDataSets << endl;
  os << indent << "TotalNumberOfCells: " << this->TotalNumberOfCells << endl;
  os << indent << "TotalNumberOfPoints: " << this->TotalNumberOfPoints << endl;

  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;

  os << indent << "GlobalIdMap: " << this->GlobalIdMap->IdTypeMap.size() << endl;
  os << indent << "GlobalCellIdMap: " << this->GlobalCellIdMap->IdTypeMap.size() << endl;

  os << indent << "PointMergeTolerance: " << this->PointMergeTolerance << endl;
  os << indent << "MergeDuplicatePoints: " << this->MergeDuplicatePoints << endl;
  os << indent << "InputIsUGrid: " << this->InputIsUGrid << endl;
  os << indent << "InputIsPointSet: " << this->InputIsPointSet << endl;
  os << indent << "UnstructuredGrid: " << this->UnstructuredGrid << endl;
  os << indent << "ptList: " << this->ptList << endl;
  os << indent << "cellList: " << this->cellList << endl;
  os << indent << "UseGlobalIds: " << this->UseGlobalIds << endl;
  os << indent << "UseGlobalCellIds: " << this->UseGlobalCellIds << endl;
}

