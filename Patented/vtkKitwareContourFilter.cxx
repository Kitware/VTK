/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKitwareContourFilter.cxx
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
#include <math.h>
#include "vtkKitwareContourFilter.h"
#include "vtkCell.h"
#include "vtkMergePoints.h"
#include "vtkContourValues.h"
#include "vtkScalarTree.h"
#include "vtkSynchronizedTemplates2D.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkGridSynchronizedTemplates3D.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKitwareContourFilter, "1.23");
vtkStandardNewMacro(vtkKitwareContourFilter);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
vtkKitwareContourFilter::vtkKitwareContourFilter()
{
}

vtkKitwareContourFilter::~vtkKitwareContourFilter()
{
}

void vtkKitwareContourFilter::ComputeInputUpdateExtents(vtkDataObject *data)
{
  vtkDataSet *input=this->GetInput();

  if (!input) 
    {
    this->vtkContourFilter::ComputeInputUpdateExtents(data);
    return;
    }

   int inputObjectType = input->GetDataObjectType();

   if ( inputObjectType == VTK_STRUCTURED_POINTS || 
        inputObjectType == VTK_IMAGE_DATA )
     {
     
     int ext[6], dims[3], dim=0;
     ((vtkImageData *)input)->GetWholeExtent(ext);
     for(int j=0; j<3; j++)
       {
       dims[j] = ext[2*j+1]-ext[2*j];
       if ( dims[j] != 0 )
         {
         dim++;
         }
       }
     
     if ((dim == 2) && (dims[2] == 0))
       {
       vtkSynchronizedTemplates2D *syncTemp2D = 
         vtkSynchronizedTemplates2D::New();
       syncTemp2D->SetInput((vtkImageData *)input);
       syncTemp2D->SetDebug(this->Debug);
       syncTemp2D->ComputeInputUpdateExtents(data);
       syncTemp2D->Delete();
       return;
       }
     else if (dim == 3)
       {
       vtkSynchronizedTemplates3D *syncTemp3D =
         vtkSynchronizedTemplates3D::New();
       syncTemp3D->SetInput((vtkImageData *)input);
       syncTemp3D->SetDebug(this->Debug);
       syncTemp3D->SetComputeNormals (this->ComputeNormals);
       syncTemp3D->SetComputeGradients (this->ComputeGradients);
       syncTemp3D->SetComputeScalars (this->ComputeScalars);
       syncTemp3D->ComputeInputUpdateExtents(data);
       syncTemp3D->Delete();
       return;
       }
     }

   if ( inputObjectType  == VTK_STRUCTURED_GRID )
     {
     int ext[6], dim=0;
     ((vtkStructuredGrid *)input)->GetWholeExtent(ext);
     for(int j=0; j<3; j++)
       {
       if ( ( ext[2*j+1]-ext[2*j] ) != 0 )
         {
         dim++;
         }
       }
     if (dim == 3)
       {
       vtkGridSynchronizedTemplates3D *gridTemp3D =
         vtkGridSynchronizedTemplates3D::New();
       gridTemp3D->SetInput((vtkStructuredGrid *)input);
       gridTemp3D->SetComputeNormals (this->ComputeNormals);
       gridTemp3D->SetComputeGradients (this->ComputeGradients);
       gridTemp3D->SetComputeScalars (this->ComputeScalars);
       gridTemp3D->SetDebug(this->Debug);
       gridTemp3D->ComputeInputUpdateExtents(data);
       gridTemp3D->Delete();
       return;
       }
     }

  this->vtkContourFilter::ComputeInputUpdateExtents(data);
  return;
}

void vtkKitwareContourFilter::ExecuteInformation()
{
}

//
// General contouring filter.  Handles arbitrary input.
//
void vtkKitwareContourFilter::Execute()
{
  vtkDataArray *inScalars;
  vtkDataSet *input=this->GetInput();
  vtkIdType numCells;
  
  vtkDebugMacro(<< "Executing contour filter");

  numCells = input->GetNumberOfCells();
  inScalars = input->GetPointData()->GetScalars();
  if ( ! inScalars || numCells < 1 )
    {
    // vtkErrorMacro(<<"No data to contour");
    return;
    }

  // If structured points and structured grid, use more efficient algorithms
  if ( input->GetDataObjectType() == VTK_STRUCTURED_POINTS || 
       input->GetDataObjectType() == VTK_IMAGE_DATA )
    {
    // We need a better way to determine dimensionality for images.
    int dim = 3;
    int *uExt = input->GetUpdateExtent();
    if (uExt[0] == uExt[1])
      {
      --dim;
      }
    if (uExt[2] == uExt[3])
      {
      --dim;
      }
    if (uExt[4] == uExt[5])
      {
      --dim;
      }

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
         
    syncTemp2D->GetOutput()->SetUpdateExtent(thisOutput->GetUpdatePiece(),
                                             thisOutput->GetUpdateNumberOfPieces(),
                                             thisOutput->GetUpdateGhostLevel());
    syncTemp2D->SelectInputScalars(this->InputScalarsSelection);
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

    syncTemp3D->GetOutput()->SetUpdateExtent(thisOutput->GetUpdatePiece(),
                                             thisOutput->GetUpdateNumberOfPieces(),
                                             thisOutput->GetUpdateGhostLevel());
    syncTemp3D->SelectInputScalars(this->InputScalarsSelection);
    syncTemp3D->Update();
    output = syncTemp3D->GetOutput();
    output->Register(this);
    syncTemp3D->Delete();
    }
  
  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  thisOutput->GetCellData()->ShallowCopy(output->GetCellData());
  output->UnRegister(this);
}
//
// Special method handles structured grids
//
void vtkKitwareContourFilter::StructuredGridContour(int dim)
{
  vtkPolyData *output = NULL;
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

    output = gridTemp3D->GetOutput();
    output->SetUpdateNumberOfPieces(thisOutput->GetUpdateNumberOfPieces());
    output->SetUpdatePiece(thisOutput->GetUpdatePiece());
    output->SetUpdateGhostLevel(thisOutput->GetUpdateGhostLevel());
    gridTemp3D->SelectInputScalars(this->InputScalarsSelection);
    gridTemp3D->Update();
    output->Register(this);
    gridTemp3D->Delete();
    }
  
  thisOutput->CopyStructure(output);
  thisOutput->GetPointData()->ShallowCopy(output->GetPointData());
  thisOutput->GetCellData()->ShallowCopy(output->GetCellData());
  output->UnRegister(this);
}

void vtkKitwareContourFilter::DataSetContour()
{
  vtkPolyData *output = this->GetOutput();
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

  contour->SelectInputScalars(this->InputScalarsSelection);
  contour->Update();
  this->SetOutput(output);
  contour->Delete();
}
