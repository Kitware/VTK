/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.cxx
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
#include "vtkSampleFunction.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSampleFunction, "1.59");
vtkStandardNewMacro(vtkSampleFunction);

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
  this->OutputScalarType = VTK_FLOAT;
}

vtkSampleFunction::~vtkSampleFunction() 
{
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
  vtkImageData *output = this->GetOutput();
  
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


void vtkSampleFunction::ExecuteData(vtkDataObject *outp)
{
  vtkIdType ptId;
  vtkFloatArray *newNormals=NULL;
  vtkIdType numPts;
  float *p, s;
  vtkImageData *output = this->AllocateOutputData(outp);
  vtkFloatArray *newScalars = 
    vtkFloatArray::SafeDownCast(output->GetPointData()->GetScalars());

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

  //
  // Traverse all points evaluating implicit function at each point
  //
  for (ptId=0; ptId < numPts; ptId++ )
    {
    p = output->GetPoint(ptId);
    s = this->ImplicitFunction->FunctionValue(p);
    newScalars->SetComponent(ptId,0,s);
    }
  //
  // If normal computation turned on, compute them
  //
  if ( this->ComputeNormals )
    {
    float n[3];
    newNormals = vtkFloatArray::New(); 
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
    for (ptId=0; ptId < numPts; ptId++ )
      {
      p = output->GetPoint(ptId);
      this->ImplicitFunction->FunctionGradient(p, n);
      n[0] *= -1;
      n[1] *= -1;
      n[2] *= -1;
      vtkMath::Normalize(n);
      newNormals->SetTuple(ptId,n);
      }
    }
  //
  // If capping is turned on, set the distances of the outside of the volume
  // to the CapValue.
  //
  if ( this->Capping )
    {
    this->Cap(newScalars);
    }
  //
  // Update self 
  //
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

void vtkSampleFunction::Cap(vtkDataArray *s)
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
      s->SetComponent(i+j*this->SampleDimensions[0], 0, this->CapValue);
      }
    }

  k = this->SampleDimensions[2] - 1;
  idx = k*d01;
  for (j=0; j<this->SampleDimensions[1]; j++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetComponent(idx+i+j*this->SampleDimensions[0], 0, this->CapValue);
      }
    }

// j-k planes
  i = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      s->SetComponent(j*this->SampleDimensions[0]+k*d01, 0, this->CapValue);
      }
    }

  i = this->SampleDimensions[0] - 1;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (j=0; j<this->SampleDimensions[1]; j++)
      {
      s->SetComponent(i+j*this->SampleDimensions[0]+k*d01, 0, this->CapValue);
      }
    }

// i-k planes
  j = 0;
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetComponent(i+k*d01, 0, this->CapValue);
      }
    }

  j = this->SampleDimensions[1] - 1;
  idx = j*this->SampleDimensions[0];
  for (k=0; k<this->SampleDimensions[2]; k++)
    {
    for (i=0; i<this->SampleDimensions[0]; i++)
      {
      s->SetComponent(idx+i+k*d01, 0, this->CapValue);
      }
    }
}


void vtkSampleFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
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

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";

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

