/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AppendF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "AppendF.hh"

vtkAppendFilter::vtkAppendFilter()
{

}

vtkAppendFilter::~vtkAppendFilter()
{
}

// Description:
// Add a dataset to the list of data to append.
void vtkAppendFilter::AddInput(vtkDataSet *ds)
{
  if ( ! this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to append.
void vtkAppendFilter::RemoveInput(vtkDataSet *ds)
{
  if ( this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.RemoveItem(ds);
    }
}

void vtkAppendFilter::Update()
{
  unsigned long int mtime, dsMtime;
  vtkDataSet *ds;

  // make sure input is available
  if ( this->InputList.GetNumberOfItems() < 1 )
    {
    vtkErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
    {
    ds->Update();
    dsMtime = ds->GetMTime();
    if ( dsMtime > mtime ) mtime = dsMtime;
    }
  this->Updating = 0;

  if (mtime > this->GetMTime() || this->GetMTime() > this->ExecuteTime ||
  this->GetDataReleased() )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    this->SetDataReleased(0);
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }

  for (this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
    if ( ds->ShouldIReleaseData() ) ds->ReleaseData();
}

// Append data sets into single unstructured grid
void vtkAppendFilter::Execute()
{
  int scalarsPresent, vectorsPresent, normalsPresent, tcoordsPresent;
  int tensorsPresent, userDefinedPresent;
  int numPts, numCells, ptOffset;
  vtkFloatPoints *newPts;
  vtkPointData *pd;
  vtkIdList ptIds(MAX_CELL_SIZE), newPtIds(MAX_CELL_SIZE);
  int i;
  vtkDataSet *ds;
  int ptId, cellId;

  vtkDebugMacro(<<"Appending data together");
  this->Initialize();

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  scalarsPresent = 1;
  vectorsPresent = 1;
  normalsPresent = 1;
  tcoordsPresent = 1;
  tensorsPresent = 1;
  userDefinedPresent = 1;

  for ( this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
    {
    numPts += ds->GetNumberOfPoints();
    numCells += ds->GetNumberOfCells();
    pd = ds->GetPointData();
    if ( pd->GetScalars() == NULL ) scalarsPresent &= 0;
    if ( pd->GetVectors() == NULL ) vectorsPresent &= 0;
    if ( pd->GetNormals() == NULL ) normalsPresent &= 0;
    if ( pd->GetTCoords() == NULL ) tcoordsPresent &= 0;
    if ( pd->GetTensors() == NULL ) tensorsPresent &= 0;
    if ( pd->GetUserDefined() == NULL ) userDefinedPresent &= 0;
    }

  if ( numPts < 1 || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to append!");
    return;
    }

// Now can allocate memory
  this->Allocate(numCells); //allocate storage for geometry/topology
  if ( !scalarsPresent ) this->PointData.CopyScalarsOff();
  if ( !vectorsPresent ) this->PointData.CopyVectorsOff();
  if ( !normalsPresent ) this->PointData.CopyNormalsOff();
  if ( !tcoordsPresent ) this->PointData.CopyTCoordsOff();
  if ( !tensorsPresent ) this->PointData.CopyTensorsOff();
  if ( !userDefinedPresent ) this->PointData.CopyUserDefinedOff();
  this->PointData.CopyAllocate(pd,numPts);

  newPts = new vtkFloatPoints(numPts);

  for ( ptOffset=0, this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); ptOffset+=numPts)
    {
    numPts = ds->GetNumberOfPoints();
    numCells = ds->GetNumberOfCells();
    pd = ds->GetPointData();

    // copy points and point data
    for (ptId=0; ptId < numPts; ptId++)
      {
      newPts->SetPoint(ptId+ptOffset,ds->GetPoint(ptId));
      this->PointData.CopyData(pd,ptId,ptId+ptOffset);
      }

    // copy cells
    for (cellId=0; cellId < numCells; cellId++)
      {
      ds->GetCellPoints(cellId,ptIds);
      for (i=0; i < ptIds.GetNumberOfIds(); i++)
        newPtIds.SetId(i,ptIds.GetId(i)+ptOffset);
      this->InsertNextCell(ds->GetCellType(cellId),newPtIds);
      }
    }

// Update ourselves
  this->SetPoints(newPts);
}

void vtkAppendFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGrid::PrintSelf(os,indent);
  vtkFilter::_PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}

int vtkAppendFilter::GetDataReleased()
{
  return this->DataReleased;
}

void vtkAppendFilter::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

