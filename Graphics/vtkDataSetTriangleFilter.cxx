/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.cxx
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
#include "vtkDataSetTriangleFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkOrderedTriangulator.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------
vtkDataSetTriangleFilter* vtkDataSetTriangleFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkObjectFactory::CreateInstance("vtkDataSetTriangleFilter");
  if(ret)
    {
    return (vtkDataSetTriangleFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataSetTriangleFilter;
}

vtkDataSetTriangleFilter::~vtkDataSetTriangleFilter()
{
  if ( this->Triangulator )
    {
    this->Triangulator->Delete();
    this->Triangulator = NULL;
    }
}

void vtkDataSetTriangleFilter::Execute()
{
  vtkDataSet *input = this->GetInput();
  if (input->IsA("vtkStructuredPoints") ||
      input->IsA("vtkStructuredGrid") || 
      input->IsA("vtkImageData"))
    {
    this->StructuredExecute();
    }
  else
    {
    this->UnstructuredExecute();
    }
}

void vtkDataSetTriangleFilter::StructuredExecute()
{
  vtkDataSet *input = this->GetInput();
  vtkUnstructuredGrid *output = this->GetOutput();
  int dimensions[3], i, j, k, l, m, inId;
  int newCellId;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  vtkPoints *cellPts = vtkPoints::New();
  vtkPoints *newPoints = vtkPoints::New();
  vtkIdList *cellPtIds = vtkIdList::New();
  int num, numSimplices, numPts, dim, type;
  vtkIdType pts[4];
  
  // Create an array of points. This does an explicit creation
  // of each point.
  num = input->GetNumberOfPoints();
  newPoints->SetNumberOfPoints(num);
  for (i = 0; i < num; ++i)
    {
    newPoints->SetPoint(i,input->GetPoint(i));
    }

  outCD->CopyAllocate(inCD,input->GetNumberOfCells()*5);
  output->Allocate(input->GetNumberOfCells()*5);
  
  if (input->IsA("vtkStructuredPoints"))
    {
    ((vtkStructuredPoints*)input)->GetDimensions(dimensions);
    }
  else if (input->IsA("vtkStructuredGrid"))
    {
    ((vtkStructuredGrid*)input)->GetDimensions(dimensions);
    }
  else if (input->IsA("vtkImageData"))
    {
    ((vtkImageData*)input)->GetDimensions(dimensions);
    }
  
  dimensions[0] = dimensions[0] - 1;
  dimensions[1] = dimensions[1] - 1;
  dimensions[2] = dimensions[2] - 1;
  for (k = 0; k < dimensions[2]; k++)
    {
    for (j = 0; j < dimensions[1]; j++)
      {
      for (i = 0; i < dimensions[0]; i++)
        {
        inId = i+(j+(k*dimensions[1]))*dimensions[0];
        input->GetCell(inId, cell);
        if ((i+j+k)%2 == 0)
          {
          cell->Triangulate(0, cellPtIds, cellPts);
          }
        else
          {
          cell->Triangulate(1, cellPtIds, cellPts);
          }
        
        dim = cell->GetCellDimension() + 1;
        
        numPts = cellPtIds->GetNumberOfIds();
        numSimplices = numPts / dim;
        type = 0;
        switch (dim)
          {
          case 1:
            type = VTK_VERTEX;    break;
          case 2:
            type = VTK_LINE;      break;
          case 3:
            type = VTK_TRIANGLE;  break;
          case 4:
            type = VTK_TETRA;     break;
          }
        for (l = 0; l < numSimplices; l++ )
          {
          for (m = 0; m < dim; m++)
            {
            pts[m] = cellPtIds->GetId(dim*l+m);
            }
          // copy cell data
          newCellId = output->InsertNextCell(type, dim, pts);
          outCD->CopyData(inCD, inId, newCellId);
          }//for all simplices
        }//i dimension
      }//j dimension
    }//k dimension
  
  // Update output
  output->SetPoints(newPoints);
  output->GetPointData()->PassData(input->GetPointData());
  output->Squeeze();
  
  cell->Delete();
  newPoints->Delete();
  cellPts->Delete();
  cellPtIds->Delete();
}

void vtkDataSetTriangleFilter::UnstructuredExecute()
{
  vtkPointSet *input = (vtkPointSet*) this->GetInput(); //has to be
  vtkUnstructuredGrid *output = this->GetOutput();
  int numCells = input->GetNumberOfCells();
  vtkGenericCell *cell = vtkGenericCell::New();
  int i, j, k, newCellId;
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkPoints *cellPts = vtkPoints::New();
  vtkIdList *cellPtIds = vtkIdList::New();
  int type, ptId, ncells, numTets;
  int npts, numSimplices, dim;
  vtkIdType pts[4];
  float *xPtr;

  // Create an array of points
  output->Allocate(input->GetNumberOfCells()*5);
  
  // Points are passed through
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  for (i = 0; i < numCells; i++)
    {
    input->GetCell(i, cell);
    dim = cell->GetCellDimension();

    if ( dim == 3 ) //use ordered triangulation
      {
      if ( ! this->Triangulator )
        {
        this->Triangulator = vtkOrderedTriangulator::New();
        }

      npts = cell->GetNumberOfPoints();
      this->Triangulator->InitTriangulation(cell->GetBounds(),npts);
      for (j=0; j<npts; j++)
        {
        ptId = cell->PointIds->GetId(j);
        xPtr = cell->Points->GetPoint(j);
        this->Triangulator->InsertPoint(ptId, xPtr, 0);
        }//for all cell points
      this->Triangulator->Triangulate();

      ncells = output->GetNumberOfCells();
      numTets = this->Triangulator->AddTetras(0,output);
        
      for (j=0; j < numTets; j++)
        {
        outCD->CopyData(inCD, i, ncells+j);
        }
      }

    else //2D or lower dimension
      {
      dim++;
      cell->Triangulate(0, cellPtIds, cellPts);
      npts = cellPtIds->GetNumberOfIds();
    
      numSimplices = npts / dim;
      type = 0;
      switch (dim)
        {
        case 1:
          type = VTK_VERTEX;    break;
        case 2:
          type = VTK_LINE;      break;
        case 3:
          type = VTK_TRIANGLE;  break;
        }

      for ( j=0; j < numSimplices; j++ )
        {
        for (k=0; k<dim; k++)
          {
          pts[k] = cellPtIds->GetId(dim*j+k);
          }
        // copy cell data
        newCellId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(inCD, i, newCellId);
        }
      } //if 2D or less cell
    } //for all cells
  
  // Update output
  output->Squeeze();
  
  cellPts->Delete();
  cellPtIds->Delete();
  cell->Delete();
}

void vtkDataSetTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToUnstructuredGridFilter::PrintSelf(os,indent);
}

