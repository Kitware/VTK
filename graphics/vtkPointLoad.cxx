/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointLoad.cxx
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
#include "vtkPointLoad.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkPointLoad* vtkPointLoad::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPointLoad");
  if(ret)
    {
    return (vtkPointLoad*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPointLoad;
}




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

//
// Generate tensors and scalars for point load on semi-infinite domain.
//
void vtkPointLoad::Execute()
{
  int i, j, k;
  vtkFloatArray *newTensors;
  float tensor[9];
  vtkFloatArray *newScalars = NULL;
  int numPts;
  float P, twoPi, xP[3], rho, rho2, rho3, rho5, nu;
  float x, x2, y, y2, z, z2, rhoPlusz2, zPlus2rho, txy, txz, tyz;
  float sx, sy, sz, seff, spacing[3], origin[3];
  vtkStructuredPoints *output = this->GetOutput();

  vtkDebugMacro(<< "Computing point load stress tensors");
  //
  // Initialize self; create output objects
  //

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];
  newTensors = vtkFloatArray::New();
  newTensors->SetNumberOfComponents(9);
  newTensors->Allocate(9*numPts);
  if ( this->ComputeEffectiveStress ) 
    {
    newScalars = vtkFloatArray::New();
    newScalars->Allocate(numPts);
    }

  // Compute origin and data spacing
  output->SetDimensions(this->GetSampleDimensions());
  for (i=0; i < 3; i++)
    {
    origin[i] = this->ModelBounds[2*i];
    spacing[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
            / (this->SampleDimensions[i] - 1);
    }
  output->SetOrigin(origin);
  output->SetSpacing(spacing);
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
          tensor[0] = VTK_LARGE_FLOAT; // Component(0,0)
	  tensor[4] = VTK_LARGE_FLOAT; // Component(1,1);
          tensor[8] = VTK_LARGE_FLOAT; // Component(2,2);
          tensor[3] = 0.0; // Component(0,1);
          tensor[6] = 0.0; // Component(0,2);
          tensor[1] = 0.0; // Component(1,0);
          tensor[7] = 0.0; // Component(1,2);
          tensor[2] = 0.0; // Component(2,0);
          tensor[5] = 0.0; // Component(2,1);
          newTensors->InsertNextTuple(tensor);
          if ( this->ComputeEffectiveStress )
	    {
	    float val = VTK_LARGE_FLOAT;
            newScalars->InsertNextTuple(&val);
	    }
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

        if ( this->ComputeEffectiveStress )
          {
          seff = 0.333333* sqrt ((sx-sy)*(sx-sy) + (sy-sz)*(sy-sz) +
                 (sz-sx)*(sz-sx) + 6.0*txy*txy + 6.0*tyz*tyz + 6.0*txz*txz);
          newScalars->InsertNextTuple(&seff);
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

