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
  unsigned long int mtime, pdMtime;
  vlPolyData *pd;

  // make sure input is available
  if ( this->InputList.GetNumberOfItems() < 1 )
    {
    vlErrorMacro(<< "No input!");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  for (mtime=0, this->InputList.InitTraversal(); pd = this->InputList.GetNextItem(); )
    {
    pd->Update();
    pdMtime = pd->GetMTime();
    if ( pdMtime > mtime ) mtime = pdMtime;
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

  for (this->InputList.InitTraversal(); pd = this->InputList.GetNextItem(); )
    if ( pd->ShouldIReleaseData() ) pd->ReleaseData();
}

// Append data sets into single unstructured grid
void vlAppendPolyData::Execute()
{
  int scalarsPresent, vectorsPresent, normalsPresent, tcoordsPresent;
  int tensorsPresent, userDefinedPresent;
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
  int npts, *pts;

  vlDebugMacro(<<"Appending data together");
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
    vlErrorMacro(<<"No data to append!");
    return;
    }

// Now can allocate memory
  if ( !scalarsPresent ) this->PointData.CopyScalarsOff();
  if ( !vectorsPresent ) this->PointData.CopyVectorsOff();
  if ( !normalsPresent ) this->PointData.CopyNormalsOff();
  if ( !tcoordsPresent ) this->PointData.CopyTCoordsOff();
  if ( !tensorsPresent ) this->PointData.CopyTensorsOff();
  if ( !userDefinedPresent ) this->PointData.CopyUserDefinedOff();
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
      newVerts->InsertNextCell(npts);
      for (i=0; i < npts; i++) newVerts->InsertCellPoint(pts[i]+ptOffset);
      }
  
    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
      {
      newLines->InsertNextCell(npts);
      for (i=0; i < npts; i++) newLines->InsertCellPoint(pts[i]+ptOffset);
      }
  
    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
      {
      newPolys->InsertNextCell(npts);
      for (i=0; i < npts; i++) newPolys->InsertCellPoint(pts[i]+ptOffset);
      }
  
    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      newStrips->InsertNextCell(npts);
      for (i=0; i < npts; i++) newStrips->InsertCellPoint(pts[i]+ptOffset);
      }
    }
//
// Update ourselves
//
  this->SetPoints(newPts);

  if ( newVerts->GetNumberOfCells() > 0 ) this->SetVerts(newVerts);
  else delete newVerts;

  if ( newLines->GetNumberOfCells() > 0 ) this->SetLines(newLines);
  else delete newLines;

  if ( newPolys->GetNumberOfCells() > 0 ) this->SetPolys(newPolys);
  else delete newPolys;

  if ( newStrips->GetNumberOfCells() > 0 ) this->SetStrips(newStrips);
  else delete newStrips;

  this->Squeeze();
}

int vlAppendPolyData::GetDataReleased()
{
  return this->DataReleased;
}

void vlAppendPolyData::SetDataReleased(int flag)
{
  this->DataReleased = flag;
}

void vlAppendPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlFilter::_PrintSelf(os,indent);

  os << indent << "Input DataSets:\n";
  this->InputList.PrintSelf(os,indent.GetNextIndent());
}

