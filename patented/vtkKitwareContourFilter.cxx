/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkObjectFactory.h"

vtkKitwareContourFilter* vtkKitwareContourFilter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkKitwareContourFilter");
  if(ret)
    {
    return (vtkKitwareContourFilter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkKitwareContourFilter;
}

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkKitwareContourFilter::vtkKitwareContourFilter()
{
}

vtkKitwareContourFilter::~vtkKitwareContourFilter()
{
}


//
// General contouring filter.  Handles arbitrary input.
//
void vtkKitwareContourFilter::Execute()
{
  vtkScalars *inScalars;
  vtkDataSet *input=this->GetInput();
  int numCells;
  
  vtkDebugMacro(<< "Executing contour filter");

  numCells = input->GetNumberOfCells();
  inScalars = input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    vtkErrorMacro(<<"No data to contour");
    return;
    }

  // If structured points and structured grid, use more efficient algorithms
  if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS || 
       input->GetDataObjectType() == VTK_IMAGE_DATA )
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

  // otherwise just use the normal one
  this->vtkContourFilter::Execute();
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
    syncTemp2D->SetInput((vtkImageData *)this->GetInput());
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
    
    syncTemp3D->SetInput((vtkImageData *)this->GetInput());
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

void vtkKitwareContourFilter::DataSetContour()
{
  vtkPolyData *output = this->GetOutput();
  vtkPolyData *thisOutput = this->GetOutput();
  int numContours=this->ContourValues->GetNumberOfContours();
  float *values=this->ContourValues->GetValues();

  vtkContourFilter *contour = vtkContourFilter::New();
  contour->SetInput((vtkImageData *)this->GetInput());
  contour->SetOutput(output);
  contour->SetComputeNormals (this->ComputeNormals);
  contour->SetComputeGradients (this->ComputeGradients);
  contour->SetComputeScalars (this->ComputeScalars);
  contour->SetDebug(this->Debug);
  contour->SetNumberOfContours(numContours);
  for (int i=0; i < numContours; i++)
    {
    contour->SetValue(i,values[i]);
    }

  contour->Update();
  this->SetOutput(output);
  contour->Delete();
}


