/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveGhostCells.cxx
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
#include "vtkRemoveGhostCells.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"

//----------------------------------------------------------------------------
vtkRemoveGhostCells* vtkRemoveGhostCells::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkRemoveGhostCells");
  if(ret)
    {
    return (vtkRemoveGhostCells*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkRemoveGhostCells;
}

// Construct with ghost level = 1.
vtkRemoveGhostCells::vtkRemoveGhostCells()
{
  this->GhostLevel = 1;
}

void vtkRemoveGhostCells::Execute()
{
  vtkCellArray *newCells;
  vtkCellData *cellData;
  vtkIdType numCells, cellId, newCellId;
  vtkPolyData *input = this->GetInput();
  vtkPolyData *output = this->GetOutput();
  vtkCell *cell;

  cellData = input->GetCellData();

  vtkFieldData* fd = cellData->GetFieldData();
  if (!fd)
    {
    vtkErrorMacro(<<"No field data found.");
    output->SetPoints(input->GetPoints());
    output->SetPolys(input->GetPolys());
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }

  vtkDataArray* temp = fd->GetArray("vtkGhostLevels");
  if ( (!temp) || (temp->GetDataType() != VTK_UNSIGNED_CHAR)
    || (temp->GetNumberOfComponents() != 1))
    {
    vtkErrorMacro(<<"No proper match for vtkGhostLeves found in the field data.");
    output->GetPointData()->PassData(input->GetPointData());
    output->SetPoints(input->GetPoints());
    output->SetPolys(input->GetPolys());
    output->GetCellData()->PassData(input->GetCellData());
    return;
    }
     
  numCells = input->GetNumberOfCells();
  newCells = vtkCellArray::New();
  newCells->Allocate(numCells);
  
  output->SetPoints(input->GetPoints());

  // Check the ghost level of each cell to determine whether to remove
  // the cell.
  unsigned char* cellGhostLevels = ((vtkUnsignedCharArray*)temp)->GetPointer(0);
  for (cellId=0; cellId < numCells; cellId++)
    {
    if (cellGhostLevels[cellId] < this->GhostLevel)
      {
      cell = input->GetCell(cellId);
      newCellId = newCells->InsertNextCell(cell);
      output->GetCellData()->CopyData(input->GetCellData(), cellId, newCellId);
      } // keep this cell
    } // for all cells

//
// Update ourselves and release memory
//
  output->SetPolys(newCells);
  newCells->Delete();

  output->GetPointData()->PassData(input->GetPointData());

  output->Squeeze();
}

void vtkRemoveGhostCells::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Ghost Level: " << this->GhostLevel << "\n";;
}
