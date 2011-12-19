/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageReslice.h"

#include "vtkIntArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkPointData.h"
#include "vtkImageInterpolator.h"
#include "vtkGarbageCollector.h"

#include "vtkImageInterpolatorInternals.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0

#include <limits.h>
#include <float.h>
#include <math.h>

vtkStandardNewMacro(vtkImageReslice);
vtkCxxSetObjectMacro(vtkImageReslice, InformationInput, vtkImageData);
vtkCxxSetObjectMacro(vtkImageReslice,ResliceAxes,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkImageReslice,Interpolator,vtkAbstractImageInterpolator);
vtkCxxSetObjectMacro(vtkImageReslice,ResliceTransform,vtkAbstractTransform);

//--------------------------------------------------------------------------
// typedef for pixel converter method
typedef void (vtkImageReslice::*vtkImageResliceConvertScalarsType)(
  void *outPtr, void *inPtr, int inputType, int inNumComponents,
  int count, int idX, int idY, int idZ, int threadId);

// typedef for the floating point type used by the code
typedef double vtkImageResliceFloatingPointType;

//----------------------------------------------------------------------------
vtkImageReslice::vtkImageReslice()
{
  // if NULL, the main Input is used
  this->InformationInput = NULL;
  this->TransformInputSampling = 1;
  this->AutoCropOutput = 0;
  this->OutputDimensionality = 3;
  this->ComputeOutputSpacing = 1;
  this->ComputeOutputOrigin = 1;
  this->ComputeOutputExtent = 1;

  // flag to use default Spacing
  this->OutputSpacing[0] = 1.0;
  this->OutputSpacing[1] = 1.0;
  this->OutputSpacing[2] = 1.0;

  // ditto
  this->OutputOrigin[0] = 0.0;
  this->OutputOrigin[1] = 0.0;
  this->OutputOrigin[2] = 0.0;

  // ditto
  this->OutputExtent[0] = 0;
  this->OutputExtent[2] = 0;
  this->OutputExtent[4] = 0;

  this->OutputExtent[1] = 0;
  this->OutputExtent[3] = 0;
  this->OutputExtent[5] = 0;

  this->OutputScalarType = -1;

  this->Wrap = 0; // don't wrap
  this->Mirror = 0; // don't mirror
  this->Border = 1; // apply a border
  this->InterpolationMode = VTK_RESLICE_NEAREST; // no interpolation

  this->SlabMode = VTK_IMAGE_SLAB_MEAN;
  this->SlabNumberOfSlices = 1;
  this->SlabTrapezoidIntegration = 0;

  this->Optimization = 1; // turn off when you're paranoid

  // default black background
  this->BackgroundColor[0] = 0;
  this->BackgroundColor[1] = 0;
  this->BackgroundColor[2] = 0;
  this->BackgroundColor[3] = 0;

  // default reslice axes are x, y, z
  this->ResliceAxesDirectionCosines[0] = 1.0;
  this->ResliceAxesDirectionCosines[1] = 0.0;
  this->ResliceAxesDirectionCosines[2] = 0.0;
  this->ResliceAxesDirectionCosines[3] = 0.0;
  this->ResliceAxesDirectionCosines[4] = 1.0;
  this->ResliceAxesDirectionCosines[5] = 0.0;
  this->ResliceAxesDirectionCosines[6] = 0.0;
  this->ResliceAxesDirectionCosines[7] = 0.0;
  this->ResliceAxesDirectionCosines[8] = 1.0;

  // default (0,0,0) axes origin
  this->ResliceAxesOrigin[0] = 0.0;
  this->ResliceAxesOrigin[1] = 0.0;
  this->ResliceAxesOrigin[2] = 0.0;

  // axes and transform are identity if set to NULL
  this->ResliceAxes = NULL;
  this->ResliceTransform = NULL;
  this->Interpolator = NULL;

  // cache a matrix that converts output voxel indices -> input voxel indices
  this->IndexMatrix = NULL;
  this->OptimizedTransform = NULL;

  // set to zero when we completely missed the input extent
  this->HitInputExtent = 1;

  // set to true if PermuteExecute fast path will be used
  this->UsePermuteExecute = 0;

  // set in subclasses that convert the scalars after they are interpolated
  this->HasConvertScalars = 0;

  // the output stencil
  this->GenerateStencilOutput = 0;

  // There is an optional second input (the stencil input)
  this->SetNumberOfInputPorts(2);
  // There is an optional second output (the stencil output)
  this->SetNumberOfOutputPorts(2);

  // Create a stencil output (empty for now)
  vtkImageStencilData *stencil = vtkImageStencilData::New();
  this->GetExecutive()->SetOutputData(1, stencil);
  stencil->ReleaseData();
  stencil->Delete();
}

//----------------------------------------------------------------------------
vtkImageReslice::~vtkImageReslice()
{
  this->SetResliceTransform(NULL);
  this->SetResliceAxes(NULL);
  if (this->IndexMatrix)
    {
    this->IndexMatrix->Delete();
    }
  if (this->OptimizedTransform)
    {
    this->OptimizedTransform->Delete();
    }
  this->SetInformationInput(NULL);
  this->SetInterpolator(NULL);
}

//----------------------------------------------------------------------------
void vtkImageReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ResliceAxes: " << this->ResliceAxes << "\n";
  if (this->ResliceAxes)
    {
    this->ResliceAxes->PrintSelf(os,indent.GetNextIndent());
    }
  this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
  os << indent << "ResliceAxesDirectionCosines: " <<
    this->ResliceAxesDirectionCosines[0] << " " <<
    this->ResliceAxesDirectionCosines[1] << " " <<
    this->ResliceAxesDirectionCosines[2] << "\n";
  os << indent << "                             " <<
    this->ResliceAxesDirectionCosines[3] << " " <<
    this->ResliceAxesDirectionCosines[4] << " " <<
    this->ResliceAxesDirectionCosines[5] << "\n";
  os << indent << "                             " <<
    this->ResliceAxesDirectionCosines[6] << " " <<
    this->ResliceAxesDirectionCosines[7] << " " <<
    this->ResliceAxesDirectionCosines[8] << "\n";
  this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
  os << indent << "ResliceAxesOrigin: " <<
    this->ResliceAxesOrigin[0] << " " <<
    this->ResliceAxesOrigin[1] << " " <<
    this->ResliceAxesOrigin[2] << "\n";
  os << indent << "ResliceTransform: " << this->ResliceTransform << "\n";
  if (this->ResliceTransform)
    {
    this->ResliceTransform->PrintSelf(os,indent.GetNextIndent());
    }
  os << indent << "Interpolator: " << this->Interpolator << "\n";
  os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "TransformInputSampling: " <<
    (this->TransformInputSampling ? "On\n":"Off\n");
  os << indent << "AutoCropOutput: " <<
    (this->AutoCropOutput ? "On\n":"Off\n");
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputExtent: " << this->OutputExtent[0] << " " <<
    this->OutputExtent[1] << " " << this->OutputExtent[2] << " " <<
    this->OutputExtent[3] << " " << this->OutputExtent[4] << " " <<
    this->OutputExtent[5] << "\n";
  os << indent << "OutputDimensionality: " <<
    this->OutputDimensionality << "\n";
  os << indent << "Wrap: " << (this->Wrap ? "On\n":"Off\n");
  os << indent << "Mirror: " << (this->Mirror ? "On\n":"Off\n");
  os << indent << "Border: " << (this->Border ? "On\n":"Off\n");
  os << indent << "InterpolationMode: "
     << this->GetInterpolationModeAsString() << "\n";
  os << indent << "SlabMode: " << this->GetSlabModeAsString() << "\n";
  os << indent << "SlabNumberOfSlices: " << this->SlabNumberOfSlices << "\n";
  os << indent << "SlabTrapezoidIntegration: "
     << (this->SlabTrapezoidIntegration ? "On\n" : "Off\n");
  os << indent << "Optimization: " << (this->Optimization ? "On\n":"Off\n");
  os << indent << "BackgroundColor: " <<
    this->BackgroundColor[0] << " " << this->BackgroundColor[1] << " " <<
    this->BackgroundColor[2] << " " << this->BackgroundColor[3] << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundColor[0] << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "GenerateStencilOutput: " << (this->GenerateStencilOutput ? "On\n":"Off\n");
  os << indent << "StencilOutput: " << this->GetStencilOutput() << "\n";
}

