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
  int ptId, i, j, k;
  vtkFloatTensors *newTensors;
  vtkTensor tensor;
  int numPts;
  vtkMath math;
  float P, pi, twoPi, xP[3], rho, rho2, rho3, rho5, nu;
  float x, x2, y, y2, z, z2, z3, rhoPlusz2, zPlus2rho, txy, txz, tyz;

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
// Compute the location of the load
//
  xP[0] = (this->ModelBounds[0] + this->ModelBounds[1]) / 2.0; //in center
  xP[1] = (this->ModelBounds[2] + this->ModelBounds[3]) / 2.0;
  xP[2] = this->ModelBounds[5]; // at top of box
//
// Traverse all points evaluating implicit function at each point
//
  twoPi = 2.0*math.Pi();
  P = -this->LoadValue;

  for (k=0; k<this->Dimensions[2]; k++)
    {
    z = this->Origin[2] + k*this->AspectRatio[2];
    for (j=0; j<this->Dimensions[1]; j++)
      {
      y = this->Origin[1] + k*this->AspectRatio[1];
      for (i=0; i<this->Dimensions[0]; i++)
        {
        x = this->Origin[0] + k*this->AspectRatio[0];
        rho = sqrt((x-xP[0])*(x-xP[0]) + (y-xP[1])*(y-xP[1]) + 
                   (z-xP[2])*(z-xP[2]));
        rho2 = rho*rho;
        rho3 = rho2*rho;
        rho5 = rho2*rho3;
        nu = (1.0 - 2.0*this->PoissonsRatio);
        x2 = x*x;
        y2 = y*y;
        rhoPlusz2 = (rho + z) * (rho + z);
        zPlus2rho = (2.0*rho + z);

        // normal stresses
        tensor.SetComponent(0,0, P/(twoPi*rho2) * (3.0*z*x2/rho3 -
                     nu*(z/rho - rho/(rho+z) + x2*(zPlus2rho)/(rho*rhoPlusz2))));
        tensor.SetComponent(1,1, P/(twoPi*rho2) * (3.0*z*y2/rho3 -
                     nu*(z/rho - rho/(rho+z) + y2*(zPlus2rho)/(rho*rhoPlusz2))));
        tensor.SetComponent(2,2, 3.0*P*z3/(twoPi*rho5));
        
        //shear stresses
        txy = P/(twoPi*rho2) * (3.0*x*y*z/rho3 - 
                                nu*x*y*(zPlus2rho)/(rho*rhoPlusz2));

        tensor.SetComponent(0,1,txy);
        tensor.SetComponent(1,0,txy);

        txz = 3.0*P*x*z2/(twoPi*rho5);
        tensor.SetComponent(0,2,txz);
        tensor.SetComponent(2,0,txz);
      
        tyz = 3.0*P*y*z2/(twoPi*rho5);
        tensor.SetComponent(1,2,tyz);
        tensor.SetComponent(2,1,tyz);

        newTensors->InsertNextTensor(tensor);
        }
      }
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
  os << indent << "Poisson's Ratio: " << this->PoissonsRatio << "\n";
  
}

