/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleFilter.cxx
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
#include "vtkTriangleFilter.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkTriangleFilter* vtkTriangleFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkTriangleFilter");
  if(ret)
    {
    return (vtkTriangleFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkTriangleFilter;
}

void vtkTriangleFilter::Execute()
{
  vtkPolyData *input = this->GetInput();
  int numCells=input->GetNumberOfCells();
  int dim, i, j, cellNum, numPts, numSimplices, newId, type;
  vtkIdType pts[3];
  vtkIdList *ptIds=vtkIdList::New();
  vtkPoints *spts=vtkPoints::New();
  vtkPolyData *output=this->GetOutput();
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkCell *cell;
  int updateInterval;
  int numPoints=input->GetNumberOfPoints();

  output->Allocate(numPoints, numPoints);
  outCD->CopyAllocate(inCD,numPoints);

  int abort=0;
  updateInterval = numCells/100 + 1;
  for (cellNum=0; cellNum < numCells && !abort; cellNum++)
    {
    if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellNum / numCells);
      abort = this->GetAbortExecute();
      }

    cell = input->GetCell(cellNum);
    dim = cell->GetCellDimension() + 1;
    
    cell->Triangulate(cellNum, ptIds, spts);
    numPts = ptIds->GetNumberOfIds();
    numSimplices = numPts / dim;
    
    if ( dim == 3 || (this->PassVerts && dim == 1) ||
    (this->PassLines && dim == 2) )
      {
      type = (dim == 3 ? VTK_TRIANGLE : (dim == 2 ? VTK_LINE : VTK_VERTEX ));
      for ( i=0; i < numSimplices; i++ )
        {
        for (j=0; j<dim; j++)
          {
          pts[j] = ptIds->GetId(dim*i+j);
          }
        // copy cell data
        newId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(inCD, cellNum, newId);
        
        }//for each simplex
      }//if polygon or strip or (line or verts and passed on)
    }//for all cells

  ptIds->Delete();
  spts->Delete();
  
  // Update output
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();

  vtkDebugMacro(<<"Converted " << input->GetNumberOfCells()
                << "input cells to "
                << output->GetNumberOfCells()
                <<" output cells");
}

void vtkTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkPolyDataToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Pass Verts: " << (this->PassVerts ? "On\n" : "Off\n");
  os << indent << "Pass Lines: " << (this->PassLines ? "On\n" : "Off\n");

}

