/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSampleFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSampleFunction.h"

#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkImplicitFunction.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkSampleFunction);
vtkCxxSetObjectMacro(vtkSampleFunction,ImplicitFunction,vtkImplicitFunction);

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
  this->CapValue = VTK_DOUBLE_MAX;

  this->ImplicitFunction = NULL;

  this->ComputeNormals = 1;
  this->OutputScalarType = VTK_DOUBLE;

  this->ScalarArrayName=0;
  this->SetScalarArrayName("scalars");

  this->NormalArrayName=0;
  this->SetNormalArrayName("normals");


  this->SetNumberOfInputPorts(0);
}

vtkSampleFunction::~vtkSampleFunction()
{
  this->SetImplicitFunction(NULL);
  this->SetScalarArrayName(NULL);
  this->SetNormalArrayName(NULL);
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

// Set the bounds of the model
void vtkSampleFunction::SetModelBounds(double bounds[6])
{
  this->SetModelBounds(bounds[0], bounds[1],
                       bounds[2], bounds[3],
                       bounds[4], bounds[5]);

}
void vtkSampleFunction::SetModelBounds(double xMin, double xMax,
                                       double yMin, double yMax,
                                       double zMin, double zMax)
{
  vtkDebugMacro(<< " setting ModelBounds to ("
                << "(" << xMin << "," << xMax << "), "
                << "(" << yMin << "," << yMax << "), "
                << "(" << zMin << "," << zMax << "), ");
  if ((xMin > xMax) ||
      (yMin > yMax) ||
      (zMin > zMax))
    {
    vtkErrorMacro("Invalid bounds: "
                  << "(" << xMin << "," << xMax << "), "
                  << "(" << yMin << "," << yMax << "), "
                  << "(" << zMin << "," << zMax << ")"
                  << " Bound mins cannot be larger that bound maxs");
    return;
    }
  if (xMin != this->ModelBounds[0] ||
      xMax != this->ModelBounds[1] ||
      yMin != this->ModelBounds[2] ||
      yMax != this->ModelBounds[3] ||
      zMin != this->ModelBounds[4] ||
      zMax != this->ModelBounds[5])
    {
    this->ModelBounds[0] = xMin;
    this->ModelBounds[1] = xMax;
    this->ModelBounds[2] = yMin;
    this->ModelBounds[3] = yMax;
    this->ModelBounds[4] = zMin;
    this->ModelBounds[5] = zMax;
    this->Modified();
    }
}

int vtkSampleFunction::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  int wExt[6];
  wExt[0] = 0; wExt[2] = 0; wExt[4] = 0;
  wExt[1] = this->SampleDimensions[0]-1;
  wExt[3] = this->SampleDimensions[1]-1;
  wExt[5] = this->SampleDimensions[2]-1;

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);

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
  outInfo->Set(vtkDataObject::ORIGIN(),origin,3);
  outInfo->Set(vtkDataObject::SPACING(),ar,3);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo,this->OutputScalarType,
                                              1);

  return 1;
}


