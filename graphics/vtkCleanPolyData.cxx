/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCleanPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkCleanPolyData.hh"
#include "vtkMergePoints.hh"

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
    this->Locator->Delete();
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
  int npts, *pts, *updatedPts= new int[input->GetMaxCellSize()];
  vtkCellArray *inVerts=input->GetVerts(), *newVerts=NULL;
  vtkCellArray *inLines=input->GetLines(), *newLines=NULL;
  vtkCellArray *inPolys=input->GetPolys(), *newPolys=NULL;
  vtkCellArray *inStrips=input->GetStrips(), *newStrips=NULL;
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<<"Cleaning data");

  if ( numPts < 1 || (inPts=input->GetPoints()) == NULL )
    {
    vtkErrorMacro(<<"No data to clean!");
    return;
    }

  pd = input->GetPointData();
  outputPD->CopyAllocate(pd);

  if ( this->Locator == NULL ) this->CreateDefaultLocator();

  this->Locator->SetDataSet(input);

  // compute absolute tolerance from relative given
  this->Locator->SetTolerance(this->Tolerance*input->GetLength());

  // compute merge list
  Index = this->Locator->MergePoints();
  this->Locator->Initialize(); //release memory.
  if (this->SelfCreatedLocator) // in case tolerance is changed
    {
    this->SelfCreatedLocator = 0;
    this->Locator->Delete();
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
      outputPD->CopyData(pd,i,numNewPts);
      numNewPts++;
      }
    }

  // need to reclaim space since we've reduced storage req.
  newPts->Squeeze();
  outputPD->Squeeze();

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
// Update ourselves and release memory
//
  delete [] Index;
  delete [] updatedPts;

  output->SetPoints(newPts);
  newPts->Delete();
  if (newVerts)
    {
    output->SetVerts(newVerts);
    newVerts->Delete();
    }
  if (newLines)
    {
    output->SetLines(newLines);
    newLines->Delete();
    }
  if (newPolys)
    {
    output->SetPolys(newPolys);
    newPolys->Delete();
    }
  if (newStrips)
    {
    output->SetStrips(newStrips);
    newStrips->Delete();
    }
}

// Description:
// Specify a spatial locator for speeding the search process. By
// default an instance of vtkLocator is used.
void vtkCleanPolyData::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkCleanPolyData::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();

  if ( this->Tolerance <= 0.0 )
    this->Locator = new vtkMergePoints;
  else
    this->Locator = new vtkPointLocator;

  this->SelfCreatedLocator = 1;
}

void vtkCleanPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyToPolyFilter::PrintSelf(os,indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