//----------------------------------------------------------------------------
void vtkImageReslice::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->InformationInput,
                            "InformationInput");
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputSpacing(double x, double y, double z)
{
  double *s = this->OutputSpacing;
  if (s[0] != x || s[1] != y || s[2] != z)
    {
    this->OutputSpacing[0] = x;
    this->OutputSpacing[1] = y;
    this->OutputSpacing[2] = z;
    this->Modified();
    }
  else if (this->ComputeOutputSpacing)
    {
    this->Modified();
    }
  this->ComputeOutputSpacing = 0;
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputSpacingToDefault()
{
  if (!this->ComputeOutputSpacing)
    {
    this->OutputSpacing[0] = 1.0;
    this->OutputSpacing[1] = 1.0;
    this->OutputSpacing[2] = 1.0;
    this->ComputeOutputSpacing = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputOrigin(double x, double y, double z)
{
  double *o = this->OutputOrigin;
  if (o[0] != x || o[1] != y || o[2] != z)
    {
    this->OutputOrigin[0] = x;
    this->OutputOrigin[1] = y;
    this->OutputOrigin[2] = z;
    this->Modified();
    }
  else if (this->ComputeOutputOrigin)
    {
    this->Modified();
    }
  this->ComputeOutputOrigin = 0;
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputOriginToDefault()
{
  if (!this->ComputeOutputOrigin)
    {
    this->OutputOrigin[0] = 0.0;
    this->OutputOrigin[1] = 0.0;
    this->OutputOrigin[2] = 0.0;
    this->ComputeOutputOrigin = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputExtent(int a, int b, int c, int d, int e, int f)
{
  int *extent = this->OutputExtent;
  if (extent[0] != a || extent[1] != b || extent[2] != c ||
      extent[3] != d || extent[4] != e || extent[5] != f)
    {
    this->OutputExtent[0] = a;
    this->OutputExtent[1] = b;
    this->OutputExtent[2] = c;
    this->OutputExtent[3] = d;
    this->OutputExtent[4] = e;
    this->OutputExtent[5] = f;
    this->Modified();
    }
  else if (this->ComputeOutputExtent)
    {
    this->Modified();
    }
  this->ComputeOutputExtent = 0;
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetOutputExtentToDefault()
{
  if (!this->ComputeOutputExtent)
    {
    this->OutputExtent[0] = 0;
    this->OutputExtent[2] = 0;
    this->OutputExtent[4] = 0;
    this->OutputExtent[1] = 0;
    this->OutputExtent[3] = 0;
    this->OutputExtent[5] = 0;
    this->ComputeOutputExtent = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const char *vtkImageReslice::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      return "NearestNeighbor";
    case VTK_RESLICE_LINEAR:
      return "Linear";
    case VTK_RESLICE_CUBIC:
      return "Cubic";
    }
  return "";
}

//----------------------------------------------------------------------------
const char *vtkImageReslice::GetSlabModeAsString()
{
  switch (this->SlabMode)
    {
    case VTK_IMAGE_SLAB_MIN:
      return "Min";
    case VTK_IMAGE_SLAB_MAX:
      return "Max";
    case VTK_IMAGE_SLAB_MEAN:
      return "Mean";
    case VTK_IMAGE_SLAB_SUM:
      return "Sum";
    }
  return "";
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetStencil(vtkImageStencilData *stencil)
{
  this->SetInput(1, stencil);
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageReslice::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetStencilOutput(vtkImageStencilData *output)
{
  this->GetExecutive()->SetOutputData(1, output);
}

//----------------------------------------------------------------------------
vtkImageStencilData *vtkImageReslice::GetStencilOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
    {
    return NULL;
    }

  return vtkImageStencilData::SafeDownCast(
    this->GetExecutive()->GetOutputData(1));
}


//----------------------------------------------------------------------------
void vtkImageReslice::SetResliceAxesDirectionCosines(double x0, double x1,
                                                     double x2, double y0,
                                                     double y1, double y2,
                                                     double z0, double z1,
                                                     double z2)
{
  if (!this->ResliceAxes)
    {
    // consistent registers/unregisters
    this->SetResliceAxes(vtkMatrix4x4::New());
    this->ResliceAxes->Delete();
    this->Modified();
    }
  this->ResliceAxes->SetElement(0,0,x0);
  this->ResliceAxes->SetElement(1,0,x1);
  this->ResliceAxes->SetElement(2,0,x2);
  this->ResliceAxes->SetElement(3,0,0);
  this->ResliceAxes->SetElement(0,1,y0);
  this->ResliceAxes->SetElement(1,1,y1);
  this->ResliceAxes->SetElement(2,1,y2);
  this->ResliceAxes->SetElement(3,1,0);
  this->ResliceAxes->SetElement(0,2,z0);
  this->ResliceAxes->SetElement(1,2,z1);
  this->ResliceAxes->SetElement(2,2,z2);
  this->ResliceAxes->SetElement(3,2,0);
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetResliceAxesDirectionCosines(double xdircos[3],
                                                     double ydircos[3],
                                                     double zdircos[3])
{
  if (!this->ResliceAxes)
    {
    xdircos[0] = ydircos[1] = zdircos[2] = 1;
    xdircos[1] = ydircos[2] = zdircos[0] = 0;
    xdircos[2] = ydircos[0] = zdircos[1] = 0;
    return;
    }

  for (int i = 0; i < 3; i++)
    {
    xdircos[i] = this->ResliceAxes->GetElement(i,0);
    ydircos[i] = this->ResliceAxes->GetElement(i,1);
    zdircos[i] = this->ResliceAxes->GetElement(i,2);
    }
}

//----------------------------------------------------------------------------
void vtkImageReslice::SetResliceAxesOrigin(double x, double y, double z)
{
  if (!this->ResliceAxes)
    {
    // consistent registers/unregisters
    this->SetResliceAxes(vtkMatrix4x4::New());
    this->ResliceAxes->Delete();
    this->Modified();
    }

  this->ResliceAxes->SetElement(0,3,x);
  this->ResliceAxes->SetElement(1,3,y);
  this->ResliceAxes->SetElement(2,3,z);
  this->ResliceAxes->SetElement(3,3,1);
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetResliceAxesOrigin(double origin[3])
{
  if (!this->ResliceAxes)
    {
    origin[0] = origin[1] = origin[2] = 0;
    return;
    }

  for (int i = 0; i < 3; i++)
    {
    origin[i] = this->ResliceAxes->GetElement(i,3);
    }
}

//----------------------------------------------------------------------------
vtkAbstractImageInterpolator *vtkImageReslice::GetInterpolator()
{
  if (this->Interpolator == NULL)
    {
    this->Interpolator = vtkImageInterpolator::New();
    }

  return this->Interpolator;
}

//----------------------------------------------------------------------------
// Account for the MTime of the transform and its matrix when determining
// the MTime of the filter
unsigned long int vtkImageReslice::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->ResliceTransform != NULL )
    {
    time = this->ResliceTransform->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    if (this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      { // this is for people who directly modify the transform matrix
      time = (static_cast<vtkHomogeneousTransform *>(this->ResliceTransform))
        ->GetMatrix()->GetMTime();
      mTime = ( time > mTime ? time : mTime );
      }
    }
  if ( this->ResliceAxes != NULL)
    {
    time = this->ResliceAxes->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  if ( this->Interpolator != NULL)
    {
    time = this->Interpolator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
int vtkImageReslice::ConvertScalarInfo(
  int &vtkNotUsed(scalarType), int &vtkNotUsed(numComponents))
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageReslice::ConvertScalars(
  void *vtkNotUsed(inPtr), void *vtkNotUsed(outPtr),
  int vtkNotUsed(inputType), int vtkNotUsed(inputComponents),
  int vtkNotUsed(count), int vtkNotUsed(idX), int vtkNotUsed(idY),
  int vtkNotUsed(idZ), int vtkNotUsed(threadId))
{
}

//----------------------------------------------------------------------------
int vtkImageReslice::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int inExt[6], outExt[6];
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);

  if (this->ResliceTransform)
    {
    this->ResliceTransform->Update();
    if (!this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      { // update the whole input extent if the transform is nonlinear
      inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
      return 1;
      }
    }

  bool wrap = (this->Wrap || this->Mirror);

  double xAxis[4], yAxis[4], zAxis[4], origin[4];

  vtkMatrix4x4 *matrix = this->GetIndexMatrix(inInfo, outInfo);

  // convert matrix from world coordinates to pixel indices
  for (int i = 0; i < 4; i++)
    {
    xAxis[i] = matrix->GetElement(i,0);
    yAxis[i] = matrix->GetElement(i,1);
    zAxis[i] = matrix->GetElement(i,2);
    origin[i] = matrix->GetElement(i,3);
    }

  for (int i = 0; i < 3; i++)
    {
    inExt[2*i] = VTK_INT_MAX;
    inExt[2*i+1] = VTK_INT_MIN;
    }

  if (this->SlabNumberOfSlices > 1)
    {
    outExt[4] -= (this->SlabNumberOfSlices+1)/2;
    outExt[5] += (this->SlabNumberOfSlices+1)/2;
    }

  // set the extent according to the interpolation kernel size
  vtkAbstractImageInterpolator *interpolator = this->GetInterpolator();
  double *elements = *matrix->Element;
  elements = ((this->OptimizedTransform == NULL) ? elements : NULL);
  int supportSize[3];
  interpolator->ComputeSupportSize(elements, supportSize);

  // check the coordinates of the 8 corners of the output extent
  // (this must be done exactly the same as the calculation in
  // vtkImageResliceExecute)
  for (int jj = 0; jj < 8; jj++)
    {
    // get output coords
    int idX = outExt[jj%2];
    int idY = outExt[2+(jj/2)%2];
    int idZ = outExt[4+(jj/4)%2];

    double inPoint0[4];
    inPoint0[0] = origin[0] + idZ*zAxis[0]; // incremental transform
    inPoint0[1] = origin[1] + idZ*zAxis[1];
    inPoint0[2] = origin[2] + idZ*zAxis[2];
    inPoint0[3] = origin[3] + idZ*zAxis[3];

    double inPoint1[4];
    inPoint1[0] = inPoint0[0] + idY*yAxis[0]; // incremental transform
    inPoint1[1] = inPoint0[1] + idY*yAxis[1];
    inPoint1[2] = inPoint0[2] + idY*yAxis[2];
    inPoint1[3] = inPoint0[3] + idY*yAxis[3];

    double point[4];
    point[0] = inPoint1[0] + idX*xAxis[0];
    point[1] = inPoint1[1] + idX*xAxis[1];
    point[2] = inPoint1[2] + idX*xAxis[2];
    point[3] = inPoint1[3] + idX*xAxis[3];

    if (point[3] != 1.0)
      {
      double f = 1/point[3];
      point[0] *= f;
      point[1] *= f;
      point[2] *= f;
      }

    for (int j = 0; j < 3; j++)
      {
      int kernelSize = supportSize[j];
      int extra = (kernelSize + 1)/2 - 1;

      // most kernels have even size
      if ((kernelSize & 1) == 0)
        {
        double f;
        int k = vtkInterpolationMath::Floor(point[j], f);
        if (k - extra < inExt[2*j])
          {
          inExt[2*j] = k - extra;
          }
        k += (f != 0);
        if (k + extra > inExt[2*j+1])
          {
          inExt[2*j+1] = k + extra;
          }
        }
      // else is for kernels with odd size
      else
        {
        int k = vtkInterpolationMath::Round(point[j]);
        if (k < inExt[2*j])
          {
          inExt[2*j] = k - extra;
          }
        if (k > inExt[2*j+1])
          {
          inExt[2*j+1] = k + extra;
          }
        }
      }
    }

  // Clip to whole extent, make sure we hit the extent
  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  this->HitInputExtent = 1;

  for (int k = 0; k < 3; k++)
    {
    if (inExt[2*k] < wholeExtent[2*k])
      {
      inExt[2*k] = wholeExtent[2*k];
      if (wrap)
        {
        inExt[2*k+1] = wholeExtent[2*k+1];
        }
      else if (inExt[2*k+1] < wholeExtent[2*k])
        {
        // didn't hit any of the input extent
        inExt[2*k+1] = wholeExtent[2*k];
        this->HitInputExtent = 0;
        }
      }
    if (inExt[2*k+1] > wholeExtent[2*k+1])
      {
      inExt[2*k+1] = wholeExtent[2*k+1];
      if (wrap)
        {
        inExt[2*k] = wholeExtent[2*k];
        }
      else if (inExt[2*k] > wholeExtent[2*k+1])
        {
        // didn't hit any of the input extent
        inExt[2*k] = wholeExtent[2*k+1];
        // finally, check for null input extent
        if (inExt[2*k] < wholeExtent[2*k])
          {
          inExt[2*k] = wholeExtent[2*k];
          }
        this->HitInputExtent = 0;
        }
      }
    }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the output extent
  if (this->GetNumberOfInputConnections(1) > 0)
    {
    vtkInformation *stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                     outExt, 6);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageReslice::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 1)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageStencilData");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageReslice::FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 1)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageStencilData");
    }
  else
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageReslice::AllocateOutputData(vtkImageData *output,
                                         int *uExtent)
{
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  output->AllocateScalars();

  vtkImageStencilData *stencil = this->GetStencilOutput();
  if (stencil && this->GenerateStencilOutput)
    {
    stencil->SetExtent(uExtent);
    stencil->AllocateExtents();
    }
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageReslice::AllocateOutputData(vtkDataObject *output)
{
  return this->Superclass::AllocateOutputData(output);
}

//----------------------------------------------------------------------------
void vtkImageReslice::GetAutoCroppedOutputBounds(vtkInformation *inInfo,
                                                 double bounds[6])
{
  int i, j;
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  double f;
  double point[4];

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
  inInfo->Get(vtkDataObject::SPACING(), inSpacing);
  inInfo->Get(vtkDataObject::ORIGIN(), inOrigin);

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  if (this->ResliceAxes)
    {
    vtkMatrix4x4::Invert(this->ResliceAxes, matrix);
    }
  vtkAbstractTransform* transform = NULL;
  if (this->ResliceTransform)
    {
    transform = this->ResliceTransform->GetInverse();
    }

  for (i = 0; i < 3; i++)
    {
    bounds[2*i] = VTK_DOUBLE_MAX;
    bounds[2*i+1] = -VTK_DOUBLE_MAX;
    }

  for (i = 0; i < 8; i++)
    {
    point[0] = inOrigin[0] + inWholeExt[i%2]*inSpacing[0];
    point[1] = inOrigin[1] + inWholeExt[2+(i/2)%2]*inSpacing[1];
    point[2] = inOrigin[2] + inWholeExt[4+(i/4)%2]*inSpacing[2];
    point[3] = 1.0;

    if (this->ResliceTransform)
      {
      transform->TransformPoint(point,point);
      }
    matrix->MultiplyPoint(point,point);

    f = 1.0/point[3];
    point[0] *= f;
    point[1] *= f;
    point[2] *= f;

    for (j = 0; j < 3; j++)
      {
      if (point[j] > bounds[2*j+1])
        {
        bounds[2*j+1] = point[j];
        }
      if (point[j] < bounds[2*j])
        {
        bounds[2*j] = point[j];
        }
      }
    }

  matrix->Delete();
}

//----------------------------------------------------------------------------
namespace {
//----------------------------------------------------------------------------
// check a matrix to ensure that it is a permutation+scale+translation
// matrix

int vtkIsPermutationMatrix(vtkMatrix4x4 *matrix)
{
  for (int i = 0; i < 3; i++)
    {
    if (matrix->GetElement(3,i) != 0)
      {
      return 0;
      }
    }
  if (matrix->GetElement(3,3) != 1)
    {
    return 0;
    }
  for (int j = 0; j < 3; j++)
    {
    int k = 0;
    for (int i = 0; i < 3; i++)
      {
      if (matrix->GetElement(i,j) != 0)
        {
        k++;
        }
      }
    if (k != 1)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// Check to see if we can do nearest-neighbor instead of linear or cubic.
// This check only works on permutation+scale+translation matrices.
int vtkCanUseNearestNeighbor(vtkMatrix4x4 *matrix, int outExt[6])
{
  // loop through dimensions
  for (int i = 0; i < 3; i++)
    {
    int j;
    for (j = 0; j < 3; j++)
      {
      if (matrix->GetElement(i,j) != 0)
        {
        break;
        }
      }
    double x = matrix->GetElement(i,j);
    double y = matrix->GetElement(i,3);
    if (outExt[2*j] == outExt[2*j+1])
      {
      y += x*outExt[2*i];
      x = 0;
      }
    double fx, fy;
    vtkInterpolationMath::Floor(x, fx);
    vtkInterpolationMath::Floor(y, fy);
    if (fx != 0 || fy != 0)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// check a matrix to see whether it is the identity matrix

int vtkIsIdentityMatrix(vtkMatrix4x4 *matrix)
{
  static double identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  int i,j;

  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      if (matrix->GetElement(i,j) != identity[4*i+j])
        {
        return 0;
        }
      }
    }
  return 1;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int vtkImageReslice::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int i,j;
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  double outSpacing[3], outOrigin[3];
  int outWholeExt[6];
  double maxBounds[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (this->InformationInput)
    {
    this->InformationInput->UpdateInformation();
    this->InformationInput->GetWholeExtent(inWholeExt);
    this->InformationInput->GetSpacing(inSpacing);
    this->InformationInput->GetOrigin(inOrigin);
    }
  else
    {
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
    inInfo->Get(vtkDataObject::SPACING(), inSpacing);
    inInfo->Get(vtkDataObject::ORIGIN(), inOrigin);
    }

  // reslice axes matrix is identity by default
  double matrix[4][4];
  double imatrix[4][4];
  for (i = 0; i < 4; i++)
    {
    matrix[i][0] = matrix[i][1] = matrix[i][2] = matrix[i][3] = 0;
    matrix[i][i] = 1;
    imatrix[i][0] = imatrix[i][1] = imatrix[i][2] = imatrix[i][3] = 0;
    imatrix[i][i] = 1;
    }
  if (this->ResliceAxes)
    {
    vtkMatrix4x4::DeepCopy(*matrix, this->ResliceAxes);
    vtkMatrix4x4::Invert(*matrix,*imatrix);
    }

  if (this->AutoCropOutput)
    {
    this->GetAutoCroppedOutputBounds(inInfo, maxBounds);
    }

  // pass the center of the volume through the inverse of the
  // 3x3 direction cosines matrix
  double inCenter[3];
  for (i = 0; i < 3; i++)
    {
    inCenter[i] = inOrigin[i] + \
      0.5*(inWholeExt[2*i] + inWholeExt[2*i+1])*inSpacing[i];
    }

  // the default spacing, extent and origin are the input spacing, extent
  // and origin,  transformed by the direction cosines of the ResliceAxes
  // if requested (note that the transformed output spacing will always
  // be positive)
  for (i = 0; i < 3; i++)
    {
    double s = 0;  // default output spacing
    double d = 0;  // default linear dimension
    double e = 0;  // default extent start
    double c = 0;  // transformed center-of-volume

    if (this->TransformInputSampling)
      {
      double r = 0.0;
      for (j = 0; j < 3; j++)
        {
        c += imatrix[i][j]*(inCenter[j] - matrix[j][3]);
        double tmp = matrix[j][i]*matrix[j][i];
        s += tmp*fabs(inSpacing[j]);
        d += tmp*(inWholeExt[2*j+1] - inWholeExt[2*j])*fabs(inSpacing[j]);
        e += tmp*inWholeExt[2*j];
        r += tmp;
        }
      s /= r;
      d /= r*sqrt(r);
      e /= r;
      }
    else
      {
      s = inSpacing[i];
      d = (inWholeExt[2*i+1] - inWholeExt[2*i])*s;
      e = inWholeExt[2*i];
      }

    if (this->ComputeOutputSpacing)
      {
      outSpacing[i] = s;
      }
    else
      {
      outSpacing[i] = this->OutputSpacing[i];
      }

    if (i >= this->OutputDimensionality)
      {
      outWholeExt[2*i] = 0;
      outWholeExt[2*i+1] = 0;
      }
    else if (this->ComputeOutputExtent)
      {
      if (this->AutoCropOutput)
        {
        d = maxBounds[2*i+1] - maxBounds[2*i];
        }
      outWholeExt[2*i] = vtkInterpolationMath::Round(e);
      outWholeExt[2*i+1] = vtkInterpolationMath::Round(outWholeExt[2*i] +
                                           fabs(d/outSpacing[i]));
      }
    else
      {
      outWholeExt[2*i] = this->OutputExtent[2*i];
      outWholeExt[2*i+1] = this->OutputExtent[2*i+1];
      }

    if (i >= this->OutputDimensionality)
      {
      outOrigin[i] = 0;
      }
    else if (this->ComputeOutputOrigin)
      {
      if (this->AutoCropOutput)
        { // set origin so edge of extent is edge of bounds
        outOrigin[i] = maxBounds[2*i] - outWholeExt[2*i]*outSpacing[i];
        }
      else
        { // center new bounds over center of input bounds
        outOrigin[i] = c - \
          0.5*(outWholeExt[2*i] + outWholeExt[2*i+1])*outSpacing[i];
        }
      }
    else
      {
      outOrigin[i] = this->OutputOrigin[i];
      }
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),outWholeExt,6);
  outInfo->Set(vtkDataObject::SPACING(), outSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);

  if (this->GenerateStencilOutput)
    {
    vtkInformation *outStencilInfo = outputVector->GetInformationObject(1);
    outStencilInfo->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt,6);
    outStencilInfo->Set(vtkDataObject::SPACING(), outSpacing, 3);
    outStencilInfo->Set(vtkDataObject::ORIGIN(), outOrigin, 3);
    }

  // get the interpolator
  vtkAbstractImageInterpolator *interpolator = this->GetInterpolator();

  // set the scalar information
  vtkInformation *inScalarInfo = vtkDataObject::GetActiveFieldInformation(
    inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS,
    vtkDataSetAttributes::SCALARS);

  int scalarType = -1;
  int numComponents = -1;

  if (inScalarInfo)
    {
    scalarType = inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());

    if (inScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
      {
      numComponents = interpolator->ComputeNumberOfComponents(
        inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
      }
    }

  if (this->HasConvertScalars)
    {
    this->ConvertScalarInfo(scalarType, numComponents);

    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, scalarType, numComponents);
    }
  else
    {
    if (this->OutputScalarType > 0)
      {
      scalarType = this->OutputScalarType;
      }

    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, scalarType, numComponents);
    }

  // create a matrix for structured coordinate conversion
  this->GetIndexMatrix(inInfo, outInfo);

  // check for possible optimizations
  int interpolationMode = this->InterpolationMode;
  this->UsePermuteExecute = 0;
  if (this->Optimization)
    {
    if (this->OptimizedTransform == NULL &&
        interpolator->IsSeparable() &&
        vtkIsPermutationMatrix(this->IndexMatrix))
      {
      this->UsePermuteExecute = 1;
      if (vtkCanUseNearestNeighbor(this->IndexMatrix, outWholeExt))
        {
        interpolationMode = VTK_NEAREST_INTERPOLATION;
        }
      }
    }

  // set the interpolator information
  if (interpolator->IsA("vtkImageInterpolator"))
    {
    static_cast<vtkImageInterpolator *>(interpolator)->
      SetInterpolationMode(interpolationMode);
    }
  int borderMode = VTK_IMAGE_BORDER_CLAMP;
  borderMode = (this->Wrap ? VTK_IMAGE_BORDER_REPEAT : borderMode);
  borderMode = (this->Mirror ? VTK_IMAGE_BORDER_MIRROR : borderMode);
  interpolator->SetBorderMode(borderMode);

  // set the tolerance according to the border mode, use infinite
  // (or at least very large) tolerance for wrap and mirror
  static double mintol = VTK_INTERPOLATE_FLOOR_TOL;
  static double maxtol = 2.0*VTK_INT_MAX;
  double tol = 0.5*this->Border;
  tol = ((borderMode == VTK_IMAGE_BORDER_CLAMP) ? tol : maxtol);
  tol = ((tol > mintol) ? tol : mintol);
  interpolator->SetTolerance(tol);

  return 1;
}

//----------------------------------------------------------------------------
// rounding functions for each type, where 'F' is a floating-point type

namespace {

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeInt8& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeUInt8& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeInt16& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeUInt16& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeInt32& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeUInt32& rnd)
{
  rnd = vtkInterpolationMath::Round(val);
}
#endif

#if (VTK_USE_FLOAT32 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeFloat32& rnd)
{
  rnd = val;
}
#endif

#if (VTK_USE_FLOAT64 != 0)
template <class F>
inline void vtkInterpolateRound(F val, vtkTypeFloat64& rnd)
{
  rnd = val;
}
#endif

//----------------------------------------------------------------------------
// clamping functions for each type

template <class F>
inline F vtkResliceClamp(F x, F xmin, F xmax)
{
  // do not change this code: it compiles into min/max opcodes
  x = (x > xmin ? x : xmin);
  x = (x < xmax ? x : xmax);
  return x;
}

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt8& clamp)
{
  static F minval = static_cast<F>(-128.0);
  static F maxval = static_cast<F>(127.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt8& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(255.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt16& clamp)
{
  static F minval = static_cast<F>(-32768.0);
  static F maxval = static_cast<F>(32767.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt16& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(65535.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt32& clamp)
{
  static F minval = static_cast<F>(-2147483648.0);
  static F maxval = static_cast<F>(2147483647.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt32& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(4294967295.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkInterpolateRound(val,clamp);
}
#endif

#if (VTK_USE_FLOAT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeFloat32& clamp)
{
  clamp = val;
}
#endif

#if (VTK_USE_FLOAT64 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeFloat64& clamp)
{
  clamp = val;
}
#endif

//----------------------------------------------------------------------------
// Convert from float to any type, with clamping or not.
template<class F, class T>
struct vtkImageResliceConversion
{
  static void Convert(
    void *&outPtr, const F *inPtr, int numscalars, int n);

  static void Clamp(
    void *&outPtr, const F *inPtr, int numscalars, int n);
};

template <class F, class T>
void vtkImageResliceConversion<F, T>::Convert(
  void *&outPtr0, const F *inPtr, int numscalars, int n)
{
  if (n > 0)
    {
    // This is a very hot loop, so it is unrolled
    T* outPtr = static_cast<T*>(outPtr0);
    int m = n*numscalars;
    for (int q = m >> 2; q > 0; --q)
      {
      vtkInterpolateRound(inPtr[0], outPtr[0]);
      vtkInterpolateRound(inPtr[1], outPtr[1]);
      vtkInterpolateRound(inPtr[2], outPtr[2]);
      vtkInterpolateRound(inPtr[3], outPtr[3]);
      inPtr += 4;
      outPtr += 4;
      }
    for (int r = m & 0x0003; r > 0; --r)
      {
      vtkInterpolateRound(*inPtr++, *outPtr++);
      }
    outPtr0 = outPtr;
    }
}

template <class F, class T>
void vtkImageResliceConversion<F, T>::Clamp(
  void *&outPtr0, const F *inPtr, int numscalars, int n)
{
  T* outPtr = static_cast<T*>(outPtr0);
  for (int m = n*numscalars; m > 0; --m)
    {
    vtkResliceClamp(*inPtr++, *outPtr++);
    }
  outPtr0 = outPtr;
}

// get the convertion function
template<class F>
void vtkGetConversionFunc(
  void (**conversion)(void *&out, const F *in, int numscalars, int n),
  int inputType, int dataType, int interpolationMode, int slabMode)
{
  if (interpolationMode <= VTK_LINEAR_INTERPOLATION &&
      slabMode != VTK_IMAGE_SLAB_SUM &&
      vtkDataArray::GetDataTypeMin(dataType) <=
        vtkDataArray::GetDataTypeMin(inputType) &&
      vtkDataArray::GetDataTypeMax(dataType) >=
        vtkDataArray::GetDataTypeMax(inputType))
    {
    // linear and nearest-neighbor do not need range checking
    switch (dataType)
      {
      vtkTemplateAliasMacro(
        *conversion = &(vtkImageResliceConversion<F, VTK_TT>::Convert)
        );
      default:
        *conversion = 0;
      }
    }
  else
    {
    // cubic interpolation needs range checking, so use clamp
    switch (dataType)
      {
      vtkTemplateAliasMacro(
        *conversion = &(vtkImageResliceConversion<F, VTK_TT>::Clamp)
        );
      default:
        *conversion = 0;
      }
    }
}

//----------------------------------------------------------------------------
// Various pixel compositors for slab views
template<class F>
struct vtkImageResliceComposite
{
  static void MeanValue(F *inPtr, int numscalars, int n);
  static void MeanTrap(F *inPtr, int numscalars, int n);
  static void SumValues(F *inPtr, int numscalars, int n);
  static void SumTrap(F *inPtr, int numscalars, int n);
  static void MinValue(F *inPtr, int numscalars, int n);
  static void MaxValue(F *inPtr, int numscalars, int n);
};

template<class F>
void vtkImageResliceSlabSum(F *inPtr, int numscalars, int n, F f)
{
  int m = numscalars;
  --n;
  do
    {
    F result = *inPtr;
    int k = n;
    do
      {
      inPtr += numscalars;
      result += *inPtr;
      }
    while (--k);
    inPtr -= n*numscalars;
    *inPtr++ = result*f;
    }
  while (--m);
}

template<class F>
void vtkImageResliceSlabTrap(F *inPtr, int numscalars, int n, F f)
{
  int m = numscalars;
  --n;
  do
    {
    F result = *inPtr*0.5;
    for (int k = n-1; k != 0; --k)
      {
      inPtr += numscalars;
      result += *inPtr;
      }
    inPtr += numscalars;
    result += *inPtr*0.5;
    inPtr -= n*numscalars;
    *inPtr++ = result*f;
    }
  while (--m);
}

template <class F>
void vtkImageResliceComposite<F>::MeanValue(F *inPtr, int numscalars, int n)
{
  F f = 1.0/n;
  vtkImageResliceSlabSum(inPtr, numscalars, n, f);
}

template <class F>
void vtkImageResliceComposite<F>::MeanTrap(F *inPtr, int numscalars, int n)
{
  F f = 1.0/(n-1);
  vtkImageResliceSlabTrap(inPtr, numscalars, n, f);
}

template <class F>
void vtkImageResliceComposite<F>::SumValues(F *inPtr, int numscalars, int n)
{
  vtkImageResliceSlabSum(inPtr, numscalars, n, static_cast<F>(1.0));
}

template <class F>
void vtkImageResliceComposite<F>::SumTrap(F *inPtr, int numscalars, int n)
{
  vtkImageResliceSlabTrap(inPtr, numscalars, n, static_cast<F>(1.0));
}

template <class F>
void vtkImageResliceComposite<F>::MinValue(F *inPtr, int numscalars, int n)
{
  int m = numscalars;
  --n;
  do
    {
    F result = *inPtr;
    int k = n;
    do
      {
      inPtr += numscalars;
      result = (result < *inPtr ? result : *inPtr);
      }
    while (--k);
    inPtr -= n*numscalars;
    *inPtr++ = result;
    }
  while (--m);
}

template <class F>
void vtkImageResliceComposite<F>::MaxValue(F *inPtr, int numscalars, int n)
{
  int m = numscalars;
  --n;
  do
    {
    F result = *inPtr;
    int k = n;
    do
      {
      inPtr += numscalars;
      result = (result > *inPtr ? result : *inPtr);
      }
    while (--k);
    inPtr -= n*numscalars;
    *inPtr++ = result;
    }
  while (--m);
}

// get the composite function
template<class F>
void vtkGetCompositeFunc(
  void (**composite)(F *in, int numscalars, int n),
  int slabMode, int trpz)
{
  switch (slabMode)
    {
    case VTK_IMAGE_SLAB_MIN:
      *composite = &(vtkImageResliceComposite<F>::MinValue);
      break;
    case VTK_IMAGE_SLAB_MAX:
      *composite = &(vtkImageResliceComposite<F>::MaxValue);
      break;
    case VTK_IMAGE_SLAB_MEAN:
      if (trpz) { *composite = &(vtkImageResliceComposite<F>::MeanTrap); }
      else { *composite = &(vtkImageResliceComposite<F>::MeanValue); }
      break;
    case VTK_IMAGE_SLAB_SUM:
      if (trpz) { *composite = &(vtkImageResliceComposite<F>::SumTrap); }
      else { *composite = &(vtkImageResliceComposite<F>::SumValues); }
      break;
    default:
      *composite = 0;
    }
}

//----------------------------------------------------------------------------
// Some helper functions for 'RequestData'
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// Check pointer memory alignment with 4-byte words
inline int vtkImageReslicePointerAlignment(void *ptr, int n)
{
#if (VTK_SIZEOF_VOID_P == 8)
  return ((reinterpret_cast<vtkTypeUInt64>(ptr) % n) == 0);
#else
  return ((reinterpret_cast<vtkTypeUInt32>(ptr) % n) == 0);
#endif
}

//--------------------------------------------------------------------------
// pixel copy function, templated for different scalar types
template <class T>
struct vtkImageResliceSetPixels
{
static void Set(void *&outPtrV, const void *inPtrV, int numscalars, int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    const T *tmpPtr = inPtr;
    int m = numscalars;
    do
      {
      *outPtr++ = *tmpPtr++;
      }
    while (--m);
    }
  outPtrV = outPtr;
}

// optimized for 1 scalar components
static void Set1(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  T val = *inPtr;
  for (; n > 0; --n)
    {
    *outPtr++ = val;
    }
  outPtrV = outPtr;
}

// optimized for 2 scalar components
static void Set2(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr += 2;
    }
  outPtrV = outPtr;
}

// optimized for 3 scalar components
static void Set3(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    outPtr += 3;
    }
  outPtrV = outPtr;
}

// optimized for 4 scalar components
static void Set4(void *&outPtrV, const void *inPtrV,
                 int vtkNotUsed(numscalars), int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (; n > 0; --n)
    {
    outPtr[0] = inPtr[0];
    outPtr[1] = inPtr[1];
    outPtr[2] = inPtr[2];
    outPtr[3] = inPtr[3];
    outPtr += 4;
    }
  outPtrV = outPtr;
}

};

// get a pixel copy function that is appropriate for the data type
void vtkGetSetPixelsFunc(
  void (**setpixels)(void *&out, const void *in, int numscalars, int n),
  int dataType, int dataSize, int numscalars, void *dataPtr)
{
  // If memory is 4-byte aligned, copy in 4-byte chunks
  if (vtkImageReslicePointerAlignment(dataPtr, 4) &&
      ((dataSize*numscalars) & 0x03) == 0 &&
      dataSize < 4 && dataSize*numscalars <= 16)
    {
    switch ((dataSize*numscalars) >> 2)
      {
      case 1:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set1;
        break;
      case 2:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set2;
        break;
      case 3:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set3;
        break;
      case 4:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set4;
        break;
      }
    return;
    }

  switch (numscalars)
    {
    case 1:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set1
          );
        default:
          *setpixels = 0;
        }
    case 2:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set2
          );
        default:
          *setpixels = 0;
        }
    case 3:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set3
          );
        default:
          *setpixels = 0;
        }
    case 4:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set4
          );
        default:
          *setpixels = 0;
        }
    default:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set
          );
        default:
          *setpixels = 0;
        }
    }
}

//----------------------------------------------------------------------------
// Convert background color from float to appropriate type
template <class T>
void vtkCopyBackgroundColor(
  double dcolor[4], T *background, int numComponents)
{
  int c = (numComponents < 4 ? numComponents : 4);
  for (int i = 0; i < c; i++)
    {
    vtkResliceClamp(dcolor[i], background[i]);
    }
  for (int j = c; j < numComponents; j++)
    {
    background[j] = 0;
    }
}

void vtkAllocBackgroundPixel(
  void **rval, double dcolor[4], int scalarType, int scalarSize,
  int numComponents)
{
  int bytesPerPixel = numComponents*scalarSize;

  // allocate as an array of doubles to guarantee alignment
  // (this is probably more paranoid than necessary)
  int n = (bytesPerPixel + VTK_SIZEOF_DOUBLE - 1)/VTK_SIZEOF_DOUBLE;
  double *doublePtr = new double[n];
  *rval = doublePtr;

  switch (scalarType)
    {
    vtkTemplateAliasMacro(vtkCopyBackgroundColor(
      dcolor, (VTK_TT *)(*rval), numComponents));
    }
}

void vtkFreeBackgroundPixel(void **rval)
{
  double *doublePtr = static_cast<double *>(*rval);
  delete [] doublePtr;

  *rval = 0;
}

//----------------------------------------------------------------------------
// helper function for clipping of the output with a stencil
int vtkResliceGetNextExtent(vtkImageStencilData *stencil,
                            int &r1, int &r2, int rmin, int rmax,
                            int yIdx, int zIdx,
                            void *&outPtr, void *background,
                            int numscalars,
                            void (*setpixels)(void *&out,
                                              const void *in,
                                              int numscalars,
                                              int n),
                            int &iter)
{
  // trivial case if stencil is not set
  if (!stencil)
    {
    if (iter++ == 0)
      {
      r1 = rmin;
      r2 = rmax;
      return 1;
      }
    return 0;
    }

  // for clearing, start at last r2 plus 1
  int clear1 = r2 + 1;
  if (iter == 0)
    { // if no 'last time', start at rmin
    clear1 = rmin;
    }

  int rval = stencil->GetNextExtent(r1, r2, rmin, rmax, yIdx, zIdx, iter);
  int clear2 = r1 - 1;
  if (rval == 0)
    {
    clear2 = rmax;
    }

  setpixels(outPtr, background, numscalars, clear2 - clear1 + 1);

  return rval;
}

//----------------------------------------------------------------------------
// This function simply clears the entire output to the background color,
// for cases where the transformation places the output extent completely
// outside of the input extent.
void vtkImageResliceClearExecute(vtkImageReslice *self,
                                 vtkImageData *outData, void *outPtr,
                                 int outExt[6], int threadId)
{
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // for the progress meter
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Get Increments to march through data
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int numscalars = outData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  void *background;
  vtkAllocBackgroundPixel(&background,
     self->GetBackgroundColor(), scalarType, scalarSize, numscalars);
  // get the appropriate function for pixel copying
  vtkGetSetPixelsFunc(&setpixels,
    scalarType, scalarSize, numscalars, outPtr);

  // Loop through output voxels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (threadId == 0)
        { // update the progress if this is the main thread
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      // clear the pixels to background color and go to next row
      setpixels(outPtr, background, numscalars, outExt[1]-outExt[0]+1);
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(&background);
}

//----------------------------------------------------------------------------
// application of the transform has different forms for fixed-point
// vs. floating-point
template<class F>
void vtkResliceApplyTransform(vtkAbstractTransform *newtrans,
                              F inPoint[3], F inOrigin[3],
                              F inInvSpacing[3])
{
  newtrans->InternalTransformPoint(inPoint, inPoint);
  inPoint[0] -= inOrigin[0];
  inPoint[1] -= inOrigin[1];
  inPoint[2] -= inOrigin[2];
  inPoint[0] *= inInvSpacing[0];
  inPoint[1] *= inInvSpacing[1];
  inPoint[2] *= inInvSpacing[2];
}

//----------------------------------------------------------------------------
// the main execute function
template<class F>
void vtkImageResliceExecute(vtkImageReslice *self,
                            vtkDataArray *scalars,
                            vtkAbstractImageInterpolator *interpolator,
                            vtkImageData *outData, void *outPtr,
                            vtkImageResliceConvertScalarsType convertScalars,
                            int outExt[6], int threadId, F newmat[4][4],
                            vtkAbstractTransform *newtrans)
{
  void (*convertpixels)(void *&out, const F *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  void (*composite)(F *in, int numscalars, int n);

  // for the progress meter
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // get the input stencil
  vtkImageStencilData *stencil = self->GetStencil();
  // get the output stencil
  vtkImageStencilData *outputStencil = 0;
  if (self->GetGenerateStencilOutput())
    {
    outputStencil = self->GetStencilOutput();
    }

  // multiple samples for thick slabs
  int nsamples = self->GetSlabNumberOfSlices();
  nsamples = ((nsamples > 1) ? nsamples : 1);

  // check for perspective transformation
  bool perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 ||
      newmat[3][2] != 0 || newmat[3][3] != 1)
    {
    perspective = 1;
    }

  // extra scalar info for nearest-neighbor optimization
  void *inPtr = scalars->GetVoidPointer(0);
  int inputScalarSize = scalars->GetDataTypeSize();
  int inputScalarType = scalars->GetDataType();
  int inComponents = interpolator->GetNumberOfComponents();
  int componentOffset = interpolator->GetComponentOffset();
  int borderMode = interpolator->GetBorderMode();
  int *inExt = interpolator->GetExtent();
  int *inWholeExt = interpolator->GetWholeExtent();
  vtkIdType inInc[3];
  inInc[0] = scalars->GetNumberOfComponents();
  inInc[1] = inInc[0]*(inExt[1] - inExt[0] + 1);
  inInc[2] = inInc[1]*(inExt[3] - inExt[2] + 1);
  vtkIdType fullSize = (inExt[1] - inExt[0] + 1);
  fullSize *= (inExt[3] - inExt[2] + 1);
  fullSize *= (inExt[5] - inExt[4] + 1);
  if (componentOffset > 0 && componentOffset + inComponents < inInc[0])
    {
    inPtr = static_cast<char *>(inPtr) + inputScalarSize*componentOffset;
    }

  int interpolationMode = VTK_INT_MAX;
  if (interpolator->IsA("vtkImageInterpolator"))
    {
    interpolationMode =
      static_cast<vtkImageInterpolator *>(interpolator)
        ->GetInterpolationMode();
    }

  // is nearest neighbor optimization possible?
  bool optimizeNearest = 0;
  if (interpolationMode == VTK_NEAREST_INTERPOLATION &&
      borderMode == VTK_IMAGE_BORDER_CLAMP &&
      !(newtrans || perspective || convertScalars) &&
      inputScalarType == outData->GetScalarType() &&
      fullSize == scalars->GetNumberOfTuples() &&
      self->GetBorder() == 1 && nsamples <= 1 &&
      inExt[0] >= inWholeExt[0] && inExt[1] <= inWholeExt[1] &&
      inExt[2] >= inWholeExt[2] && inExt[3] <= inWholeExt[3] &&
      inExt[4] >= inWholeExt[4] && inExt[5] <= inWholeExt[5])
    {
    optimizeNearest = 1;
    }

  // get Increments to march through data
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int outComponents = outData->GetNumberOfScalarComponents();

  // the floating point type used
  int floatType = vtkTypeTraits<F>::VTKTypeID();

  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  F xAxis[4], yAxis[4], zAxis[4], origin[4];
  for (int i = 0; i < 4; i++)
    {
    xAxis[i] = newmat[i][0];
    yAxis[i] = newmat[i][1];
    zAxis[i] = newmat[i][2];
    origin[i] = newmat[i][3];
    }

  // get the input origin and spacing for conversion purposes
  double temp[3];
  F inOrigin[3];
  interpolator->GetOrigin(temp);
  inOrigin[0] = F(temp[0]);
  inOrigin[1] = F(temp[1]);
  inOrigin[2] = F(temp[2]);

  F inInvSpacing[3];
  interpolator->GetSpacing(temp);
  inInvSpacing[0] = F(1.0/temp[0]);
  inInvSpacing[1] = F(1.0/temp[1]);
  inInvSpacing[2] = F(1.0/temp[2]);

  // allocate an output row of type double
  F *floatPtr = 0;
  if (!optimizeNearest)
    {
    floatPtr = new F [inComponents*(outExt[1] - outExt[0] + nsamples)];
    }

  // set color for area outside of input volume extent
  void *background;
  vtkAllocBackgroundPixel(&background,
     self->GetBackgroundColor(), scalarType, scalarSize, outComponents);

  // get various helper functions
  vtkGetConversionFunc(&convertpixels,
    floatType, scalarType, interpolationMode, self->GetSlabMode());
  vtkGetSetPixelsFunc(&setpixels,
    scalarType, scalarSize, outComponents, outPtr);
  vtkGetCompositeFunc(&composite,
    self->GetSlabMode(), self->GetSlabTrapezoidIntegration());

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    F inPoint0[4];
    inPoint0[0] = origin[0] + idZ*zAxis[0]; // incremental transform
    inPoint0[1] = origin[1] + idZ*zAxis[1];
    inPoint0[2] = origin[2] + idZ*zAxis[2];
    inPoint0[3] = origin[3] + idZ*zAxis[3];

    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      F inPoint1[4];
      inPoint1[0] = inPoint0[0] + idY*yAxis[0]; // incremental transform
      inPoint1[1] = inPoint0[1] + idY*yAxis[1];
      inPoint1[2] = inPoint0[2] + idY*yAxis[2];
      inPoint1[3] = inPoint0[3] + idY*yAxis[3];

      if (!threadId)
        {
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      int iter = 0;
      int idXmin, idXmax;
      while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                     outExt[0], outExt[1], idY, idZ,
                                     outPtr, background, outComponents,
                                     setpixels, iter))
        {
        if (!optimizeNearest)
          {
          bool wasInBounds = 1;
          bool isInBounds = 1;
          int startIdX = idXmin;
          int idX = idXmin;
          F *tmpPtr = floatPtr;

          while (startIdX <= idXmax)
            {
            for (; idX <= idXmax && isInBounds == wasInBounds; idX++)
              {
              F inPoint2[4];
              inPoint2[0] = inPoint1[0] + idX*xAxis[0];
              inPoint2[1] = inPoint1[1] + idX*xAxis[1];
              inPoint2[2] = inPoint1[2] + idX*xAxis[2];
              inPoint2[3] = inPoint1[3] + idX*xAxis[3];

              F *inPoint = inPoint2;
              isInBounds = 0;

              int sampleCount = 0;
              for (int sample = 0; sample < nsamples; sample++)
                {
                if (nsamples > 1)
                  {
                  double s = sample - 0.5*(nsamples - 1);
                  F inPoint3[4];
                  inPoint3[0] = inPoint2[0] + s*zAxis[0];
                  inPoint3[1] = inPoint2[1] + s*zAxis[1];
                  inPoint3[2] = inPoint2[2] + s*zAxis[2];
                  inPoint3[3] = inPoint2[3] + s*zAxis[3];
                  inPoint = inPoint3;
                  }

                if (perspective)
                  { // only do perspective if necessary
                  F f = 1/inPoint[3];
                  inPoint[0] *= f;
                  inPoint[1] *= f;
                  inPoint[2] *= f;
                  }

                if (newtrans)
                  { // apply the AbstractTransform if there is one
                  vtkResliceApplyTransform(newtrans, inPoint, inOrigin,
                                           inInvSpacing);
                  }

                if (interpolator->CheckBoundsIJK(inPoint))
                  {
                  // do the interpolation
                  sampleCount++;
                  isInBounds = 1;
                  interpolator->InterpolateIJK(inPoint, tmpPtr);
                  tmpPtr += inComponents;
                  }
                }

              tmpPtr -= sampleCount*inComponents;
              if (sampleCount > 1)
                {
                composite(tmpPtr, inComponents, sampleCount);
                }
              tmpPtr += inComponents;

              // set "was in" to "is in" if first pixel
              wasInBounds = ((idX > idXmin) ? wasInBounds : isInBounds);
              }

            // write a segment to the output
            int endIdX = idX - 1 - (isInBounds != wasInBounds);
            int numpixels = endIdX - startIdX + 1;

            if (wasInBounds)
              {
              if (outputStencil)
                {
                outputStencil->InsertNextExtent(startIdX, endIdX, idY, idZ);
                }

              if (convertScalars)
                {
                (self->*convertScalars)(tmpPtr - inComponents*(idX-startIdX),
                                        outPtr,
                                        vtkTypeTraits<F>::VTKTypeID(),
                                        inComponents, numpixels,
                                        startIdX, idY, idZ, threadId);

                outPtr = static_cast<void *>(static_cast<char *>(outPtr)
                           + numpixels*outComponents*scalarSize);
                }
              else
                {
                convertpixels(outPtr, tmpPtr - inComponents*(idX - startIdX),
                              outComponents, numpixels);
                }
              }
            else
              {
              setpixels(outPtr, background, outComponents, numpixels);
              }

            startIdX += numpixels;
            wasInBounds = isInBounds;
            }
          }
        else // optimize for nearest-neighbor interpolation
          {
          char *outPtrTmp = static_cast<char *>(outPtr);

          int inExtX = inExt[1] - inExt[0] + 1;
          int inExtY = inExt[3] - inExt[2] + 1;
          int inExtZ = inExt[5] - inExt[4] + 1;

          int startIdX = idXmin;
          int endIdX = idXmin-1;
          bool isInBounds = false;

          for (int iidX = idXmin; iidX <= idXmax; iidX++)
            {
            char *inPtrTmp = static_cast<char *>(background);
            int bytesPerPixel = inputScalarSize*inComponents;

            F inPoint[3];
            inPoint[0] = inPoint1[0] + iidX*xAxis[0];
            inPoint[1] = inPoint1[1] + iidX*xAxis[1];
            inPoint[2] = inPoint1[2] + iidX*xAxis[2];

            int inIdX = vtkInterpolationMath::Round(inPoint[0]) - inExt[0];
            int inIdY = vtkInterpolationMath::Round(inPoint[1]) - inExt[2];
            int inIdZ = vtkInterpolationMath::Round(inPoint[2]) - inExt[4];

            if ((inIdX >= 0) & (inIdX < inExtX) &
                (inIdY >= 0) & (inIdY < inExtY) &
                (inIdZ >= 0) & (inIdZ < inExtZ))
              {
              inPtrTmp = static_cast<char *>(inPtr) +
                (inIdX*inInc[0] + inIdY*inInc[1] + inIdZ*inInc[2])*
                  inputScalarSize;

              startIdX = (isInBounds ? startIdX : iidX);
              endIdX = iidX;
              isInBounds = true;
              }

            int oc = bytesPerPixel;
            do { *outPtrTmp++ = *inPtrTmp++; } while (--oc);
            }

          outPtr = outPtrTmp;

          if (outputStencil && endIdX >= startIdX)
            {
            outputStencil->InsertNextExtent(startIdX, endIdX, idY, idZ);
            }
          }
        }
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(&background);

  if (!optimizeNearest)
    {
    delete [] floatPtr;
    }
}

//----------------------------------------------------------------------------
// vtkReslicePermuteExecute is specifically optimized for
// cases where the IndexMatrix has only one non-zero component
// per row, i.e. when the matrix is permutation+scale+translation.
// All of the interpolation coefficients are calculated ahead
// of time instead of on a pixel-by-pixel basis.

namespace {

//----------------------------------------------------------------------------
// Optimized routines for nearest-neighbor interpolation

template <class T>
struct vtkImageResliceRowInterpolate
{
  static void Nearest(
    void *&outPtr0, int idX, int idY, int idZ, int, int n,
    vtkInterpolationWeights *weights);

  static void Nearest1(
    void *&outPtr0, int idX, int idY, int idZ, int, int n,
    vtkInterpolationWeights *weights);

  static void Nearest2(
    void *&outPtr0, int idX, int idY, int idZ, int, int n,
    vtkInterpolationWeights *weights);

  static void Nearest3(
    void *&outPtr0, int idX, int idY, int idZ, int, int n,
    vtkInterpolationWeights *weights);

  static void Nearest4(
    void *&outPtr0, int idX, int idY, int idZ, int, int n,
    vtkInterpolationWeights *weights);
};

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template<class T>
void vtkImageResliceRowInterpolate<T>::Nearest(
  void *&outPtr0, int idX, int idY, int idZ, int numscalars, int n,
  vtkInterpolationWeights *weights)
{
  const vtkIdType *iX = weights->Positions[0] + idX;
  const vtkIdType *iY = weights->Positions[1] + idY;
  const vtkIdType *iZ = weights->Positions[2] + idZ;
  const T *inPtr0 = static_cast<const T *>(weights->Pointer) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    int m = numscalars;
    do
      {
      *outPtr++ = *tmpPtr++;
      }
    while (--m);
    }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for 1 scalar component
template<class T>
void vtkImageResliceRowInterpolate<T>::Nearest1(
  void *&outPtr0, int idX, int idY, int idZ, int, int n,
  vtkInterpolationWeights *weights)
{
  const vtkIdType *iX = weights->Positions[0] + idX;
  const vtkIdType *iY = weights->Positions[1] + idY;
  const vtkIdType *iZ = weights->Positions[2] + idZ;
  const T *inPtr0 = static_cast<const T *>(weights->Pointer) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    *outPtr++ = *tmpPtr;
    }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for 2 scalar components
template<class T>
void vtkImageResliceRowInterpolate<T>::Nearest2(
  void *&outPtr0, int idX, int idY, int idZ, int, int n,
  vtkInterpolationWeights *weights)
{
  const vtkIdType *iX = weights->Positions[0] + idX;
  const vtkIdType *iY = weights->Positions[1] + idY;
  const vtkIdType *iZ = weights->Positions[2] + idZ;
  const T *inPtr0 = static_cast<const T *>(weights->Pointer) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    outPtr[0] = tmpPtr[0];
    outPtr[1] = tmpPtr[1];
    outPtr += 2;
    }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for 3 scalar components
template<class T>
void vtkImageResliceRowInterpolate<T>::Nearest3(
  void *&outPtr0, int idX, int idY, int idZ, int, int n,
  vtkInterpolationWeights *weights)
{
  const vtkIdType *iX = weights->Positions[0] + idX;
  const vtkIdType *iY = weights->Positions[1] + idY;
  const vtkIdType *iZ = weights->Positions[2] + idZ;
  const T *inPtr0 = static_cast<const T *>(weights->Pointer) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    outPtr[0] = tmpPtr[0];
    outPtr[1] = tmpPtr[1];
    outPtr[2] = tmpPtr[2];
    outPtr += 3;
    }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for 4 scalar components
template<class T>
void vtkImageResliceRowInterpolate<T>::Nearest4(
  void *&outPtr0, int idX, int idY, int idZ, int, int n,
  vtkInterpolationWeights *weights)
{
  const vtkIdType *iX = weights->Positions[0] + idX;
  const vtkIdType *iY = weights->Positions[1] + idY;
  const vtkIdType *iZ = weights->Positions[2] + idZ;
  const T *inPtr0 = static_cast<const T *>(weights->Pointer) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    outPtr[0] = tmpPtr[0];
    outPtr[1] = tmpPtr[1];
    outPtr[2] = tmpPtr[2];
    outPtr[3] = tmpPtr[3];
    outPtr += 4;
    }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
void vtkGetSummationFunc(
  void (**summation)(void *&outPtr, int idX, int idY, int idZ, int numscalars,
                     int n, vtkInterpolationWeights *weights),
  int scalarType, int numScalars)
{
  *summation = 0;

  if (numScalars == 1)
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        *summation = &(vtkImageResliceRowInterpolate<VTK_TT>::Nearest1)
        );
      default:
        *summation = 0;
      }
    }
  else if (numScalars == 2)
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        *summation = &(vtkImageResliceRowInterpolate<VTK_TT>::Nearest2)
        );
      default:
        *summation = 0;
      }
    }
  else if (numScalars == 3)
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        *summation = &(vtkImageResliceRowInterpolate<VTK_TT>::Nearest3)
        );
      default:
        *summation = 0;
      }
    }
  else if (numScalars == 4)
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        *summation = &(vtkImageResliceRowInterpolate<VTK_TT>::Nearest4)
        );
      default:
        *summation = 0;
      }
    }
  else
    {
    switch (scalarType)
      {
      vtkTemplateAliasMacro(
        *summation = &(vtkImageResliceRowInterpolate<VTK_TT>::Nearest)
        );
      default:
        *summation = 0;
      }
    }
}

