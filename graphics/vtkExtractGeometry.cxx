/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractGeometry.cxx
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
#include "vtkExtractGeometry.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

//-------------------------------------------------------------------------
vtkExtractGeometry* vtkExtractGeometry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractGeometry");
  if(ret)
    {
    return (vtkExtractGeometry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractGeometry;
}

// Construct object with ExtractInside turned on.
vtkExtractGeometry::vtkExtractGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
    {
    this->ImplicitFunction->Register(this);
    }
    
  this->ExtractInside = 1;
  this->ExtractBoundaryCells = 0;
}

vtkExtractGeometry::~vtkExtractGeometry()
{
  this->SetImplicitFunction(NULL);
}

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
  vtkIdType ptId, numPts, numCells, i, cellId, newCellId, newId, *pointMap;
  vtkIdList *cellPts;
  vtkCell *cell;
  int numCellPts;
  float *x;
  float multiplier;
  vtkPoints *newPts;
  vtkIdList *newCellPts;
  vtkDataSet *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  int npts;
  numCells = input->GetNumberOfCells();
  
  vtkDebugMacro(<< "Extracting geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  newCellPts = vtkIdList::New();
  newCellPts->Allocate(VTK_CELL_SIZE);

  if ( this->ExtractInside )
    {
    multiplier = 1.0;
    }
  else 
    {
    multiplier = -1.0;
    }

  // Loop over all points determining whether they are inside the
  // implicit function. Copy the points and point data if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new vtkIdType[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++)
    {
    pointMap[i] = -1;
    }

  output->Allocate(numCells/4); //allocate storage for geometry/topology
  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);
  vtkFloatArray *newScalars;
  
  if ( ! this->ExtractBoundaryCells )
    {
    for ( ptId=0; ptId < numPts; ptId++ )
      {
      x = input->GetPoint(ptId);
      if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) < 0.0 )
        {
        newId = newPts->InsertNextPoint(x);
        pointMap[ptId] = newId;
        outputPD->CopyData(pd,ptId,newId);
        }
      }
    }
  else
    {
    // To extract boundary cells, we have to create supplemental information
    if ( this->ExtractBoundaryCells )
      {
      float val;
      newScalars = vtkFloatArray::New();
      newScalars->SetNumberOfValues(numPts);

      for (ptId=0; ptId < numPts; ptId++ )
        {
        x = input->GetPoint(ptId);
        val = this->ImplicitFunction->FunctionValue(x) * multiplier;
        newScalars->SetValue(ptId, val);
        if ( val < 0.0 )
          {
          newId = newPts->InsertNextPoint(x);
          pointMap[ptId] = newId;
          outputPD->CopyData(pd,ptId,newId);
          }
        }
      }
    }

  // Now loop over all cells to see whether they are inside implicit
  // function (or on boundary if ExtractBoundaryCells is on).
  //
  for (cellId=0; cellId < numCells; cellId++)
    {
    cell = input->GetCell(cellId);
    cellPts = cell->GetPointIds();
    numCellPts = cell->GetNumberOfPoints();

    newCellPts->Reset();
    if ( ! this->ExtractBoundaryCells ) //requires less work
      {
      for ( npts=0, i=0; i < numCellPts; i++, npts++)
        {
        ptId = cellPts->GetId(i);
        if ( pointMap[ptId] < 0 )
          {
          break; //this cell won't be inserted
          }
        else
          {
          newCellPts->InsertId(i,pointMap[ptId]);
          }
        }
      } //if don't want to extract boundary cells
    
    else //want boundary cells
      {
      for ( npts=0, i=0; i < numCellPts; i++ )
        {
        ptId = cellPts->GetId(i);
        if ( newScalars->GetValue(ptId) <= 0.0 )
          {
          npts++;
          }
        }
      if ( npts > 0 )
        {
        for ( i=0; i < numCellPts; i++ )
          {
          ptId = cellPts->GetId(i);
          if ( pointMap[ptId] < 0 )
            {
            x = input->GetPoint(ptId);
            newId = newPts->InsertNextPoint(x);
            pointMap[ptId] = newId;
            outputPD->CopyData(pd,ptId,newId);
            }
          newCellPts->InsertId(i,pointMap[ptId]);
          }
        }//a boundary or interior cell
      }//if mapping boundary cells
      
    if ( npts >= numCellPts || (this->ExtractBoundaryCells && npts > 0) )
      {
      newCellId = output->InsertNextCell(cell->GetCellType(),newCellPts);
      outputCD->CopyData(cd,cellId,newCellId);
      }
    }//for all cells

  // Update ourselves and release memory
  //
  delete [] pointMap;
  newCellPts->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  if ( this->ExtractBoundaryCells )
    {
    newScalars->Delete();
    }

  output->Squeeze();
}

void vtkExtractGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Implicit Function: " 
     << (void *)this->ImplicitFunction << "\n";
  os << indent << "Extract Inside: " 
     << (this->ExtractInside ? "On\n" : "Off\n");
  os << indent << "Extract Boundary Cells: " 
     << (this->ExtractBoundaryCells ? "On\n" : "Off\n");
}
