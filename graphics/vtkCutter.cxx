/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.cxx
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
#include <math.h>
#include "vtkCutter.h"
#include "vtkMergePoints.h"
#include "vtkImplicitFunction.h"
#include "vtkContourValues.h"

// Description:
// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
vtkCutter::vtkCutter(vtkImplicitFunction *cf)
{
  this->SortBy = VTK_SORT_BY_VALUE;
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = NULL;
  this->SelfCreatedLocator = 0;
}

// Description:
// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
unsigned long vtkCutter::GetMTime()
{
  unsigned long mTime=this->vtkDataSetFilter::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
  unsigned long cutFuncMTime;
 
  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->CutFunction != NULL )
    {
    cutFuncMTime = this->CutFunction->GetMTime();
    mTime = ( cutFuncMTime > mTime ? cutFuncMTime : mTime );
    }

  return mTime;
}

//
// Cut through data generating surface.
//
void vtkCutter::Execute()
{
  int cellId, i, iter;
  vtkFloatPoints *cellPts;
  vtkFloatScalars *cellScalars=vtkFloatScalars::New();
  vtkCell *cell;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPoints;
  vtkFloatScalars *cutScalars;
  float value, s;
  vtkPolyData *output = this->GetOutput();
  vtkDataSet *input=this->GetInput();
  int estimatedSize, numCells=input->GetNumberOfCells();
  int numPts=input->GetNumberOfPoints(), numCellPts;
  vtkPointData *inPD, *outPD;
  vtkIdList *cellIds;
  int numContours=this->ContourValues->GetNumberOfContours();
  
  vtkDebugMacro(<< "Executing cutter");
  
  //
  // Initialize self; create output objects
  //
  if ( !this->CutFunction )
    {
    vtkErrorMacro(<<"No cut function specified");
    return;
    }

  if ( numPts < 1 )
    {
    vtkErrorMacro(<<"No data to cut");
    return;
    }
//
// Create objects to hold output of contour operation
//
  estimatedSize = (int) pow ((double) numCells, .75) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  newPoints = vtkFloatPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);
  cutScalars = vtkFloatScalars::New();
  cutScalars->Allocate(numPts);

  // Interpolate data along edge. If generating cut scalars, do the necessary setup.
  if ( this->GenerateCutScalars )
    {
    inPD = new vtkPointData(*(input->GetPointData()));
    inPD->SetScalars(cutScalars);
    }
  else 
    {
    inPD = input->GetPointData();
    }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
    
  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  //
  // Loop over all cells creating scalar function determined by evaluating cell
  // points using cut function.
  //
  for ( i=0; i < numPts; i++ )
    {
    s = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->InsertScalar(i,s);
    }

  switch (this->SortBy)
    {
    case VTK_SORT_BY_CELL:
      {
    //
    // Loop over all contour values.  Then for each contour value, 
    // loop over all cells.
    //
      for (iter=0; iter < numContours; iter++)
        {
      //
      // Loop over all cells creating scalar function determined by evaluating cell
      // points using cut function.
      //
        for (cellId=0; cellId < numCells; cellId++)
          {
          cell = input->GetCell(cellId);
          cellPts = cell->GetPoints();
          cellIds = cell->GetPointIds();

          numCellPts = cellPts->GetNumberOfPoints();
          cellScalars->SetNumberOfScalars(numCellPts);
          for (i=0; i < numCellPts; i++)
            {
            s = cutScalars->GetScalar(cellIds->GetId(i));
            cellScalars->SetScalar(i,s);
            }

          value = this->ContourValues->GetValue(iter);
          cell->Contour(value, cellScalars, this->Locator, 
                    newVerts, newLines, newPolys, inPD, outPD);

          } // for all cells
        } // for all contour values
      } // sort by cell
    case VTK_SORT_BY_VALUE:
      {
    //
    // Loop over all cells creating scalar function determined by evaluating cell
    // points using cut function.
    //
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = input->GetCell(cellId);
      cellPts = cell->GetPoints();
      cellIds = cell->GetPointIds();

      numCellPts = cellPts->GetNumberOfPoints();
      cellScalars->SetNumberOfScalars(numCellPts);
      for (i=0; i < numCellPts; i++)
        {
        s = cutScalars->GetScalar(cellIds->GetId(i));
        cellScalars->SetScalar(i,s);
        }

    //
    // Loop over all contour values.  Then for each contour value, 
    // loop over all cells.
    //
      for (iter=0; iter < numContours; iter++)
        {
        value = this->ContourValues->GetValue(iter);
        cell->Contour(value, cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPD, outPD);

        } // for all contour values
      } // for all cells
    } // sort by value
  } // end switch
//
// Update ourselves.  Because we don't know upfront how many verts, lines,
// polys we've created, take care to reclaim memory. 
//
  cellScalars->Delete();
  cutScalars->Delete();

  if ( this->GenerateCutScalars ) inPD->Delete();

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells()) output->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) output->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) output->SetPolys(newPolys);
  newPolys->Delete();

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkCutter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkCutter::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = vtkMergePoints::New();
  this->SelfCreatedLocator = 1;
}

void vtkCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Generate Cut Scalars: " << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