//----------------------------------------------------------------------------
template<class F>
struct vtkImageResliceRowComp
{
  static void SumRow(F *op, const F *ip, int nc, int m, int i, int n);
  static void SumRowTrap(F *op, const F *ip, int nc, int m, int i, int n);
  static void MeanRow(F *op, const F *ip, int nc, int m, int i, int n);
  static void MeanRowTrap(F *op, const F *ip, int nc, int m, int i, int n);
  static void MinRow(F *op, const F *ip, int nc, int m, int i, int n);
  static void MaxRow(F *op, const F *ip, int nc, int m, int i, int n);
};

template<class F>
void vtkImageResliceRowComp<F>::SumRow(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = *inPtr++; } while (--m);
      }
    else
      {
      do { *outPtr++ += *inPtr++; } while (--m);
      }
    }
}

template<class F>
void vtkImageResliceRowComp<F>::SumRowTrap(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int n)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = 0.5*(*inPtr++); } while (--m);
      }
    else if (i == n-1)
      {
      do { *outPtr++ += 0.5*(*inPtr++); } while (--m);
      }
    else
      {
      do { *outPtr++ += *inPtr++; } while (--m);
      }
    }
}

template<class F>
void vtkImageResliceRowComp<F>::MeanRow(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int n)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = *inPtr++; } while (--m);
      }
    else if (i == n-1)
      {
      F f = F(1.0/n);
      do { *outPtr += *inPtr++; *outPtr *= f; outPtr++; } while (--m);
      }
    else
      {
      do { *outPtr++ += *inPtr++; } while (--m);
      }
    }
}

