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
  vtkIdType numCells=input->GetNumberOfCells();
  vtkIdType cellNum=0, numPts, newId, npts, *pts;
  int i, j;
  vtkPolyData *output=this->GetOutput();
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkIdType updateInterval;
  vtkCellArray *cells, *newCells;
  vtkPoints *inPts=input->GetPoints();

  int abort=0;
  updateInterval = numCells/100 + 1;
  outCD->CopyAllocate(inCD,numCells);

  // Do each of the verts, lines, polys, and strips separately
  // verts
  if ( !abort && input->GetVerts()->GetNumberOfCells() > 0 )
    {
    cells = input->GetVerts();
    if ( this->PassVerts )
      {
      newId = output->GetNumberOfCells();
      newCells = vtkCellArray::New();
      newCells->EstimateSize(cells->GetNumberOfCells(),1);
      for (cells->InitTraversal(); cells->GetNextCell(npts,pts) && !abort; cellNum++)
        {
        if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
          {
          this->UpdateProgress ((float)cellNum / numCells);
          abort = this->GetAbortExecute();
          }
        if ( npts > 1 )
          {
          for (i=0; i<npts; i++)
            {
            newCells->InsertNextCell(1,pts+i);
            outCD->CopyData(inCD, cellNum, newId++);
            }
          }
        else
          {
          newCells->InsertNextCell(1,pts);
          outCD->CopyData(inCD, cellNum, newId++);
          }
        }
      output->SetVerts(newCells);
      newCells->Delete();
      }
    else
      {
      cellNum += cells->GetNumberOfCells(); //skip over verts
      }
    }
  
  // lines
  if ( !abort && input->GetLines()->GetNumberOfCells() > 0 )
    {
    cells = input->GetLines();
    if ( this->PassVerts )
      {
      newId = output->GetNumberOfCells();
      newCells = vtkCellArray::New();
      newCells->EstimateSize(cells->GetNumberOfCells(),2);
      for (cells->InitTraversal(); cells->GetNextCell(npts,pts) && !abort; cellNum++)
        {
        if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
          {
          this->UpdateProgress ((float)cellNum / numCells);
          abort = this->GetAbortExecute();
          }
        if ( npts > 2 )
          {
          for (i=0; i<(npts-1); i++)
            {
            newCells->InsertNextCell(2,pts+i);
            outCD->CopyData(inCD, cellNum, newId++);
            }
          }
        else
          {
          newCells->InsertNextCell(2,pts);
          outCD->CopyData(inCD, cellNum, newId++);
          }
        }//for all lines
      output->SetLines(newCells);
      newCells->Delete();
      }
    else
      {
      cellNum += cells->GetNumberOfCells(); //skip over lines
      }
    }

  vtkCellArray *newPolys=NULL;
  if ( !abort && input->GetPolys()->GetNumberOfCells() > 0 )
    {
    cells = input->GetPolys();
    newId = output->GetNumberOfCells();
    newPolys = vtkCellArray::New();
    newPolys->EstimateSize(cells->GetNumberOfCells(),3);
    output->SetPolys(newPolys);
    vtkIdList *ptIds = vtkIdList::New();
    ptIds->Allocate(VTK_CELL_SIZE);
    int numSimplices;
    vtkPolygon *poly=vtkPolygon::New();
    vtkIdType triPts[3];
    
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts) && !abort; cellNum++)
      {
      if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
        {
        this->UpdateProgress ((float)cellNum / numCells);
        abort = this->GetAbortExecute();
        }
      if ( npts == 3 )
        {
        newPolys->InsertNextCell(3,pts);
        outCD->CopyData(inCD, cellNum, newId++);
        }
      else //triangulate polygon
        {
        //initialize polygon
        poly->PointIds->SetNumberOfIds(npts);
        poly->Points->SetNumberOfPoints(npts);
        for (i=0; i<npts; i++)
          {
          poly->PointIds->SetId(i,pts[i]);
          poly->Points->SetPoint(i,inPts->GetPoint(pts[i]));
          }
        poly->Triangulate(ptIds);
        numPts = ptIds->GetNumberOfIds();
        numSimplices = numPts / 3;
        for ( i=0; i < numSimplices; i++ )
          {
          for (j=0; j<3; j++)
            {
            triPts[j] = poly->PointIds->GetId(ptIds->GetId(3*i+j));
            }
          newPolys->InsertNextCell(3, triPts);
          outCD->CopyData(inCD, cellNum, newId++);
          }//for each simplex
        }//triangulate polygon
      }
    ptIds->Delete();
    poly->Delete();
    }
  
  //strips
  if ( !abort && input->GetStrips()->GetNumberOfCells() > 0 )
    {
    cells = input->GetStrips();
    newId = output->GetNumberOfCells();
    if ( newPolys == NULL )
      {
      newPolys = vtkCellArray::New();
      newPolys->EstimateSize(cells->GetNumberOfCells(),3);
      output->SetPolys(newPolys);
      }
    for (cells->InitTraversal(); cells->GetNextCell(npts,pts) && !abort; cellNum++)
      {
      if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
        {
        this->UpdateProgress ((float)cellNum / numCells);
        abort = this->GetAbortExecute();
        }
      vtkTriangleStrip::DecomposeStrip(npts,pts,newPolys);
      for (i=0; i<(npts-2); i++)
        {
        outCD->CopyData(inCD, cellNum, newId++);
        }
      }//for all strips
    }

  if ( newPolys != NULL )
    {
    newPolys->Delete();
    }

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
