/*=========================================================================

  Program:   Visualization Library
  Module:    AppendF.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "AppendF.hh"

vlAppendFilter::vlAppendFilter()
{

}

vlAppendFilter::~vlAppendFilter()
{
}

// Description:
// Add a dataset to the list of data to append.
void vlAppendFilter::AddInput(vlDataSet *ds)
{
  if ( ! this->Input.IsItemPresent(ds) )
    {
    this->Modified();
    this->Input.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to append.
void vlAppendFilter::RemoveInput(vlDataSet *ds)
{
  if ( this->Input.IsItemPresent(ds) )
    {
    this->Modified();
    this->Input.RemoveItem(ds);
    }
}

void vlAppendFilter::Update()
{
  unsigned long int mtime, ds_mtime;
  vlDataSet *ds;

  // make sure input is available
  if ( this->Input.GetNumberOfItems() < 1 )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->Input.InitTraversal(); ds = this->Input.GetNextItem(); )
    {
    ds->Update();
    ds_mtime = ds->GetMTime();
    if ( ds_mtime > mtime ) mtime = ds_mtime;
    }
  this->Updating = 0;

  if (mtime > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)(this->StartMethodArg);
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)(this->EndMethodArg);
    }
}

// Append data sets into single unstructured grid
void vlAppendFilter::Execute()
{
  int scalarsPresent, vectorsPresent, normalsPresent, tcoordsPresent;
  int numPts, numCells, ptOffset;
  vlFloatPoints *newPts;
  vlPointData *pd;
  vlIdList ptIds(MAX_CELL_SIZE), newPtIds(MAX_CELL_SIZE);
  int i, j;
  vlDataSet *ds;
  int ptId, cellId;

  vlDebugMacro(<<"Appending data together");
  this->Initialize();

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  scalarsPresent = 1;
  vectorsPresent = 1;
  normalsPresent = 1;
  tcoordsPresent = 1;

  for ( this->Input.InitTraversal(); ds = this->Input.GetNextItem(); )
    {
    numPts += ds->GetNumberOfPoints();
    numCells += ds->GetNumberOfCells();
    pd = ds->GetPointData();
    if ( pd->GetScalars() == NULL ) scalarsPresent &= 0;
    if ( pd->GetVectors() == NULL ) vectorsPresent &= 0;
    if ( pd->GetNormals() == NULL ) normalsPresent &= 0;
    if ( pd->GetTCoords() == NULL ) tcoordsPresent &= 0;
    }

  if ( numPts < 1 || numCells < 1 )
    {
    vlErrorMacro(<<"No data to append!");
    return;
    }

// Now can allocate memory
  if ( !scalarsPresent ) this->PointData.CopyScalarsOff();
  if ( !vectorsPresent ) this->PointData.CopyVectorsOff();
  if ( !normalsPresent ) this->PointData.CopyNormalsOff();
  if ( !tcoordsPresent ) this->PointData.CopyTCoordsOff();
  this->PointData.CopyAllocate(pd,numPts);

  newPts = new vlFloatPoints(numPts);

  for ( ptOffset=0, this->Input.InitTraversal(); ds = this->Input.GetNextItem(); ptOffset+=numPts)
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
      for (j=0; j < ptIds.GetNumberOfIds(); j++)
        newPtIds.SetId(j,ptIds.GetId(i)+ptOffset);
      this->InsertNextCell(ds->GetCellType(cellId),newPtIds);
      }
    }

// Update ourselves
  this->SetPoints(newPts);
}

void vlAppendFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlAppendFilter::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlUnstructuredGrid::PrintSelf(os,indent);
    vlFilter::PrintSelf(os,indent);

    os << indent << "Input DataSets:\n";
    this->Input.PrintSelf(os,indent.GetNextIndent());

    this->PrintWatchOff(); // stop worrying about it now
    }
}