void vtkSampleFunction::ExecuteDataWithInformation(vtkDataObject *outp, vtkInformation *outInfo)
{
  vtkIdType idx, i, j, k;
  vtkFloatArray *newNormals=NULL;
  vtkIdType numPts;
  double p[3], s;
  vtkImageData *output=this->GetOutput();
  int* extent =
    this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  output->SetExtent(extent);
  output = this->AllocateOutputData(outp, outInfo);
  vtkDataArray *newScalars =output->GetPointData()->GetScalars();

  vtkDebugMacro(<< "Sampling implicit function");

  // Initialize self; create output objects
  //
  if ( !this->ImplicitFunction )
    {
    vtkErrorMacro(<<"No implicit function specified");
    return;
    }

  numPts = newScalars->GetNumberOfTuples();

  // Traverse all points evaluating implicit function at each point
  //
  double spacing[3];
  output->GetSpacing(spacing);

  for ( idx=0, k=extent[4]; k <= extent[5]; k++ )
    {
    p[2] = this->ModelBounds[4] + k*spacing[2];
    for ( j=extent[2]; j <= extent[3]; j++ )
      {
      p[1] = this->ModelBounds[2] + j*spacing[1];
      for ( i=extent[0]; i <= extent[1]; i++ )
        {
        p[0] = this->ModelBounds[0] + i*spacing[0];
        s = this->ImplicitFunction->FunctionValue(p);
        newScalars->SetTuple1(idx++,s);
        }
      }
    }

  // If normal computation turned on, compute them
  //
  if ( this->ComputeNormals )
    {
    double n[3];
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
    for ( idx=0, k=extent[4]; k <= extent[5]; k++ )
      {
      p[2] = this->ModelBounds[4] + k*spacing[2];
      for ( j=extent[2]; j <= extent[3]; j++ )
        {
        p[1] = this->ModelBounds[2] + j*spacing[1];
        for ( i=extent[0]; i <= extent[1]; i++ )
          {
          p[0] = this->ModelBounds[0] + i*spacing[0];
          this->ImplicitFunction->FunctionGradient(p, n);
          n[0] *= -1;
          n[1] *= -1;
          n[2] *= -1;
          vtkMath::Normalize(n);
          newNormals->SetTuple(idx++,n);
          }
        }
      }
    }

  newScalars->SetName(this->ScalarArrayName);


  // If capping is turned on, set the distances of the outside of the volume
  // to the CapValue.
  //
  if ( this->Capping )
    {
    this->Cap(newScalars);
    }

  // Update self
  //
  if (newNormals)
    {
    // For an unknown reason yet, if the following line is not commented out,
    // it will make ImplicitSum, TestBoxFunction and TestDiscreteMarchingCubes
    // to fail.
    newNormals->SetName(this->NormalArrayName);

    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
    }
}


unsigned long vtkSampleFunction::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
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
  const int* extent =
    this->GetExecutive()->GetOutputInformation(0)->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // i-j planes
  //k = extent[4];
  for (j=extent[2]; j<=extent[3]; j++)
    {
    for (i=extent[0]; i<=extent[1]; i++)
      {
      s->SetComponent(i+j*this->SampleDimensions[0], 0, this->CapValue);
      }
    }

  k = extent[5];
  idx = k*d01;
  for (j=extent[2]; j<=extent[3]; j++)
    {
    for (i=extent[0]; i<=extent[1]; i++)
      {
      s->SetComponent(idx+i+j*this->SampleDimensions[0], 0, this->CapValue);
      }
    }

  // j-k planes
  //i = extent[0];
  for (k=extent[4]; k<=extent[5]; k++)
    {
    for (j=extent[2]; j<=extent[3]; j++)
      {
      s->SetComponent(j*this->SampleDimensions[0]+k*d01, 0, this->CapValue);
      }
    }

  i = extent[1];
  for (k=extent[4]; k<=extent[5]; k++)
    {
    for (j=extent[2]; j<=extent[3]; j++)
      {
      s->SetComponent(i+j*this->SampleDimensions[0]+k*d01, 0, this->CapValue);
      }
    }

  // i-k planes
  //j = extent[2];
  for (k=extent[4]; k<=extent[5]; k++)
    {
    for (i=extent[0]; i<=extent[1]; i++)
      {
      s->SetComponent(i+k*d01, 0, this->CapValue);
      }
    }

  j = extent[3];
  idx = j*this->SampleDimensions[0];
  for (k=extent[4]; k<=extent[5]; k++)
    {
    for (i=extent[0]; i<=extent[1]; i++)
      {
      s->SetComponent(idx+i+k*d01, 0, this->CapValue);
      }
    }
}

#ifndef VTK_LEGACY_REMOVE
void vtkSampleFunction::SetScalars(vtkDataArray *da)
{
  VTK_LEGACY_BODY(vtkSampleFunction::SetScalars, "VTK 6.0");
  if (da)
    {
    this->SetOutputScalarType(da->GetDataType());
    }
}
#endif

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

  os << indent << "ScalarArrayName: ";
  if(this->ScalarArrayName!=0)
    {
    os  << this->ScalarArrayName << endl;
    }
  else
    {
    os  << "(none)" << endl;
    }

  os << indent << "NormalArrayName: ";
  if(this->NormalArrayName!=0)
    {
    os  << this->NormalArrayName << endl;
    }
  else
    {
    os  << "(none)" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkSampleFunction::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->ImplicitFunction,
                            "ImplicitFunction");
}
