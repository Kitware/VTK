/*=========================================================================

  Program:   Visualization Library
  Module:    PtLoad.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include "PtLoad.hh"
#include "vtkMath.hh"
#include "FTensors.hh"

// Description:
// Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
// and LoadValue = 1.
vtkPointLoad::vtkPointLoad()
{
 this->LoadValue = 1.0;

  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;
}

// Description:
// Specify the dimensions of the volume. A stress tensor will be computed for
// each point in the volume.
void vtkPointLoad::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

// Description:
// Specify the dimensions of the volume. A stress tensor will be computed for
// each point in the volume.
void vtkPointLoad::SetSampleDimensions(int dim[3])
{
  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] || dim[1] != SampleDimensions[1] ||
  dim[2] != SampleDimensions[2] )
    {
    for ( int i=0; i<3; i++) 
      this->SampleDimensions[i] = (dim[i] > 0 ? dim[i] : 1);
    this->Modified();
    }
}

// Description:
// Specify the region in space over which the tensors are computed. The point
// load is assumed to be applied at top center of the volume.
void vtkPointLoad::SetModelBounds(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
{
  if (this->ModelBounds[0] != xmin || this->ModelBounds[1] != xmax ||
  this->ModelBounds[2] != ymin || this->ModelBounds[3] != ymax ||
  this->ModelBounds[4] != zmin || this->ModelBounds[5] != zmax )
    {
    this->Modified();
    this->ModelBounds[0] = xmin;
    this->ModelBounds[1] = xmax;
    this->ModelBounds[2] = ymin;
    this->ModelBounds[3] = ymax;
    this->ModelBounds[4] = zmin;
    this->ModelBounds[5] = zmax;
    }
}

// Description:
// Specify the region in space over which the tensors are computed. The point
// load is assumed to be applied at top center of the volume.
void vtkPointLoad::SetModelBounds(float *bounds)
{
  vtkPointLoad::SetModelBounds(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
}

void vtkPointLoad::Execute()
{
  int ptId, i;
  vtkFloatTensors *newTensors;
  vtkTensor tensor;
  int numPts;
  vtkMath math;

  vtkDebugMacro(<< "Computing point load stress tensors");
//
// Initialize self; create output objects
//
  this->Initialize();

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  newTensors = new vtkFloatTensors(numPts);

  // Compute origin and aspect ratio
  this->SetDimensions(this->GetSampleDimensions());
  for (i=0; i < 3; i++)
    {
    this->Origin[i] = this->ModelBounds[2*i];
    this->AspectRatio[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
                           / (this->SampleDimensions[i] - 1);
    }
//
// Traverse all points evaluating implicit function at each point
//
  for (ptId=0; ptId < numPts; ptId++ )
    {
    }
//
// Update self
//
  this->PointData.SetTensors(newTensors);
}


void vtkPointLoad::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Load Value: " << this->LoadValue << "\n";
  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";
}
