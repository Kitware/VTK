/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CleanP.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "CleanP.hh"
#include "MergePts.hh"

// Description:
// Construct object with initial tolerance of 0.0.
vtkCleanPolyData::vtkCleanPolyData()
{
  this->Tolerance = 0.0;
  this->Locator = NULL;
  this->SelfCreatedLocator = 0;
}

vtkCleanPolyData::~vtkCleanPolyData()
{
  if ( this->SelfCreatedLocator && this->Locator != NULL) 
    delete this->Locator;
}

void vtkCleanPolyData::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int numPts=input->GetNumberOfPoints();
  vtkFloatPoints *newPts;
  int numNewPts;
  
  vtkPointData *pd;
  vtkPoints *inPts;
  int *Index;
  int i, j, count;
  int npts, *pts, updatedPts[MAX_CELL_SIZE];
  vtkCellArray *inVerts=input->GetVerts(), *newVerts=NULL;
  vtkCellArray *inLines=input->GetLines(), *newLines=NULL;
  vtkCellArray *inPolys=input->GetPolys(), *newPolys=NULL;
  vtkCellArray *inStrips=input->GetStrips(), *newStrips=NULL;

  vtkDebugMacro(<<"Cleaning data");
  this->Initialize();

  if ( numPts < 1 || (inPts=input->GetPoints()) == NULL )
    {
    vtkErrorMacro(<<"No data to clean!");
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
  newPts = new vtkFloatPoints(numPts);

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

  vtkDebugMacro(<<"Removed " << numPts-numNewPts << " points");

//
// Begin to adjust topology.
//
  // Vertices are renumbered and we remove duplicate vertices
  if ( inVerts->GetNumberOfCells() > 0 )
    {
    int resultingNumPoints;
    int found;
    int nnewpts, *newpts;
    int k;

    newVerts = new vtkCellArray;
    for (inVerts->InitTraversal(); inVerts->GetNextCell(npts,pts); )
      {
      resultingNumPoints = 0;
      for (j=0; j < npts; j++) 
	{
	// is the vertex already there
	found = 0;
	for (newVerts->InitTraversal(); newVerts->GetNextCell(nnewpts,newpts);)
	  {
	  for (k = 0; k < nnewpts; k++)
	    {
	    if (newpts[k] == Index[pts[j]])
	      {
	      found = 1;
	      }
	    }
	  }
	for (k = 0; k < resultingNumPoints; k++)
	  {
	  if (updatedPts[k] == Index[pts[j]])
	    {
	    found = 1;
	    }
	  }
	if (!found)
	  {
	  updatedPts[resultingNumPoints++] = Index[pts[j]];
	  }
	}
      if (resultingNumPoints)
	{
	newVerts->InsertNextCell(resultingNumPoints, updatedPts);
	}
      }
    newVerts->Squeeze();
    }

  // lines reduced to one point are eliminated
  if ( inLines->GetNumberOfCells() > 0 )
    {
    newLines = new vtkCellArray(inLines->GetSize());

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
    vtkDebugMacro(<<"Removed " << inLines->GetNumberOfCells() -
                 newLines->GetNumberOfCells() << " lines");
    }

  // polygons reduced to two points or less are eliminated
  if ( inPolys->GetNumberOfCells() > 0 )
    {
    newPolys = new vtkCellArray(inPolys->GetSize());

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
    vtkDebugMacro(<<"Removed " << inPolys->GetNumberOfCells() -
                 newPolys->GetNumberOfCells() << " polys");
    }

  // triangle strips reduced to two points or less are eliminated
  if ( inStrips->GetNumberOfCells() > 0 ) 
    {
    newStrips = new vtkCellArray(inStrips->GetSize());

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
    vtkDebugMacro(<<"Removed " << inStrips->GetNumberOfCells() -
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
// default an instance of vtkLocator is used.
void vtkCleanPolyData::SetLocator(vtkLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) delete this->Locator;
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkCleanPolyData::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) delete this->Locator;

  if ( this->Tolerance <= 0.0 )
    this->Locator = new vtkMergePoints;
  else
    this->Locator = new vtkLocator;

  this->SelfCreatedLocator = 1;
}

void vtkCleanPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";

}

