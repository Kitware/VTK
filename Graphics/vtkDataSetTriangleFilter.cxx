/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetTriangleFilter.h"
#include "vtkStructuredPoints.h"
#include "vtkImageData.h"
#include "vtkStructuredGrid.h"
#include "vtkOrderedTriangulator.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDataSetTriangleFilter, "1.13");
vtkStandardNewMacro(vtkDataSetTriangleFilter);

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
  if (!input)
    {
    return;
    }
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
  int dimensions[3], i, j, k, l, m;
  vtkIdType newCellId, inId;
  vtkGenericCell *cell = vtkGenericCell::New();
  vtkCellData *inCD = input->GetCellData();
  vtkCellData *outCD = output->GetCellData();
  vtkPoints *cellPts = vtkPoints::New();
  vtkPoints *newPoints = vtkPoints::New();
  vtkIdList *cellPtIds = vtkIdList::New();
  int numSimplices, numPts, dim, type;
  vtkIdType pts[4], num;
  
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
  vtkIdType numCells = input->GetNumberOfCells();
  vtkGenericCell *cell;
  vtkIdType newCellId, i, j;
  int k;
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkPoints *cellPts;
  vtkIdList *cellPtIds;
  vtkIdType ptId, numTets, ncells;
  int type;
  int npts, numSimplices, dim;
  vtkIdType pts[4];
  float *xPtr;

  if (numCells == 0)
    {
    return;
    }

  cell = vtkGenericCell::New();
  cellPts = vtkPoints::New();
  cellPtIds = vtkIdList::New();


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
  this->Superclass::PrintSelf(os,indent);
}

