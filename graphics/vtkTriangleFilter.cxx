/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkTriangleFilter.h"
#include "vtkPolygon.h"
#include "vtkTriangleStrip.h"

void vtkTriangleFilter::Execute()
{
  vtkPolyData *input=(vtkPolyData *)this->Input;
  int numCells=input->GetNumberOfCells();
  int dim, i, j, pts[3], cellNum, numPts, numSimplices, newId, type;
  vtkIdList *ptIds=vtkIdList::New();
  vtkPoints *spts=vtkPoints::New();
  vtkPolyData *output=this->GetOutput();
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkCell *cell;
  int updateInterval;
  int numPoints=input->GetNumberOfPoints();

  
  output->Allocate(numPoints, numPoints);

  updateInterval = numCells/100.0;
  if (updateInterval < 1)
    {
    updateInterval = 1;
    }
  for (cellNum=0; cellNum < numCells; cellNum++)
    {
    if ( ! (cellNum % updateInterval) ) //manage progress reports / early abort
      {
      this->UpdateProgress ((float)cellNum / numCells);
      if ( this->GetAbortExecute() ) 
        {
        break;
        }
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

