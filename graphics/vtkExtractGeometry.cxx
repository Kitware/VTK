/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGeometry.cxx
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
#include "vtkExtractGeometry.h"

// Description:
// Construct object with ExtractInside turned on.
vtkExtractGeometry::vtkExtractGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  this->ExtractInside = 1;
}

// Description:
// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkExtractGeometry::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

void vtkExtractGeometry::Execute()
{
  int ptId, numPts, numCells, i, cellId;
  vtkIdList *cellPts;
  vtkCell *cell;
  int numCellPts, newId, *pointMap;
  vtkPointData *pd;
  float *x;
  float multiplier;
  vtkFloatPoints *newPts;
  vtkIdList newCellPts(VTK_CELL_SIZE);
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  
  vtkDebugMacro(<< "Extracting geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  if ( this->ExtractInside ) multiplier = 1.0;
  else multiplier = -1.0;
//
// Loop over all points determining whether they are inside implicit function.
// Copy if they are.
//
  numPts = this->Input->GetNumberOfPoints();
  numCells = this->Input->GetNumberOfCells();
  pointMap = new int[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++) pointMap[i] = -1;

  output->Allocate(numCells/4); //allocate storage for geometry/topology
  newPts = new vtkFloatPoints(numPts/4,numPts);
  pd = this->Input->GetPointData();
  outputPD->CopyAllocate(pd);
  
  for ( ptId=0; ptId < numPts; ptId++ )
    {
    x = this->Input->GetPoint(ptId);
    if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) < 0.0 )
      {
      newId = newPts->InsertNextPoint(x);
      pointMap[ptId] = newId;
      outputPD->CopyData(pd,ptId,newId);
      }
    }
//
// Now loop over all cells to see whether they are inside implicit
// function. Copy if they are.
//
  for (cellId=0; cellId < this->Input->GetNumberOfCells(); cellId++)
    {
    cell = this->Input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    newCellPts.Reset();
    for ( i=0; i < numCellPts; i++)
      {
      ptId = cellPts->GetId(i);
      if ( pointMap[ptId] < 0 ) break;
      newCellPts.InsertId(i,pointMap[ptId]);
      }

    if ( i >= numCellPts )
      {
      output->InsertNextCell(cell->GetCellType(),newCellPts);
      }
    }
//
// Update ourselves and release memory
//
  delete [] pointMap;

  output->SetPoints(newPts);
  newPts->Delete();

  output->Squeeze();
}

void vtkExtractGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Implicit Function: " << (void *)this->ImplicitFunction << "\n";
  os << indent << "Extract Inside: " << (this->ExtractInside ? "On\n" : "Off\n");
}
