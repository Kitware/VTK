/*=========================================================================

  Program:   Visualization Library
  Module:    CleanP.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CleanP.hh"
#include "MergePts.hh"

// Description:
// Construct object with initial tolerance of 0.0.
vlCleanPolyData::vlCleanPolyData()
{
  this->Tolerance = 0.0;
  this->Locator = NULL;
  this->SelfCreatedLocator = 0;
}

vlCleanPolyData::~vlCleanPolyData()
{
  if ( this->SelfCreatedLocator && this->Locator != NULL) 
    delete this->Locator;
}

void vlCleanPolyData::Execute()
{
  vlPolyData *input=(vlPolyData *)this->Input;
  int numPts=input->GetNumberOfPoints();
  vlFloatPoints *newPts;
  int numNewPts;
  
  vlPointData *pd;
  vlPoints *inPts;
  int *Index;
  int i, j, count;
  int npts, *pts, updatedPts[MAX_CELL_SIZE];
  vlCellArray *inVerts=input->GetVerts(), *newVerts=NULL;
  vlCellArray *inLines=input->GetLines(), *newLines=NULL;
  vlCellArray *inPolys=input->GetPolys(), *newPolys=NULL;
  vlCellArray *inStrips=input->GetStrips(), *newStrips=NULL;

  vlDebugMacro(<<"Cleaning data");
  this->Initialize();

  if ( numPts < 1 || (inPts=input->GetPoints()) == NULL )
    {
    vlErrorMacro(<<"No data to clean!");
    return;
    }

  pd = input->GetPointData();
  this->PointData.CopyAllocate(pd);

  if ( this->Locator == NULL ) this->CreateDefaultLocator();

  this->Locator->SetPoints(inPts);

  // compute absolute tolerance from relative given
  this->Locator->SetTolerance(this->Tolerance*input->GetLength());

  // compute merge list
  Index = this->Locator->MergePoints();
  this->Locator->Initialize(); //release memory.
  if (this->SelfCreatedLocator) // in case tolerance is changed
    {
    this->SelfCreatedLocator = 0;
    delete this->Locator;
    this->Locator = NULL;
    }
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

// Description:
// Specify a spatial locator for speeding the search process. By
// default an instance of vlLocator is used.
void vlCleanPolyData::SetLocator(vlLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) delete this->Locator;
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vlCleanPolyData::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) delete this->Locator;

  if ( this->Tolerance <= 0.0 )
    this->Locator = new vlMergePoints;
  else
    this->Locator = new vlLocator;

  this->SelfCreatedLocator = 1;
}

void vlCleanPolyData::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";

}