template<class F>
void vtkImageResliceRowComp<F>::MeanRowTrap(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int n)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = 0.5*(*inPtr++); } while (--m);
      }
    else if (i == n-1)
      {
      F f = F(1.0/(n-1));
      do { *outPtr += 0.5*(*inPtr++); *outPtr *= f; outPtr++; } while (--m);
      }
    else
      {
      do { *outPtr++ += *inPtr++; } while (--m);
      }
    }
}

template<class F>
void vtkImageResliceRowComp<F>::MinRow(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = *inPtr++; } while (--m);
      }
    else
      {
      do
        {
        *outPtr = ((*outPtr < *inPtr) ? *outPtr : *inPtr);
        outPtr++; inPtr++;
        }
      while (--m);
      }
    }
}

template<class F>
void vtkImageResliceRowComp<F>::MaxRow(
  F *outPtr, const F *inPtr, int numComp, int count, int i, int)
{
  int m = count*numComp;
  if (m)
    {
    if (i == 0)
      {
      do { *outPtr++ = *inPtr++; } while (--m);
      }
    else
      {
      do
        {
        *outPtr = ((*outPtr > *inPtr) ? *outPtr : *inPtr);
        outPtr++; inPtr++;
        }
      while (--m);
      }
    }
}

// get the composite function
template<class F>
void vtkGetRowCompositeFunc(
  void (**composite)(F *op, const F *ip, int nc, int count, int i, int n),
  int slabMode, int trpz)
{
  switch (slabMode)
    {
    case VTK_IMAGE_SLAB_MIN:
      *composite = &(vtkImageResliceRowComp<F>::MinRow);
      break;
    case VTK_IMAGE_SLAB_MAX:
      *composite = &(vtkImageResliceRowComp<F>::MaxRow);
      break;
    case VTK_IMAGE_SLAB_MEAN:
      if (trpz) { *composite = &(vtkImageResliceRowComp<F>::MeanRowTrap); }
      else { *composite = &(vtkImageResliceRowComp<F>::MeanRow); }
      break;
    case VTK_IMAGE_SLAB_SUM:
      if (trpz) { *composite = &(vtkImageResliceRowComp<F>::SumRowTrap); }
      else { *composite = &(vtkImageResliceRowComp<F>::SumRow); }
      break;
    default:
      *composite = 0;
    }
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// the ReslicePermuteExecute path is taken when the output slices are
// orthogonal to the input slices
template <class F>
void vtkReslicePermuteExecute(vtkImageReslice *self,
                              vtkDataArray *scalars,
                              vtkAbstractImageInterpolator *interpolator,
                              vtkImageData *outData, void *outPtr,
                              vtkImageResliceConvertScalarsType convertScalars,
                              int outExt[6], int threadId, F matrix[4][4])
{
  // Get Increments to march through data
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int outComponents = outData->GetNumberOfScalarComponents();

  // the floating point type used
  int floatType = vtkTypeTraits<F>::VTKTypeID();

  // slab mode
  int nsamples = self->GetSlabNumberOfSlices();
  nsamples = ((nsamples > 0) ? nsamples : 1);
  F (*newmat)[4];
  newmat = matrix;
  F smatrix[4][4];
  int *extent = outExt;
  int sextent[6];
  if (nsamples > 1)
    {
    F *tmpm1 = *matrix;
    F *tmpm2 = *smatrix;
    for (int ii = 0; ii < 16; ii++)
      {
      *tmpm2++ = *tmpm1++;
      }
    smatrix[0][3] -= 0.5*smatrix[0][2]*nsamples;
    smatrix[1][3] -= 0.5*smatrix[1][2]*nsamples;
    smatrix[2][3] -= 0.5*smatrix[2][2]*nsamples;
    newmat = smatrix;
    for (int jj = 0; jj < 6; jj++)
      {
      sextent[jj] = outExt[jj];
      }
    sextent[5] += nsamples-1;
    extent = sextent;
    }

  // get the input stencil
  vtkImageStencilData *stencil = self->GetStencil();
  // get the output stencil
  vtkImageStencilData *outputStencil = 0;
  if (self->GetGenerateStencilOutput())
    {
    outputStencil = self->GetStencilOutput();
    }

  // if doConversion is false, a special fast-path will be used
  int interpolationMode = self->GetInterpolationMode();
  bool doConversion = true;
  int inputScalarType = scalars->GetDataType();
  if (interpolationMode == VTK_NEAREST_INTERPOLATION &&
      interpolator->IsA("vtkImageInterpolator") &&
      inputScalarType == scalarType && !convertScalars && nsamples == 1)
    {
    doConversion = false;
    }

  // useful information from the interpolator
  int inComponents = interpolator->GetNumberOfComponents();

  // fill in the interpolation tables
  int clipExt[6];
  vtkInterpolationWeights *weights;
  interpolator->PrecomputeWeightsForExtent(
    *newmat, extent, clipExt, weights);

  // get type-specific functions
  void (*summation)(void *&out, int idX, int idY, int idZ, int numscalars,
                    int n, vtkInterpolationWeights *weights);
  void (*conversion)(void *&out, const F *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  vtkGetSummationFunc(&summation, scalarType, outComponents);
  vtkGetConversionFunc(&conversion,
    floatType, scalarType, interpolationMode, self->GetSlabMode());
  vtkGetSetPixelsFunc(&setpixels,
    scalarType, scalarSize, outComponents, outPtr);

  // get the slab compositing function
  void (*composite)(F *op, const F *ip, int nc, int count, int i, int n);
  vtkGetRowCompositeFunc(&composite,
    self->GetSlabMode(), self->GetSlabTrapezoidIntegration());

  // get temp float space for type conversion
  F *floatPtr = 0;
  F *floatSumPtr = 0;
  if (doConversion)
    {
    floatPtr = new F [inComponents*(outExt[1] - outExt[0] + 1)];
    }
  if (nsamples > 1)
    {
    floatSumPtr = new F [inComponents*(outExt[1] - outExt[0] + 1)];
    }

  // set color for area outside of input volume extent
  void *background;
  vtkAllocBackgroundPixel(&background,
     self->GetBackgroundColor(), scalarType, scalarSize, outComponents);

  // for tracking progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (threadId == 0)
        { // track progress if we are main thread
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      // do extent check
      if (idZ < clipExt[4]-(nsamples-1) || idZ > clipExt[5]+(nsamples-1) ||
          idY < clipExt[2] || idY > clipExt[3])
        { // just clear, we're completely outside
        setpixels(outPtr, background, outComponents, outExt[1]-outExt[0]+1);
        }
      else
        {
        // clear pixels to left of input extent
        setpixels(outPtr, background, outComponents, clipExt[0]-outExt[0]);

        int iter = 0;
        int idXmin, idXmax;
        while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                       clipExt[0], clipExt[1], idY, idZ,
                                       outPtr, background, outComponents,
                                       setpixels, iter))
          {
          int idX = idXmin;

          if (doConversion)
            {
            // these six lines are for handling incomplete slabs
            int lowerSkip = clipExt[4] - idZ;
            lowerSkip = (lowerSkip >= 0 ? lowerSkip : 0);
            int upperSkip = idZ + (nsamples-1) - clipExt[5];
            upperSkip = (upperSkip >= 0 ? upperSkip : 0);
            int idZ1 = idZ + lowerSkip;
            int nsamples1 = nsamples - lowerSkip - upperSkip;

            for (int isample = 0; isample < nsamples1; isample++)
              {
              F *tmpPtr = ((nsamples1 > 1) ? floatSumPtr : floatPtr);
              interpolator->InterpolateRow(
                weights, idX, idY, idZ1, tmpPtr, idXmax - idXmin + 1);

              if (nsamples1 > 1)
                {
                composite(floatPtr, floatSumPtr, inComponents,
                          idXmax - idXmin + 1, isample, nsamples1);
                }

              idZ1++;
              }

            if (convertScalars)
              {
              (self->*convertScalars)(floatPtr, outPtr,
                                      vtkTypeTraits<F>::VTKTypeID(),
                                      inComponents, idXmax - idXmin + 1,
                                      idXmin, idY, idZ, threadId);

              outPtr = static_cast<void *>(static_cast<char *>(outPtr)
                + (idXmax-idXmin+1)*outComponents*scalarSize);
              }
            else
              {
              conversion(outPtr, floatPtr, inComponents, idXmax - idXmin + 1);
              }
            }
          else
            {
            // fast path for when no conversion is necessary
            summation(outPtr, idX, idY, idZ, inComponents,
                      idXmax - idXmin + 1, weights);
            }

          if (outputStencil)
            {
            outputStencil->InsertNextExtent(idXmin, idXmax, idY, idZ);
            }
          }

        // clear pixels to right of input extent
        setpixels(outPtr, background, outComponents, outExt[1] - clipExt[1]);
        }

      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(&background);

  if (doConversion)
    {
    delete [] floatPtr;
    }
  if (nsamples > 1)
    {
    delete [] floatSumPtr;
    }

  interpolator->FreePrecomputedWeights(weights);
}

//----------------------------------------------------------------------------
} // end of anonymous namespace

