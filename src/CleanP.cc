/*=========================================================================

  Program:   Visualization Library
  Module:    CleanP.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CleanP.hh"

vlCleanPolyData::vlCleanPolyData()
{
  this->Tolerance = 0.0;
  this->Locator = NULL;
}

vlCleanPolyData::~vlCleanPolyData()
{
  if ( this->Locator ) this->Locator->UnRegister(this);
}

void vlCleanPolyData::Execute()
{
  int numPts=this->Input->GetNumberOfPoints();
  vlFloatPoints *newPts;
  int numNewPts;
  
  vlPointData *pd;
  vlLocator *locator;
  vlPoints *inPts;
  int *Index;
  int i, j, count;
  int npts, *pts, updatedPts[MAX_CELL_SIZE];
  vlCellArray *inVerts=this->Input->GetVerts(), *newVerts=NULL;
  vlCellArray *inLines=this->Input->GetLines(), *newLines=NULL;
  vlCellArray *inPolys=this->Input->GetPolys(), *newPolys=NULL;
  vlCellArray *inStrips=this->Input->GetStrips(), *newStrips=NULL;

  vlDebugMacro(<<"Cleaning data");
  this->Initialize();

  if ( numPts < 1 || (inPts=this->Input->GetPoints()) == NULL )
    {
    vlErrorMacro(<<"No data to clean!");
    return;
    }

  this->PointData.CopyAllocate(pd);

  if ( this->Locator == NULL )
    {
    this->Locator = new vlLocator;
    this->Locator->Register(this);
    }

  this->Locator->SetPoints(inPts);

  // compute absolute tolerance from relative given
  this->Locator->SetTolerance(this->Tolerance*this->Input->GetLength());

  // compute merge list
  Index = this->Locator->MergePoints();
//
//  Load new array of points using index.
//
  newPts = new vlFloatPoints(numPts);

  for (numNewPts=0, i=0; i < numPts; i++) 
    {
    if ( Index[i] == numNewPts ) 
      {
      newPts->SetPoint(numNewPts,inPts->GetPoint(i));
      this->PointData.CopyData(pd,i,numNewPts);
      numNewPts++;
      }
    }

  // need to reclaim space since we've reduced storage req.
  newPts->Squeeze();
  this->PointData.Squeeze();

  vlDebugMacro(<<"Removed " << numPts-numNewPts << " points");

//
// Begin to adjust topology.
//
  // Vertices are just renumbered.
  if ( inVerts->GetNumberOfCells() > 0 )
    {
    newVerts = new vlCellArray(inVerts->GetSize());
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
      {
      for (j=0; j < npts; j++) updatedPts[j] = Index[pts[j]];
      newVerts->InsertNextCell(npts, updatedPts);
      }
    newVerts->Squeeze();
    }

  // lines reduced to one point are eliminated
  if ( inLines->GetNumberOfCells() > 0 )
    {
    newLines = new vlCellArray(inLines->GetSize());

    for (inLines->InitTraversal(); inLines->GetNextCell(npts,pts); )
      {
      updatedPts[0] = Index[pts[0]];
      for (count=1, j=1; j < npts; j++) 
        if ( Index[pts[j]] != Index[pts[j-1]] )
          updatedPts[count++] = Index[pts[j]];

      if ( count >= 2 ) 
        {
        newLines->InsertNextCell(count,updatedPts);
        }
      }
    newLines->Squeeze();
    vlDebugMacro(<<"Removed " << inLines->GetNumberOfCells() -
                 newLines->GetNumberOfCells() << " lines");
    }

  // polygons reduced to two points or less are eliminated
  if ( inPolys->GetNumberOfCells() > 0 )
    {
    newPolys = new vlCellArray(inPolys->GetSize());

    for (inPolys->InitTraversal(); inPolys->GetNextCell(npts,pts); )
      {
      updatedPts[0] = Index[pts[0]];
      for (count=1, j=1; j < npts; j++) 
        if ( Index[pts[j]] != Index[pts[j-1]] )
          updatedPts[count++] = Index[pts[j]];

      if ( Index[pts[0]] == Index[pts[npts-1]] ) count--;

      if ( count >= 3 ) 
        {
        newPolys->InsertNextCell(count,updatedPts);
        }
      }
    newPolys->Squeeze();
    vlDebugMacro(<<"Removed " << inPolys->GetNumberOfCells() -
                 newPolys->GetNumberOfCells() << " polys");
    }

  // triangle strips reduced to two points or less are eliminated
  if ( inStrips->GetNumberOfCells() > 0 ) 
    {
    newStrips = new vlCellArray(inStrips->GetSize());

    for (inStrips->InitTraversal(); inStrips->GetNextCell(npts,pts); )
      {
      updatedPts[0] = Index[pts[0]];
      for (count=1, j=1; j < npts; j++) 
        if ( Index[pts[j]] != Index[pts[j-1]] )
          updatedPts[count++] = Index[pts[j]];

      if ( count >= 3 ) 
        {
        newStrips->InsertNextCell(count,updatedPts);
        }
      }
    newStrips->Squeeze();
    vlDebugMacro(<<"Removed " << inStrips->GetNumberOfCells() -
                 newStrips->GetNumberOfCells() << " strips");
    }
//
// Update ourselves
//
  delete [] Index;

  this->SetPoints(newPts);
  this->SetVerts(newVerts);
  this->SetLines(newLines);
  this->SetPolys(newPolys);
  this->SetStrips(newStrips);

}

void vlCleanPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlCleanPolyData::GetClassName()))
    {
    vlPolyToPolyFilter::PrintSelf(os,indent);

    os << indent << "Tolerance: " << this->Tolerance << "\n";
    }

}

