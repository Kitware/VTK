/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractPolyDataGeometry.cxx
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
#include "vtkExtractPolyDataGeometry.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkExtractPolyDataGeometry* vtkExtractPolyDataGeometry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkExtractPolyDataGeometry");
  if(ret)
    {
    return (vtkExtractPolyDataGeometry*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkExtractPolyDataGeometry;
}




// Construct object with ExtractInside turned on.
vtkExtractPolyDataGeometry::vtkExtractPolyDataGeometry(vtkImplicitFunction *f)
{
  this->ImplicitFunction = f;
  if (this->ImplicitFunction)
    {
    this->ImplicitFunction->Register(this);
    }
    
  this->ExtractInside = 1;
}

vtkExtractPolyDataGeometry::~vtkExtractPolyDataGeometry()
{
  this->SetImplicitFunction(NULL);
}

// Overload standard modified time function. If implicit function is modified,
// then this object is modified as well.
unsigned long vtkExtractPolyDataGeometry::GetMTime()
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

void vtkExtractPolyDataGeometry::Execute()
{
  vtkPolyData *input = this->GetInput();
  vtkPointData *pd = input->GetPointData();
  vtkCellData *cd = input->GetCellData();
  vtkPolyData *output = this->GetOutput();
  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();
  vtkIdType ptId, numPts, numCells, i, cellId, *pointMap, newId;
  int npts, allInside;
  vtkIdType updateInterval;
  float multiplier, x[3];
  vtkPoints *newPts;

  vtkDebugMacro(<< "Extracting poly data geometry");

  if ( ! this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  if ( this->ExtractInside )
    {
    multiplier = 1.0;
    }
  else 
    {
    multiplier = -1.0;
    }

  // Loop over all points determining whether they are inside implicit function
  // Copy if they are.
  //
  numPts = input->GetNumberOfPoints();
  numCells = input->GetNumberOfCells();
  pointMap = new vtkIdType[numPts]; // maps old point ids into new
  for (i=0; i < numPts; i++)
    {
    pointMap[i] = -1;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(numPts/4,numPts);
  outputPD->CopyAllocate(pd);
  
  for ( allInside=1, ptId=0; ptId < numPts; ptId++ )
    {
    input->GetPoint(ptId, x);
    if ( (this->ImplicitFunction->FunctionValue(x)*multiplier) <= 0.0 )
      {
      newId = newPts->InsertNextPoint(x);
      pointMap[ptId] = newId;
      outputPD->CopyData(pd,ptId,newId);
      }
    else
      {
      allInside = 0;
      }
    }

  if ( allInside ) //we'll just pass the data through
    {
    output->CopyStructure(input);
    outputCD->PassData(cd);
    newPts->Delete();
    return;
    }

  // Now loop over all cells to see whether they are inside the implicit
  // function. Copy if they are.
  //
  vtkGenericCell *cell=vtkGenericCell::New();
  vtkIdList *ptIds=vtkIdList::New();
  
  output->Allocate(numCells);
  outputCD->CopyAllocate(cd);

  updateInterval = numCells/20 + 1;
  int abort=0;

  // Loop over all cells inserting those that are "in"
  for (cellId=0; cellId < numCells && !abort; cellId++)
    {

    //manage progress reports / early abort
    if ( !(cellId % updateInterval) ) 
      {
      this->UpdateProgress ((float)cellId / numCells);
      abort = this->GetAbortExecute();
      }

    //check to see whether points are inside
    input->GetCell(cellId, cell);
    npts = cell->PointIds->GetNumberOfIds();
    ptIds->SetNumberOfIds(npts);
    for (i=0; i<npts; i++)
      {
      newId = cell->PointIds->GetId(i);
      if ( pointMap[newId] < 0 )
        {
        break;
        }
      ptIds->SetId(i,pointMap[newId]);
      }
    if ( i >= npts )
      {
      newId = output->InsertNextCell(cell->GetCellType(), ptIds);
      outputCD->CopyData(cd, cellId, newId);
      }

    }//for all cells

  // Update ourselves and release memory
  //
  ptIds->Delete();
  delete [] pointMap;
  cell->Delete();
  output->SetPoints(newPts);
  newPts->Delete();

  output->Squeeze();
}

void vtkExtractPolyDataGeometry::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  if (this->ImplicitFunction)
    {
    os << indent << "Implicit Function: " 
       << (void *)this->ImplicitFunction << "\n";
    }
  else
    {
    os << indent << "Implicit Function: (null)\n";      
    }
  os << indent << "Extract Inside: " 
     << (this->ExtractInside ? "On\n" : "Off\n");
}
