/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.cxx
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
#include <math.h>
#include "vtkKitwareContourFilter.h"
#include "vtkScalars.h"
#include "vtkCell.h"
#include "vtkMergePoints.h"
#include "vtkContourValues.h"
#include "vtkScalarTree.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkGridSynchronizedTemplates3D.h"

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkKitwareContourFilter::vtkKitwareContourFilter()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = NULL;

  this->UseScalarTree = 0;
  this->ScalarTree = NULL;
}

vtkKitwareContourFilter::~vtkKitwareContourFilter()
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
unsigned long vtkKitwareContourFilter::GetMTime()
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

//
// General contouring filter.  Handles arbitrary input.
//
void vtkKitwareContourFilter::Execute()
{
  int cellId, i;
  vtkIdList *cellPts;
  vtkScalars *inScalars;
  vtkCell *cell;
  float range[2];
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkPoints *newPts;
  vtkDataSet *input=this->GetInput();
  vtkPolyData *output=this->GetOutput();
  int numCells, estimatedSize;
  vtkPointData *inPd=input->GetPointData(), *outPd=output->GetPointData();
  vtkCellData *inCd=input->GetCellData(), *outCd=output->GetCellData();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  vtkScalars *cellScalars;
  
  vtkDebugMacro(<< "Executing contour filter");

  numCells = input->GetNumberOfCells();
  inScalars = input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to contour");
    return;
    }

  // If structured points and structured grid, use more efficient algorithms
  if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS )
    {
    int dim = input->GetCell(0)->GetCellDimension();

    if ( input->GetCell(0)->GetCellDimension() >= 2 ) 
      {
      this->StructuredPointsContour(dim);
      return;
      }
    }

  if ( input->GetDataObjectType() == VTK_STRUCTURED_GRID )
    {
    int dim = input->GetCell(0)->GetCellDimension();
    // only do 3D structured grids (to be extended in the future)
    if ( input->GetCell(0)->GetCellDimension() >= 3 ) 
      {
      this->StructuredGridContour(dim);
      return;
      }
    }

  inScalars->GetRange(range);
//
// Create objects to hold output of contour operation. First estimate allocation size.
//
  estimatedSize = (int) pow ((double) numCells, .75);
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
  cellScalars = vtkScalars::New();
  cellScalars->Allocate(VTK_CELL_SIZE);
  
  
  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL )
    {
    this->CreateDefaultLocator();
    }
  this->Locator->InitPointInsertion (newPts, input->GetBounds(),estimatedSize);

  // interpolate data along edge
  outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);
  outCd->CopyAllocate(inCd,estimatedSize,estimatedSize);

  // If enabled, build a scalar tree to accelerate search
  //
  if ( !this->UseScalarTree )
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      inScalars->GetScalars(cellPts,cellScalars);

      for (i=0; i < numContours; i++)
        {
        cell->Contour(values[i], cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPd, outPd,
		      inCd, cellId, outCd);

        } // for all contour values
      } // for all cells
    } //if using scalar tree
  else
    {
    if ( this->ScalarTree == NULL )
      {
      this->ScalarTree = new vtkScalarTree;
      }
    this->ScalarTree->SetDataSet(input);
    //
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

  this->Locator->Initialize();//releases leftover memory
  output->Squeeze();
}

//
// Special method handles structured points
//
void vtkKitwareContourFilter::StructuredPointsContour(int dim)
{
  vtkPolyData *output;
  vtkPolyData *thisOutput = this->GetOutput();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();

  if ( dim == 2 )
    {
    vtkSynchronizedTemplates2D *syncTemp2D;
    int i;
    
    syncTemp2D = vtkSynchronizedTemplates2D::New();
    syncTemp2D->SetInput((vtkStructuredPoints *)this->GetInput());
    syncTemp2D->SetDebug(this->Debug);
    syncTemp2D->SetNumberOfContours(numContours);
    for (i=0; i < numContours; i++)
      {
      syncTemp2D->SetValue(i,values[i]);
      }
         
    syncTemp2D->Update();
    output = syncTemp2D->GetOutput();
    output->Register(this);
    syncTemp2D->Delete();
    }

  else 
    {
    vtkSynchronizedTemplates3D *syncTemp3D;
    int i;
    
    syncTemp3D = vtkSynchronizedTemplates3D::New();
    
    syncTemp3D->SetInput((vtkStructuredPoints *)this->GetInput());
    syncTemp3D->SetComputeNormals (this->ComputeNormals);
    syncTemp3D->SetComputeGradients (this->ComputeGradients);
    syncTemp3D->SetComputeScalars (this->ComputeScalars);
    syncTemp3D->SetDebug(this->Debug);
    syncTemp3D->SetNumberOfContours(numContours);
    for (i=0; i < numContours; i++)
      {
      syncTemp3D->SetValue(i,values[i]);
      }

    syncTemp3D->Update();
    output = syncTemp3D->GetOutput();
    output->Register(this);
    syncTemp3D->Delete();
    }
  
  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  output->UnRegister(this);
}
//
// Special method handles structured grids
//
void vtkKitwareContourFilter::StructuredGridContour(int dim)
{
  vtkPolyData *output;
  vtkPolyData *thisOutput = this->GetOutput();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();

  if ( dim == 3 )
    {
    vtkGridSynchronizedTemplates3D *gridTemp3D;
    int i;
    
    gridTemp3D = vtkGridSynchronizedTemplates3D::New();
    gridTemp3D->SetInput((vtkStructuredGrid*)(this->GetInput()));
    gridTemp3D->SetComputeNormals (this->ComputeNormals);
    gridTemp3D->SetComputeGradients (this->ComputeGradients);
    gridTemp3D->SetComputeScalars (this->ComputeScalars);
    gridTemp3D->SetDebug(this->Debug);
    gridTemp3D->SetNumberOfContours(numContours);
    for (i=0; i < numContours; i++)
      {
      gridTemp3D->SetValue(i,values[i]);
      }

    gridTemp3D->Update();
    output = gridTemp3D->GetOutput();
    output->Register(this);
    gridTemp3D->Delete();
    }
  
  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  output->UnRegister(this);
}

// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkKitwareContourFilter::SetLocator(vtkPointLocator *locator)
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

void vtkKitwareContourFilter::CreateDefaultLocator()
{
  if ( this->Locator == NULL )
    {
    this->Locator = vtkMergePoints::New();
    }
}

void vtkKitwareContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyDataFilter::PrintSelf(os,indent);

  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Use Scalar Tree: " << (this->UseScalarTree ? "On\n" : "Off\n");

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

