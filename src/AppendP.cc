/*=========================================================================

  Program:   Visualization Library
  Module:    AppendP.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "AppendP.hh"

vlAppendPolyData::vlAppendPolyData()
{

}

vlAppendPolyData::~vlAppendPolyData()
{
  vlDataSet *ds;

  for ( int i=0; i < this->Input.GetNumberOfItems(); i++ )
    {
    ds = this->Input.GetItem(i+1);
    ds->UnRegister(this);
    }
}

// Description:
// Add a dataset to the list of data to append.
void vlAppendPolyData::AddInput(vlPolyData *ds)
{
  if ( ! this->Input.IsItemPresent(ds) )
    {
    this->Modified();
    ds->Register(this);
    this->Input.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to append.
void vlAppendPolyData::RemoveInput(vlPolyData *ds)
{
  if ( this->Input.IsItemPresent(ds) )
    {
    this->Modified();
    ds->UnRegister(this);
    this->Input.RemoveItem(ds);
    }
}

void vlAppendPolyData::Update()
{
  unsigned long int mtime, ds_mtime;
  int i;
  vlPolyData *ds;

  // make sure input is available
  if ( this->Input.GetNumberOfItems() < 1 )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, i=0; i < this->Input.GetNumberOfItems(); i++)
    {
    ds = this->Input.GetItem(i+1);
    ds_mtime = ds->GetMTime();
    if ( ds_mtime > mtime ) mtime = ds_mtime;
    ds->Update();
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
void vlAppendPolyData::Execute()
{
  int scalarsPresent, vectorsPresent, normalsPresent, tcoordsPresent;
  vlPolyData *ds;
  vlPoints  *inPts;
  vlFloatPoints *newPts;
  vlCellArray *inVerts, *newVerts;
  vlCellArray *inLines, *newLines;
  vlCellArray *inPolys, *newPolys;
  vlCellArray *inStrips, *newStrips;
  int i, j, ptId, ptOffset;
  int numPts, numCells;
  vlPointData *pd;
  int npts, *pts, newPtIds[MAX_CELL_SIZE];

  vlDebugMacro(<<"Appending data together");
  this->Initialize();

  // loop over all data sets, checking to see what point data is available.
  numPts = 0;
  numCells = 0;
  scalarsPresent = 1;
  vectorsPresent = 1;
  normalsPresent = 1;
  tcoordsPresent = 1;

  for ( i=0; i < this->Input.GetNumberOfItems(); i++)
    {
    ds = this->Input.GetItem(i+1);
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

  newVerts = new vlCellArray;
  newVerts->Allocate(numCells*4);

  newLines = new vlCellArray;
  newLines->Allocate(numCells*4);

  newPolys = new vlCellArray;
  newPolys->Allocate(numCells*4);

  newStrips = new vlCellArray;
  newStrips->Allocate(numCells*4);

  // loop over all input sets
  for ( ptOffset=0, i=0; i < this->Input.GetNumberOfItems(); i++, ptOffset+=numPts)
    {
    ds = this->Input.GetItem(i+1);
    pd = ds->GetPointData();

    numPts = ds->GetNumberOfPoints();
    inPts = ds->GetPoints();
    inVerts = ds->GetVerts();
    inLines = ds->GetLines();
    inPolys = ds->GetPolys();
    inStrips = ds->GetStrips();

    // copy points and point data
    for (ptId=0; ptId < numPts; ptId++)
      {
      newPts->SetPoint(ptId+ptOffset,inPts->GetPoint(ptId));
      this->PointData.CopyData(pd,ptId,ptId+ptOffset);
      }

    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
      {
      for (j=0; j < npts; j++)  newPtIds[j] = pts[j] + ptOffset;
      newVerts->InsertNextCell(npts,newPtIds);
      }
  
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
      {
      for (j=0; j < npts; j++)  newPtIds[j] = pts[j] + ptOffset;
      newLines->InsertNextCell(npts,newPtIds);
      }
  
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
      {
      for (j=0; j < npts; j++)  newPtIds[j] = pts[j] + ptOffset;
      newPolys->InsertNextCell(npts,newPtIds);
      }
  
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      for (j=0; j < npts; j++)  newPtIds[j] = pts[j] + ptOffset;
      newStrips->InsertNextCell(npts,newPtIds);
      }
    }
//
// Update ourselves
//
  newVerts->Squeeze();  
  newLines->Squeeze();  
  newPolys->Squeeze();  
  newStrips->Squeeze();  

  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  this->SetLines(newLines);
  this->SetPolys(newPolys);
  this->SetStrips(newStrips);

}

void vlAppendPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlAppendPolyData::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlPolyData::PrintSelf(os,indent);
    vlFilter::PrintSelf(os,indent);

    os << indent << "Input DataSets:\n";
    this->Input.PrintSelf(os,indent.GetNextIndent());

    this->PrintWatchOff(); // stop worrying about it now
    }
}

