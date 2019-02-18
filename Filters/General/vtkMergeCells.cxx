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

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkKdTree.h"
#include "vtkMergePoints.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <cstdlib>
#include <map>

vtkStandardNewMacro(vtkMergeCells);

vtkCxxSetObjectMacro(vtkMergeCells, UnstructuredGrid, vtkUnstructuredGrid);

class vtkMergeCellsSTLCloak
{
public:
  std::map<vtkIdType, vtkIdType> IdTypeMap;
};

//-----------------------------------------------------------------------------
vtkMergeCells::vtkMergeCells()
{
  this->TotalNumberOfDataSets = 0;
  this->TotalNumberOfCells = 0;
  this->TotalNumberOfPoints = 0;

  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  this->PointMergeTolerance = 10e-4;
  this->MergeDuplicatePoints = 1;

  this->InputIsUGrid = false;
  this->InputIsPointSet = false;

  this->PointList = nullptr;
  this->CellList = nullptr;

  this->UnstructuredGrid = nullptr;

  this->GlobalIdMap = new vtkMergeCellsSTLCloak;
  this->GlobalCellIdMap = new vtkMergeCellsSTLCloak;

  this->UseGlobalIds = 0;
  this->UseGlobalCellIds = 0;

  this->NextGrid = 0;
}

//-----------------------------------------------------------------------------
vtkMergeCells::~vtkMergeCells()
{
  this->FreeLists();

  delete this->GlobalIdMap;
  delete this->GlobalCellIdMap;

  this->SetUnstructuredGrid(nullptr);
}

//-----------------------------------------------------------------------------
void vtkMergeCells::FreeLists()
{
  delete this->PointList;
  this->PointList = nullptr;

  delete this->CellList;
  this->CellList = nullptr;
}

//-----------------------------------------------------------------------------
int vtkMergeCells::MergeDataSet(vtkDataSet* set)
{
  vtkUnstructuredGrid* grid = this->UnstructuredGrid;

  if (!grid)
  {
    vtkErrorMacro(<< "SetUnstructuredGrid first");
    return -1;
  }

  if (this->TotalNumberOfDataSets <= 0)
  {
    // TotalNumberOfCells and TotalNumberOfPoints may both be zero
    // if all data sets to be merged are empty

    vtkErrorMacro(<< "Must SetTotalNumberOfCells, SetTotalNumberOfPoints and "
                     "SetTotalNumberOfDataSets (upper bounds at least)"
                     " before starting to MergeDataSets");

    return -1;
  }

  vtkPointData* pointArrays = set->GetPointData();
  vtkCellData* cellArrays = set->GetCellData();

  // Since vtkMergeCells is to be used only on distributed vtkDataSets,
  // each DataSet should have the same field arrays.  However the field arrays
  // may get rearranged in the process of Marshalling/UnMarshalling.
  // So we use a vtkDataSetAttributes::FieldList to ensure the field arrays are
  // merged in the right order.
  if (grid->GetNumberOfCells() == 0)
  {
    this->InputIsPointSet = vtkPointSet::SafeDownCast(set) ? 1 : 0;
    this->InputIsUGrid = vtkUnstructuredGrid::SafeDownCast(set) ? 1 : 0;
    this->StartUGrid(set);
  }
  else
  {
    this->PointList->IntersectFieldList(pointArrays);
    this->CellList->IntersectFieldList(cellArrays);
  }

  vtkIdType numPoints = set->GetNumberOfPoints();
  vtkIdType numCells = set->GetNumberOfCells();

  if (numCells == 0)
  {
    return 0;
  }

  vtkIdType* idMap = nullptr;
  if (this->MergeDuplicatePoints)
  {
    if (this->UseGlobalIds) // faster by far
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

  vtkIdType nextPt = this->NumberOfPoints;
  vtkPoints* pts = grid->GetPoints();

  for (vtkIdType oldPtId = 0; oldPtId < numPoints; oldPtId++)
  {
    vtkIdType newPtId = idMap ? idMap[oldPtId] : nextPt;

    if (newPtId == nextPt)
    {
      pts->SetPoint(nextPt, set->GetPoint(oldPtId));
      grid->GetPointData()->CopyData(
        *this->PointList, pointArrays, this->NextGrid, oldPtId, nextPt);
      nextPt++;
    }
  }

  pts->Modified(); // so that subsequent GetBounds will be correct

  vtkIdType newCellId = this->InputIsUGrid ? this->AddNewCellsUnstructuredGrid(set, idMap)
                                           : this->AddNewCellsDataSet(set, idMap);

  delete[] idMap;

  this->NumberOfPoints = nextPt;
  this->NumberOfCells = newCellId;

  this->NextGrid++;

  return 0;
}

//-----------------------------------------------------------------------------
vtkIdType vtkMergeCells::AddNewCellsDataSet(vtkDataSet* set, vtkIdType* idMap)
{
  vtkUnstructuredGrid* grid = this->UnstructuredGrid;
  vtkCellData* cellArrays = set->GetCellData();
  vtkIdType numCells = set->GetNumberOfCells();

  vtkNew<vtkIdList> cellPoints;
  cellPoints->Allocate(VTK_CELL_SIZE);

  vtkIdType nextCellId = 0;

  bool duplicateCellTest = false;

  if (this->UseGlobalCellIds)
  {
    if (this->GlobalCellIdAccessStart(set))
    {
      nextCellId = static_cast<vtkIdType>(this->GlobalCellIdMap->IdTypeMap.size());
      duplicateCellTest = true;
    }
  }

  vtkIdType newCellId = 0;

  for (vtkIdType oldCellId = 0; oldCellId < numCells; oldCellId++)
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
        continue; // skip it, we already have this cell
      }
    }

    set->GetCellPoints(oldCellId, cellPoints);

    for (vtkIdType pid = 0; pid < cellPoints->GetNumberOfIds(); pid++)
    {
      vtkIdType oldPtId = cellPoints->GetId(pid);
      vtkIdType newPtId = idMap ? idMap[oldPtId] : this->NumberOfPoints + oldPtId;
      cellPoints->SetId(pid, newPtId);
    }

    newCellId = grid->InsertNextCell(set->GetCellType(oldCellId), cellPoints);

    grid->GetCellData()->CopyData(
      *(this->CellList), cellArrays, this->NextGrid, oldCellId, newCellId);
  }

  return newCellId;
}

