/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkCutter.h"
#include "vtkMergePoints.h"
#include "vtkImplicitFunction.h"
#include "vtkContourValues.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

//---------------------------------------------------------------------------
vtkCutter* vtkCutter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCutter");
  if(ret)
    {
    return (vtkCutter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCutter;
}

// Construct with user-specified implicit function; initial value of 0.0; and
// generating cut scalars turned off.
vtkCutter::vtkCutter(vtkImplicitFunction *cf)
{
  this->ContourValues = vtkContourValues::New();
  this->SortBy = VTK_SORT_BY_VALUE;
  this->CutFunction = cf;
  this->GenerateCutScalars = 0;
  this->Locator = NULL;
}

vtkCutter::~vtkCutter()
{
  this->ContourValues->Delete();
  this->SetCutFunction(NULL);
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Overload standard modified time function. If cut functions is modified,
// or contour values modified, then this object is modified as well.
unsigned long vtkCutter::GetMTime()
{
  unsigned long mTime=this->vtkDataSetToPolyDataFilter::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();
  unsigned long time;
 
  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  if ( this->CutFunction != NULL )
    {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Locator != NULL )
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//
// Cut through data generating surface.
//
void vtkCutter::Execute()
{
  int cellId, i, iter;
  vtkPoints *cellPts;
  vtkScalars *cellScalars=vtkScalars::New();
  vtkGenericCell *cell;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPoints;
  vtkFloatArray *cutScalars;
  float value, s;
  vtkPolyData *output = this->GetOutput();
  vtkDataSet *input=this->GetInput();
  int estimatedSize, numCells=input->GetNumberOfCells();
  int numPts=input->GetNumberOfPoints(), numCellPts;
  vtkPointData *inPD, *outPD;
  vtkCellData *inCD=input->GetCellData(), *outCD=output->GetCellData();
  vtkIdList *cellIds;
  int numContours=this->ContourValues->GetNumberOfContours();
  int abortExecute=0;
  
  vtkDebugMacro(<< "Executing cutter");
  
  // Initialize self; do some error checking
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

  // Create objects to hold output of contour operation
  //
  estimatedSize = (int) pow ((double) numCells, .75) * numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPoints = vtkPoints::New();
  newPoints->Allocate(estimatedSize,estimatedSize/2);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize/2);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize/2);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize/2);
  cutScalars = vtkFloatArray::New();
  cutScalars->SetNumberOfTuples(numPts);

  // Interpolate data along edge. If generating cut scalars, do necessary setup
  if ( this->GenerateCutScalars )
    {
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());//copies original attributes
    inPD->SetScalars(cutScalars);
    }
  else 
    {
    inPD = input->GetPointData();
    }
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);
    
  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  // Loop over all points evaluating scalar function at each point
  //
  for ( i=0; i < numPts; i++ )
    {
    s = this->CutFunction->FunctionValue(input->GetPoint(i));
    cutScalars->SetComponent(i,0,s);
    }

  // Compute some information for progress methods
  //
  cell = vtkGenericCell::New();
  int numCuts = numContours*numCells;
  int progressInterval = numCuts/20 + 1;
  int cut=0;
  
  if ( this->SortBy == VTK_SORT_BY_CELL )
    {
    // Loop over all contour values.  Then for each contour value, 
    // loop over all cells.
    //
    for (iter=0; iter < numContours && !abortExecute; iter++)
      {
      // Loop over all cells; get scalar values for all cell points
      // and process each cell.
      //
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        if ( !(++cut % progressInterval) )
          {
          vtkDebugMacro(<<"Cutting #" << cut);
          this->UpdateProgress ((float)cut/numCuts);
          abortExecute = this->GetAbortExecute();
          }

        input->GetCell(cellId,cell);
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        numCellPts = cellPts->GetNumberOfPoints();
        cellScalars->SetNumberOfScalars(numCellPts);
        for (i=0; i < numCellPts; i++)
          {
          s = cutScalars->GetComponent(cellIds->GetId(i),0);
          cellScalars->SetScalar(i,s);
          }

        value = this->ContourValues->GetValue(iter);
        cell->Contour(value, cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPD, outPD,
                      inCD, cellId, outCD);

        } // for all cells
      } // for all contour values
    } // sort by cell

  else // VTK_SORT_BY_VALUE:
    {
    // Loop over all cells; get scalar values for all cell points
    // and process each cell.
    //
    for (cellId=0; cellId < numCells && !abortExecute; cellId++)
      {
      input->GetCell(cellId,cell);
      cellPts = cell->GetPoints();
      cellIds = cell->GetPointIds();

      numCellPts = cellPts->GetNumberOfPoints();
      cellScalars->SetNumberOfScalars(numCellPts);
      for (i=0; i < numCellPts; i++)
        {
        s = cutScalars->GetComponent(cellIds->GetId(i),0);
        cellScalars->SetScalar(i,s);
        }

      // Loop over all contour values.
      for (iter=0; iter < numContours && !abortExecute; iter++)
        {
        if ( !(++cut % progressInterval) )
          {
          vtkDebugMacro(<<"Cutting #" << cut);
          this->UpdateProgress ((float)cut/numCuts);
          abortExecute = this->GetAbortExecute();
          }
        value = this->ContourValues->GetValue(iter);
        cell->Contour(value, cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPD, outPD,
                      inCD, cellId, outCD);

        } // for all contour values
      } // for all cells
    } // sort by value

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory. 
  //
  cell->Delete();
  cellScalars->Delete();
  cutScalars->Delete();

  if ( this->GenerateCutScalars )
    {
    inPD->Delete();
    }

  output->SetPoints(newPoints);
  newPoints->Delete();

  if (newVerts->GetNumberOfCells())
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();
}

// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkCutter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkCutter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Cut Function: " << this->CutFunction << "\n";

  os << indent << "Sort By: " << this->GetSortByAsString() << "\n";

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }

  this->ContourValues->PrintSelf(os,indent);

  os << indent << "Generate Cut Scalars: " 
     << (this->GenerateCutScalars ? "On\n" : "Off\n");
}
