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
#include "vtkDataSetAttributes.h"
#include "vtkGarbageCollector.h"
#include "vtkTypeTraits.h"

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
vtkCxxSetObjectMacro(vtkImageReslice,ResliceTransform,vtkAbstractTransform);

//--------------------------------------------------------------------------
// DO NOT SET MAX KERNEL SIZE TO LARGER THAN 14
#define VTK_RESLICE_MAX_KERNEL_SIZE 14

//--------------------------------------------------------------------------
// typedef for pixel converter method
typedef void (vtkImageReslice::*vtkImageResliceConvertScalarsType)(
  void *outPtr, void *inPtr, int inputType, int inNumComponents,
  int count, int idX, int idY, int idZ, int threadId);

// typedef for the floating point type used by the code
typedef double vtkImageResliceFloatingPointType;

//--------------------------------------------------------------------------
// The 'floor' function is slow, so we want to do an integer
// cast but keep the "floor" behavior of always rounding down,
// rather than truncating, i.e. we want -0.6 to become -1.
// The easiest way to do this is to add a large value in
// order to make the value "unsigned", then cast to int, and
// then subtract off the large value.

// On the old i386 architecture, even a cast to int is very
// expensive because it requires changing the rounding mode
// on the FPU.  So we use a bit-trick similar to the one
// described at http://www.stereopsis.com/FPU.html

#if defined ia64 || defined __ia64__ || defined _M_IA64
#define VTK_RESLICE_64BIT_FLOOR
#elif defined __ppc64__ || defined __x86_64__ || defined _M_X64
#define VTK_RESLICE_64BIT_FLOOR
#elif defined __ppc__ || defined sparc || defined mips
#define VTK_RESLICE_32BIT_FLOOR
#elif defined i386 || defined _M_IX86
#define VTK_RESLICE_I386_FLOOR
#endif

// We add a tolerance of 2^-17 (around 7.6e-6) so that float
// values that are just less than the closest integer are
// rounded up.  This adds robustness against rounding errors.

#define VTK_RESLICE_FLOOR_TOL 7.62939453125e-06