//-----------------------------------------------------------------------------
vtkIdType vtkMergeCells::AddNewCellsUnstructuredGrid(vtkDataSet* set, vtkIdType* idMap)
{
  bool firstSet = (this->NextGrid == 0);

  vtkUnstructuredGrid* newGrid = vtkUnstructuredGrid::SafeDownCast(set);
  vtkUnstructuredGrid* grid = this->UnstructuredGrid;

  // Connectivity information for the new data set
  vtkIdType newNumCells = newGrid->GetNumberOfCells();
  vtkIdType newNumConnections = newGrid->GetCells()->GetData()->GetNumberOfTuples();

  // If we are checking for duplicate cells, create a list now of
  // any cells in the new data set that we already have.
  vtkIdList* duplicateCellIds = nullptr;
  vtkIdType numDuplicateCells = 0;
  vtkIdType numDuplicateConnections = 0;

  if (this->UseGlobalCellIds)
  {
    if (this->GlobalCellIdAccessStart(set))
    {
      vtkIdType nextLocalId = static_cast<vtkIdType>(this->GlobalCellIdMap->IdTypeMap.size());

      duplicateCellIds = vtkIdList::New();

      for (vtkIdType cid = 0; cid < newNumCells; cid++)
      {
        vtkIdType globalId = this->GlobalCellIdAccessGetId(cid);

        std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =
          this->GlobalCellIdMap->IdTypeMap.insert(
            std::map<vtkIdType, vtkIdType>::value_type(globalId, nextLocalId));

        if (inserted.second)
        {
          nextLocalId++;
        }
        else
        {
          duplicateCellIds->InsertNextId(cid);
          numDuplicateCells++;
          vtkIdType npts, *pts;
          newGrid->GetCellPoints(cid, npts, pts);
          numDuplicateConnections += (npts + 1);
        }
      }

      if (numDuplicateCells == 0)
      {
        duplicateCellIds->Delete();
        duplicateCellIds = nullptr;
      }
    }
  }

  // Connectivity for the merged grid so far

  vtkCellArray* cellArray = nullptr;
  vtkIdType* cells = nullptr;
  vtkIdType* locs = nullptr;
  vtkIdType* flocs = nullptr;
  vtkIdType* faces = nullptr;
  unsigned char* types = nullptr;

  vtkIdType numCells = 0;
  vtkIdType numConnections = 0;
  vtkIdType numFacesConnections = 0;

  if (!firstSet)
  {
    cellArray = grid->GetCells();
    cells = cellArray->GetPointer();
    locs = grid->GetCellLocationsArray()->GetPointer(0);
    types = grid->GetCellTypesArray()->GetPointer(0);
    flocs = grid->GetFaceLocations() ? grid->GetFaceLocations()->GetPointer(0) : nullptr;
    faces = grid->GetFaces() ? grid->GetFaces()->GetPointer(0) : nullptr;

    numCells = grid->GetNumberOfCells();
    numConnections = cellArray->GetData()->GetNumberOfTuples();
    numFacesConnections = faces ? grid->GetFaces()->GetNumberOfValues() : 0;
  }

  // New output grid: merging of existing and incoming grids

  // CELL ARRAY
  vtkIdType totalNumCells = numCells + newNumCells - numDuplicateCells;
  vtkIdType totalNumConnections = numConnections + newNumConnections - numDuplicateConnections;

  vtkNew<vtkIdTypeArray> mergedcells;
  mergedcells->SetNumberOfValues(totalNumConnections);

  if (!firstSet && cells)
  {
    vtkIdType* idptr = mergedcells->GetPointer(0);
    memcpy(idptr, cells, sizeof(vtkIdType) * numConnections);
  }

  vtkNew<vtkCellArray> finalCellArray;
  finalCellArray->SetCells(totalNumCells, mergedcells);

  // LOCATION ARRAY
  vtkNew<vtkIdTypeArray> locationArray;
  locationArray->SetNumberOfValues(totalNumCells);
  if (!firstSet && locs)
  {
    vtkIdType* iptr = locationArray->GetPointer(0);   // new output dataset
    memcpy(iptr, locs, numCells * sizeof(vtkIdType)); // existing set
  }

  // TYPE ARRAY
  vtkNew<vtkUnsignedCharArray> typeArray;
  typeArray->SetNumberOfValues(totalNumCells);
  if (!firstSet && types)
  {
    unsigned char* cptr = typeArray->GetPointer(0);
    memcpy(cptr, types, numCells * sizeof(unsigned char));
  }

  // FACES LOCATION ARRAY
  vtkNew<vtkIdTypeArray> facesLocationArray;
  facesLocationArray->SetNumberOfValues(totalNumCells);
  if (!firstSet && flocs)
  {
    vtkIdType* fiptr = facesLocationArray->GetPointer(0); // new output dataset
    memcpy(fiptr, flocs, numCells * sizeof(vtkIdType));   // existing set
  }
  else if (!firstSet)
  {
    facesLocationArray->FillComponent(0, -1);
  }

  bool havePolyhedron = false;

  // FACES ARRAY
  vtkNew<vtkIdTypeArray> facesArray;
  facesArray->SetNumberOfValues(numFacesConnections);
  if (!firstSet && faces)
  {
    havePolyhedron = true;
    vtkIdType* faptr = facesArray->GetPointer(0);                  // new output dataset
    memcpy(faptr, faces, numFacesConnections * sizeof(vtkIdType)); // existing set
  }

  // set up new cell data

  vtkIdType finalCellId = numCells;
  vtkIdType nextCellArrayIndex = numConnections;
  vtkCellData* cellArrays = set->GetCellData();

  vtkIdType oldPtId, finalPtId, nextDuplicateCellId = 0;

  for (vtkIdType oldCellId = 0; oldCellId < newNumCells; oldCellId++)
  {
    if (duplicateCellIds && nextDuplicateCellId < duplicateCellIds->GetNumberOfIds())
    {
      vtkIdType skipId = duplicateCellIds->GetId(nextDuplicateCellId);
      if (skipId == oldCellId)
      {
        nextDuplicateCellId++;
        continue;
      }
    }

    vtkIdType npts, *pts;
    newGrid->GetCellPoints(oldCellId, npts, pts);

    locationArray->SetValue(finalCellId, nextCellArrayIndex);
    mergedcells->SetValue(nextCellArrayIndex++, npts);
    unsigned char cellType = newGrid->GetCellType(oldCellId);
    typeArray->SetValue(finalCellId, cellType);

    for (vtkIdType i = 0; i < npts; i++)
    {
      oldPtId = pts[i];
      finalPtId = idMap ? idMap[oldPtId] : this->NumberOfPoints + oldPtId;
      mergedcells->SetValue(nextCellArrayIndex++, finalPtId);
    }

    if (cellType == VTK_POLYHEDRON)
    {
      havePolyhedron = true;
      vtkIdType nfaces, *ptIds;
      newGrid->GetFaceStream(oldCellId, nfaces, ptIds);

      facesLocationArray->SetValue(finalCellId, facesArray->GetNumberOfValues());
      facesArray->InsertNextValue(nfaces);

      for (vtkIdType i = 0; i < nfaces; i++)
      {
        vtkIdType nfpts = *ptIds++;
        facesArray->InsertNextValue(nfpts);
        for (vtkIdType j = 0; j < nfpts; j++)
        {
          oldPtId = *ptIds++;
          finalPtId = idMap ? idMap[oldPtId] : this->NumberOfPoints + oldPtId;
          facesArray->InsertNextValue(finalPtId);
        }
      }
    }
    else
    {
      facesLocationArray->SetValue(finalCellId, -1);
    }

    grid->GetCellData()->CopyData(
      *(this->CellList), cellArrays, this->NextGrid, oldCellId, finalCellId);

    finalCellId++;
  }

  if (havePolyhedron)
  {
    grid->SetCells(typeArray, locationArray, finalCellArray, facesLocationArray, facesArray);
  }
  else
  {
    grid->SetCells(typeArray, locationArray, finalCellArray, nullptr, nullptr);
  }

  if (duplicateCellIds)
  {
    duplicateCellIds->Delete();
  }

  return finalCellId;
}