//----------------------------------------------------------------------------
// The transform matrix supplied by the user converts output coordinates
// to input coordinates.
// To speed up the pixel lookup, the following function provides a
// matrix which converts output pixel indices to input pixel indices.
//
// This will also concatenate the ResliceAxes and the ResliceTransform
// if possible (if the ResliceTransform is a 4x4 matrix transform).
// If it does, this->OptimizedTransform will be set to NULL, otherwise
// this->OptimizedTransform will be equal to this->ResliceTransform.

vtkMatrix4x4 *vtkImageReslice::GetIndexMatrix(vtkInformation *inInfo,
                                              vtkInformation *outInfo)
{
  // first verify that we have to update the matrix
  if (this->IndexMatrix == NULL)
    {
    this->IndexMatrix = vtkMatrix4x4::New();
    }

  int isIdentity = 0;
  double inOrigin[3];
  double inSpacing[3];
  double outOrigin[3];
  double outSpacing[3];

  inInfo->Get(vtkDataObject::SPACING(), inSpacing);
  inInfo->Get(vtkDataObject::ORIGIN(), inOrigin);
  outInfo->Get(vtkDataObject::SPACING(), outSpacing);
  outInfo->Get(vtkDataObject::ORIGIN(), outOrigin);

  vtkTransform *transform = vtkTransform::New();
  vtkMatrix4x4 *inMatrix = vtkMatrix4x4::New();
  vtkMatrix4x4 *outMatrix = vtkMatrix4x4::New();

  if (this->OptimizedTransform)
    {
    this->OptimizedTransform->Delete();
    }
  this->OptimizedTransform = NULL;

  if (this->ResliceAxes)
    {
    transform->SetMatrix(this->GetResliceAxes());
    }
  if (this->ResliceTransform)
    {
    if (this->ResliceTransform->IsA("vtkHomogeneousTransform"))
      {
      transform->PostMultiply();
      transform->Concatenate(
        static_cast<vtkHomogeneousTransform *>(
          this->ResliceTransform)->GetMatrix());
      }
    else
      {
      this->ResliceTransform->Register(this);
      this->OptimizedTransform = this->ResliceTransform;
      }
    }

  // check to see if we have an identity matrix
  isIdentity = vtkIsIdentityMatrix(transform->GetMatrix());

  // the outMatrix takes OutputData indices to OutputData coordinates,
  // the inMatrix takes InputData coordinates to InputData indices
  for (int i = 0; i < 3; i++)
    {
    if ((this->OptimizedTransform == NULL &&
         (inSpacing[i] != outSpacing[i] || inOrigin[i] != outOrigin[i])) ||
        (this->OptimizedTransform != NULL &&
         (outSpacing[i] != 1.0 || outOrigin[i] != 0.0)))
      {
      isIdentity = 0;
      }
    inMatrix->Element[i][i] = 1.0/inSpacing[i];
    inMatrix->Element[i][3] = -inOrigin[i]/inSpacing[i];
    outMatrix->Element[i][i] = outSpacing[i];
    outMatrix->Element[i][3] = outOrigin[i];
    }
  outInfo->Get(vtkDataObject::ORIGIN(), outOrigin);

  if (!isIdentity)
    {
    transform->PreMultiply();
    transform->Concatenate(outMatrix);
    // the OptimizedTransform requires data coords, not
    // index coords, as its input
    if (this->OptimizedTransform == NULL)
      {
      transform->PostMultiply();
      transform->Concatenate(inMatrix);
      }
    }

  transform->GetMatrix(this->IndexMatrix);

  transform->Delete();
  inMatrix->Delete();
  outMatrix->Delete();

  return this->IndexMatrix;
}

