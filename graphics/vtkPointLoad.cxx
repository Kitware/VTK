/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkPointLoad.h"
#include "vtkMath.h"
#include "vtkFloatTensors.h"

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

  this->ComputeEffectiveStress = 1;
  this->PoissonsRatio = 0.3;
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

void vtkPointLoad::SetModelBounds(float xmin, float xmax, float ymin, 
                                  float ymax, float zmin, float zmax)
{
  float bounds[6];

  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->SetModelBounds(bounds);
}

//
// Generate tensors and scalars for point load on semi-infinite domain.
//
void vtkPointLoad::Execute()
{
  int i, j, k;
  vtkFloatTensors *newTensors;
  vtkTensor tensor;
  vtkFloatScalars *newScalars = NULL;
  int numPts;
  float P, twoPi, xP[3], rho, rho2, rho3, rho5, nu;
  float x, x2, y, y2, z, z2, rhoPlusz2, zPlus2rho, txy, txz, tyz;
  float sx, sy, sz, seff, ar[3], origin[3];
  vtkStructuredPoints *output = (vtkStructuredPoints *)this->Output;

  vtkDebugMacro(<< "Computing point load stress tensors");
//
// Initialize self; create output objects
//

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  newTensors = new vtkFloatTensors(numPts);
  if ( this->ComputeEffectiveStress ) newScalars = new vtkFloatScalars(numPts);

  // Compute origin and aspect ratio
  output->SetDimensions(this->GetSampleDimensions());
  for (i=0; i < 3; i++)
    {
    origin[i] = this->ModelBounds[2*i];
    ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
            / (this->SampleDimensions[i] - 1);
    }
  output->SetOrigin(origin);
  output->SetAspectRatio(ar);
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

  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    z = xP[2] - (origin[2] + k*ar[2]);
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      y = xP[1] - (origin[1] + j*ar[1]);
      for (i=0; i<this->SampleDimensions[0]; i++)
        {
        x = (origin[0] + i*ar[0]) - xP[0];
        rho = sqrt(x*x + y*y + z*z);//in local coordinates
        if ( rho < 1.0e-10 )
          {
          vtkWarningMacro(<<"Attempting to set singularity, resetting");
          tensor.SetComponent(0,0,VTK_LARGE_FLOAT);
          tensor.SetComponent(1,1,VTK_LARGE_FLOAT);
          tensor.SetComponent(2,2,VTK_LARGE_FLOAT);
          tensor.SetComponent(0,1,0.0);
          tensor.SetComponent(0,2,0.0);
          tensor.SetComponent(1,0,0.0);
          tensor.SetComponent(1,2,0.0);
          tensor.SetComponent(2,0,0.0);
          tensor.SetComponent(2,1,0.0);
          newTensors->InsertNextTensor(&tensor);
          if ( this->ComputeEffectiveStress )
            newScalars->InsertNextScalar(VTK_LARGE_FLOAT);
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

        tensor.SetComponent(0,0, sx);
        tensor.SetComponent(1,1, sy);
        tensor.SetComponent(2,2, sz);
        tensor.SetComponent(0,1, txy); // real symmetric matrix
        tensor.SetComponent(1,0, txy);
        tensor.SetComponent(0,2, txz);
        tensor.SetComponent(2,0, txz);
        tensor.SetComponent(1,2, tyz);
        tensor.SetComponent(2,1, tyz);
        newTensors->InsertNextTensor(&tensor);

        if ( this->ComputeEffectiveStress )
          {
          seff = 0.333333* sqrt ((sx-sy)*(sx-sy) + (sy-sz)*(sy-sz) +
                 (sz-sx)*(sz-sx) + 6.0*txy*txy + 6.0*tyz*tyz + 6.0*txz*txz);
          newScalars->InsertNextScalar(seff);
          }
        }
      }
    }
//
// Update self and free memory
//
  output->GetPointData()->SetTensors(newTensors);
  newTensors->Delete();

  if ( this->ComputeEffectiveStress)
    {
    output->GetPointData()->SetScalars(newScalars);
    newScalars->Delete();
    }
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
  os << indent << "Compute Effective Stress: " << 
        (this->ComputeEffectiveStress ? "On\n" : "Off\n");
  
}

