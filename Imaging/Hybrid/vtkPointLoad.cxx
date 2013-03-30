/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointLoad.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkPointLoad);

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

  this->PoissonsRatio = 0.3;

  this->SetNumberOfInputPorts(0);
}

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

// Specify the dimensions of the volume. A stress tensor will be computed for
// each point in the volume.
void vtkPointLoad::SetSampleDimensions(int dim[3])
{
  vtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if ( dim[0] != this->SampleDimensions[0] ||
       dim[1] != this->SampleDimensions[1] ||
       dim[2] != this->SampleDimensions[2] )
    {
    for ( int i=0; i<3; i++)
      {
      this->SampleDimensions[i] = (dim[i] > 0 ? dim[i] : 1);
      }
    this->Modified();
    }
}

int vtkPointLoad::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // use model bounds
  double origin[3];
  origin[0] = this->ModelBounds[0];
  origin[1] = this->ModelBounds[2];
  origin[2] = this->ModelBounds[4];
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  // Set volume origin and data spacing
  int i;
  double spacing[3];
  for (i=0; i<3; i++)
    {
    spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
      / (this->SampleDimensions[i] - 1);
    if ( spacing[i] <= 0.0 )
      {
      spacing[i] = 1.0;
      }
    }
  outInfo->Set(vtkDataObject::SPACING(),spacing,3);

  int wExt[6];
  wExt[0] = 0; wExt[2] = 0; wExt[4] = 0;
  wExt[1] = this->SampleDimensions[0] - 1;
  wExt[3] = this->SampleDimensions[1] - 1;
  wExt[5] = this->SampleDimensions[2] - 1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//
// Generate tensors and scalars for point load on semi-infinite domain.
//
void vtkPointLoad::ExecuteDataWithInformation(vtkDataObject *outp, vtkInformation* outInfo)
{
  int i, j, k;
  vtkFloatArray *newTensors;
  double tensor[9];
  vtkIdType numPts;
  double P, twoPi, xP[3], rho, rho2, rho3, rho5, nu;
  double x, x2, y, y2, z, z2, rhoPlusz2, zPlus2rho, txy, txz, tyz;
  double sx, sy, sz, seff;
  vtkImageData *output = this->AllocateOutputData(outp, outInfo);
  vtkFloatArray *newScalars =
    vtkFloatArray::SafeDownCast(output->GetPointData()->GetScalars());
  double *spacing, *origin;

  vtkDebugMacro(<< "Computing point load stress tensors");

  //
  // Initialize self; create output objects
  //
  numPts = this->SampleDimensions[0] * this->SampleDimensions[1]
           * this->SampleDimensions[2];
  spacing = output->GetSpacing();
  origin = output->GetOrigin();
  newTensors = vtkFloatArray::New();
  newTensors->SetNumberOfComponents(9);
  newTensors->Allocate(9*numPts);

  //
  // Compute the location of the load
  //
  xP[0] = (this->ModelBounds[0] + this->ModelBounds[1]) / 2.0; //in center
  xP[1] = (this->ModelBounds[2] + this->ModelBounds[3]) / 2.0;
  xP[2] = this->ModelBounds[5]; // at top of box
  //
  // Traverse all points evaluating implicit function at each point. Note that
  // points are evaluated in local coordinate system of applied force.
  //
  twoPi = 2.0*vtkMath::Pi();
  P = -this->LoadValue;
  int pointCount = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    z = xP[2] - (origin[2] + k*spacing[2]);
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      y = xP[1] - (origin[1] + j*spacing[1]);
      for (i=0; i<this->SampleDimensions[0]; i++)
        {
        x = (origin[0] + i*spacing[0]) - xP[0];
        rho = sqrt(x*x + y*y + z*z);//in local coordinates
        if ( rho < 1.0e-10 )
          {
          vtkWarningMacro(<<"Attempting to set singularity, resetting");
          tensor[0] = VTK_FLOAT_MAX; // Component(0,0)
          tensor[4] = VTK_FLOAT_MAX; // Component(1,1);
          tensor[8] = VTK_FLOAT_MAX; // Component(2,2);
          tensor[3] = 0.0; // Component(0,1);
          tensor[6] = 0.0; // Component(0,2);
          tensor[1] = 0.0; // Component(1,0);
          tensor[7] = 0.0; // Component(1,2);
          tensor[2] = 0.0; // Component(2,0);
          tensor[5] = 0.0; // Component(2,1);
          newTensors->InsertNextTuple(tensor);
          double val = VTK_FLOAT_MAX;
          newScalars->InsertTuple(pointCount,&val);
          pointCount++;
          continue;
          }

        rho2 = rho*rho;
        rho3 = rho2*rho;
        rho5 = rho2*rho3;
        nu = (1.0 - 2.0*this->PoissonsRatio);
        x2 = x*x;
        y2 = y*y;
        z2 = z*z;
        rhoPlusz2 = (rho + z) * (rho + z);
        zPlus2rho = (2.0*rho + z);

        // normal stresses
        sx = P/(twoPi*rho2) * (3.0*z*x2/rho3 - nu*(z/rho - rho/(rho+z) +
                               x2*(zPlus2rho)/(rho*rhoPlusz2)));
        sy = P/(twoPi*rho2) * (3.0*z*y2/rho3 - nu*(z/rho - rho/(rho+z) +
                               y2*(zPlus2rho)/(rho*rhoPlusz2)));
        sz = 3.0*P*z2*z/(twoPi*rho5);

        //shear stresses - negative signs are coordinate transformations
        //that is, equations (in text) are in different coordinate system
        //than volume is in.
        txy = -(P/(twoPi*rho2) * (3.0*x*y*z/rho3 -
                                nu*x*y*(zPlus2rho)/(rho*rhoPlusz2)));
        txz = -(3.0*P*x*z2/(twoPi*rho5));
        tyz = 3.0*P*y*z2/(twoPi*rho5);

        tensor[0] = sx;  // Component(0,0);
        tensor[4] = sy;  // Component(1,1);
        tensor[8] = sz;  // Component(2,2);
        tensor[3] = txy; // Component(0,1);  real symmetric matrix
        tensor[1] = txy; // Component(1,0);
        tensor[6] = txz; // Component(0,2);
        tensor[2] = txz; // Component(2,0);
        tensor[7] = tyz; // Component(1,2);
        tensor[5] = tyz; // Component(2,1);
        newTensors->InsertNextTuple(tensor);

        seff = 0.333333* sqrt ((sx-sy)*(sx-sy) + (sy-sz)*(sy-sz) +
                               (sz-sx)*(sz-sx) + 6.0*txy*txy + 6.0*tyz*tyz +
                               6.0*txz*txz);
        newScalars->InsertTuple(pointCount,&seff);
        pointCount++;
        }
      }
    }
  //
  // Update self and free memory
  //
  output->GetPointData()->SetTensors(newTensors);
  newTensors->Delete();
}


void vtkPointLoad::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

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