//----------------------------------------------------------------------------
// RequestData is where the interpolator is updated, since it must be updated
// before the threads are split
int vtkImageReslice::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkAbstractImageInterpolator *interpolator = this->GetInterpolator();
  vtkInformation* info = inputVector[0]->GetInformationObject(0);
  interpolator->Initialize(info->Get(vtkDataObject::DATA_OBJECT()));

  int rval = this->Superclass::RequestData(request, inputVector, outputVector);

  interpolator->ReleaseData();

  return rval;
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageReslice::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int threadId)
{
  vtkDebugMacro(<< "Execute: inData = " << inData[0][0]
                      << ", outData = " << outData[0]);

  int inExt[6];
  inData[0][0]->GetExtent(inExt);
  // check for empty input extent
  if (inExt[1] < inExt[0] ||
      inExt[3] < inExt[2] ||
      inExt[5] < inExt[4])
    {
    return;
    }

  // Get the input scalars
  vtkDataArray *scalars = inData[0][0]->GetPointData()->GetScalars();

  // Get the output pointer
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // change transform matrix so that instead of taking
  // input coords -> output coords it takes output indices -> input indices
  vtkMatrix4x4 *matrix = this->IndexMatrix;

  // get the portion of the transformation that remains apart from
  // the IndexMatrix
  vtkAbstractTransform *newtrans = this->OptimizedTransform;

  vtkImageResliceFloatingPointType newmat[4][4];
  for (int i = 0; i < 4; i++)
    {
    newmat[i][0] = matrix->GetElement(i,0);
    newmat[i][1] = matrix->GetElement(i,1);
    newmat[i][2] = matrix->GetElement(i,2);
    newmat[i][3] = matrix->GetElement(i,3);
    }

  if (this->HitInputExtent == 0)
    {
    vtkImageResliceClearExecute(this, outData[0], outPtr, outExt, threadId);
    }
  else if (this->UsePermuteExecute)
    {
    vtkReslicePermuteExecute(this, scalars, this->Interpolator,
                             outData[0], outPtr,
                             (this->HasConvertScalars ?
                                &vtkImageReslice::ConvertScalarsBase : 0),
                             outExt, threadId, newmat);
    }
  else
    {
    vtkImageResliceExecute(this, scalars, this->Interpolator,
                           outData[0], outPtr,
                           (this->HasConvertScalars ?
                              &vtkImageReslice::ConvertScalarsBase : 0),
                           outExt, threadId, newmat, newtrans);
    }
}
