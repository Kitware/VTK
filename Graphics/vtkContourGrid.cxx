/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourGrid.cxx
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
#include <math.h>
#include "vtkContourGrid.h"
#include "vtkCell.h"
#include "vtkMergePoints.h"
#include "vtkContourValues.h"
#include "vtkScalarTree.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"

//---------------------------------------------------------------------------
vtkContourGrid* vtkContourGrid::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkContourGrid");
  if(ret)
    {
    return (vtkContourGrid*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkContourGrid;
}

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourGrid::vtkContourGrid()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = NULL;

  this->UseScalarTree = 0;
  this->ScalarTree = NULL;
}

vtkContourGrid::~vtkContourGrid()
{
  this->ContourValues->Delete();
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
}

// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkContourGrid::GetMTime()
{
  unsigned long mTime=this->vtkUnstructuredGridToPolyDataFilter::GetMTime();
  unsigned long time;

  if (this->ContourValues)
    {
    time = this->ContourValues->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  if (this->Locator)
    {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

template <class T>
static void vtkContourGridExecute(vtkContourGrid *self,
                                  vtkDataSet *input,
                                  vtkDataArray *inScalars, T *scalarArrayPtr,
                                  int numContours, float *values, 
                                  vtkPointLocator *locator, int computeScalars,
                                  int useScalarTree,vtkScalarTree *&scalarTree)
{
  vtkIdType cellId, i;
  int abortExecute=0;
  vtkPolyData *output=self->GetOutput();
  vtkIdList *cellPts;
  vtkCell *cell;
  float range[2];
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPts;
  vtkIdType numCells, estimatedSize;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();
  vtkDataArray *cellScalars;
  vtkUnstructuredGrid *grid = (vtkUnstructuredGrid *)input;
  //In this case, we know that the input is an unstructured grid.
  vtkIdType numPoints, cellArrayIt = 0;
  int needCell = 0;
  vtkIdType *cellArrayPtr;
  T tempScalar;

  numCells = input->GetNumberOfCells();

  //
  // Create objects to hold output of contour operation. First estimate
  // allocation size.
  //
  estimatedSize = (vtkIdType) pow ((double) numCells, .75);
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
    {
    estimatedSize = 1024;
    }

  newPts = vtkPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize);
  cellScalars = inScalars->MakeObject();
  cellScalars->Allocate(VTK_CELL_SIZE*inScalars->GetNumberOfComponents());
  
   // locator used to merge potentially duplicate points
  locator->InitPointInsertion (newPts, input->GetBounds(),estimatedSize);

  // interpolate data along edge
  // if we did not ask for scalars to be computed, don't copy them
  if (!computeScalars)
    {
    outPd->CopyScalarsOff();
    }
  outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);
  outCd->CopyAllocate(inCd,estimatedSize,estimatedSize);

  // If enabled, build a scalar tree to accelerate search
  //
  if ( !useScalarTree )
    {
    cellArrayPtr = grid->GetCells()->GetPointer();
    for (cellId=0; cellId < numCells && !abortExecute; cellId++)
      {
      numPoints = cellArrayPtr[cellArrayIt];
      cellArrayIt++;
      
      //find min and max values in scalar data
      range[0] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
      range[1] = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
      cellArrayIt++;
      
      for (i = 1; i < numPoints; i++)
        {
        tempScalar = scalarArrayPtr[cellArrayPtr[cellArrayIt]];
        cellArrayIt++;
        if (tempScalar <= range[0])
          {
          range[0] = tempScalar;
          } //if tempScalar <= min range value
        if (tempScalar >= range[1])
          {
          range[1] = tempScalar;
          } //if tempScalar >= max range value
        } // for all points in this cell
      
      if ( ! (cellId % 5000) ) 
        {
        self->UpdateProgress ((float)cellId/numCells);
        if (self->GetAbortExecute())
          {
          abortExecute = 1;
          break;
          }
        }
      
      for (i = 0; i < numContours; i++)
        {
        if ((values[i] >= range[0]) && (values[i] <= range[1]))
          {
          needCell = 1;
          } // if contour value in range for this cell
        } // end for numContours
      
      if (needCell)
        {
        cell = input->GetCell(cellId);
        cellPts = cell->GetPointIds();
        inScalars->GetTuples(cellPts,cellScalars);
        
        for (i=0; i < numContours; i++)
          {
          if ((values[i] >= range[0]) && (values[i] <= range[1]))
            {
            cell->Contour(values[i], cellScalars, locator,
                          newVerts, newLines, newPolys, inPd, outPd,
                          inCd, cellId, outCd);
            } // if contour value in range of values for this cell
          } // for all contour values
        } // if contour goes through this cell
      needCell = 0;
      } // for all cells
    } //if using scalar tree
  else
    {
    if ( scalarTree == NULL )
      {
      scalarTree = vtkScalarTree::New();
      }
    scalarTree->SetDataSet(input);
    //
    // Loop over all contour values.  Then for each contour value, 
    // loop over all cells.
    //
    for (i=0; i < numContours; i++)
      {
      for ( scalarTree->InitTraversal(values[i]); 
          (cell=scalarTree->GetNextCell(cellId,cellPts,cellScalars)) != NULL; )
        {
        cell->Contour(values[i], cellScalars, locator,
                      newVerts, newLines, newPolys, inPd, outPd,
                      inCd, cellId, outCd);
           //don't want to call Contour any more than necessary
        } //for all cells
      } //for all contour values
    } //using scalar tree

  //
  // Update ourselves.  Because we don't know up front how many verts, lines,
  // polys we've created, take care to reclaim memory. 
  //
  output->SetPoints(newPts);
  newPts->Delete();
  cellScalars->Delete();
  
  if (newVerts->GetNumberOfCells())
    {
    output->SetVerts(newVerts);
    }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
    {
    output->SetLines(newLines);
    }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
    {
    output->SetPolys(newPolys);
    }
  newPolys->Delete();

  locator->Initialize();//releases leftover memory
  output->Squeeze();
}

//
// Contouring filter for unstructured grids.
//
void vtkContourGrid::Execute()
{
  vtkDataArray *inScalars;
  vtkDataSet *input=this->GetInput();
  void *scalarArrayPtr;
  vtkIdType numCells;
  int numContours = this->ContourValues->GetNumberOfContours();
  float *values = this->ContourValues->GetValues();
  int computeScalars = this->ComputeScalars;
  int useScalarTree = this->UseScalarTree;
  vtkScalarTree *&scalarTree = this->ScalarTree;

  vtkDebugMacro(<< "Executing contour filter");

  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }

  numCells = input->GetNumberOfCells();
  inScalars = input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to contour");
    return;
    }

  scalarArrayPtr = inScalars->GetVoidPointer(0);
        
  switch (inScalars->GetDataType())
    {
    vtkTemplateMacro10(vtkContourGridExecute, this, input, inScalars,
                       (VTK_TT *)(scalarArrayPtr), numContours, values,
                       this->Locator, computeScalars, useScalarTree, 
                       scalarTree);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }       
}

// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkContourGrid::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator == locator ) 
    {
    return;
    }
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( locator )
    {
    locator->Register(this);
    }
  this->Locator = locator;
  this->Modified();
}

void vtkContourGrid::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkContourGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkUnstructuredGridToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Compute Gradients: " 
     << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " 
     << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " 
     << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Use Scalar Tree: " 
     << (this->UseScalarTree ? "On\n" : "Off\n");

  this->ContourValues->PrintSelf(os,indent);

  if ( this->Locator )
    {
    os << indent << "Locator: " << this->Locator << "\n";
    }
  else
    {
    os << indent << "Locator: (none)\n";
    }
}
