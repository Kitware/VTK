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
}

// Description:
// Add a dataset to the list of data to append.
void vlAppendPolyData::AddInput(vlPolyData *ds)
{
  if ( ! this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.AddItem(ds);
    }
}

// Description:
// Remove a dataset from the list of data to append.
void vlAppendPolyData::RemoveInput(vlPolyData *ds)
{
  if ( this->InputList.IsItemPresent(ds) )
    {
    this->Modified();
    this->InputList.RemoveItem(ds);
    }
}

void vlAppendPolyData::Update()
{
  unsigned long int mtime, pd_mtime;
  vlPolyData *pd;

  // make sure input is available
  if ( this->InputList.GetNumberOfItems() < 1 )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); pd = this->InputList.GetNextItem(); )
    {
    pd->Update();
    pd_mtime = pd->GetMTime();
    if ( pd_mtime > mtime ) mtime = pd_mtime;
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
  int i, ptId, ptOffset;
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

  for ( this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); )
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

  newVerts = new vlCellArray;
  newVerts->Allocate(numCells*4);

  newLines = new vlCellArray;
  newLines->Allocate(numCells*4);

  newPolys = new vlCellArray;
  newPolys->Allocate(numCells*4);

  newStrips = new vlCellArray;
  newStrips->Allocate(numCells*4);

  // loop over all input sets
  for ( ptOffset=0, this->InputList.InitTraversal(); ds = this->InputList.GetNextItem(); ptOffset+=numPts)
    {
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
      for (i=0; i < npts; i++)  newPtIds[i] = pts[i] + ptOffset;
      newVerts->InsertNextCell(npts,newPtIds);
      }
  
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
      {
      for (i=0; i < npts; i++)  newPtIds[i] = pts[i] + ptOffset;
      newLines->InsertNextCell(npts,newPtIds);
      }
  
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
      {
      for (i=0; i < npts; i++)  newPtIds[i] = pts[i] + ptOffset;
      newPolys->InsertNextCell(npts,newPtIds);
      }
  
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      for (i=0; i < npts; i++)  newPtIds[i] = pts[i] + ptOffset;
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
  vlPolyData::PrintSelf(os,indent);
  vlFilter::_PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}

