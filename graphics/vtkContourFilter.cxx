/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ContourF.cc
  Language:  C++
  Date:      02/07/94
  Version:   1.4


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkContourFilter.h"
#include "vtkFloatScalars.h"
#include "vtkCell.h"
#include "vtkMergePoints.h"
#include "vtkContourValues.h"
#include "vtkScalarTree.h"

#ifdef VTK_USE_PATENTED
#include "vtkMarchingSquares.h"
#include "vtkMarchingCubes.h"
#endif

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkContourFilter::vtkContourFilter()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;

  this->Locator = NULL;
  this->SelfCreatedLocator = 0;

  this->UseScalarTree = 0;
  this->ScalarTree = NULL;
}

vtkContourFilter::~vtkContourFilter()
{
  this->ContourValues->Delete();
  if ( this->SelfCreatedLocator && this->Locator ) this->Locator->Delete();
  if ( this->ScalarTree ) this->ScalarTree->Delete();
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
unsigned long vtkContourFilter::GetMTime()
{
  unsigned long mTime=this->vtkDataSetToPolyFilter::GetMTime();
  unsigned long contourValuesMTime=this->ContourValues->GetMTime();

  mTime = ( contourValuesMTime > mTime ? contourValuesMTime : mTime );

  return mTime;
}

//
// General contouring filter.  Handles arbitrary input.
//
void vtkContourFilter::Execute()
{
  int cellId, i;
  vtkIdList *cellPts;
  vtkScalars *inScalars;
  vtkFloatScalars cellScalars(VTK_CELL_SIZE);
  vtkCell *cell;
  float range[2];
  vtkCellArray *newVerts, *newLines, *newPolys;
  vtkFloatPoints *newPts;
  cellScalars.ReferenceCountingOff();
  vtkPolyData *output = this->GetOutput();
  int numCells, estimatedSize;
  vtkPointData *inPd, *outPd;
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();
  
  vtkDebugMacro(<< "Executing contour filter");

  numCells = this->Input->GetNumberOfCells();
  inScalars = this->Input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to contour");
    return;
    }

  // If structured points, use more efficient algorithms
  if ( ! strcmp(this->Input->GetDataType(),"vtkStructuredPoints") )
    {
    int dim = this->Input->GetCell(0)->GetCellDimension();

#ifdef VTK_USE_PATENTED
    if ( this->Input->GetCell(0)->GetCellDimension() >= 2 ) 
      {
      this->StructuredPointsContour(dim);
      return;
      }
#endif
    }

  inScalars->GetRange(range);
//
// Create objects to hold output of contour operation. First estimate allocation size.
//
  estimatedSize = (int) pow ((double) numCells, .75);
  estimatedSize *= numContours;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024) estimatedSize = 1024;

  newPts = vtkFloatPoints::New();
  newPts->Allocate(estimatedSize,estimatedSize);
  newVerts = vtkCellArray::New();
  newVerts->Allocate(estimatedSize,estimatedSize);
  newLines = vtkCellArray::New();
  newLines->Allocate(estimatedSize,estimatedSize);
  newPolys = vtkCellArray::New();
  newPolys->Allocate(estimatedSize,estimatedSize);

  // locator used to merge potentially duplicate points
  if ( this->Locator == NULL ) this->CreateDefaultLocator();
  this->Locator->InitPointInsertion (newPts, this->Input->GetBounds());

  // interpolate data along edge
  inPd = this->Input->GetPointData();
  outPd = output->GetPointData();
  outPd->InterpolateAllocate(inPd,estimatedSize,estimatedSize);

  // If enabled, build a scalar tree to accelerate search
  //
  if ( !this->UseScalarTree )
    {
    for (cellId=0; cellId < numCells; cellId++)
      {
      cell = this->Input->GetCell(cellId);
      cellPts = cell->GetPointIds();
      inScalars->GetScalars(*cellPts,cellScalars);

      for (i=0; i < numContours; i++)
        {
        cell->Contour(values[i], &cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPd, outPd);

        } // for all contour values
      } // for all cells
    } //if using scalar tree
  else
    {
    if ( this->ScalarTree == NULL ) this->ScalarTree = new vtkScalarTree;
    this->ScalarTree->SetDataSet(this->Input);
    //
    // Loop over all contour values.  Then for each contour value, 
    // loop over all cells.
    //
    for (i=0; i < numContours; i++)
      {
      for ( this->ScalarTree->InitTraversal(values[i]); 
      (cell=this->ScalarTree->GetNextCell(cellId,cellPts,cellScalars)) != NULL; )
        {
        cell->Contour(values[i], &cellScalars, this->Locator, 
                      newVerts, newLines, newPolys, inPd, outPd);

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

  if (newVerts->GetNumberOfCells()) output->SetVerts(newVerts);
  newVerts->Delete();

  if (newLines->GetNumberOfCells()) output->SetLines(newLines);
  newLines->Delete();

  if (newPolys->GetNumberOfCells()) output->SetPolys(newPolys);
  newPolys->Delete();

  this->Locator->Initialize();//releases leftover memory
  output->Squeeze();
}

//
// Special method handles structured points
//
void vtkContourFilter::StructuredPointsContour(int dim)
{
  vtkPolyData *output;
  vtkPolyData *thisOutput = (vtkPolyData *)this->Output;
  int i, numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();

#ifdef VTK_USE_PATENTED  
  if ( dim == 2 ) //marching squares
    {
    static vtkMarchingSquares msquares;

    msquares.SetInput((vtkStructuredPoints *)this->Input);
    msquares.SetDebug(this->Debug);
    msquares.SetNumberOfContours(numContours);
    for (i=0; i < numContours; i++)
      msquares.SetValue(i,values[i]);
         
    msquares.Update();
    output = msquares.GetOutput();
    }

  else //marching cubes
    {
    static vtkMarchingCubes mcubes;

    mcubes.SetInput((vtkStructuredPoints *)this->Input);
    mcubes.SetComputeNormals (this->ComputeNormals);
    mcubes.SetComputeGradients (this->ComputeGradients);
    mcubes.SetComputeScalars (this->ComputeScalars);
    mcubes.SetDebug(this->Debug);
    mcubes.SetNumberOfContours(numContours);
    for (i=0; i < numContours; i++)
      mcubes.SetValue(i,values[i]);

    mcubes.Update();
    output = mcubes.GetOutput();
    }
#endif
  
  thisOutput->CopyStructure(output);
  *thisOutput->GetPointData() = *output->GetPointData();
  output->Initialize();
}

// Description:
// Specify a spatial locator for merging points. By default, 
// an instance of vtkMergePoints is used.
void vtkContourFilter::SetLocator(vtkPointLocator *locator)
{
  if ( this->Locator != locator ) 
    {
    if ( this->SelfCreatedLocator ) this->Locator->Delete();
    this->SelfCreatedLocator = 0;
    this->Locator = locator;
    this->Modified();
    }
}

void vtkContourFilter::CreateDefaultLocator()
{
  if ( this->SelfCreatedLocator ) this->Locator->Delete();
  this->Locator = vtkMergePoints::New();
  this->SelfCreatedLocator = 1;
}

void vtkContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataSetToPolyFilter::PrintSelf(os,indent);

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