template<class F>
inline int vtkResliceFloor(double x, F &f)
{
#if defined VTK_RESLICE_64BIT_FLOOR
  x += (103079215104.0 + VTK_RESLICE_FLOOR_TOL);
#ifdef VTK_TYPE_USE___INT64
  __int64 i = static_cast<__int64>(x);
  f = x - i;
  return static_cast<int>(i - 103079215104i64);
#else
  long long i = static_cast<long long>(x);
  f = x - i;
  return static_cast<int>(i - 103079215104LL);
#endif
#elif defined VTK_RESLICE_32BIT_FLOOR
  x += (2147483648.0 + VTK_RESLICE_FLOOR_TOL);
  unsigned int i = static_cast<unsigned int>(x);
  f = x - i;
  return static_cast<int>(i - 2147483648U);
#elif defined VTK_RESLICE_I386_FLOOR
  union { double d; unsigned short s[4]; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.0;  // (2**(52-16))*1.5
  f = dual.s[0]*0.0000152587890625; // 2**(-16)
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#else
  int i = vtkMath::Floor(x + VTK_RESLICE_FLOOR_TOL);
  f = x - i;
  return i;
#endif
}


inline int vtkResliceRound(double x)
{
#if defined VTK_RESLICE_64BIT_FLOOR
  x += (103079215104.5 + VTK_RESLICE_FLOOR_TOL);
#ifdef VTK_TYPE_USE___INT64
  __int64 i = static_cast<__int64>(x);
  return static_cast<int>(i - 103079215104i64);
#else
  long long i = static_cast<long long>(x);
  return static_cast<int>(i - 103079215104LL);
#endif
#elif defined VTK_RESLICE_32BIT_FLOOR
  x += (2147483648.5 + VTK_RESLICE_FLOOR_TOL);
  unsigned int i = static_cast<unsigned int>(x);
  return static_cast<int>(i - 2147483648U);
#elif defined VTK_RESLICE_I386_FLOOR
  union { double d; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.5;  // (2**(52-16))*1.5
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#else
  return vtkMath::Floor(x + (0.5 + VTK_RESLICE_FLOOR_TOL));
#endif
}

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
  this->InterpolationSizeParameter = 3; // for Lanczos and Kaiser

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

  // cache a matrix that converts output voxel indices -> input voxel indices
  this->IndexMatrix = NULL;
  this->OptimizedTransform = NULL;

  // set to zero when we completely missed the input extent
  this->HitInputExtent = 1;

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
  os << indent << "InterpolationSizeParameter: " << this->InterpolationSizeParameter << "\n";
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
    case VTK_RESLICE_RESERVED_2:
      return "Reserved";
    case VTK_RESLICE_CUBIC:
      return "Cubic";
    case VTK_RESLICE_LANCZOS:
      return "Lanczos";
    case VTK_RESLICE_KAISER:
      return "Kaiser";
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

  double *inOrigin = inInfo->Get(vtkDataObject::ORIGIN());
  double *inSpacing = inInfo->Get(vtkDataObject::SPACING());
  double *outOrigin = outInfo->Get(vtkDataObject::ORIGIN());
  double *outSpacing = outInfo->Get(vtkDataObject::SPACING());

  double xAxis[4], yAxis[4], zAxis[4], origin[4];
  double inInvSpacing[3];

  inInvSpacing[0] = 0.0;
  inInvSpacing[1] = 0.0;
  inInvSpacing[2] = 0.0;

  if (this->Optimization)
    {
    vtkMatrix4x4 *matrix = this->GetIndexMatrix(inInfo, outInfo);

    // convert matrix from world coordinates to pixel indices
    for (int i = 0; i < 4; i++)
      {
      xAxis[i] = matrix->GetElement(i,0);
      yAxis[i] = matrix->GetElement(i,1);
      zAxis[i] = matrix->GetElement(i,2);
      origin[i] = matrix->GetElement(i,3);
      }
    }
  else
    {
    // save effor later: invert inSpacing
    inInvSpacing[0] = 1.0/inSpacing[0];
    inInvSpacing[1] = 1.0/inSpacing[1];
    inInvSpacing[2] = 1.0/inSpacing[2];
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

  // check the coordinates of the 8 corners of the output extent
  // (this must be done exactly the same as the calculation in
  // vtkImageResliceExecute)
  for (int jj = 0; jj < 8; jj++)
    {
    // get output coords
    int idX = outExt[jj%2];
    int idY = outExt[2+(jj/2)%2];
    int idZ = outExt[4+(jj/4)%2];

    double point[4];

    if (this->Optimization)
      {
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
      }
    else
      {
      point[0] = idX*outSpacing[0] + outOrigin[0];
      point[1] = idY*outSpacing[1] + outOrigin[1];
      point[2] = idZ*outSpacing[2] + outOrigin[2];
      point[3] = 1.0;

      if (this->ResliceAxes)
        {
        this->ResliceAxes->MultiplyPoint(point, point);
        double f = 1.0/point[3];
        point[0] *= f;
        point[1] *= f;
        point[2] *= f;
        }
      if (this->ResliceTransform)
        {
        this->ResliceTransform->TransformPoint(point, point);
        }

      point[0] = (point[0] - inOrigin[0])*inInvSpacing[0];
      point[1] = (point[1] - inOrigin[1])*inInvSpacing[1];
      point[2] = (point[2] - inOrigin[2])*inInvSpacing[2];
      }

    // set the extent appropriately according to the interpolation mode
    int interpolationMode = this->GetInterpolationMode();
    if (interpolationMode != VTK_RESLICE_NEAREST)
      {
      int extra = 0;
      switch (interpolationMode)
        {
        case VTK_RESLICE_CUBIC:
          extra = 1;
          break;
        case VTK_RESLICE_LANCZOS:
        case VTK_RESLICE_KAISER:
          extra = this->GetInterpolationSizeParameter() - 1;
          break;
        }

      for (int j = 0; j < 3; j++)
        {
        double f;
        int k = vtkResliceFloor(point[j], f);
        if (f == 0)
          {
          if (k < inExt[2*j])
            {
            inExt[2*j] = k;
            }
          if (k > inExt[2*j+1])
            {
            inExt[2*j+1] = k;
            }
          }
        else
          {
          if (k - extra < inExt[2*j])
            {
            inExt[2*j] = k - extra;
            }
          if (k + 1 + extra > inExt[2*j+1])
            {
            inExt[2*j+1] = k + 1 + extra;
            }
          }
        }
      }
    else
      {
      for (int j = 0; j < 3; j++)
        {
        int k = vtkResliceRound(point[j]);
        if (k < inExt[2*j])
          {
          inExt[2*j] = k;
          }
        if (k > inExt[2*j+1])
          {
          inExt[2*j+1] = k;
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
      outWholeExt[2*i] = vtkResliceRound(e);
      outWholeExt[2*i+1] = vtkResliceRound(outWholeExt[2*i] +
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

  if (this->HasConvertScalars)
    {
    int scalarType = VTK_DOUBLE;
    int numComponents = 1;

    vtkInformation *inScalarInfo =
      vtkDataObject::GetActiveFieldInformation(inInfo,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);

    if (inScalarInfo)
      {
      if (inScalarInfo->Has(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
        {
        numComponents =
          inScalarInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
        }
      scalarType = inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
      }

    this->ConvertScalarInfo(scalarType, numComponents);

    vtkDataObject::SetPointDataActiveScalarInfo(
        outInfo, scalarType, numComponents);
    }
  else
    {
    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, this->OutputScalarType, -1);
    }

  this->GetIndexMatrix(inInfo, outInfo);

  this->BuildInterpolationTables();

  return 1;
}

//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code
//----------------------------------------------------------------------------

// These interpolation functions are supported: NearestNeighbor, Trilinear,
// Tricubic, and windowed-sinc interpolation with Lanczos and Kaiser.
// The result of the interpolation is put in *outPtr, and outPtr is
// incremented.

//----------------------------------------------------------------------------
template<class F, class T>
struct vtkImageResliceInterpolate
{
  static void NearestNeighbor(
    F *outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int mode);

  static void Trilinear(
    F *outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int mode);

  static void Tricubic(
    F *outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int mode);

  static void General(
    F *outPtr, const void *inPtr, const int inExt[6],
    const vtkIdType inInc[3], int numscalars,
    const F point[3], int mode);
};

//----------------------------------------------------------------------------
// constants for different boundary-handling modes

#define VTK_RESLICE_MODE_MASK 0x000f   // the interpolation modes
#define VTK_RESLICE_WRAP_MASK 0x0030   // the border handling modes
#define VTK_RESLICE_CLAMP     0x0010   // clamp to bounds of image
#define VTK_RESLICE_REPEAT    0x0020   // wrap to opposite side of image
#define VTK_RESLICE_MIRROR    0x0030   // mirror off of the boundary
#define VTK_RESLICE_N_MASK    0x0f00   // one less than kernel size
#define VTK_RESLICE_N_SHIFT   8        // position of size info
#define VTK_RESLICE_X_NEAREST 0x1000   // don't interpolate in x (hint)
#define VTK_RESLICE_Y_NEAREST 0x2000   // don't interpolate in y (hint)
#define VTK_RESLICE_Z_NEAREST 0x4000   // don't interpolate in z (hint)

static int vtkResliceGetMode(vtkImageReslice *self)
{
  int mode = self->GetInterpolationMode();

  if (self->GetMirror())
    {
    mode |= VTK_RESLICE_MIRROR;
    }
  else if (self->GetWrap())
    {
    mode |= VTK_RESLICE_REPEAT;
    }
  else
    {
    mode |= VTK_RESLICE_CLAMP;
    }

  // n is the kernel size subtract one, where the kernel size
  // must be an even number not larger than eight
  int n = 1;
  switch (mode & VTK_RESLICE_MODE_MASK)
    {
    case VTK_RESLICE_NEAREST:
      n = 1;
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      n = 2;
      break;
    case VTK_RESLICE_CUBIC:
      n = 4;
      break;
    case VTK_RESLICE_LANCZOS:
    case VTK_RESLICE_KAISER:
      n = 2*self->GetInterpolationSizeParameter();
      break;
    }

  mode |= ((n - 1) << VTK_RESLICE_N_SHIFT);

  return mode;
}

//----------------------------------------------------------------------------
// rounding functions for each type, where 'F' is a floating-point type

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt8& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt8& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt16& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt16& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeInt32& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeUInt32& rnd)
{
  rnd = vtkResliceRound(val);
}
#endif

#if (VTK_USE_FLOAT32 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeFloat32& rnd)
{
  rnd = val;
}
#endif

#if (VTK_USE_FLOAT64 != 0)
template <class F>
inline void vtkResliceRound(F val, vtkTypeFloat64& rnd)
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
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt8& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(255.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt16& clamp)
{
  static F minval = static_cast<F>(-32768.0);
  static F maxval = static_cast<F>(32767.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt16& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(65535.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt32& clamp)
{
  static F minval = static_cast<F>(-2147483648.0);
  static F maxval = static_cast<F>(2147483647.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt32& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(4294967295.0);
  val = vtkResliceClamp(val, minval, maxval);
  vtkResliceRound(val,clamp);
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
  void *&outPtrV, const F *inPtr, int numscalars, int n)
{
  if (n > 0)
    {
    // This is a very hot loop, so it is unrolled
    T* outPtr = static_cast<T*>(outPtrV);
    int m = n*numscalars;
    for (int q = m >> 2; q > 0; --q)
      {
      vtkResliceRound(inPtr[0], outPtr[0]);
      vtkResliceRound(inPtr[1], outPtr[1]);
      vtkResliceRound(inPtr[2], outPtr[2]);
      vtkResliceRound(inPtr[3], outPtr[3]);
      inPtr += 4;
      outPtr += 4;
      }
    for (int r = m & 0x0003; r > 0; --r)
      {
      vtkResliceRound(*inPtr++, *outPtr++);
      }
    outPtrV = outPtr;
    }
}

template <class F, class T>
void vtkImageResliceConversion<F, T>::Clamp(
  void *&outPtrV, const F *inPtr, int numscalars, int n)
{
  T* outPtr = static_cast<T*>(outPtrV);
  for (int m = n*numscalars; m > 0; --m)
    {
    vtkResliceClamp(*inPtr++, *outPtr++);
    }
  outPtrV = outPtr;
}

// get the convertion function
template<class F>
void vtkGetConversionFunc(vtkImageReslice *self,
                         void (**conversion)(void *&out, const F *in,
                                             int numscalars, int n))
{
  vtkImageData *input = static_cast<vtkImageData *>(self->GetInput());
  int inputType = input->GetScalarType();
  int dataType = self->GetOutput()->GetScalarType();

  if (self->GetInterpolationMode() <= VTK_RESLICE_LINEAR &&
      self->GetSlabMode() != VTK_IMAGE_SLAB_SUM &&
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
void vtkGetCompositeFunc(vtkImageReslice *self,
                         void (**composite)(F *in, int numscalars, int n))
{
  int slabMode = self->GetSlabMode();
  int trpz = self->GetSlabTrapezoidIntegration();

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
// Perform a clamp to limit an index to [b, c] and subtract b.

inline int vtkInterpolateClamp(int a, int b, int c)
{
  a = (a <= c ? a : c);
  a -= b;
  a = (a >= 0 ? a : 0);
  return a;
}

//----------------------------------------------------------------------------
// Perform a wrap to limit an index to [b, c] and subtract b.

inline int vtkInterpolateWrap(int a, int b, int c)
{
  int range = c - b + 1;
  a -= b;
  a %= range;
  // required for some % implementations
  a = (a >= 0 ? a : a + range);
  return a;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [b, c] and subtract b.

inline int vtkInterpolateMirror(int a, int b, int c)
{
  int range1 = c - b;
  int range = range1 + 1;
  a -= b;
  a = (a >= 0 ? a : -a - 1);
  int count = a/range;
  a -= count*range;
  a = ((count & 0x1) == 0 ? a : range1 - a);
  return a;
}


//----------------------------------------------------------------------------
// Do nearest-neighbor interpolation of the input data 'inPtr' of extent
// 'inExt' at the 'point'.  The result is placed at 'outPtr'.
// If the lookup data is beyond the extent 'inExt', return 0,
// otherwise advance outPtr by numscalars.
template <class F, class T>
void vtkImageResliceInterpolate<F, T>::NearestNeighbor(
  F *outPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int mode)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);

  int inIdX0 = vtkResliceRound(point[0]);
  int inIdY0 = vtkResliceRound(point[1]);
  int inIdZ0 = vtkResliceRound(point[2]);

  switch (mode & VTK_RESLICE_WRAP_MASK)
    {
    case VTK_RESLICE_REPEAT:
      inIdX0 = vtkInterpolateWrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExt[4], inExt[5]);
      break;

    case VTK_RESLICE_MIRROR:
      inIdX0 = vtkInterpolateMirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = vtkInterpolateClamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateClamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateClamp(inIdZ0, inExt[4], inExt[5]);
      break;
    }

  inPtr += inIdX0*inInc[0]+inIdY0*inInc[1]+inIdZ0*inInc[2];
  do
    {
    *outPtr++ = *inPtr++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Do trilinear interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.
// If the lookup data is beyond the extent 'inExt', return 0,
// otherwise advance outPtr by numscalars.
template <class F, class T>
void vtkImageResliceInterpolate<F, T>::Trilinear(
  F *outPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int mode)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);

  F fx, fy, fz;
  int inIdX0 = vtkResliceFloor(point[0], fx);
  int inIdY0 = vtkResliceFloor(point[1], fy);
  int inIdZ0 = vtkResliceFloor(point[2], fz);

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  switch (mode & VTK_RESLICE_WRAP_MASK)
    {
    case VTK_RESLICE_REPEAT:
      inIdX0 = vtkInterpolateWrap(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolateWrap(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolateWrap(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolateWrap(inIdZ1, inExt[4], inExt[5]);
      break;

    case VTK_RESLICE_MIRROR:
      inIdX0 = vtkInterpolateMirror(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolateMirror(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolateMirror(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolateMirror(inIdZ1, inExt[4], inExt[5]);
      break;

    default:
      inIdX0 = vtkInterpolateClamp(inIdX0, inExt[0], inExt[1]);
      inIdY0 = vtkInterpolateClamp(inIdY0, inExt[2], inExt[3]);
      inIdZ0 = vtkInterpolateClamp(inIdZ0, inExt[4], inExt[5]);

      inIdX1 = vtkInterpolateClamp(inIdX1, inExt[0], inExt[1]);
      inIdY1 = vtkInterpolateClamp(inIdY1, inExt[2], inExt[3]);
      inIdZ1 = vtkInterpolateClamp(inIdZ1, inExt[4], inExt[5]);
      break;
    }

  vtkIdType factX0 = inIdX0*inInc[0];
  vtkIdType factX1 = inIdX1*inInc[0];
  vtkIdType factY0 = inIdY0*inInc[1];
  vtkIdType factY1 = inIdY1*inInc[1];
  vtkIdType factZ0 = inIdZ0*inInc[2];
  vtkIdType factZ1 = inIdZ1*inInc[2];

  vtkIdType i00 = factY0 + factZ0;
  vtkIdType i01 = factY0 + factZ1;
  vtkIdType i10 = factY1 + factZ0;
  vtkIdType i11 = factY1 + factZ1;

  F rx = 1 - fx;
  F ry = 1 - fy;
  F rz = 1 - fz;

  F ryrz = ry*rz;
  F fyrz = fy*rz;
  F ryfz = ry*fz;
  F fyfz = fy*fz;

  const T *inPtr0 = inPtr + factX0;
  const T *inPtr1 = inPtr + factX1;

  do
    {
    *outPtr++ = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                     fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                 fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                     fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));
    inPtr0++;
    inPtr1++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Do tricubic interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.
// If the lookup data is beyond the extent 'inExt', return 0,
// otherwise advance outPtr by numscalars.

// helper function: set up the lookup indices and the interpolation
// coefficients

template <class T>
void vtkTricubicInterpWeights(T F[4], int l, int h, T f)
{
  static const T half = T(0.5);

  if (l*h == 1)
    { // no interpolation
    F[0] = 0;
    F[1] = 1;
    F[2] = 0;
    F[3] = 0;
    return;
    }

  // cubic interpolation
  T fm1 = f - 1;
  T fd2 = f*half;
  T ft3 = f*3;
  F[0] = -fd2*fm1*fm1;
  F[1] = ((ft3 - 2)*fd2 - 1)*fm1;
  F[2] = -((ft3 - 4)*f - 1)*fd2;
  F[3] = f*fd2*fm1;

  if (h - l == 3)
    {
    return;
    }

  // if we are at an edge, extrapolate: edge pixel repeats

  if (l == 1)
    {
    F[1] += F[0];
    F[0] = 0;
    }
  if (l == 2)
    {
    F[2] += F[1];
    F[1] = 0;
    }

  if (h == 2)
    {
    F[2] += F[3];
    F[3] = 0;
    }
  if (h == 1)
    {
    F[1] += F[2];
    F[2] = 0;
    }
}

// tricubic interpolation
template <class F, class T>
void vtkImageResliceInterpolate<F, T>::Tricubic(
  F *outPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int mode)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);

  F fx, fy, fz;
  int inIdX0 = vtkResliceFloor(point[0], fx);
  int inIdY0 = vtkResliceFloor(point[1], fy);
  int inIdZ0 = vtkResliceFloor(point[2], fz);

  // change arrays into locals
  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  int minX = inExt[0];
  int maxX = inExt[1];
  int minY = inExt[2];
  int maxY = inExt[3];
  int minZ = inExt[4];
  int maxZ = inExt[5];

  // the memory offsets
  vtkIdType factX[4], factY[4], factZ[4];

  switch (mode & VTK_RESLICE_WRAP_MASK)
    {
    case VTK_RESLICE_REPEAT:
      factX[0] = vtkInterpolateWrap(inIdX0-1, minX, maxX)*inIncX;
      factX[1] = vtkInterpolateWrap(inIdX0,   minX, maxX)*inIncX;
      factX[2] = vtkInterpolateWrap(inIdX0+1, minX, maxX)*inIncX;
      factX[3] = vtkInterpolateWrap(inIdX0+2, minX, maxX)*inIncX;

      factY[0] = vtkInterpolateWrap(inIdY0-1, minY, maxY)*inIncY;
      factY[1] = vtkInterpolateWrap(inIdY0,   minY, maxY)*inIncY;
      factY[2] = vtkInterpolateWrap(inIdY0+1, minY, maxY)*inIncY;
      factY[3] = vtkInterpolateWrap(inIdY0+2, minY, maxY)*inIncY;

      factZ[0] = vtkInterpolateWrap(inIdZ0-1, minZ, maxZ)*inIncZ;
      factZ[1] = vtkInterpolateWrap(inIdZ0,   minZ, maxZ)*inIncZ;
      factZ[2] = vtkInterpolateWrap(inIdZ0+1, minZ, maxZ)*inIncZ;
      factZ[3] = vtkInterpolateWrap(inIdZ0+2, minZ, maxZ)*inIncZ;
      break;

    case VTK_RESLICE_MIRROR:
      factX[0] = vtkInterpolateMirror(inIdX0-1, minX, maxX)*inIncX;
      factX[1] = vtkInterpolateMirror(inIdX0,   minX, maxX)*inIncX;
      factX[2] = vtkInterpolateMirror(inIdX0+1, minX, maxX)*inIncX;
      factX[3] = vtkInterpolateMirror(inIdX0+2, minX, maxX)*inIncX;

      factY[0] = vtkInterpolateMirror(inIdY0-1, minY, maxY)*inIncY;
      factY[1] = vtkInterpolateMirror(inIdY0,   minY, maxY)*inIncY;
      factY[2] = vtkInterpolateMirror(inIdY0+1, minY, maxY)*inIncY;
      factY[3] = vtkInterpolateMirror(inIdY0+2, minY, maxY)*inIncY;

      factZ[0] = vtkInterpolateMirror(inIdZ0-1, minZ, maxZ)*inIncZ;
      factZ[1] = vtkInterpolateMirror(inIdZ0,   minZ, maxZ)*inIncZ;
      factZ[2] = vtkInterpolateMirror(inIdZ0+1, minZ, maxZ)*inIncZ;
      factZ[3] = vtkInterpolateMirror(inIdZ0+2, minZ, maxZ)*inIncZ;
      break;

    default:
      factX[0] = vtkInterpolateClamp(inIdX0-1, minX, maxX)*inIncX;
      factX[1] = vtkInterpolateClamp(inIdX0,   minX, maxX)*inIncX;
      factX[2] = vtkInterpolateClamp(inIdX0+1, minX, maxX)*inIncX;
      factX[3] = vtkInterpolateClamp(inIdX0+2, minX, maxX)*inIncX;

      factY[0] = vtkInterpolateClamp(inIdY0-1, minY, maxY)*inIncY;
      factY[1] = vtkInterpolateClamp(inIdY0,   minY, maxY)*inIncY;
      factY[2] = vtkInterpolateClamp(inIdY0+1, minY, maxY)*inIncY;
      factY[3] = vtkInterpolateClamp(inIdY0+2, minY, maxY)*inIncY;

      factZ[0] = vtkInterpolateClamp(inIdZ0-1, minZ, maxZ)*inIncZ;
      factZ[1] = vtkInterpolateClamp(inIdZ0,   minZ, maxZ)*inIncZ;
      factZ[2] = vtkInterpolateClamp(inIdZ0+1, minZ, maxZ)*inIncZ;
      factZ[3] = vtkInterpolateClamp(inIdZ0+2, minZ, maxZ)*inIncZ;
      break;
    }

  // check if only one slice in a particular direction
  int multipleX = (minX != maxX);
  int multipleY = (minY != maxY);
  int multipleZ = (minZ != maxZ);

  // if not b-spline, can use an even better rule
  if ((mode & VTK_RESLICE_MODE_MASK) == VTK_RESLICE_CUBIC)
    {
    multipleX &= (fx != 0);
    multipleY &= (fy != 0);
    multipleZ &= (fz != 0);
    }

  // the limits to use when doing the interpolation
  int i1 = 1 - multipleX;
  int i2 = 1 + 2*multipleX;

  int j1 = 1 - multipleY;
  int j2 = 1 + 2*multipleY;

  int k1 = 1 - multipleZ;
  int k2 = 1 + 2*multipleZ;

  // get the interpolation coefficients
  F fX[4], fY[4], fZ[4];
  vtkTricubicInterpWeights(fX, i1, i2, fx);
  vtkTricubicInterpWeights(fY, j1, j2, fy);
  vtkTricubicInterpWeights(fZ, k1, k2, fz);

  do // loop over components
    {
    F val = 0;
    int k = k1;
    do // loop over z
      {
      F ifz = fZ[k];
      vtkIdType factz = factZ[k];
      int j = j1;
      do // loop over y
        {
        F ify = fY[j];
        F fzy = ifz*ify;
        vtkIdType factzy = factz + factY[j];
        const T *tmpPtr = inPtr + factzy;
        // loop over x is unrolled (significant performance boost)
        val += fzy*(fX[0]*tmpPtr[factX[0]] +
                    fX[1]*tmpPtr[factX[1]] +
                    fX[2]*tmpPtr[factX[2]] +
                    fX[3]*tmpPtr[factX[3]]);
        }
      while (++j <= j2);
      }
    while (++k <= k2);

    *outPtr++ = val;
    inPtr++;
    }
  while (--numscalars);
}

//----------------------------------------------------------------------------
// Methods to support windowed sinc interpolators

// sinc(x) from 0 to 8 with 256 bins per unit x
#define VTK_SINC_TABLE_SIZE ((VTK_RESLICE_MAX_KERNEL_SIZE + 2)*128 + 4)
static float vtkSincTable256[VTK_SINC_TABLE_SIZE];

static void vtkBuildSincTable256()
{
  static int built = 0;

  if (built == 0)
    {
    vtkSincTable256[0] = 1.0;
    double p = vtkMath::DoublePi();
    double f = p/256.0;
    for (int i = 1; i < VTK_SINC_TABLE_SIZE; i++)
      {
      double x = i*f;
      vtkSincTable256[i] = sin(x)/x;
      }
    built = 1;
    }
}

template<class T>
T vtkSinc256(T x)
{
  // linear interpolation of sinc function
  T y = fabs(x);
  int i = static_cast<int>(y);
  T f = y - i;
  return (1 - f)*vtkSincTable256[i] + f*vtkSincTable256[i+1];
}

template<class T>
void vtkLanczosInterpWeights(T *F, T f, int m)
{
  // The table is only big enough for n=7
  if (m <= VTK_RESLICE_MAX_KERNEL_SIZE)
    {
    const T p = T(256); // table bins per unit
    int n = (m >> 1);
    T pn = p/n;
    T g = 1 - n - f;
    T x = p*g;
    T y = pn*g;
    T s = 0;
    int i = 0;
    do
      {
      T z = vtkSinc256(y)*vtkSinc256(x);
      s += z;
      F[i] = z;
      x += p;
      y += pn;
      }
    while (++i < m);

    // normalize
    s = 1/s;
    do
      {
      F[0] *= s;
      F[1] *= s;
      F += 2;
      }
    while (--n > 0);
    }
}

//----------------------------------------------------------------------------
// Compute the modified bessel function I0
static double vtkBesselI0(double x)
{
  int m = 0;
  double x2 = 0.25*x*x;
  double p = 1;
  double b = 1;
  do
    {
    m++;
    p *= x2/(m*m);
    b += p;
    }
  while (p > b*VTK_DBL_EPSILON);

  return b;
}

#define VTK_BESSEL_TABLE_SIZE ((VTK_RESLICE_MAX_KERNEL_SIZE + 2)*144 + 4)
static float vtkBesselTable96[VTK_BESSEL_TABLE_SIZE];

static void vtkBuildBesselTable96()
{
  static int built = 0;

  if (built == 0)
    {
    for (int i = 0; i < VTK_BESSEL_TABLE_SIZE; i++)
      {
      vtkBesselTable96[i] = vtkBesselI0(i/96.0);
      }
    built = 1;
    }
}

template<class T>
T vtkBessel96(T x)
{
  // linear interpolation of bessel from the table
  int i = static_cast<int>(x);
  T f = x - i;
  return (1 - f)*vtkBesselTable96[i] + f*vtkBesselTable96[i+1];
}

template<class T>
void vtkKaiserInterpWeights(T *F, T f, int m)
{
  if (m <= VTK_RESLICE_MAX_KERNEL_SIZE)
    {
    // The Kaiser window has a tunable parameter "alpha", where
    // a smaller alpha increases sharpness (and ringing) while a
    // larger alpha can cause blurring.  I set the alpha to 3*n,
    // which closely approximates the optimal alpha values shown in
    // Helwig Hauser, Eduard Groller, Thomas Theussl,
    // "Mastering Windows: Improving Reconstruction,"
    // IEEE Symposium on Volume Visualization and Graphics (VV 2000),
    // pp. 101-108, 2000
    int n = (m >> 1);
    T a = 3*n;
    T q = 1.0/vtkBessel96(a*96);
    T g = 1.0/(n*n);
    T x = 1 - n - f;
    T s = 0;
    int i = 0;
    do
      {
      T y = (1 - x*x*g);
      y *= (y > 0);
      T z = q*vtkBessel96(a*sqrt(y)*96)*vtkSinc256(x*256);
      s += z;
      F[i] = z;
      x++;
      }
    while (++i < m);

    // normalize
    s = 1/s;
    do
      {
      F[0] *= s;
      F[1] *= s;
      F += 2;
      }
    while (--n > 0);
    }
}


// General interpolation for high-order kernels.
// Requirements: kernel size must be even.

template <class F, class T>
void vtkImageResliceInterpolate<F, T>::General(
  F *outPtr, const void *inVoidPtr, const int inExt[6],
  const vtkIdType inInc[3], int numscalars, const F point[3],
  int mode)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  // size of kernel
  int m = ((mode & VTK_RESLICE_N_MASK) >> VTK_RESLICE_N_SHIFT) + 1;
  // index to kernel midpoint position
  int m2 = ((m - 1) >> 1);

  F fx, fy, fz;
  int inIdX0 = vtkResliceFloor(point[0], fx);
  int inIdY0 = vtkResliceFloor(point[1], fy);
  int inIdZ0 = vtkResliceFloor(point[2], fz);

  // change arrays into locals
  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  int minX = inExt[0];
  int maxX = inExt[1];
  int minY = inExt[2];
  int maxY = inExt[3];
  int minZ = inExt[4];
  int maxZ = inExt[5];

  // the memory offsets
  vtkIdType factX[VTK_RESLICE_MAX_KERNEL_SIZE];
  vtkIdType factY[VTK_RESLICE_MAX_KERNEL_SIZE];
  vtkIdType factZ[VTK_RESLICE_MAX_KERNEL_SIZE];

  switch (mode & VTK_RESLICE_WRAP_MASK)
    {
    case VTK_RESLICE_REPEAT:
      {
      int i = inIdX0 - m2;
      int j = inIdY0 - m2;
      int k = inIdZ0 - m2;
      for (int l = 0; l < m; i++, j++, k++, l++)
        {
        factX[l] = vtkInterpolateWrap(i, minX, maxX)*inIncX;
        factY[l] = vtkInterpolateWrap(j, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolateWrap(k, minZ, maxZ)*inIncZ;
        }
      }
      break;

    case VTK_RESLICE_MIRROR:
      {
      int i = inIdX0 - m2;
      int j = inIdY0 - m2;
      int k = inIdZ0 - m2;
      for (int l = 0; l < m; i++, j++, k++, l++)
        {
        factX[l] = vtkInterpolateMirror(i, minX, maxX)*inIncX;
        factY[l] = vtkInterpolateMirror(j, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolateMirror(k, minZ, maxZ)*inIncZ;
        }
      }
      break;

    default:
      {
      int i = inIdX0 - m2;
      int j = inIdY0 - m2;
      int k = inIdZ0 - m2;
      for (int l = 0; l < m; i++, j++, k++, l++)
        {
        factX[l] = vtkInterpolateClamp(i, minX, maxX)*inIncX;
        factY[l] = vtkInterpolateClamp(j, minY, maxY)*inIncY;
        factZ[l] = vtkInterpolateClamp(k, minZ, maxZ)*inIncZ;
        }
      }
      break;
    }

  // several high order kernels could be supported here
  F fX[VTK_RESLICE_MAX_KERNEL_SIZE];
  F fY[VTK_RESLICE_MAX_KERNEL_SIZE];
  F fZ[VTK_RESLICE_MAX_KERNEL_SIZE];
  switch (mode & VTK_RESLICE_MODE_MASK)
    {
    case VTK_RESLICE_LANCZOS:
      vtkLanczosInterpWeights(fX, fx, m);
      vtkLanczosInterpWeights(fY, fy, m);
      vtkLanczosInterpWeights(fZ, fz, m);
      break;
    case VTK_RESLICE_KAISER:
      vtkKaiserInterpWeights(fX, fx, m);
      vtkKaiserInterpWeights(fY, fy, m);
      vtkKaiserInterpWeights(fZ, fz, m);
      break;
    }

  // check if only one slice in a particular direction
  int multipleY = (minY != maxY);
  int multipleZ = (minZ != maxZ);

  // the limits to use when doing the interpolation
  int k1 = m2*(1 - multipleZ);
  int k2 = (m2 + 1)*(multipleZ + 1) - 1;
  int j1 = m2*(1 - multipleY);
  int j2 = (m2 + 1)*(multipleY + 1) - 1;

  do // loop over components
    {
    F val = 0;
    int k = k1;
    do // loop over z
      {
      F ifz = fZ[k];
      vtkIdType factz = factZ[k];
      int j = j1;
      do // loop over y
        {
        F ify = fY[j];
        F fzy = ifz*ify;
        vtkIdType factzy = factz + factY[j];
        // loop over x
        const T *tmpPtr = inPtr + factzy;
        const F *tmpfX = fX;
        const vtkIdType *tmpfactX = factX;
        F tmpval = 0;
        int l = m;
        do
          {
          tmpval += (*tmpfX++)*tmpPtr[*tmpfactX++];
          }
        while (--l > 0);
        val += fzy*tmpval;
        }
      while (++j <= j2);
      }
    while (++k <= k2);

    *outPtr++ = val;
    inPtr++;
    }
  while (--numscalars);
}

//--------------------------------------------------------------------------
// get appropriate interpolation function according to interpolation mode
// and scalar type
template <class F>
void vtkGetResliceInterpFunc(vtkImageReslice *self,
                             void (**interpolate)(F *outPtr,
                                                 const void *inPtr,
                                                 const int inExt[6],
                                                 const vtkIdType inInc[3],
                                                 int numscalars,
                                                 const F point[3],
                                                 int mode))
{
  vtkImageData *input = static_cast<vtkImageData *>(self->GetInput());
  int dataType = input->GetScalarType();
  int interpolationMode = self->GetInterpolationMode();

  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageResliceInterpolate<F, VTK_TT>::NearestNeighbor)
          );
        default:
          *interpolate = 0;
        }
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageResliceInterpolate<F, VTK_TT>::Trilinear)
          );
        default:
          *interpolate = 0;
        }
      break;
    case VTK_RESLICE_CUBIC:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageResliceInterpolate<F, VTK_TT>::Tricubic)
          );
        default:
          *interpolate = 0;
        }
      break;
    default:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *interpolate =
            &(vtkImageResliceInterpolate<F, VTK_TT>::General)
          );
        default:
          *interpolate = 0;
        }
      break;
    }
}

//----------------------------------------------------------------------------
// build any tables required for the interpolation
void vtkImageReslice::BuildInterpolationTables()
{
  switch (this->GetInterpolationMode())
    {
    case VTK_RESLICE_LANCZOS:
      vtkBuildSincTable256();
      break;
    case VTK_RESLICE_KAISER:
      vtkBuildSincTable256();
      vtkBuildBesselTable96();
      break;
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
void vtkGetSetPixelsFunc(vtkImageReslice *self,
                         void (**setpixels)(void *&out, const void *in,
                                            int numscalars, int n))
{
  vtkImageData *output = self->GetOutput();
  int dataType = output->GetScalarType();
  int dataSize = output->GetScalarSize();
  int numscalars = output->GetNumberOfScalarComponents();
  void *dataPtr = output->GetScalarPointer();

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
void vtkCopyBackgroundColor(vtkImageReslice *self,
                            T *background, int numComponents)
{
  for (int i = 0; i < numComponents; i++)
    {
    if (i < 4)
      {
      vtkResliceClamp(self->GetBackgroundColor()[i], background[i]);
      }
    else
      {
      background[i] = 0;
      }
    }
}

void vtkAllocBackgroundPixel(vtkImageReslice *self, void **rval,
                             int numComponents)
{
  vtkImageData *output = self->GetOutput();
  int scalarType = output->GetScalarType();
  int bytesPerPixel = numComponents*output->GetScalarSize();

  // allocate as an array of doubles to guarantee alignment
  // (this is probably more paranoid than necessary)
  int n = (bytesPerPixel + VTK_SIZEOF_DOUBLE - 1)/VTK_SIZEOF_DOUBLE;
  double *doublePtr = new double[n];
  *rval = doublePtr;

  switch (scalarType)
    {
    vtkTemplateAliasMacro(vtkCopyBackgroundColor(self, (VTK_TT *)(*rval),
                                                 numComponents));
    }
}

void vtkFreeBackgroundPixel(vtkImageReslice *vtkNotUsed(self), void **rval)
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
// Get the bounds for checking points before interpolation
template<class F>
void vtkResliceGetStructuredBounds(
  vtkImageReslice *self, const int extent[6], F bounds[6])
{
  if (self->GetWrap() || self->GetMirror())
    {
    // use int limits subtract half the kernel size
    int extra = 0;

    switch (self->GetInterpolationMode())
      {
      case VTK_RESLICE_CUBIC:
        extra = 1;
        break;
      case VTK_RESLICE_LANCZOS:
      case VTK_RESLICE_KAISER:
        extra = self->GetInterpolationSizeParameter() - 1;
        break;
      }

    for (int i = 0; i < 6; i += 2)
      {
      bounds[i] = VTK_INT_MIN + extra;
      bounds[i+1] = VTK_INT_MAX - extra;
      }
    }
  else
    {
    // use extent plus border
    F border = static_cast<F>(0.5*self->GetBorder());
    for (int i = 0; i < 6; i += 2)
      {
      F b = border;
      if (b == 0)
        {
        // border is at least a tolerance value of 2^-17
        b = static_cast<F>(VTK_RESLICE_FLOOR_TOL);
        // automatic border of 0.5 if limited dimensionality
        b = (extent[i] < extent[i+1] ? b : static_cast<F>(0.5));
        }
      bounds[i] = extent[i] - b;
      bounds[i+1] = extent[i+1] + b;
      }
    }
}

//----------------------------------------------------------------------------
// This function simply clears the entire output to the background color,
// for cases where the transformation places the output extent completely
// outside of the input extent.
void vtkImageResliceClearExecute(vtkImageReslice *self,
                                 vtkImageData *, void *,
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
  int scalarSize = outData->GetScalarSize();
  int numscalars = outData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  void *background;
  vtkAllocBackgroundPixel(self, &background, numscalars);
  // get the appropriate function for pixel copying
  vtkGetSetPixelsFunc(self, &setpixels);

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

  vtkFreeBackgroundPixel(self, &background);
}

//----------------------------------------------------------------------------
// This function executes the filter for any type of data.  It is much simpler
// in structure than vtkImageResliceOptimizedExecute.
void vtkImageResliceExecute(vtkImageReslice *self,
                            vtkImageData *inData, void *inPtr,
                            vtkImageData *outData, void *outPtr,
                            vtkImageResliceConvertScalarsType convertScalars,
                            int outExt[6], int threadId)
{
  // type-specific functions
  void (*interpolate)(double *outPtr, const void *inPtr,
                      const int inExt[6], const vtkIdType inInc[3],
                      int numscalars, const double point[3], int mode);
  void (*convertpixels)(void *&out, const double *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // for the progress meter
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // the 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = vtkResliceGetMode(self);

  // the transformation to apply to the data
  vtkAbstractTransform *transform = self->GetResliceTransform();
  vtkMatrix4x4 *matrix = self->GetResliceAxes();

  // for conversion to data coordinates
  double *inOrigin = inData->GetOrigin();
  double *inSpacing = inData->GetSpacing();
  double *outOrigin = outData->GetOrigin();
  double *outSpacing = outData->GetSpacing();

  // save effor later: invert inSpacing
  double inInvSpacing[3];
  inInvSpacing[0] = 1.0/inSpacing[0];
  inInvSpacing[1] = 1.0/inSpacing[1];
  inInvSpacing[2] = 1.0/inSpacing[2];

  // find maximum input range
  int inExt[6];
  inData->GetExtent(inExt);

  // Get Increments to march through data
  vtkIdType inInc[3];
  inData->GetIncrements(inInc);
  int scalarSize = outData->GetScalarSize();
  int outComponents = outData->GetNumberOfScalarComponents();
  int inComponents = inData->GetNumberOfScalarComponents();

  // allocate an output row of type double
  double *floatPtr = new double [inComponents*(outExt[1] - outExt[0] + 1)];

  // allocate a voxel to copy into the background (out-of-bounds) regions
  void *background;
  vtkAllocBackgroundPixel(self, &background, outComponents);

  // get the appropriate functions for interpolation and pixel copying
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);
  vtkGetConversionFunc(self, &convertpixels);

  // get the input stencil
  vtkImageStencilData *stencil = self->GetStencil();
  // get the output stencil
  vtkImageStencilData *outputStencil = 0;
  if (self->GetGenerateStencilOutput())
    {
    outputStencil = self->GetStencilOutput();
    }

  // compute the bounds in structured coords
  double bounds[6];
  vtkResliceGetStructuredBounds(self, inExt, bounds);

  // Loop through output voxels
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (threadId == 0)
        { // update the progress if this is the main thread
        if (!(count%target))
          {
          self->UpdateProgress(count/(target + 1.0));
          }
        count++;
        }

      int iter = 0; // if there is a stencil, it is applied here
      int idXmin, idXmax;
      while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                     outExt[0], outExt[1], idY, idZ,
                                     outPtr, background, outComponents,
                                     setpixels, iter))
        {
        bool wasInBounds = 1;
        bool isInBounds = 1;
        int startIdX = idXmin;
        int idX = idXmin;
        double *tmpPtr = floatPtr;

        while (startIdX <= idXmax)
          {
          for (; idX <= idXmax && isInBounds == wasInBounds; idX++)
            {
            // convert to data coordinates
            double point[4];
            point[0] = idX*outSpacing[0] + outOrigin[0];
            point[1] = idY*outSpacing[1] + outOrigin[1];
            point[2] = idZ*outSpacing[2] + outOrigin[2];
            point[3] = 1.0;

            // apply ResliceAxes matrix
            if (matrix)
              {
              matrix->MultiplyPoint(point, point);
              double f = 1.0/point[3];
              point[0] *= f;
              point[1] *= f;
              point[2] *= f;
              }

            // apply ResliceTransform
            if (transform)
              {
              transform->InternalTransformPoint(point, point);
              }

            // convert back to voxel indices
            point[0] = (point[0] - inOrigin[0])*inInvSpacing[0];
            point[1] = (point[1] - inOrigin[1])*inInvSpacing[1];
            point[2] = (point[2] - inOrigin[2])*inInvSpacing[2];

            isInBounds = 0;
            if (point[0] >= bounds[0] && point[0] <= bounds[1] &&
                point[1] >= bounds[2] && point[1] <= bounds[3] &&
                point[2] >= bounds[4] && point[2] <= bounds[5])
              {
              // do the interpolation
              isInBounds = 1;
              interpolate(tmpPtr, inPtr, inExt, inInc, inComponents,
                          point, mode);
              }

            tmpPtr += inComponents;
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
              (self->*convertScalars)(tmpPtr - inComponents*(idX - startIdX),
                                      outPtr, VTK_DOUBLE, inComponents,
                                      numpixels, startIdX, idY, idZ, threadId);

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

      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);

  delete [] floatPtr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// The remainder of this file is the 'optimized' version of the code.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// application of the transform has different forms for fixed-point
// vs. floating-point
template<class F>
inline
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

// The vtkOptimizedExecute() is like vtkImageResliceExecute, except that
// it provides a few optimizations:
// 1) the ResliceAxes and ResliceTransform are joined to create a
// single 4x4 matrix if possible
// 2) the transformation is calculated incrementally to increase efficiency
// 3) nearest-neighbor interpolation is treated specially in order to
// increase efficiency

template <class F>
void vtkOptimizedExecute(vtkImageReslice *self,
                         vtkImageData *inData, void *inPtr,
                         vtkImageData *outData, void *outPtr,
                         vtkImageResliceConvertScalarsType convertScalars,
                         int outExt[6], int threadId, F newmat[4][4],
                         vtkAbstractTransform *newtrans)
{
  void (*interpolate)(F *outPtr, const void *inPtr,
                      const int inExt[6], const vtkIdType inInc[3],
                      int numscalars, const F point[3], int mode);
  void (*convertpixels)(void *&out, const F *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  void (*composite)(F *in, int numscalars, int n);

  // for the progress meter
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  int nsamples = self->GetSlabNumberOfSlices();
  nsamples = ((nsamples > 1) ? nsamples : 1);

  int mode = vtkResliceGetMode(self);
  bool wrap = (self->GetWrap() || self->GetMirror());

  bool perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 ||
      newmat[3][2] != 0 || newmat[3][3] != 1)
    {
    perspective = 1;
    }

  bool optimizeNearest = 0;
  if (self->GetInterpolationMode() == VTK_RESLICE_NEAREST &&
      !(wrap || newtrans || perspective || convertScalars) &&
      inData->GetScalarType() == outData->GetScalarType() &&
      self->GetBorder() == 1 && nsamples <= 1)
    {
    optimizeNearest = 1;
    }

  // find maximum input range
  int inExt[6];
  inData->GetExtent(inExt);

  // Get Increments to march through data
  vtkIdType inInc[3];
  inData->GetIncrements(inInc);
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarSize = outData->GetScalarSize();
  int inputScalarSize = inData->GetScalarSize();
  int inComponents = inData->GetNumberOfScalarComponents();
  int outComponents = outData->GetNumberOfScalarComponents();

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
  inData->GetOrigin(temp);
  inOrigin[0] = F(temp[0]);
  inOrigin[1] = F(temp[1]);
  inOrigin[2] = F(temp[2]);

  F inInvSpacing[3];
  inData->GetSpacing(temp);
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
  vtkAllocBackgroundPixel(self, &background, outComponents);

  // Set interpolation method
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetConversionFunc(self, &convertpixels);
  vtkGetSetPixelsFunc(self, &setpixels);
  vtkGetCompositeFunc(self, &composite);

  // get the input
  vtkImageStencilData *stencil = self->GetStencil();
  // get the output stencil
  vtkImageStencilData *outputStencil = 0;
  if (self->GetGenerateStencilOutput())
    {
    outputStencil = self->GetStencilOutput();
    }

  // compute the bounds in structured coords
  F bounds[6];
  vtkResliceGetStructuredBounds(self, inExt, bounds);

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

                if ((inPoint[0] >= bounds[0]) & (inPoint[0] <= bounds[1]) &
                    (inPoint[1] >= bounds[2]) & (inPoint[1] <= bounds[3]) &
                    (inPoint[2] >= bounds[4]) & (inPoint[2] <= bounds[5]))
                  {
                  // do the interpolation
                  sampleCount++;
                  isInBounds = 1;
                  interpolate(tmpPtr, inPtr, inExt, inInc, inComponents,
                              inPoint, mode);
                  tmpPtr += inComponents;
                  }
                }

              tmpPtr -= sampleCount*inComponents;
              if (sampleCount > 1)
                {
                composite(tmpPtr, inComponents, sampleCount);
                }
              tmpPtr += inComponents;
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

          for (int iidX = idXmin; iidX <= idXmax; iidX++)
            {
            char *inPtrTmp = static_cast<char *>(background);
            int bytesPerPixel = inputScalarSize*inComponents;

            F inPoint[3];
            inPoint[0] = inPoint1[0] + iidX*xAxis[0];
            inPoint[1] = inPoint1[1] + iidX*xAxis[1];
            inPoint[2] = inPoint1[2] + iidX*xAxis[2];

            int inIdX = vtkResliceRound(inPoint[0]) - inExt[0];
            int inIdY = vtkResliceRound(inPoint[1]) - inExt[2];
            int inIdZ = vtkResliceRound(inPoint[2]) - inExt[4];

            if ((inIdX >= 0) & (inIdX < inExtX) &
                (inIdY >= 0) & (inIdY < inExtY) &
                (inIdZ >= 0) & (inIdZ < inExtZ))
              {
              inPtrTmp = static_cast<char *>(inPtr) +
                (inIdX*inInc[0] + inIdY*inInc[1] + inIdZ*inInc[2])*
                  inputScalarSize;
              }

            int oc = bytesPerPixel;
            do { *outPtrTmp++ = *inPtrTmp++; } while (--oc);
            }
          outPtr = outPtrTmp;
          }
        }
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);

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

// For nearest neighbor, the interpolation is further optimized
// for 1-component, 3-component, and 4-component scalars.

template <class F, class T>
struct vtkImageResliceSummation
{
  static void NearestNeighbor(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void NearestNeighbor1(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void NearestNeighbor2(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void NearestNeighbor3(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void NearestNeighbor4(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void Trilinear(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void Tricubic(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);

  static void General(
    void *&outPtr, const void *inPtr, int numscalars, int n, int mode,
    const vtkIdType *iX, const F *fX, const vtkIdType *iY, const F *fY,
    const vtkIdType *iZ, const F *fZ);
};

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template<class F, class T>
void vtkImageResliceSummation<F, T>::NearestNeighbor(
                                void *&outVoidPtr, const void *inVoidPtr,
                                int numscalars, int n, int vtkNotUsed(mode),
                                const vtkIdType *iX, const F *,
                                const vtkIdType *iY, const F *,
                                const vtkIdType *iZ, const F *)
{
  const T *inPtr0 = static_cast<const T *>(inVoidPtr) + iY[0] + iZ[0];
  F *outPtr = static_cast<F *>(outVoidPtr);

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
  outVoidPtr = outPtr;
}

// ditto, but optimized for numscalars = 1
template<class F, class T>
void vtkImageResliceSummation<F, T>::NearestNeighbor1(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int, int n, int vtkNotUsed(mode),
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *)
{
  const T *inPtr0 = static_cast<const T *>(inVoidPtr) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outVoidPtr);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
    {
    *outPtr++ = inPtr0[iX[0]];
    iX++;
    }
  outVoidPtr = outPtr;
}

// ditto, but optimized for numscalars = 2
template<class F, class T>
void vtkImageResliceSummation<F, T>::NearestNeighbor2(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int, int n, int vtkNotUsed(mode),
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *)
{
  const T *inPtr0 = static_cast<const T *>(inVoidPtr) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outVoidPtr);

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
  outVoidPtr = outPtr;
}

// ditto, but optimized for numscalars = 3
template<class F, class T>
void vtkImageResliceSummation<F, T>::NearestNeighbor3(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int, int n, int vtkNotUsed(mode),
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *)
{
  const T *inPtr0 = static_cast<const T *>(inVoidPtr) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outVoidPtr);

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
  outVoidPtr = outPtr;
}

// ditto, but optimized for numscalars = 4
template<class F, class T>
void vtkImageResliceSummation<F, T>::NearestNeighbor4(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int, int n, int vtkNotUsed(mode),
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *)
{
  const T *inPtr0 = static_cast<const T *>(inVoidPtr) + iY[0] + iZ[0];
  T *outPtr = static_cast<T *>(outVoidPtr);

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
  outVoidPtr = outPtr;
}

//----------------------------------------------------------------------------
// helper function for linear interpolation
template<class F, class T>
void vtkImageResliceSummation<F, T>::Trilinear(
                                  void *&outVoidPtr, const void *inVoidPtr,
                                  int numscalars, int n, int mode,
                                  const vtkIdType *iX, const F *fX,
                                  const vtkIdType *iY, const F *fY,
                                  const vtkIdType *iZ, const F *fZ)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = static_cast<F *>(outVoidPtr);

  vtkIdType i00 = iY[0] + iZ[0];
  vtkIdType i01 = iY[0] + iZ[1];
  vtkIdType i10 = iY[1] + iZ[0];
  vtkIdType i11 = iY[1] + iZ[1];

  F ry = fY[0];
  F fy = fY[1];
  F rz = fZ[0];
  F fz = fZ[1];

  F ryrz = ry*rz;
  F ryfz = ry*fz;
  F fyrz = fy*rz;
  F fyfz = fy*fz;

  if ((mode & VTK_RESLICE_X_NEAREST) != 0 && fy == 0 && fz == 0)
    { // no interpolation needed at all
    for (int i = n; i > 0; --i)
      {
      vtkIdType t0 = iX[0];
      iX += 2;

      const T *inPtr0 = inPtr + i00 + t0;
      int m = numscalars;
      do
        {
        *outPtr++ = *inPtr0++;
        }
      while (--m);
      }
    }
  else if ((mode & VTK_RESLICE_X_NEAREST) != 0 && fy == 0)
    { // only need linear z interpolation
    for (int i = n; i > 0; --i)
      {
      vtkIdType t0 = iX[0];
      iX += 2;

      const T *inPtr0 = inPtr + t0;
      int m = numscalars;
      do
        {
        *outPtr++ = (rz*inPtr0[i00] + fz*inPtr0[i01]);
        inPtr0++;
        }
      while (--m);
      }
    }
  else if (fz == 0)
    { // bilinear interpolation in x,y
    for (int i = n; i > 0; --i)
      {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      vtkIdType t0 = iX[0];
      vtkIdType t1 = iX[1];
      iX += 2;

      const T *inPtr0 = inPtr + t0;
      const T *inPtr1 = inPtr + t1;
      int m = numscalars;
      do
        {
        *outPtr++ = (rx*(ry*inPtr0[i00] + fy*inPtr0[i10]) +
                     fx*(ry*inPtr1[i00] + fy*inPtr1[i10]));
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
  else
    { // do full trilinear interpolation
    for (int i = n; i > 0; --i)
      {
      F rx = fX[0];
      F fx = fX[1];
      fX += 2;

      vtkIdType t0 = iX[0];
      vtkIdType t1 = iX[1];
      iX += 2;

      const T *inPtr0 = inPtr + t0;
      const T *inPtr1 = inPtr + t1;
      int m = numscalars;
      do
        {
        *outPtr++ = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                         fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                     fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                         fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
  outVoidPtr = outPtr;
}

//--------------------------------------------------------------------------
// helper function for tricubic interpolation
template<class F, class T>
void vtkImageResliceSummation<F, T>::Tricubic(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int numscalars, int n, int mode,
                                 const vtkIdType *iX, const F *fX,
                                 const vtkIdType *iY, const F *fY,
                                 const vtkIdType *iZ, const F *fZ)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = static_cast<F *>(outVoidPtr);

  // speed things up a bit for bicubic interpolation
  int k1 = 0;
  int k2 = 3;
  if ((mode & VTK_RESLICE_Z_NEAREST) != 0)
    {
    k1 = k2 = 1;
    }

  for (int i = n; i > 0; --i)
    {
    vtkIdType iX0 = iX[0];
    vtkIdType iX1 = iX[1];
    vtkIdType iX2 = iX[2];
    vtkIdType iX3 = iX[3];
    iX += 4;

    F fX0 = fX[0];
    F fX1 = fX[1];
    F fX2 = fX[2];
    F fX3 = fX[3];
    fX += 4;

    const T *inPtr0 = inPtr;
    int c = numscalars;
    do
      { // loop over components
      F result = 0;

      int k = k1;
      do
        { // loop over z
        F fz = fZ[k];
        if (fz != 0)
          {
          vtkIdType iz = iZ[k];
          int j = 0;
          do
            { // loop over y
            F fy = fY[j];
            F fzy = fz*fy;
            vtkIdType izy = iz + iY[j];
            const T *tmpPtr = inPtr0 + izy;
            // loop over x is unrolled (significant performance boost)
            result += fzy*(fX0*tmpPtr[iX0] +
                           fX1*tmpPtr[iX1] +
                           fX2*tmpPtr[iX2] +
                           fX3*tmpPtr[iX3]);
            }
          while (++j <= 3);
          }
        }
      while (++k <= k2);

      *outPtr++ = result;
      inPtr0++;
      }
    while (--c);
    }
  outVoidPtr = outPtr;
}

//--------------------------------------------------------------------------
// helper function for high-order interpolation
template<class F, class T>
void vtkImageResliceSummation<F, T>::General(
                                 void *&outVoidPtr, const void *inVoidPtr,
                                 int numscalars, int n, int mode,
                                 const vtkIdType *factX, const F *fX,
                                 const vtkIdType *factY, const F *fY,
                                 const vtkIdType *factZ, const F *fZ)
{
  const T *inPtr = static_cast<const T *>(inVoidPtr);
  F *outPtr = static_cast<F *>(outVoidPtr);

  int m = ((mode & VTK_RESLICE_N_MASK) >> VTK_RESLICE_N_SHIFT) + 1;
  int m2 = ((m - 1) >> 1);

  // speed things up a bit for 2D interpolation
  int k1 = 0;
  int k2 = m-1;
  if ((mode & VTK_RESLICE_Z_NEAREST) != 0)
    {
    k1 = k2 = m2;
    }

  for (int i = n; i > 0; --i)
    {
    const T *inPtr0 = inPtr;
    int c = numscalars;
    do // loop over components
      {
      F val = 0;
      int k = k1;
      do // loop over z
        {
        F ifz = fZ[k];
        vtkIdType factz = factZ[k];
        int j = 0;
        do // loop over y
          {
          F ify = fY[j];
          F fzy = ifz*ify;
          vtkIdType factzy = factz + factY[j];
          // loop over x
          const T *tmpPtr = inPtr0 + factzy;
          const F *tmpfX = fX;
          const vtkIdType *tmpfactX = factX;
          F tmpval = 0;
          int l = m;
          do
            {
            tmpval += (*tmpfX++)*tmpPtr[*tmpfactX++];
            }
          while (--l > 0);
          val += fzy*tmpval;
          }
        while (++j < m);
        }
      while (++k <= k2);

      *outPtr++ = val;
      inPtr0++;
      }
    while (--c);

    factX += m;
    fX += m;
    }
  outVoidPtr = outPtr;
}

//----------------------------------------------------------------------------
// get appropriate summation function for different interpolation modes
// and different scalar types
template<class F>
void vtkGetResliceSummationFunc(vtkImageReslice *self,
  void (**summation)(void *&out, const void *in,
                     int numscalars, int n, int mode,
                     const vtkIdType *iX, const F *fX,
                     const vtkIdType *iY, const F *fY,
                     const vtkIdType *iZ, const F *fZ),
  int interpolationMode, int doConversion)
{
  vtkImageData *input = static_cast<vtkImageData *>(self->GetInput());
  int scalarType = input->GetScalarType();
  int numScalars = input->GetNumberOfScalarComponents();

  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      if (numScalars == 1 && !doConversion)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(
            *summation = &(vtkImageResliceSummation<F,VTK_TT>::NearestNeighbor1)
            );
          default:
            *summation = 0;
          }
        }
      else if (numScalars == 2 && !doConversion)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(
            *summation = &(vtkImageResliceSummation<F,VTK_TT>::NearestNeighbor2)
            );
          default:
            *summation = 0;
          }
        }
      else if (numScalars == 3 && !doConversion)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(
            *summation = &(vtkImageResliceSummation<F,VTK_TT>::NearestNeighbor3)
            );
          default:
            *summation = 0;
          }
        }
      else if (numScalars == 4 && !doConversion)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(
            *summation = &(vtkImageResliceSummation<F,VTK_TT>::NearestNeighbor4)
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
            *summation = &(vtkImageResliceSummation<F,VTK_TT>::NearestNeighbor)
            );
          default:
            *summation = 0;
          }
        }
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      switch (scalarType)
        {
        vtkTemplateAliasMacro(
          *summation = &(vtkImageResliceSummation<F,VTK_TT>::Trilinear)
          );
        default:
          *summation = 0;
        }
      break;
    case VTK_RESLICE_CUBIC:
      switch (scalarType)
        {
        vtkTemplateAliasMacro(
          *summation = &(vtkImageResliceSummation<F,VTK_TT>::Tricubic)
          );
        default:
          *summation = 0;
        }
      break;
    default:
      switch (scalarType)
        {
        vtkTemplateAliasMacro(
          *summation = &(vtkImageResliceSummation<F,VTK_TT>::General)
          );
        default:
          *summation = 0;
        }
      break;
    }
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteNearestTable(const int outExt[6], const int inExt[6],
                            const vtkIdType inInc[3], int clipExt[6],
                            vtkIdType **traversal, F **,
                            int *modep, F newmat[4][4], F bounds[6])
{
  int mode = *modep;
  *modep = (mode |
    VTK_RESLICE_X_NEAREST | VTK_RESLICE_Y_NEAREST | VTK_RESLICE_Z_NEAREST);

  // set up input traversal table for nearest-neighbor interpolation
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    int minExt = inExt[2*k];
    int maxExt = inExt[2*k + 1];
    F minBounds = bounds[2*k];
    F maxBounds = bounds[2*k + 1];

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];

      if (point >= minBounds && point <= maxBounds)
        {
        int inId = vtkResliceRound(point);

        switch (mode & VTK_RESLICE_WRAP_MASK)
          {
          case VTK_RESLICE_REPEAT:
            inId = vtkInterpolateWrap(inId, minExt, maxExt);
            break;

          case VTK_RESLICE_MIRROR:
            inId = vtkInterpolateMirror(inId, minExt, maxExt);
            break;

          default:
            inId = vtkInterpolateClamp(inId, minExt, maxExt);
            break;
          }

        traversal[j][i] = inId*inInc[k];

        if (region == 0)
          { // entering the input extent
          region = 1;
          clipExt[2*j] = i;
          }
        }
      else
        {
        if (region == 1)
          { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
          }
        }
      }

    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteLinearTable(const int outExt[6], const int inExt[6],
                           const vtkIdType inInc[3], int clipExt[6],
                           vtkIdType **traversal, F **constants,
                           int *modep, F newmat[4][4], F bounds[6])
{
  int mode = *modep;

  // set up input traversal table for linear interpolation
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // do the output pixels lie exactly on top of the input pixels?
    F f1, f2;
    vtkResliceFloor(newmat[k][j], f1);
    vtkResliceFloor(newmat[k][3], f2);
    if (f1 == 0 && f2 == 0)
      {
      mode |= (VTK_RESLICE_X_NEAREST << j);
      }

    int minExt = inExt[2*k];
    int maxExt = inExt[2*k + 1];
    F minBounds = bounds[2*k];
    F maxBounds = bounds[2*k + 1];

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];

      if (point >= minBounds && point <= maxBounds)
        {
        F f;
        int inId0 = vtkResliceFloor(point, f);
        int inId1 = inId0 + (f != 0);

        switch (mode & VTK_RESLICE_WRAP_MASK)
          {
          case VTK_RESLICE_REPEAT:
            inId0 = vtkInterpolateWrap(inId0, minExt, maxExt);
            inId1 = vtkInterpolateWrap(inId1, minExt, maxExt);
            break;

          case VTK_RESLICE_MIRROR:
            inId0 = vtkInterpolateMirror(inId0, minExt, maxExt);
            inId1 = vtkInterpolateMirror(inId1, minExt, maxExt);
            break;

          default:
            inId0 = vtkInterpolateClamp(inId0, minExt, maxExt);
            inId1 = vtkInterpolateClamp(inId1, minExt, maxExt);
            break;
          }

        constants[j][2*i] = 1 - f;
        constants[j][2*i+1] = f;
        traversal[j][2*i] = inId0*inInc[k];
        traversal[j][2*i+1] = inId1*inInc[k];

        if (region == 0)
          { // entering the input extent
          region = 1;
          clipExt[2*j] = i;
          }
        }
      else
        {
        if (region == 1)
          { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
          }
        }
      }

    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }

  *modep = mode;
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteCubicTable(const int outExt[6], const int inExt[6],
                          const vtkIdType inInc[3], int clipExt[6],
                          vtkIdType **traversal, F **constants,
                          int *modep, F newmat[4][4], F bounds[6])
{
  int mode = *modep;

  // set up input traversal table for cubic interpolation
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // do the output pixels lie exactly on top of the input pixels?
    F f1, f2;
    vtkResliceFloor(newmat[k][j], f1);
    vtkResliceFloor(newmat[k][3], f2);
    if ((mode & VTK_RESLICE_MODE_MASK) == VTK_RESLICE_CUBIC &&
        f1 == 0 && f2 == 0)
      {
      mode |= (VTK_RESLICE_X_NEAREST << j);
      }

    int minExt = inExt[2*k];
    int maxExt = inExt[2*k + 1];
    F minBounds = bounds[2*k];
    F maxBounds = bounds[2*k + 1];

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];

      if (point >= minBounds && point <= maxBounds)
        {
        F f;
        int inId0 = vtkResliceFloor(point, f);
        int fIsNotZero = (f != 0);

        // is there more than one slice in this direction
        int multiple = (minExt != maxExt);
        if ((mode & VTK_RESLICE_MODE_MASK) == VTK_RESLICE_CUBIC)
          {
          // if not b-spline, this condition is better
          multiple &= fIsNotZero;
          }

        int inId[4];
        switch (mode & VTK_RESLICE_WRAP_MASK)
          {
          case VTK_RESLICE_REPEAT:
            inId[0] = vtkInterpolateWrap(inId0-1, minExt, maxExt);
            inId[1] = vtkInterpolateWrap(inId0,   minExt, maxExt);
            inId[2] = vtkInterpolateWrap(inId0+1, minExt, maxExt);
            inId[3] = vtkInterpolateWrap(inId0+2, minExt, maxExt);
            break;

          case VTK_RESLICE_MIRROR:
            inId[0] = vtkInterpolateMirror(inId0-1, minExt, maxExt);
            inId[1] = vtkInterpolateMirror(inId0,   minExt, maxExt);
            inId[2] = vtkInterpolateMirror(inId0+1, minExt, maxExt);
            inId[3] = vtkInterpolateMirror(inId0+2, minExt, maxExt);
            break;

          default:
            inId[0] = vtkInterpolateClamp(inId0-1, minExt, maxExt);
            inId[1] = vtkInterpolateClamp(inId0,   minExt, maxExt);
            inId[2] = vtkInterpolateClamp(inId0+1, minExt, maxExt);
            inId[3] = vtkInterpolateClamp(inId0+2, minExt, maxExt);
            break;
          }

        // range of indices to use
        int low = 1 - multiple;
        int high = 1 + 2*multiple;

        vtkTricubicInterpWeights(&constants[j][4*i], low, high, f);

        // set the memory offsets
        int l;
        for (l = 0; l < low; l++)
          {
          traversal[j][4*i+l] = inId[low]*inInc[k];
          }
        for (; l <= high; l++)
          {
          traversal[j][4*i+l] = inId[l]*inInc[k];
          }
        for (; l < 4; l++)
          {
          traversal[j][4*i+l] = inId[high]*inInc[k];
          }

        if (region == 0)
          { // entering the input extent
          region = 1;
          clipExt[2*j] = i;
          }
        }
      else
        {
        if (region == 1)
          { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
          }
        }
      }

    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }

  *modep = mode;
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteGeneralTable(const int outExt[6], const int inExt[6],
                            const vtkIdType inInc[3], int clipExt[6],
                            vtkIdType **traversal, F **constants,
                            int *modep, F newmat[4][4], F bounds[6])
{
  int mode = *modep;

  // set up input traversal table for interpolation
  for (int j = 0; j < 3; j++)
    {
    int k;
    for (k = 0; k < 3; k++)
      { // set k to the element which is nonzero
      if (newmat[k][j] != 0)
        {
        break;
        }
      }

    // do the output pixels lie exactly on top of the input pixels?
    F f1, f2;
    vtkResliceFloor(newmat[k][j], f1);
    vtkResliceFloor(newmat[k][3], f2);
    if (f1 == 0 && f2 == 0)
      {
      mode |= (VTK_RESLICE_X_NEAREST << j);
      }

    int m = ((mode & VTK_RESLICE_N_MASK) >> VTK_RESLICE_N_SHIFT) + 1;
    int m2 = ((m - 1) >> 1);
    int minExt = inExt[2*k];
    int maxExt = inExt[2*k + 1];
    F minBounds = bounds[2*k];
    F maxBounds = bounds[2*k + 1];

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];

      if (point >= minBounds && point <= maxBounds)
        {
        F f;
        int idx = vtkResliceFloor(point, f) - m2;
        int inId[VTK_RESLICE_MAX_KERNEL_SIZE];

        // is there more than one slice in this direction
        int multiple = (minExt != maxExt);

        int l;
        int low = m2*(1 - multiple);
        int high = (m2 + 1)*(multiple + 1) - 1;
        idx += low;

        switch (mode & VTK_RESLICE_WRAP_MASK)
          {
          case VTK_RESLICE_REPEAT:
            for (l = low; l <= high; l++)
              {
              inId[l] = vtkInterpolateWrap(idx++, minExt, maxExt);
              }
            break;

          case VTK_RESLICE_MIRROR:
            for (l = low; l <= high; l++)
              {
              inId[l] = vtkInterpolateMirror(idx++, minExt, maxExt);
              }
            break;

          default:
            for (l = low; l <= high; l++)
              {
              inId[l] = vtkInterpolateClamp(idx++, minExt, maxExt);
              }
            break;
          }

        // other high-order kernels could be added here
        switch (mode & VTK_RESLICE_MODE_MASK)
          {
          case VTK_RESLICE_LANCZOS:
            vtkLanczosInterpWeights(&constants[j][m*i], f, m);
            break;
          case VTK_RESLICE_KAISER:
            vtkKaiserInterpWeights(&constants[j][m*i], f, m);
            break;
          }

        // set the memory offsets
        for (l = 0; l < low; l++)
          {
          traversal[j][m*i+l] = inId[low]*inInc[k];
          }
        for (; l <= high; l++)
          {
          traversal[j][m*i+l] = inId[l]*inInc[k];
          }
        for (; l < m; l++)
          {
          traversal[j][m*i+l] = inId[high]*inInc[k];
          }

        if (region == 0)
          { // entering the input extent
          region = 1;
          clipExt[2*j] = i;
          }
        }
      else
        {
        if (region == 1)
          { // leaving the input extent
          region = 2;
          clipExt[2*j+1] = i - 1;
          }
        }
      }

    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }

  *modep = mode;
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
void vtkGetRowCompositeFunc(vtkImageReslice *self,
  void (**composite)(F *op, const F *ip, int nc, int count, int i, int n))
{
  int slabMode = self->GetSlabMode();
  int trpz = self->GetSlabTrapezoidIntegration();

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

//----------------------------------------------------------------------------
// Check to see if we can do nearest-neighbor instead of linear or cubic.
// This check only works on permutation+scale+translation matrices.
template <class F>
inline int vtkCanUseNearestNeighbor(F matrix[4][4], int outExt[6])
{
  // loop through dimensions
  for (int i = 0; i < 3; i++)
    {
    int j;
    for (j = 0; j < 3; j++)
      {
      if (matrix[i][j] != 0)
        {
        break;
        }
      }
    F x = matrix[i][j];
    F y = matrix[i][3];
    if (outExt[2*j] == outExt[2*j+1])
      {
      y += x*outExt[2*i];
      x = 0;
      }
    F fx, fy;
    vtkResliceFloor(x, fx);
    vtkResliceFloor(y, fy);
    if (fx != 0 || fy != 0)
      {
      return 0;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
// the ReslicePermuteExecute path is taken when the output slices are
// orthogonal to the input slices
template <class F>
void vtkReslicePermuteExecute(vtkImageReslice *self,
                              vtkImageData *inData, void *inPtr,
                              vtkImageData *outData, void *outPtr,
                              vtkImageResliceConvertScalarsType convertScalars,
                              int outExt[6], int threadId, F matrix[4][4])
{
  // find maximum input range
  int inExt[6];
  inData->GetExtent(inExt);

  // Get Increments to march through data
  vtkIdType inInc[3];
  inData->GetIncrements(inInc);
  vtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarSize = outData->GetScalarSize();
  int inComponents = inData->GetNumberOfScalarComponents();
  int outComponents = outData->GetNumberOfScalarComponents();

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

  // interpolation mode
  int interpolationMode = self->GetInterpolationMode();
  if (vtkCanUseNearestNeighbor(newmat, outExt))
    {
    interpolationMode = VTK_RESLICE_NEAREST;
    }

  bool doConversion = true;
  if (interpolationMode == VTK_RESLICE_NEAREST &&
      inData->GetScalarType() == outData->GetScalarType() &&
      !convertScalars && nsamples == 1 && inComponents <= 4)
    {
    doConversion = false;
    }

  // the step size is the number of coefficients per dimension
  int step = 1;
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      step = 1;
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      step = 2;
      break;
    case VTK_RESLICE_CUBIC:
      step = 4;
      break;
    case VTK_RESLICE_LANCZOS:
    case VTK_RESLICE_KAISER:
      step = 2*self->GetInterpolationSizeParameter();
      break;
    }

  // allocate the interpolation tables
  vtkIdType *traversal[3];
  F *constants[3];
  for (int i = 0; i < 3; i++)
    {
    int extentI = extent[2*i+1] - extent[2*i] + 1;

    traversal[i] = new vtkIdType[extentI*step];
    traversal[i] -= step*extent[2*i];
    constants[i] = new F[extentI*step];
    constants[i] -= step*extent[2*i];
    }

  // this 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = vtkResliceGetMode(self);

  // compute the bounds in structured coords
  F bounds[6];
  vtkResliceGetStructuredBounds(self, inExt, bounds);

  // clipExt will be set to portion of extent that matches bounds
  int clipExt[6];
  for (int j = 0; j < 6; j++)
    {
    clipExt[j] = extent[j];
    }

  // fill in the interpolation tables
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      vtkPermuteNearestTable(extent, inExt, inInc, clipExt,
                             traversal, constants, &mode, newmat, bounds);
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      vtkPermuteLinearTable(extent, inExt, inInc, clipExt,
                            traversal, constants, &mode, newmat, bounds);
      break;
    case VTK_RESLICE_CUBIC:
      vtkPermuteCubicTable(extent, inExt, inInc, clipExt,
                           traversal, constants, &mode, newmat, bounds);
      break;
    default:
      vtkPermuteGeneralTable(extent, inExt, inInc, clipExt,
                             traversal, constants, &mode, newmat, bounds);
      break;
    }

  // get type-specific functions
  void (*summation)(void *&out, const void *in, int numscalars,
                    int n, int mode,
                    const vtkIdType *iX, const F *fX,
                    const vtkIdType *iY, const F *fY,
                    const vtkIdType *iZ, const F *fZ);
  void (*conversion)(void *&out, const F *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  vtkGetResliceSummationFunc(self, &summation, interpolationMode, doConversion);
  vtkGetConversionFunc(self, &conversion);
  vtkGetSetPixelsFunc(self, &setpixels);

  // get the slab compositing function
  void (*composite)(F *op, const F *ip, int nc, int count, int i, int n);
  vtkGetRowCompositeFunc(self, &composite);

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
  vtkAllocBackgroundPixel(self, &background, outComponents);

  // get the input stencil
  vtkImageStencilData *stencil = self->GetStencil();
  // get the output stencil
  vtkImageStencilData *outputStencil = 0;
  if (self->GetGenerateStencilOutput())
    {
    outputStencil = self->GetStencilOutput();
    }

  // for tracking progress
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Loop through output pixels
  for (int idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    int idZ0 = idZ*step;

    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      int idY0 = idY*step;

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
        setpixels(outPtr, background, outComponents, outExt[1] - outExt[0] + 1);
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
          int idX0 = idXmin*step;

          if (doConversion)
            {
            // these six lines are for handling incomplete slabs
            int lowerSkip = clipExt[4] - idZ;
            lowerSkip = (lowerSkip >= 0 ? lowerSkip : 0);
            int upperSkip = idZ + (nsamples-1) - clipExt[5];
            upperSkip = (upperSkip >= 0 ? upperSkip : 0);
            int idZ1 = idZ0 + step*lowerSkip;
            int nsamples1 = nsamples - lowerSkip - upperSkip;  

            for (int isample = 0; isample < nsamples1; isample++)
              {
              void *tmpPtr = ((nsamples1 > 1) ? floatSumPtr : floatPtr);
              summation(tmpPtr, inPtr, inComponents, idXmax - idXmin + 1, mode,
                        &traversal[0][idX0], &constants[0][idX0],
                        &traversal[1][idY0], &constants[1][idY0],
                        &traversal[2][idZ1], &constants[2][idZ1]);

              if (nsamples1 > 1)
                {
                composite(floatPtr, floatSumPtr, inComponents,
                          idXmax - idXmin + 1, isample, nsamples1);
                }

              idZ1 += step;
              }

            if (outputStencil)
              {
              outputStencil->InsertNextExtent(idXmin, idXmax, idY, idZ);
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
            summation(outPtr, inPtr, inComponents, idXmax - idXmin + 1, mode,
                      &traversal[0][idX0], &constants[0][idX0],
                      &traversal[1][idY0], &constants[1][idY0],
                      &traversal[2][idZ0], &constants[2][idZ0]);
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

  vtkFreeBackgroundPixel(self, &background);

  if (doConversion)
    {
    delete [] floatPtr;
    }
  if (nsamples > 1)
    {
    delete [] floatSumPtr;
    }

  for (int k = 0; k < 3; k++)
    {
    traversal[k] += step*extent[2*k];
    constants[k] += step*extent[2*k];
    delete [] traversal[k];
    delete [] constants[k];
    }
}

//----------------------------------------------------------------------------
// check a matrix to ensure that it is a permutation+scale+translation
// matrix

template <class F>
int vtkIsPermutationMatrix(F matrix[4][4])
{
  for (int i = 0; i < 3; i++)
    {
    if (matrix[3][i] != 0)
      {
      return 0;
      }
    }
  if (matrix[3][3] != 1)
    {
    return 0;
    }
  for (int j = 0; j < 3; j++)
    {
    int k = 0;
    for (int i = 0; i < 3; i++)
      {
      if (matrix[i][j] != 0)
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
        static_cast<vtkHomogeneousTransform *>(this->ResliceTransform)->GetMatrix());
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

  // Get the output pointer
  void *outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  if (this->HitInputExtent == 0)
    {
    vtkImageResliceClearExecute(this, inData[0][0], 0, outData[0], outPtr,
                                outExt, threadId);
    return;
    }

  // Now that we know that we need the input, get the input pointer
  void *inPtr = inData[0][0]->GetScalarPointerForExtent(inExt);

  if (this->Optimization)
    {
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

    if (vtkIsPermutationMatrix(newmat) && newtrans == NULL)
      {
      vtkReslicePermuteExecute(this, inData[0][0], inPtr, outData[0], outPtr,
                               (this->HasConvertScalars ?
                                  &vtkImageReslice::ConvertScalarsBase : 0),
                               outExt, threadId, newmat);
      }
    else
      {
      vtkOptimizedExecute(this, inData[0][0], inPtr, outData[0], outPtr,
                          (this->HasConvertScalars ?
                             &vtkImageReslice::ConvertScalarsBase : 0),
                          outExt, threadId, newmat, newtrans);
      }
    }
  else
    {
    vtkImageResliceExecute(this, inData[0][0], inPtr, outData[0], outPtr,
                           (this->HasConvertScalars ?
                              &vtkImageReslice::ConvertScalarsBase : 0),
                           outExt, threadId);
    }
}
