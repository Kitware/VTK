/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.cxx
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
#include "vtkSampleFunction.h"
#include "vtkMath.h"
#include "vtkScalars.h"
#include "vtkNormals.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkSampleFunction* vtkSampleFunction::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkSampleFunction");
  if(ret)
    {
    return (vtkSampleFunction*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkSampleFunction;
}




// Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
// Capping turned off, and normal generation on.
vtkSampleFunction::vtkSampleFunction()
{
  this->ModelBounds[0] = -1.0;
  this->ModelBounds[1] = 1.0;
  this->ModelBounds[2] = -1.0;
  this->ModelBounds[3] = 1.0;
  this->ModelBounds[4] = -1.0;
  this->ModelBounds[5] = 1.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Capping = 0;
  this->CapValue = VTK_LARGE_FLOAT;

  this->ImplicitFunction = NULL;

  this->ComputeNormals = 1;
  this->Scalars = NULL;
}

vtkSampleFunction::~vtkSampleFunction() 
{
  this->SetScalars(NULL);
  this->SetImplicitFunction(NULL);
}


// Specify the dimensions of the data on which to sample.
void vtkSampleFunction::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

// Specify the dimensions of the data on which to sample.
void vtkSampleFunction::SetSampleDimensions(int dim[3])
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

void vtkSampleFunction::ExecuteInformation()
{
  int i;
  float ar[3], origin[3];
  vtkStructuredPoints *output = this->GetOutput();
  
  output->SetScalarType(VTK_FLOAT);
  output->SetNumberOfScalarComponents(1);
  output->SetWholeExtent(0, this->SampleDimensions[0]-1,
			 0, this->SampleDimensions[1]-1,
			 0, this->SampleDimensions[2]-1);

  for (i=0; i < 3; i++)
    {
    origin[i] = this->ModelBounds[2*i];
    if ( this->SampleDimensions[i] <= 1 )
      {
      ar[i] = 1;
      }
    else
      {
      ar[i] = (this->ModelBounds[2*i+1] - this->ModelBounds[2*i])
              / (this->SampleDimensions[i] - 1);
      }
    }
  output->SetOrigin(origin);
  output->SetSpacing(ar);
}


void vtkSampleFunction::Execute()
{
  vtkIdType ptId;
  vtkNormals *newNormals=NULL;
  vtkIdType numPts;
  float *p, s;
  vtkStructuredPoints *output = this->GetOutput();

  output->SetDimensions(this->GetSampleDimensions());

  vtkDebugMacro(<< "Sampling implicit function");
  //
  // Initialize self; create output objects
  //

  if ( !this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  numPts = this->SampleDimensions[0] * this->SampleDimensions[1] 
           * this->SampleDimensions[2];

  if (this->Scalars == NULL) 
    {
    this->Scalars = vtkScalars::New(); //ref count is 1
    this->Scalars->Register(this);
    this->Scalars->Delete();
    }
  this->Scalars->SetNumberOfScalars(numPts);

  //
  // Traverse all points evaluating implicit function at each point
  //
  for (ptId=0; ptId < numPts; ptId++ )
    {
    p = output->GetPoint(ptId);
    s = this->ImplicitFunction->FunctionValue(p);
    this->Scalars->SetScalar(ptId,s);
    }
  //
  // If normal computation turned on, compute them
  //
  if ( this->ComputeNormals )
    {
    float n[3];
    newNormals = vtkNormals::New(); 
    newNormals->SetNumberOfNormals(numPts);
    for (ptId=0; ptId < numPts; ptId++ )
      {
      p = output->GetPoint(ptId);
      this->ImplicitFunction->FunctionGradient(p, n);
      n[0] *= -1;
      n[1] *= -1;
      n[2] *= -1;
      vtkMath::Normalize(n);
      newNormals->SetNormal(ptId,n);
      }
    }
  //
  // If capping is turned on, set the distances of the outside of the volume
  // to the CapValue.
  //
  if ( this->Capping )
    {
    this->Cap(this->Scalars);
    }
  //
  // Update self 
  //
  output->GetPointData()->SetScalars(this->Scalars); //ref count is now 2

  if (newNormals)
    {
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
    }
}


unsigned long vtkSampleFunction::GetMTime()
{
  unsigned long mTime=this->vtkSource::GetMTime();
  unsigned long impFuncMTime;

  if ( this->ImplicitFunction != NULL )
    {
    impFuncMTime = this->ImplicitFunction->GetMTime();
    mTime = ( impFuncMTime > mTime ? impFuncMTime : mTime );
    }

  return mTime;
}

void vtkSampleFunction::Cap(vtkScalars *s)
{
  int i,j,k;
  vtkIdType idx;
  int d01=this->SampleDimensions[0]*this->SampleDimensions[1];

// i-j planes
  k = 0;
  for (j=0; j<this->SampleDimensions[1]; j++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetScalar(i+j*this->SampleDimensions[0], this->CapValue);
      }
    }

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetScalar(idx+i+j*this->SampleDimensions[0], this->CapValue);
      }
    }

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      s->SetScalar(j*this->SampleDimensions[0]+k*d01, this->CapValue);
      }
    }

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      s->SetScalar(i+j*this->SampleDimensions[0]+k*d01, this->CapValue);
      }
    }

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetScalar(i+k*d01, this->CapValue);
      }
    }

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetScalar(idx+i+k*d01, this->CapValue);
      }
    }
}


void vtkSampleFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
               << this->SampleDimensions[1] << ", "
               << this->SampleDimensions[2] << ")\n";
  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] 
     << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] 
     << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] 
     << ", " << this->ModelBounds[5] << ")\n";

  if ( this->Scalars )
    {
    os << indent << "Scalars: " << this->Scalars << "\n";
    }
  else
    {
    os << indent << "Scalars: (none)\n";
    }

  if ( this->ImplicitFunction )
    {
    os << indent << "Implicit Function: " << this->ImplicitFunction << "\n";
    }
  else
    {
    os << indent << "No Implicit function defined\n";
    }

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
}

