/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.cxx
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
#include "vtkShrinkFilter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkShrinkFilter* vtkShrinkFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkShrinkFilter");
  if(ret)
    {
    return (vtkShrinkFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkShrinkFilter;
}

vtkShrinkFilter::vtkShrinkFilter(float sf)
{
  sf = ( sf < 0.0 ? 0.0 : (sf > 1.0 ? 1.0 : sf));
  this->ShrinkFactor = sf;
}

void vtkShrinkFilter::Execute()
{
  vtkPoints *newPts;
  int i, j, numIds;
  vtkIdType cellId, numCells, numPts;
  vtkIdType oldId, newId;
  float center[3], *p, pt[3];
  vtkPointData *pd, *outPD;;
  vtkIdList *ptIds, *newPtIds;
  vtkDataSet *input= this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkIdType tenth;
  float decimal;

  vtkDebugMacro(<<"Shrinking cells");

  numCells=input->GetNumberOfCells();
  numPts = input->GetNumberOfPoints();
  if (numCells < 1 || numPts < 1)
    {
    vtkErrorMacro(<<"No data to shrink!");
    return;
    }

  ptIds = vtkIdList::New();
  ptIds->Allocate(VTK_CELL_SIZE);
  newPtIds = vtkIdList::New();
  newPtIds->Allocate(VTK_CELL_SIZE);

  output->Allocate(numCells);
  newPts = vtkPoints::New();
  newPts->Allocate(numPts*8,numPts);
  pd = input->GetPointData();
  outPD = output->GetPointData();
  outPD->CopyAllocate(pd,numPts*8,numPts);

  // Traverse all cells, obtaining node coordinates.  Compute "center" of cell,
  // then create new vertices shrunk towards center.
  //
  tenth   = numCells / 10;
  decimal = 0.0;
  if (tenth == 0)
    {
    tenth = 1;
    }

  for (cellId=0; cellId < numCells; cellId++)
    {
    input->GetCellPoints(cellId, ptIds);
    numIds = ptIds->GetNumberOfIds();

    //abort/progress methods
  
    if (cellId % tenth == 0) 
      {
      decimal += 0.1;
      this->UpdateProgress (decimal);
      if (this->GetAbortExecute())
        {
        break; //out of cell loop
        }
      }

    // get the center of the cell
    center[0] = center[1] = center[2] = 0.0;
    for (i=0; i < numIds; i++)
      {
      p = input->GetPoint(ptIds->GetId(i));
      for (j=0; j < 3; j++)
        {
        center[j] += p[j];
        }
      }
    for (j=0; j<3; j++)
      {
      center[j] /= numIds;
      }

    // Create new points and cells
    newPtIds->Reset();
    for (i=0; i < numIds; i++)
      {
      p = input->GetPoint(ptIds->GetId(i));
      for (j=0; j < 3; j++)
        {
        pt[j] = center[j] + this->ShrinkFactor*(p[j] - center[j]);
        }

      oldId = ptIds->GetId(i);
      newId = newPts->InsertNextPoint(pt);
      newPtIds->InsertId(i,newId);

      outPD->CopyData(pd, oldId, newId);
      }
    output->InsertNextCell(input->GetCellType(cellId), newPtIds);
    }//for all cells

  // Update ourselves and release memory
  //
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  output->Squeeze();

  ptIds->Delete();
  newPtIds->Delete();
  newPts->Delete();
}

void vtkShrinkFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);

  os << indent << "Shrink Factor: " << this->ShrinkFactor << "\n";
}