//-----------------------------------------------------------------------------
void vtkMergeCells::StartUGrid(vtkDataSet* set)
{
  vtkUnstructuredGrid* grid = this->UnstructuredGrid;

  if (!this->InputIsUGrid)
  {
    grid->Allocate(this->TotalNumberOfCells);
  }

  vtkNew<vtkPoints> pts;
  // If the input has a vtkPoints object, we'll make the merged output
  // grid have a vtkPoints object of the same data type.  Otherwise,
  // the merged output grid will have the default of points of type float.
  if (this->InputIsPointSet)
  {
    vtkPointSet* ps = vtkPointSet::SafeDownCast(set);
    pts->SetDataType(ps->GetPoints()->GetDataType());
  }
  pts->SetNumberOfPoints(this->TotalNumberOfPoints); // allocate for upper bound
  grid->SetPoints(pts);

  // Order of field arrays may get changed when data sets are
  // marshalled/sent/unmarshalled.  So we need to re-index the
  // field arrays before copying them using a FieldList
  this->PointList = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);
  this->CellList = new vtkDataSetAttributes::FieldList(this->TotalNumberOfDataSets);

  this->PointList->InitializeFieldList(set->GetPointData());
  this->CellList->InitializeFieldList(set->GetCellData());

  if (this->UseGlobalIds)
  {
    grid->GetPointData()->CopyGlobalIdsOn();
  }
  grid->GetPointData()->CopyAllocate(*this->PointList, this->TotalNumberOfPoints);

  if (this->UseGlobalCellIds)
  {
    grid->GetCellData()->CopyGlobalIdsOn();
  }
  grid->GetCellData()->CopyAllocate(*this->CellList, this->TotalNumberOfCells);
}

