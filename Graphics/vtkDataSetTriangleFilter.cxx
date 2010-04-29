/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetTriangleFilter.h"

#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedTriangulator.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkDataSetTriangleFilter);

vtkDataSetTriangleFilter::vtkDataSetTriangleFilter()
{
  this->Triangulator = vtkOrderedTriangulator::New();
  this->Triangulator->PreSortedOff();
  this->Triangulator->UseTemplatesOn();
  this->TetrahedraOnly = 0;
}

vtkDataSetTriangleFilter::~vtkDataSetTriangleFilter()
{
  this->Triangulator->Delete();
  this->Triangulator = NULL;
}

int vtkDataSetTriangleFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input->IsA("vtkStructuredPoints") ||
      input->IsA("vtkStructuredGrid") || 
      input->IsA("vtkImageData") ||
      input->IsA("vtkRectilinearGrid"))
    {
    this->StructuredExecute(input, output);
    }
  else
    {
    this->UnstructuredExecute(input, output);
    }

  vtkDebugMacro(<<"Produced " << this->GetOutput()->GetNumberOfCells() << " cells");

  return 1;
}

void vtkDataSetTriangleFilter::StructuredExecute(vtkDataSet *input,
                                                 vtkUnstructuredGrid *output)
{
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
    static_cast<vtkStructuredPoints*>(input)->GetDimensions(dimensions);
    }
  else if (input->IsA("vtkStructuredGrid"))
    {
    static_cast<vtkStructuredGrid*>(input)->GetDimensions(dimensions);
    }
  else if (input->IsA("vtkImageData"))
    {
    static_cast<vtkImageData*>(input)->GetDimensions(dimensions);
    }
  else if (input->IsA("vtkRectilinearGrid"))
    {
    static_cast<vtkRectilinearGrid*>(input)->GetDimensions(dimensions);
    }
  
  dimensions[0] = dimensions[0] - 1;
  dimensions[1] = dimensions[1] - 1;
  dimensions[2] = dimensions[2] - 1;

  vtkIdType numSlices = ( dimensions[2] > 0 ? dimensions[2] : 1 );
  int abort=0;
  for (k = 0; k < numSlices && !abort; k++)
    {
    this->UpdateProgress(static_cast<double>(k) / numSlices);
    abort = this->GetAbortExecute();

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
        if (!this->TetrahedraOnly || type == VTK_TETRA)
          {
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
          }
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

// 3D cells use the ordered triangulator. The ordered triangulator is used
// to create templates on the fly. Once the templates are created then they
// are used to produce the final triangulation.
//
void vtkDataSetTriangleFilter::UnstructuredExecute(vtkDataSet *dataSetInput,
                                                   vtkUnstructuredGrid *output)
{
  vtkPointSet *input = static_cast<vtkPointSet*>(dataSetInput); //has to be
  vtkIdType numCells = input->GetNumberOfCells();
  vtkGenericCell *cell;
  vtkIdType newCellId, j;
  int k;
  vtkCellData *inCD=input->GetCellData();
  vtkCellData *outCD=output->GetCellData();
  vtkPoints *cellPts;
  vtkIdList *cellPtIds;
  vtkIdType ptId, numTets, ncells;
  int numPts, type;
  int numSimplices, dim;
  vtkIdType pts[4];
  double x[3];

  if (numCells == 0)
    {
    return;
    }

  vtkUnstructuredGrid * inUgrid =
    vtkUnstructuredGrid::SafeDownCast(dataSetInput);
  if (inUgrid)
    {
    //avoid doing cell simplification if all cells are already simplices
    vtkUnsignedCharArray* cellTypes = inUgrid->GetCellTypesArray();
    if (cellTypes)
      {
      int allsimplices = 1;
      for (vtkIdType cellId = 0; cellId < cellTypes->GetSize() && allsimplices; cellId++)
        {
        switch (cellTypes->GetValue(cellId))
          {
          case VTK_TETRA:
            break;
          case VTK_VERTEX:
          case VTK_LINE:
          case VTK_TRIANGLE:          
            if (this->TetrahedraOnly)
              {
              allsimplices = 0; //don't shallowcopy need to stip non tets
              }
            break;
          default:
            allsimplices = 0;
            break;
          }
        }
      if (allsimplices)
        {
        output->ShallowCopy(input);
        return;
        }
      }
    }

  cell = vtkGenericCell::New();
  cellPts = vtkPoints::New();
  cellPtIds = vtkIdList::New();

  // Create an array of points
  outCD->CopyAllocate(inCD,input->GetNumberOfCells()*5);
  output->Allocate(input->GetNumberOfCells()*5);
  
  // Points are passed through
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());

  int abort=0;
  vtkIdType updateTime = numCells/20 + 1;  // update roughly every 5%
  for (vtkIdType cellId=0; cellId < numCells && !abort; cellId++)
    {
    if ( !(cellId % updateTime) )
      {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
      }

    input->GetCell(cellId, cell);
    dim = cell->GetCellDimension();

    if (cell->GetCellType() == VTK_POLYHEDRON) //polyhedron
      {
      dim = 4;
      cell->Triangulate(0, cellPtIds, cellPts);
      numPts = cellPtIds->GetNumberOfIds();
    
      numSimplices = numPts / dim;
      type = VTK_TETRA;

      for ( j=0; j < numSimplices; j++ )
        {
        for (k=0; k<dim; k++)
          {
          pts[k] = cellPtIds->GetId(dim*j+k);
          }
        // copy cell data
        newCellId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(inCD, cellId, newCellId);
        }
      }

    else if ( dim == 3 ) //use ordered triangulation
      {
      numPts = cell->GetNumberOfPoints();
      double *p, *pPtr=cell->GetParametricCoords();
      this->Triangulator->InitTriangulation(0.0,1.0, 0.0,1.0, 0.0,1.0, numPts);
      for (p=pPtr, j=0; j<numPts; j++, p+=3)
        {
        ptId = cell->PointIds->GetId(j);
        cell->Points->GetPoint(j, x);
        this->Triangulator->InsertPoint(ptId, x, p, 0);
        }//for all cell points
      if ( cell->IsPrimaryCell() ) //use templates if topology is fixed
        {
        int numEdges=cell->GetNumberOfEdges();
        this->Triangulator->TemplateTriangulate(cell->GetCellType(),
                                                numPts,numEdges);
        }
      else //use ordered triangulator
        {
        this->Triangulator->Triangulate();
        }

      ncells = output->GetNumberOfCells();
      numTets = this->Triangulator->AddTetras(0,output);
        
      for (j=0; j < numTets; j++)
        {
        outCD->CopyData(inCD, cellId, ncells+j);
        }
      }

    else if (!this->TetrahedraOnly) //2D or lower dimension
      {
      dim++;
      cell->Triangulate(0, cellPtIds, cellPts);
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
        }

      for ( j=0; j < numSimplices; j++ )
        {
        for (k=0; k<dim; k++)
          {
          pts[k] = cellPtIds->GetId(dim*j+k);
          }
        // copy cell data
        newCellId = output->InsertNextCell(type, dim, pts);
        outCD->CopyData(inCD, cellId, newCellId);
        }
      } //if 2D or less cell
    } //for all cells
  
  // Update output
  output->Squeeze();
  
  cellPts->Delete();
  cellPtIds->Delete();
  cell->Delete();
}

int vtkDataSetTriangleFilter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

void vtkDataSetTriangleFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TetrahedraOnly: " << (this->TetrahedraOnly ? "On":"Off") << "\n";
}

