/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourFilter.cxx
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
#include "vtkContourFilter.h"
#include "vtkCell.h"
#include "vtkMergePoints.h"
#include "vtkContourValues.h"
#include "vtkScalarTree.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkContourGrid.h"

//------------------------------------------------------------------------------
vtkContourFilter* vtkContourFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkContourFilter");
  if(ret)
    {
    return (vtkContourFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkContourFilter;
}




// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourFilter::vtkContourFilter()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = NULL;

  this->UseScalarTree = 0;
  this->ScalarTree = NULL;
}

vtkContourFilter::~vtkContourFilter()
{
  this->ContourValues->Delete();
  if ( this->Locator )
    {
    this->Locator->UnRegister(this);
    this->Locator = NULL;
    }
  if ( this->ScalarTree )
    {
    this->ScalarTree->Delete();
    }
}

// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkContourFilter::GetMTime()
{
  unsigned long mTime=this->vtkDataSetToPolyDataFilter::GetMTime();
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

// General contouring filter.  Handles arbitrary input.
//
void vtkContourFilter::Execute()
{
  vtkIdType cellId;
  int i, abortExecute=0;
  vtkIdList *cellPts;
  vtkDataArray *inScalars;
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPts;
  vtkDataSet *input=this->GetInput();
  if (input == NULL) {return;}
  vtkPolyData *output=this->GetOutput();
  vtkIdType numCells, estimatedSize;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  vtkDataArray *cellScalars;

  vtkDebugMacro(<< "Executing contour filter");

  if (input->GetDataObjectType() == VTK_UNSTRUCTURED_GRID)
    {
    vtkDebugMacro(<< "Processing unstructured grid");
    vtkContourGrid *cgrid;

    cgrid = vtkContourGrid::New();
    cgrid->SetInput((vtkUnstructuredGrid *)input);
    for (i = 0; i < numContours; i++)
      {
      cgrid->SetValue(i, values[i]);
      }
    cgrid->GetOutput()->SetUpdateExtent(output->GetUpdatePiece(),
					output->GetUpdateNumberOfPieces(),
					output->GetUpdateGhostLevel());
    cgrid->Update();
    output->ShallowCopy(cgrid->GetOutput());
    cgrid->Delete();
    } //if type VTK_UNSTRUCTURED_GRID
  else
    {
    numCells = input->GetNumberOfCells();
    inScalars = input->GetPointData()->GetScalars();
    if ( ! inScalars || numCells < 1 )
      {
      vtkErrorMacro(<<"No data to contour");
      return;
      }

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
    cellScalars->Allocate(cellScalars->GetNumberOfComponents()*VTK_CELL_SIZE);
    
    // locator used to merge potentially duplicate points
    if ( this->Locator == NULL )
      {
      this->CreateDefaultLocator();
      }
    this->Locator->InitPointInsertion (newPts, 
                                       input->GetBounds(),estimatedSize);

    // interpolate data along edge
    // if we did not ask for scalars to be computed, don't copy them
    if (!this->ComputeScalars)
      {
      outPd->CopyScalarsOff();
      }
    outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);
    outCd->CopyAllocate(inCd,estimatedSize,estimatedSize);
    
    // If enabled, build a scalar tree to accelerate search
    //
    if ( !this->UseScalarTree )
      {
      vtkGenericCell *cell = vtkGenericCell::New();
      for (cellId=0; cellId < numCells && !abortExecute; cellId++)
        {
        input->GetCell(cellId,cell);
        cellPts = cell->GetPointIds();
        inScalars->GetTuples(cellPts,cellScalars);
        
        if ( ! (cellId % 5000) ) 
          {
          vtkDebugMacro(<<"Contouring #" << cellId);
          this->UpdateProgress ((float)cellId/numCells);
          abortExecute = this->GetAbortExecute();
          }
        
        for (i=0; i < numContours; i++)
          {
          cell->Contour(values[i], cellScalars, this->Locator, 
                        newVerts, newLines, newPolys, inPd, outPd,
                        inCd, cellId, outCd);
          
          } // for all contour values
        } // for all cells
      cell->Delete();
      } //if using scalar tree
    else
      {
      vtkCell *cell;
      if ( this->ScalarTree == NULL )
        {
        this->ScalarTree = vtkScalarTree::New();
        }
      this->ScalarTree->SetDataSet(input);
      // Loop over all contour values.  Then for each contour value, 
      // loop over all cells.
      //
      for (i=0; i < numContours; i++)
        {
        for ( this->ScalarTree->InitTraversal(values[i]); 
              (cell=this->ScalarTree->GetNextCell(cellId,cellPts,cellScalars)) != NULL; )
          {
          cell->Contour(values[i], cellScalars, this->Locator, 
                        newVerts, newLines, newPolys, inPd, outPd,
                        inCd, cellId, outCd);
          
          } //for all cells
        } //for all contour values
      } //using scalar tree
    
    vtkDebugMacro(<<"Created: " 
                  << newPts->GetNumberOfPoints() << " points, " 
                  << newVerts->GetNumberOfCells() << " verts, " 
                  << newLines->GetNumberOfCells() << " lines, " 
                  << newPolys->GetNumberOfCells() << " triangles");

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
    
    this->Locator->Initialize();//releases leftover memory
    output->Squeeze();
    } //else if not vtkUnstructuredGrid
}

// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkContourFilter::SetLocator(vtkPointLocator *locator)
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

void vtkContourFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

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