//-----------------------------------------------------------------------------
void vtkMergeCells::Finish()
{
  this->FreeLists();

  vtkUnstructuredGrid* grid = this->UnstructuredGrid;

  if (this->NumberOfPoints < this->TotalNumberOfPoints)
  {
    // if we don't do this, grid->GetNumberOfPoints() gives the wrong value
    grid->GetPoints()->GetData()->Resize(this->NumberOfPoints);
  }

  grid->Squeeze();
}

//-----------------------------------------------------------------------------
// Use an array of global node ids to map all points to
// their new ids in the merged grid.
vtkIdType* vtkMergeCells::MapPointsToIdsUsingGlobalIds(vtkDataSet* set)
{
  if (!this->GlobalNodeIdAccessStart(set))
  {
    vtkErrorMacro("global id array is not available");
    return nullptr;
  }

  vtkIdType npoints = set->GetNumberOfPoints();

  vtkIdType* idMap = new vtkIdType[npoints];

  vtkIdType nextNewLocalId = static_cast<vtkIdType>(this->GlobalIdMap->IdTypeMap.size());

  // map global point ids to ids in the new data set

  for (vtkIdType oldId = 0; oldId < npoints; oldId++)
  {
    vtkIdType globalId = this->GlobalNodeIdAccessGetId(oldId);

    std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =
      this->GlobalIdMap->IdTypeMap.insert(
        std::map<vtkIdType, vtkIdType>::value_type(globalId, nextNewLocalId));

    if (inserted.second)
    {
      // this is a new global node id
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

//-----------------------------------------------------------------------------
// Use a spatial locator to filter out duplicate points and map
// the new ids to their ids in the merged grid.
vtkIdType* vtkMergeCells::MapPointsToIdsUsingLocator(vtkDataSet* set)
{
  vtkUnstructuredGrid* grid = this->UnstructuredGrid;
  vtkPoints* points0 = grid->GetPoints();
  vtkIdType npoints0 = this->NumberOfPoints;

  vtkPointSet* ps = vtkPointSet::SafeDownCast(set);
  vtkIdType npoints1 = set->GetNumberOfPoints();
  vtkSmartPointer<vtkPoints> points1;

  if (ps)
  {
    points1 = ps->GetPoints();
  }
  else
  {
    points1 = vtkSmartPointer<vtkPoints>::New();
    points1->SetNumberOfPoints(npoints1);

    for (vtkIdType ptId = 0; ptId < npoints1; ptId++)
    {
      points1->SetPoint(ptId, set->GetPoint(ptId));
    }
  }

  vtkIdType* idMap = new vtkIdType[npoints1];

  if (this->PointMergeTolerance == 0.0)
  {
    // testing shows vtkMergePoints is fastest when tolerance is 0
    vtkNew<vtkMergePoints> locator;
    vtkNew<vtkPoints> ptarray;

    double bounds[6];
    set->GetBounds(bounds);

    if (npoints0 > 0)
    {
      double tmpBounds[6];

      // Prior to MapPointsToIdsUsingLocator(), points0->SetNumberOfPoints()
      // has been called to set the number of points to the upper bound on the
      // points TO BE merged and now points0->GetNumberOfPoints() does not
      // refer to the number of the points merged so far. Thus we need to
      // temporarily set the number to the latter such that grid->GetBounds()
      // is able to return the correct bounding information. This is a fix to
      // bug #0009626.

      points0->GetData()->SetNumberOfTuples(npoints0);
      grid->GetBounds(tmpBounds); // safe to call GetBounds() for real info
      points0->GetData()->SetNumberOfTuples(this->TotalNumberOfPoints);

      bounds[0] = ((tmpBounds[0] < bounds[0]) ? tmpBounds[0] : bounds[0]);
      bounds[2] = ((tmpBounds[2] < bounds[2]) ? tmpBounds[2] : bounds[2]);
      bounds[4] = ((tmpBounds[4] < bounds[4]) ? tmpBounds[4] : bounds[4]);

      bounds[1] = ((tmpBounds[1] > bounds[1]) ? tmpBounds[1] : bounds[1]);
      bounds[3] = ((tmpBounds[3] > bounds[3]) ? tmpBounds[3] : bounds[3]);
      bounds[5] = ((tmpBounds[5] > bounds[5]) ? tmpBounds[5] : bounds[5]);
    }

    locator->InitPointInsertion(ptarray, bounds);

    vtkIdType newId;
    double x[3];

    for (vtkIdType ptId = 0; ptId < npoints0; ptId++)
    {
      // We already know there are no duplicates in this array.
      // Just add them to the locator's point array.

      points0->GetPoint(ptId, x);
      locator->InsertUniquePoint(x, newId);
    }
    for (vtkIdType ptId = 0; ptId < npoints1; ptId++)
    {
      points1->GetPoint(ptId, x);
      locator->InsertUniquePoint(x, newId);

      idMap[ptId] = newId;
    }
  }
  else
  {
    // testing shows vtkKdTree is fastest when tolerance is > 0
    vtkKdTree* kd = vtkKdTree::New();

    vtkPoints* ptArrays[2];
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

    vtkIdType nextNewLocalId = npoints0;

    kd->BuildLocatorFromPoints(ptArrays, numArrays);

    vtkIdTypeArray* pointToEquivClassMap =
      kd->BuildMapForDuplicatePoints(this->PointMergeTolerance);

    kd->Delete();

    if (npoints0 > 0)
    {
      points0->GetData()->SetNumberOfTuples(this->TotalNumberOfPoints);
    }

    // The map we get back isn't quite what we need. The range of
    // the map is a subset of original point IDs which each
    // represent an equivalence class of duplicate points. But the
    // point chosen to represent the class could be any one of the
    // equivalent points. We need to create a map that uses IDs
    // of points in the points0 array as the representative, and
    // then new logical contiguous point IDs
    // (npoints0, npoints0+1, ..., numUniquePoints-1) for the
    // points in the new set that are not duplicates of points
    // in the points0 array.
    std::map<vtkIdType, vtkIdType> newIdMap;

    if (npoints0 > 0) // these were already a unique set
    {
      for (vtkIdType ptId = 0; ptId < npoints0; ptId++)
      {
        vtkIdType eqClassRep = pointToEquivClassMap->GetValue(ptId);

        if (eqClassRep != ptId)
        {
          newIdMap.insert(std::map<vtkIdType, vtkIdType>::value_type(eqClassRep, ptId));
        }
      }
    }
    for (vtkIdType ptId = 0; ptId < npoints1; ptId++)
    {
      vtkIdType eqClassRep = pointToEquivClassMap->GetValue(ptId + npoints0);

      if (eqClassRep < npoints0)
      {
        idMap[ptId] = eqClassRep; // a duplicate of a point in the first set
        continue;
      }

      std::pair<std::map<vtkIdType, vtkIdType>::iterator, bool> inserted =
        newIdMap.insert(std::map<vtkIdType, vtkIdType>::value_type(eqClassRep, nextNewLocalId));

      bool newEqClassRep = inserted.second;

      if (newEqClassRep)
      {
        idMap[ptId] = nextNewLocalId; // here's a new unique point
        nextNewLocalId++;
      }
      else
      {
        idMap[ptId] = inserted.first->second; // a duplicate of a point in the new set
      }
    }

    pointToEquivClassMap->Delete();
    newIdMap.clear();
  }

  return idMap;
}

//-------------------------------------------------------------------------
// Help with the complex business of efficient access to the node ID arrays.
// The array was given to us by the user, and we don't know the data type or
// size.
vtkIdType vtkMergeCells::GlobalCellIdAccessGetId(vtkIdType idx)
{
  if (this->GlobalCellIdArray)
  {
    switch (this->GlobalCellIdArrayType)
    {
      vtkTemplateMacro(VTK_TT* ids = static_cast<VTK_TT*>(this->GlobalCellIdArray);
                       return static_cast<vtkIdType>(ids[idx]));
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkMergeCells::GlobalCellIdAccessStart(vtkDataSet* set)
{
  if (this->UseGlobalCellIds)
  {
    vtkDataArray* da = set->GetCellData()->GetGlobalIds();
    if (da)
    {
      this->GlobalCellIdArray = da->GetVoidPointer(0);
      this->GlobalCellIdArrayType = da->GetDataType();
      return true;
    }
  }
  this->GlobalCellIdArray = nullptr;
  return false;
}

//-----------------------------------------------------------------------------
vtkIdType vtkMergeCells::GlobalNodeIdAccessGetId(vtkIdType idx)
{
  if (this->GlobalIdArray)
  {
    switch (this->GlobalIdArrayType)
    {
      vtkTemplateMacro(VTK_TT* ids = static_cast<VTK_TT*>(this->GlobalIdArray);
                       return static_cast<vtkIdType>(ids[idx]));
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
bool vtkMergeCells::GlobalNodeIdAccessStart(vtkDataSet* set)
{
  if (this->UseGlobalIds)
  {
    vtkDataArray* da = set->GetPointData()->GetGlobalIds();
    if (da)
    {
      this->GlobalIdArray = da->GetVoidPointer(0);
      this->GlobalIdArrayType = da->GetDataType();
      return true;
    }
  }

  this->GlobalIdArray = nullptr;
  return false;
}

//-----------------------------------------------------------------------------
void vtkMergeCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

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
  os << indent << "PointList: " << this->PointList << endl;
  os << indent << "CellList: " << this->CellList << endl;
  os << indent << "UseGlobalIds: " << this->UseGlobalIds << endl;
  os << indent << "UseGlobalCellIds: " << this->UseGlobalCellIds << endl;
}
