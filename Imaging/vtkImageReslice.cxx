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
// The 'floor' function on x86 and mips is many times slower than these
// and is used a lot in this code, optimize for different CPU architectures
template<class F>
inline int vtkResliceFloor(double x, F &f)
{
#if defined mips || defined sparc || defined __ppc__
  x += 2147483648.0;
  unsigned int i = static_cast<unsigned int>(x);
  f = x - i;
  return static_cast<int>(i - 2147483648U);
#elif defined i386 || defined _M_IX86
  union { double d; unsigned short s[4]; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.0;  // (2**(52-16))*1.5
  f = dual.s[0]*0.0000152587890625; // 2**(-16)
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#elif defined ia64 || defined __ia64__ || defined IA64
  x += 103079215104.0;
  long long i = static_cast<long long>(x);
  f = x - i;
  return static_cast<int>(i - 103079215104LL);
#else
  double y = floor(x);
  f = x - y;
  return static_cast<int>(y);
#endif
}

inline int vtkResliceRound(double x)
{
#if defined mips || defined sparc || defined __ppc__
  return static_cast<int>(static_cast<unsigned int>(x + 2147483648.5) - 2147483648U);
#elif defined i386 || defined _M_IX86
  union { double d; unsigned int i[2]; } dual;
  dual.d = x + 103079215104.5;  // (2**(52-16))*1.5
  return static_cast<int>((dual.i[1]<<16)|((dual.i[0])>>16));
#elif defined ia64 || defined __ia64__ || defined IA64
  x += 103079215104.5;
  long long i = static_cast<long long>(x);
  return static_cast<int>(i - 103079215104LL);
#else
  return static_cast<int>(floor(x+0.5));
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

  // flag to use default Spacing
  this->OutputSpacing[0] = VTK_DOUBLE_MAX;
  this->OutputSpacing[1] = VTK_DOUBLE_MAX;
  this->OutputSpacing[2] = VTK_DOUBLE_MAX;

  // ditto
  this->OutputOrigin[0] = VTK_DOUBLE_MAX;
  this->OutputOrigin[1] = VTK_DOUBLE_MAX;
  this->OutputOrigin[2] = VTK_DOUBLE_MAX;

  // ditto
  this->OutputExtent[0] = VTK_INT_MIN;
  this->OutputExtent[2] = VTK_INT_MIN;
  this->OutputExtent[4] = VTK_INT_MIN;

  this->OutputExtent[1] = VTK_INT_MAX;
  this->OutputExtent[3] = VTK_INT_MAX;
  this->OutputExtent[5] = VTK_INT_MAX;

  this->Wrap = 0; // don't wrap
  this->Mirror = 0; // don't mirror
  this->Border = 1; // apply a border
  this->InterpolationMode = VTK_RESLICE_NEAREST; // no interpolation
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

  // There is an optional second input.
  this->SetNumberOfInputPorts(2);
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
  os << indent << "Optimization: " << (this->Optimization ? "On\n":"Off\n");
  os << indent << "BackgroundColor: " <<
    this->BackgroundColor[0] << " " << this->BackgroundColor[1] << " " <<
    this->BackgroundColor[2] << " " << this->BackgroundColor[3] << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundColor[0] << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
}

//----------------------------------------------------------------------------
void vtkImageReslice::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->InformationInput,
                            "InformationInput");
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
  unsigned long mTime=this->vtkObject::GetMTime();
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

  int i,j,k;
  int idX,idY,idZ;
  double xAxis[4], yAxis[4], zAxis[4], origin[4];
  double inPoint0[4];
  double inPoint1[4];
  double point[4],f;
  double *inSpacing,*inOrigin,*outSpacing,*outOrigin,inInvSpacing[3];

  inInvSpacing[0] = 0.0;
  inInvSpacing[1] = 0.0;
  inInvSpacing[2] = 0.0;

  int wrap = this->Wrap || this->Mirror;

  inOrigin = inInfo->Get(vtkDataObject::ORIGIN());
  inSpacing = inInfo->Get(vtkDataObject::SPACING());
  outOrigin = outInfo->Get(vtkDataObject::ORIGIN());
  outSpacing = outInfo->Get(vtkDataObject::SPACING());

  if (this->Optimization)
    {
    vtkMatrix4x4 *matrix = this->GetIndexMatrix(inInfo, outInfo);

    // convert matrix from world coordinates to pixel indices
    for (i = 0; i < 4; i++)
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

  for (i = 0; i < 3; i++)
    {
    inExt[2*i] = VTK_INT_MAX;
    inExt[2*i+1] = VTK_INT_MIN;
    }

  // check the coordinates of the 8 corners of the output extent
  // (this must be done exactly the same as the calculation in
  // vtkImageResliceExecute)
  for (i = 0; i < 8; i++)  
    {
    // get output coords
    idX = outExt[i%2];
    idY = outExt[2+(i/2)%2];
    idZ = outExt[4+(i/4)%2];

    if (this->Optimization)
      {
      inPoint0[0] = origin[0] + idZ*zAxis[0]; // incremental transform
      inPoint0[1] = origin[1] + idZ*zAxis[1]; 
      inPoint0[2] = origin[2] + idZ*zAxis[2]; 
      inPoint0[3] = origin[3] + idZ*zAxis[3]; 

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
        f = 1/point[3];
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
    
      if (this->ResliceAxes)
        {
        point[3] = 1.0;
        this->ResliceAxes->MultiplyPoint(point, point);
        f = 1.0/point[3];
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
    if (this->GetInterpolationMode() != VTK_RESLICE_NEAREST)
      {
      int extra = (this->GetInterpolationMode() == VTK_RESLICE_CUBIC); 
      for (j = 0; j < 3; j++) 
        {
        k = vtkResliceFloor(point[j], f);
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
      for (j = 0; j < 3; j++) 
        {
        k = vtkResliceRound(point[j]);
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

  for (i = 0; i < 3; i++)
    {
    if (inExt[2*i] < wholeExtent[2*i])
      {
      inExt[2*i] = wholeExtent[2*i];
      if (wrap)
        {
        inExt[2*i+1] = wholeExtent[2*i+1];
        }
      else if (inExt[2*i+1] < wholeExtent[2*i])
        {
        // didn't hit any of the input extent
        inExt[2*i+1] = wholeExtent[2*i];
        this->HitInputExtent = 0;
        }
      }
    if (inExt[2*i+1] > wholeExtent[2*i+1])
      {
      inExt[2*i+1] = wholeExtent[2*i+1];
      if (wrap)
        {
        inExt[2*i] = wholeExtent[2*i];
        }
      else if (inExt[2*i] > wholeExtent[2*i+1])
        {
        // didn't hit any of the input extent
        inExt[2*i] = wholeExtent[2*i+1];
        // finally, check for null input extent
        if (inExt[2*i] < wholeExtent[2*i])
          {
          inExt[2*i] = wholeExtent[2*i];
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
    // the stencil input is optional
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  else
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    }
  return 1;
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
  vtkInformation* inInfo2 = inputVector[1]->GetInformationObject(0);
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

    if (this->OutputSpacing[i] == VTK_DOUBLE_MAX)
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
    else if (this->OutputExtent[2*i] == VTK_INT_MIN ||
             this->OutputExtent[2*i+1] == VTK_INT_MAX)
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
    else if (this->OutputOrigin[i] == VTK_DOUBLE_MAX)
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

  this->GetIndexMatrix(inInfo, outInfo);

  // need to set the spacing and origin of the stencil to match the output
  if (inInfo2)
    {
    vtkImageStencilData *stencil = 
      vtkImageStencilData::SafeDownCast(
        inInfo2->Get(vtkDataObject::DATA_OBJECT()));
    // need to call the set methods on the actual data object, not
    // on the pipeline, since the pipeline cannot back-propagate
    // this information
    if (stencil)
      {
      stencil->SetSpacing(inInfo->Get(vtkDataObject::SPACING()));
      stencil->SetOrigin(inInfo->Get(vtkDataObject::ORIGIN()));
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
//  Interpolation subroutines and associated code 
//----------------------------------------------------------------------------

// Three interpolation functions are supported: NearestNeighbor, Trilinear,
// and Tricubic.  These routines have the following signature:
//
//int interpolate(T *&outPtr,
//                const T *inPtr,
//                const int inExt[6],
//                const vtkIdType inInc[3],
//                int numscalars,
//                const F point[3],
//                int mode,
//                const T *background)
//
// where 'T' is any arithmetic type and 'F' is a float type
//
// The result of the interpolation is put in *outPtr, and outPtr is
// incremented.

//----------------------------------------------------------------------------
// constants for different boundary-handling modes

#define VTK_RESLICE_BACKGROUND 0   // use background if out-of-bounds
#define VTK_RESLICE_WRAP       1   // wrap to opposite side of image
#define VTK_RESLICE_MIRROR     2   // mirror off of the boundary 
#define VTK_RESLICE_BORDER     3   // use a half-voxel border
#define VTK_RESLICE_NULL       4   // do nothing to *outPtr if out-of-bounds

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

#if (VTK_USE_INT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt8& clamp)
{
  if (val < -128.0)
    { 
    val = -128.0;
    }
  if (val > 127.0)
    { 
    val = 127.0;
    }
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT8 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt8& clamp)
{
  if (val < 0)
    { 
    val = 0;
    }
  if (val > 255.0)
    { 
    val = 255.0;
    }
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt16& clamp)
{
  if (val < -32768.0)
    { 
    val = -32768.0;
    }
  if (val > 32767.0)
    { 
    val = 32767.0;
    }
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT16 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt16& clamp)
{
  if (val < 0)
    { 
    val = 0;
    }
  if (val > 65535.0)
    { 
    val = 65535.0;
    }
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_INT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeInt32& clamp)
{
  if (val < -2147483648.0) 
    {
    val = -2147483648.0;
    }
  if (val > 2147483647.0) 
    {
    val = 2147483647.0;
    }
  vtkResliceRound(val,clamp);
}
#endif

#if (VTK_USE_UINT32 != 0)
template <class F>
inline void vtkResliceClamp(F val, vtkTypeUInt32& clamp)
{
  if (val < 0)
    { 
    val = 0;
    }
  if (val > 4294967295.0)
    { 
    val = 4294967295.0;
    }
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
// Perform a wrap to limit an index to [0,range).
// Ensures correct behaviour when the index is negative.
 
inline int vtkInterpolateWrap(int num, int range)
{
  if ((num %= range) < 0)
    {
    num += range; // required for some % implementations
    } 
  return num;
}

//----------------------------------------------------------------------------
// Perform a mirror to limit an index to [0,range).
 
inline int vtkInterpolateMirror(int num, int range)
{
  if (num < 0)
    {
    num = -num - 1;
    }
  int count = num/range;
  num %= range;
  if (count & 0x1)
    {
    num = range - num - 1;
    }
  return num;
}

//----------------------------------------------------------------------------
// If the value is within one half voxel of the range [0,inExtX), then
// set it to "0" or "inExtX-1" as appropriate.

inline  int vtkInterpolateBorder(int &inIdX0, int &inIdX1, int inExtX,
                                 double fx)
{
  if (inIdX0 >= 0 && inIdX1 < inExtX)
    {
    return 0;
    }
  if (inIdX0 == -1 && fx >= 0.5)
    {
    inIdX1 = inIdX0 = 0;
    return 0;
    }
  if (inIdX0 == inExtX - 1 && fx < 0.5)
    {
    inIdX1 = inIdX0;
    return 0;
    }

  return 1;
}

inline  int vtkInterpolateBorderCheck(int inIdX0, int inIdX1, int inExtX,
                                      double fx)
{
  if ((inIdX0 >= 0 && inIdX1 < inExtX) ||
      (inIdX0 == -1 && fx >= 0.5) ||
      (inIdX0 == inExtX - 1 && fx < 0.5))
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Do nearest-neighbor interpolation of the input data 'inPtr' of extent 
// 'inExt' at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'
template <class F, class T>
int vtkNearestNeighborInterpolation(T *&outPtr, const T *inPtr,
                                    const int inExt[6],
                                    const vtkIdType inInc[3],
                                    int numscalars, const F point[3],
                                    int mode, const T *background)
{
  int inIdX0 = vtkResliceRound(point[0]) - inExt[0];
  int inIdY0 = vtkResliceRound(point[1]) - inExt[2];
  int inIdZ0 = vtkResliceRound(point[2]) - inExt[4];

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX0 >= inExtX ||
      inIdY0 < 0 || inIdY0 >= inExtY ||
      inIdZ0 < 0 || inIdZ0 >= inExtZ)
    {
    if (mode == VTK_RESLICE_WRAP)
      {
      inIdX0 = vtkInterpolateWrap(inIdX0, inExtX);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExtZ);
      }
    else if (mode == VTK_RESLICE_MIRROR)
      {
      inIdX0 = vtkInterpolateMirror(inIdX0, inExtX);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExtZ);
      }
    else if (mode == VTK_RESLICE_BACKGROUND ||
             mode == VTK_RESLICE_BORDER)
      {
      do
        {
        *outPtr++ = *background++;
        }
      while (--numscalars);
      return 0;
      }
    else
      {
      return 0;
      }
    }

  inPtr += inIdX0*inInc[0]+inIdY0*inInc[1]+inIdZ0*inInc[2];
  do
    {
    *outPtr++ = *inPtr++;
    }
  while (--numscalars);

  return 1;
} 

//----------------------------------------------------------------------------
// Do trilinear interpolation of the input data 'inPtr' of extent 'inExt'
// at the 'point'.  The result is placed at 'outPtr'.  
// If the lookup data is beyond the extent 'inExt', set 'outPtr' to
// the background color 'background'.  
// The number of scalar components in the data is 'numscalars'
template <class F, class T>
int vtkTrilinearInterpolation(T *&outPtr, const T *inPtr,
                              const int inExt[6], const vtkIdType inInc[3],
                              int numscalars, const F point[3],
                              int mode, const T *background)
{
  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inIdX1 = inIdX0 + (fx != 0);
  int inIdY1 = inIdY0 + (fy != 0);
  int inIdZ1 = inIdZ0 + (fz != 0);

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  if (inIdX0 < 0 || inIdX1 >= inExtX ||
      inIdY0 < 0 || inIdY1 >= inExtY ||
      inIdZ0 < 0 || inIdZ1 >= inExtZ)
    {
    if (mode == VTK_RESLICE_BORDER)
      {
      if (vtkInterpolateBorder(inIdX0, inIdX1, inExtX, fx) ||
          vtkInterpolateBorder(inIdY0, inIdY1, inExtY, fy) ||
          vtkInterpolateBorder(inIdZ0, inIdZ1, inExtZ, fz))
        {
        do
          {
          *outPtr++ = *background++;
          }
        while (--numscalars);
        return 0;
        }
      }
    else if (mode == VTK_RESLICE_WRAP)
      {
      inIdX0 = vtkInterpolateWrap(inIdX0, inExtX);
      inIdY0 = vtkInterpolateWrap(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateWrap(inIdZ0, inExtZ);

      inIdX1 = vtkInterpolateWrap(inIdX1, inExtX);
      inIdY1 = vtkInterpolateWrap(inIdY1, inExtY);
      inIdZ1 = vtkInterpolateWrap(inIdZ1, inExtZ);
      }
    else if (mode == VTK_RESLICE_MIRROR)
      {
      inIdX0 = vtkInterpolateMirror(inIdX0, inExtX);
      inIdY0 = vtkInterpolateMirror(inIdY0, inExtY);
      inIdZ0 = vtkInterpolateMirror(inIdZ0, inExtZ);

      inIdX1 = vtkInterpolateMirror(inIdX1, inExtX);
      inIdY1 = vtkInterpolateMirror(inIdY1, inExtY);
      inIdZ1 = vtkInterpolateMirror(inIdZ1, inExtZ);
      }
    else if (mode == VTK_RESLICE_BACKGROUND)
      {
      do
        {
        *outPtr++ = *background++;
        }
      while (--numscalars);
      return 0;
      }
    else
      {
      return 0;
      }
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
    F result = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                    fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                    fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));

    vtkResliceRound(result, *outPtr++);
    inPtr0++;
    inPtr1++;
    }
  while (--numscalars);

  return 1;
}

//----------------------------------------------------------------------------
// Do tricubic interpolation of the input data 'inPtr' of extent 'inExt' 
// at the 'point'.  The result is placed at 'outPtr'.  
// The number of scalar components in the data is 'numscalars'
// The tricubic interpolation ensures that both the intensity and
// the first derivative of the intensity are smooth across the
// image.  The first derivative is estimated using a 
// centered-difference calculation.


// helper function: set up the lookup indices and the interpolation 
// coefficients

template <class T>
void vtkTricubicInterpCoeffs(T F[4], int l, int h, T f)
{
  static const T half = T(0.5);

  int order = h - l;

  if (order == 0)
    { // no interpolation
    F[0] = 0;
    F[1] = 1;
    F[2] = 0;
    F[3] = 0;
    return;
    }
  if (order == 3)
    { // cubic interpolation
    T fm1 = f - 1;
    T fd2 = f*half;
    T ft3 = f*3; 
    F[0] = -fd2*fm1*fm1;
    F[1] = ((ft3 - 2)*fd2 - 1)*fm1;
    F[2] = -((ft3 - 4)*f - 1)*fd2;
    F[3] = f*fd2*fm1;
    return;
    }
  if (order == 1)
    { // linear interpolation
    F[0] = 0;
    F[1] = 1 - f;
    F[2] = f;
    F[3] = 0;
    return;
    }
  if (l == 0)
    { // quadratic interpolation
    T fp1 = f + 1;
    T fm1 = f - 1; 
    T fd2 = f*half;
    F[0] = fd2*fm1;
    F[1] = -fp1*fm1;
    F[2] = fp1*fd2;
    F[3] = 0;
    return;
    }
  // else
    { // quadratic interpolation
    T fm1 = f - 1;
    T fm2 = fm1 - 1;
    T fm1d2 = fm1*half;
    F[0] = 0;
    F[1] = fm1d2*fm2;
    F[2] = -f*fm2;
    F[3] = f*fm1d2;
    return;
    }
}

// tricubic interpolation
template <class F, class T>
int vtkTricubicInterpolation(T *&outPtr, const T *inPtr,
                             const int inExt[6], const vtkIdType inInc[3],
                             int numscalars, const F point[3],
                             int mode, const T *background)
{
  F fx, fy, fz;
  int floorX = vtkResliceFloor(point[0], fx);
  int floorY = vtkResliceFloor(point[1], fy);
  int floorZ = vtkResliceFloor(point[2], fz);

  int fxIsNotZero = (fx != 0);
  int fyIsNotZero = (fy != 0);
  int fzIsNotZero = (fz != 0);

  int inIdX0 = floorX - inExt[0];
  int inIdY0 = floorY - inExt[2];
  int inIdZ0 = floorZ - inExt[4];

  int inIdX1 = inIdX0 + fxIsNotZero;
  int inIdY1 = inIdY0 + fyIsNotZero;
  int inIdZ1 = inIdZ0 + fzIsNotZero;

  int inExtX = inExt[1] - inExt[0] + 1;
  int inExtY = inExt[3] - inExt[2] + 1;
  int inExtZ = inExt[5] - inExt[4] + 1;

  vtkIdType inIncX = inInc[0];
  vtkIdType inIncY = inInc[1];
  vtkIdType inIncZ = inInc[2];

  vtkIdType factX[4], factY[4], factZ[4];

  if (inIdX0 < 0 || inIdX1 >= inExtX ||
      inIdY0 < 0 || inIdY1 >= inExtY ||
      inIdZ0 < 0 || inIdZ1 >= inExtZ)
    {
    if (mode == VTK_RESLICE_BORDER)
      {
      if (vtkInterpolateBorderCheck(inIdX0, inIdX1, inExtX, fx) ||
          vtkInterpolateBorderCheck(inIdY0, inIdY1, inExtY, fy) ||
          vtkInterpolateBorderCheck(inIdZ0, inIdZ1, inExtZ, fz))
        {
        do
          {
          *outPtr++ = *background++;
          }
        while (--numscalars);
        return 0;
        }
      }
    else if (mode != VTK_RESLICE_WRAP && mode != VTK_RESLICE_MIRROR)
      {
      if (mode == VTK_RESLICE_BACKGROUND)
        {
        do
          {
          *outPtr++ = *background++;
          }
        while (--numscalars);
        return 0;
        }
      else
        {
        return 0;
        }
      }
    }

  F fX[4], fY[4], fZ[4];
  int k1, k2, j1, j2, i1, i2;

  if (mode == VTK_RESLICE_WRAP || mode == VTK_RESLICE_MIRROR)
    {
    i1 = 0;
    i2 = 3;
    vtkTricubicInterpCoeffs(fX, i1, i2, fx);

    j1 = 1 - fyIsNotZero;
    j2 = 1 + (fyIsNotZero<<1);
    vtkTricubicInterpCoeffs(fY, j1, j2, fy);

    k1 = 1 - fzIsNotZero;
    k2 = 1 + (fzIsNotZero<<1);
    vtkTricubicInterpCoeffs(fZ, k1, k2, fz);

    if (mode == VTK_RESLICE_WRAP)
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateWrap(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateWrap(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateWrap(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    else
      {
      for (int i = 0; i < 4; i++)
        {
        factX[i] = vtkInterpolateMirror(inIdX0 + i - 1, inExtX)*inIncX;
        factY[i] = vtkInterpolateMirror(inIdY0 + i - 1, inExtY)*inIncY;
        factZ[i] = vtkInterpolateMirror(inIdZ0 + i - 1, inExtZ)*inIncZ;
        }
      }
    }
  else if (mode == VTK_RESLICE_BORDER)
    {
    // clamp to the border of the input extent
   
    i1 = 1 - fxIsNotZero;
    j1 = 1 - fyIsNotZero;
    k1 = 1 - fzIsNotZero;

    i2 = 1 + 2*fxIsNotZero;
    j2 = 1 + 2*fyIsNotZero;
    k2 = 1 + 2*fzIsNotZero;
    
    vtkTricubicInterpCoeffs(fX, i1, i2, fx);
    vtkTricubicInterpCoeffs(fY, j1, j2, fy);
    vtkTricubicInterpCoeffs(fZ, k1, k2, fz);

    int tmpExt = inExtX - 1;
    int tmpId = tmpExt - inIdX0 - 1;
    factX[0] = (inIdX0 - 1)*(inIdX0 - 1 >= 0)*inIncX;
    factX[1] = (inIdX0)*(inIdX0 >= 0)*inIncX;
    factX[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncX;
    factX[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncX;

    tmpExt = inExtY - 1;
    tmpId = tmpExt - inIdY0 - 1;
    factY[0] = (inIdY0 - 1)*(inIdY0 - 1 >= 0)*inIncY;
    factY[1] = (inIdY0)*(inIdY0 >= 0)*inIncY;
    factY[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncY;
    factY[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncY;

    tmpExt = inExtZ - 1;
    tmpId = tmpExt - inIdZ0 - 1;
    factZ[0] = (inIdZ0 - 1)*(inIdZ0 - 1 >= 0)*inIncZ;
    factZ[1] = (inIdZ0)*(inIdZ0 >= 0)*inIncZ;
    factZ[2] = (tmpExt - (tmpId)*(tmpId >= 0))*inIncZ;
    factZ[3] = (tmpExt - (tmpId - 1)*(tmpId - 1 >= 0))*inIncZ;
    }
  else
    {
    // depending on whether we are at the edge of the 
    // input extent, choose the appropriate interpolation
    // method to use

    i1 = 1 - (inIdX0 > 0)*fxIsNotZero;
    j1 = 1 - (inIdY0 > 0)*fyIsNotZero;
    k1 = 1 - (inIdZ0 > 0)*fzIsNotZero;

    i2 = 1 + (1 + (inIdX0 + 2 < inExtX))*fxIsNotZero;
    j2 = 1 + (1 + (inIdY0 + 2 < inExtY))*fyIsNotZero;
    k2 = 1 + (1 + (inIdZ0 + 2 < inExtZ))*fzIsNotZero;

    vtkTricubicInterpCoeffs(fX, i1, i2, fx);
    vtkTricubicInterpCoeffs(fY, j1, j2, fy);
    vtkTricubicInterpCoeffs(fZ, k1, k2, fz);

    factX[1] = inIdX0*inIncX;
    factX[0] = factX[1] - inIncX;
    factX[2] = factX[1] + inIncX;
    factX[3] = factX[2] + inIncX;

    factY[1] = inIdY0*inIncY;
    factY[0] = factY[1] - inIncY;
    factY[2] = factY[1] + inIncY;
    factY[3] = factY[2] + inIncY;
    
    factZ[1] = inIdZ0*inIncZ;
    factZ[0] = factZ[1] - inIncZ;
    factZ[2] = factZ[1] + inIncZ;
    factZ[3] = factZ[2] + inIncZ;

    // this little bit of wierdness allows us to unroll the x loop
    if (i1 > 0)
      {
      factX[0] = factX[1];
      }
    if (i2 < 3)
      {
      factX[3] = factX[1];
      if (i2 < 2)
        {
        factX[2] = factX[1];
        }
      }
    }
  
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

    vtkResliceClamp(val, *outPtr++);
    inPtr++;
    }
  while (--numscalars);

  return 1;
}                   

//--------------------------------------------------------------------------
// get appropriate interpolation function according to interpolation mode
// and scalar type
template<class F>
void vtkGetResliceInterpFunc(vtkImageReslice *self,
                             int (**interpolate)(void *&outPtr,
                                                 const void *inPtr,
                                                 const int inExt[6],
                                                 const vtkIdType inInc[3],
                                                 int numscalars,
                                                 const F point[3],
                                                 int mode,
                                                 const void *background))
{
  int dataType = self->GetOutput()->GetScalarType();
  int interpolationMode = self->GetInterpolationMode();
  
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      switch (dataType)
        {
        vtkTemplateAliasMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
                                     const int inExt[6],
                                     const vtkIdType inInc[3],
                                     int numscalars, const F point[3],
                                     int mode,
                                     const VTK_TT *background))interpolate) = \
                         &vtkNearestNeighborInterpolation);
        default:
          interpolate = 0;
        }
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      switch (dataType)
        {
        vtkTemplateAliasMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
                                     const int inExt[6],
                                     const vtkIdType inInc[3],
                                     int numscalars, const F point[3],
                                     int mode,
                                     const VTK_TT *background))interpolate) = \
                         &vtkTrilinearInterpolation);
        default:
          interpolate = 0;
        }
      break;
    case VTK_RESLICE_CUBIC:
      switch (dataType)
        {
        vtkTemplateAliasMacro(*((int (**)(VTK_TT *&outPtr, const VTK_TT *inPtr,
                                     const int inExt[6],
                                     const vtkIdType inInc[3],
                                     int numscalars, const F point[3],
                                     int mode,
                                     const VTK_TT *background))interpolate) = \
                         &vtkTricubicInterpolation);
        default:
          interpolate = 0;
        }
      break;
    default:
      interpolate = 0;
    }
}


//----------------------------------------------------------------------------
// Some helper functions for 'RequestData'
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// pixel copy function, templated for different scalar types
template <class T>
struct vtkImageResliceSetPixels
{
static void Set(void *&outPtrV, const void *inPtrV, int numscalars, int n)
{
  const T* inPtr = static_cast<const T*>(inPtrV);
  T* outPtr = static_cast<T*>(outPtrV);
  for (int i = 0; i < n; i++)
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
  for (int i = 0; i < n; i++)
    {
    *outPtr++ = val;
    }
  outPtrV = outPtr;
}
};

// get a pixel copy function that is appropriate for the data type
void vtkGetSetPixelsFunc(vtkImageReslice *self,
                         void (**setpixels)(void *&out, const void *in,
                                            int numscalars, int n))
{
  int dataType = self->GetOutput()->GetScalarType();
  int numscalars = self->GetOutput()->GetNumberOfScalarComponents();

  switch (numscalars)
    {
    case 1:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set1
          );
        default:
          setpixels = 0;
        }
    default:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set
          );
        default:
          setpixels = 0;
        }
    }
}

//----------------------------------------------------------------------------
// Convert background color from float to appropriate type
template <class T>
void vtkAllocBackgroundPixelT(vtkImageReslice *self,
                              T **background_ptr, int numComponents)
{
  *background_ptr = new T[numComponents];
  T *background = *background_ptr;

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
  switch (self->GetOutput()->GetScalarType())
    {
    vtkTemplateAliasMacro(vtkAllocBackgroundPixelT(self, (VTK_TT **)rval,
                                              numComponents));
    }
}      

void vtkFreeBackgroundPixel(vtkImageReslice *self, void **rval)
{
  switch (self->GetOutput()->GetScalarType())
    {
    vtkTemplateAliasMacro(delete [] *((VTK_TT **)rval));
    }

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
                                 vtkImageData *, void *,
                                 vtkImageData *outData, void *outPtr,
                                 int outExt[6], int id)
{
  int numscalars;
  int idY, idZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int scalarSize;
  unsigned long count = 0;
  unsigned long target;
  void *background;
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // for the progress meter
  target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = outData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  vtkAllocBackgroundPixel(self, &background, numscalars);
  // get the appropriate function for pixel copying
  vtkGetSetPixelsFunc(self, &setpixels);

  // Loop through output voxels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (id == 0) 
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
                            int outExt[6], int id)
{
  int numscalars;
  int idX, idY, idZ;
  int idXmin, idXmax, iter;
  vtkIdType outIncX, outIncY, outIncZ;
  int scalarSize;
  int inExt[6];
  vtkIdType inInc[3];
  unsigned long count = 0;
  unsigned long target;
  double point[4];
  double f;
  double *inSpacing, *inOrigin, *outSpacing, *outOrigin, inInvSpacing[3];
  void *background;
  int (*interpolate)(void *&outPtr, const void *inPtr,
                     const int inExt[6], const vtkIdType inInc[3],
                     int numscalars, const double point[3],
                     int mode, const void *background);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // the 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = VTK_RESLICE_BACKGROUND;
  if (self->GetMirror())
    {
    mode = VTK_RESLICE_MIRROR;
    }
  else if (self->GetWrap())
    {
    mode = VTK_RESLICE_WRAP;
    }
  else if (self->GetBorder())
    {
    mode = VTK_RESLICE_BORDER;
    }

  // the transformation to apply to the data
  vtkAbstractTransform *transform = self->GetResliceTransform();
  vtkMatrix4x4 *matrix = self->GetResliceAxes();

  // for conversion to data coordinates
  inOrigin = inData->GetOrigin();
  inSpacing = inData->GetSpacing();
  outOrigin = outData->GetOrigin();
  outSpacing = outData->GetSpacing();

  // save effor later: invert inSpacing
  inInvSpacing[0] = 1.0/inSpacing[0];
  inInvSpacing[1] = 1.0/inSpacing[1];
  inInvSpacing[2] = 1.0/inSpacing[2];

  // find maximum input range
  inData->GetExtent(inExt);
  
  // for the progress meter
  target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // get the appropriate functions for interpolation and pixel copying
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);

  // get the stencil
  vtkImageStencilData *stencil = self->GetStencil();

  // Loop through output voxels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (id == 0) 
        { // update the progress if this is the main thread
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      
      iter = 0; // if there is a stencil, it is applied here
      while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                     outExt[0], outExt[1], idY, idZ,
                                     outPtr, background, numscalars, 
                                     setpixels, iter))
        {
        for (idX = idXmin; idX <= idXmax; idX++)
          {
          // convert to data coordinates 
          point[0] = idX*outSpacing[0] + outOrigin[0];
          point[1] = idY*outSpacing[1] + outOrigin[1];
          point[2] = idZ*outSpacing[2] + outOrigin[2];

          // apply ResliceAxes matrix
          if (matrix)
            {
            point[3] = 1.0;
            matrix->MultiplyPoint(point, point);
            f = 1.0/point[3];
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

          // interpolate output voxel from input data set
          interpolate(outPtr, inPtr, inExt, inInc, numscalars,
                      point, mode, background);
          } 
        }
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);
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
                         int outExt[6], int id, F newmat[4][4], 
                         vtkAbstractTransform *newtrans)
{
  int i, numscalars;
  int idX, idY, idZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int scalarSize;
  int inExt[6];
  vtkIdType inInc[3];
  unsigned long count = 0;
  unsigned long target;
  int iter, idXmin, idXmax;
  double temp[3];
  F inOrigin[3], inInvSpacing[3];
  F xAxis[4], yAxis[4], zAxis[4], origin[4]; 
  F inPoint0[4];
  F inPoint1[4];
  F inPoint[4], f;
  void *background;
  int (*interpolate)(void *&outPtr, const void *inPtr,
                     const int inExt[6], const vtkIdType inInc[3],
                     int numscalars, const F point[3],
                     int mode, const void *background);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  int mode = VTK_RESLICE_BACKGROUND;
  int wrap = 0;

  if (self->GetMirror())
    {
    mode = VTK_RESLICE_MIRROR;
    wrap = 1;
    }
  else if (self->GetWrap())
    {
    mode = VTK_RESLICE_WRAP;
    wrap = 1;
    }
  else if (self->GetBorder())
    {
    mode = VTK_RESLICE_BORDER;
    }
  
  int perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 ||
      newmat[3][2] != 0 || newmat[3][3] != 1)
    {
    perspective = 1;
    }

  int optimizeNearest = 0;
  if (self->GetInterpolationMode() == VTK_RESLICE_NEAREST &&
      !(wrap || newtrans || perspective))
    {
    optimizeNearest = 1;
    }

  // find maximum input range
  inData->GetExtent(inExt);

  target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;
  
  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();
  
  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  for (i = 0; i < 4; i++)
    {
    xAxis[i] = newmat[i][0];
    yAxis[i] = newmat[i][1];
    zAxis[i] = newmat[i][2];
    origin[i] = newmat[i][3];
    }

  // get the input origin and spacing for conversion purposes
  inData->GetOrigin(temp);
  inOrigin[0] = F(temp[0]);
  inOrigin[1] = F(temp[1]);
  inOrigin[2] = F(temp[2]);

  inData->GetSpacing(temp);
  inInvSpacing[0] = F(1.0/temp[0]);
  inInvSpacing[1] = F(1.0/temp[1]);
  inInvSpacing[2] = F(1.0/temp[2]);

  // set color for area outside of input volume extent
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // Set interpolation method
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetSetPixelsFunc(self, &setpixels);

  // get the stencil
  vtkImageStencilData *stencil = self->GetStencil();

  // Loop through output pixels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    inPoint0[0] = origin[0] + idZ*zAxis[0]; // incremental transform
    inPoint0[1] = origin[1] + idZ*zAxis[1]; 
    inPoint0[2] = origin[2] + idZ*zAxis[2]; 
    inPoint0[3] = origin[3] + idZ*zAxis[3]; 
    
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      inPoint1[0] = inPoint0[0] + idY*yAxis[0]; // incremental transform
      inPoint1[1] = inPoint0[1] + idY*yAxis[1];
      inPoint1[2] = inPoint0[2] + idY*yAxis[2];
      inPoint1[3] = inPoint0[3] + idY*yAxis[3];
      
      if (!id) 
        {
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      
      iter = 0;
      while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                     outExt[0], outExt[1], idY, idZ,
                                     outPtr, background, numscalars, 
                                     setpixels, iter))
        {
        if (!optimizeNearest)
          {
          for (idX = idXmin; idX <= idXmax; idX++)
            {
            inPoint[0] = inPoint1[0] + idX*xAxis[0];
            inPoint[1] = inPoint1[1] + idX*xAxis[1];
            inPoint[2] = inPoint1[2] + idX*xAxis[2];
            if (perspective)
              { // only do perspective if necessary
              inPoint[3] = inPoint1[3] + idX*xAxis[3];
              f = 1/inPoint[3];
              inPoint[0] *= f;
              inPoint[1] *= f;
              inPoint[2] *= f;
              }
            if (newtrans)
              { // apply the AbstractTransform if there is one
              vtkResliceApplyTransform(newtrans, inPoint, inOrigin,
                                       inInvSpacing);
              }
            // call the interpolation function
            interpolate(outPtr, inPtr, inExt, inInc, numscalars,
                        inPoint, mode, background);
            }
          }
        else // optimize for nearest-neighbor interpolation
          {
          int inExtX = inExt[1] - inExt[0] + 1;
          int inExtY = inExt[3] - inExt[2] + 1;
          int inExtZ = inExt[5] - inExt[4] + 1;

          for (int iidX = idXmin; iidX <= idXmax; iidX++)
            {
            void *inPtrTmp = background;

            inPoint[0] = inPoint1[0] + iidX*xAxis[0];
            inPoint[1] = inPoint1[1] + iidX*xAxis[1];
            inPoint[2] = inPoint1[2] + iidX*xAxis[2];

            int inIdX = vtkResliceRound(inPoint[0]) - inExt[0];
            int inIdY = vtkResliceRound(inPoint[1]) - inExt[2];
            int inIdZ = vtkResliceRound(inPoint[2]) - inExt[4];

            if (inIdX >= 0 && inIdX < inExtX &&
                inIdY >= 0 && inIdY < inExtY &&
                inIdZ >= 0 && inIdZ < inExtZ)
              {
              inPtrTmp = static_cast<void *>(static_cast<char *>(inPtr) +
                                             (inIdX*inInc[0] + 
                                              inIdY*inInc[1] +
                                              inIdZ*inInc[2])*scalarSize);
              }

            setpixels(outPtr, inPtrTmp, numscalars, 1);
            }
          }
        }
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }
  
  vtkFreeBackgroundPixel(self, &background);
}

//----------------------------------------------------------------------------
// vtkReslicePermuteExecute is specifically optimized for
// cases where the IndexMatrix has only one non-zero component
// per row, i.e. when the matrix is permutation+scale+translation.
// All of the interpolation coefficients are calculated ahead
// of time instead of on a pixel-by-pixel basis.

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template<class F, class T>
void vtkPermuteNearestSummation(T *&outPtr, const T *inPtr,
                                int numscalars, int n,
                                const vtkIdType *iX, const F *,
                                const vtkIdType *iY, const F *,
                                const vtkIdType *iZ, const F *,
                                const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = 0; i < n; i++)
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
}

// ditto, but optimized for numscalars = 1
template<class F, class T>
void vtkPermuteNearestSummation1(T *&outPtr, const T *inPtr,
                                 int, int n,
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *,
                                 const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = 0; i < n; i++)
    {
    *outPtr++ = inPtr0[iX[0]];
    iX++;
    }
}

// ditto, but optimized for numscalars = 3
template<class F, class T>
void vtkPermuteNearestSummation3(T *&outPtr, const T *inPtr,
                                 int, int n,
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *,
                                 const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = 0; i < n; i++)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    *outPtr++ = *tmpPtr++;
    *outPtr++ = *tmpPtr++;
    *outPtr++ = *tmpPtr;
    }  
}

// ditto, but optimized for numscalars = 4
template<class F, class T>
void vtkPermuteNearestSummation4(T *&outPtr, const T *inPtr,
                                 int, int n,
                                 const vtkIdType *iX, const F *,
                                 const vtkIdType *iY, const F *,
                                 const vtkIdType *iZ, const F *,
                                 const int [3])
{
  const T *inPtr0 = inPtr + iY[0] + iZ[0];

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = 0; i < n; i++)
    {
    const T *tmpPtr = &inPtr0[iX[0]];
    iX++;
    *outPtr++ = *tmpPtr++;
    *outPtr++ = *tmpPtr++;
    *outPtr++ = *tmpPtr++;
    *outPtr++ = *tmpPtr;
    }  
}

//----------------------------------------------------------------------------
// helper function for linear interpolation
template<class F, class T>
void vtkPermuteTrilinearSummation(T *&outPtr, const T *inPtr,
                                  int numscalars, int n,
                                  const vtkIdType *iX, const F *fX,
                                  const vtkIdType *iY, const F *fY,
                                  const vtkIdType *iZ, const F *fZ,
                                  const int useNearestNeighbor[3])
{
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

  if (useNearestNeighbor[0] && fy == 0 && fz == 0)
    { // no interpolation needed at all
    for (int i = 0; i < n; i++)
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
  else if (useNearestNeighbor[0] && fy == 0)
    { // only need linear z interpolation
    for (int i = 0; i < n; i++)
      {
      vtkIdType t0 = iX[0];
      iX += 2;
          
      const T *inPtr0 = inPtr + t0; 
      int m = numscalars;
      do 
        {
        F result = (rz*inPtr0[i00] + fz*inPtr0[i01]);
        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        }
      while (--m);
      }
    }
  else if (fz == 0)
    { // bilinear interpolation in x,y
    for (int i = 0; i < n; i++)
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
        F result = (rx*(ry*inPtr0[i00] + fy*inPtr0[i10]) +
                    fx*(ry*inPtr1[i00] + fy*inPtr1[i10]));

        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
  else
    { // do full trilinear interpolation 
    for (int i = 0; i < n; i++)
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
        F result = (rx*(ryrz*inPtr0[i00] + ryfz*inPtr0[i01] +
                        fyrz*inPtr0[i10] + fyfz*inPtr0[i11]) +
                    fx*(ryrz*inPtr1[i00] + ryfz*inPtr1[i01] +
                        fyrz*inPtr1[i10] + fyfz*inPtr1[i11]));

        vtkResliceRound(result, *outPtr++);
        inPtr0++;
        inPtr1++;
        }
      while (--m);
      }
    }
}

//--------------------------------------------------------------------------
// helper function for tricubic interpolation
template<class F, class T>
void vtkPermuteTricubicSummation(T *&outPtr, const T *inPtr,
                                 int numscalars, int n,
                                 const vtkIdType *iX, const F *fX,
                                 const vtkIdType *iY, const F *fY,
                                 const vtkIdType *iZ, const F *fZ,
                                 const int useNearestNeighbor[3])
{
  // speed things up a bit for bicubic interpolation
  int k1 = 0;
  int k2 = 3;
  if (useNearestNeighbor[2])
    {
    k1 = k2 = 1;
    }

  for (int i = 0; i < n; i++)
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

      vtkResliceClamp(result, *outPtr++);
      inPtr0++;
      }
    while (--c);
    }
}

//----------------------------------------------------------------------------
// get approprate summation function for different interpolation modes
// and different scalar types
template<class F>
void vtkGetResliceSummationFunc(vtkImageReslice *self,
                                void (**summation)(void *&out, const void *in,
                                                   int numscalars, int n,
                                                   const vtkIdType *iX, const F *fX,
                                                   const vtkIdType *iY, const F *fY,
                                                   const vtkIdType *iZ, const F *fZ,
                                                   const int useNearest[3]),
                                int interpolationMode)
{
  int scalarType = self->GetOutput()->GetScalarType();
  int numScalars = self->GetOutput()->GetNumberOfScalarComponents();
  
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      if (numScalars == 1)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                        int numscalars, int n,
                                        const vtkIdType *iX, const F *fX,
                                        const vtkIdType *iY, const F *fY,
                                        const vtkIdType *iZ, const F *fZ,
                                        const int useNearest[3]))summation) = \
                           vtkPermuteNearestSummation1);
          default:
            summation = 0;
          }
        }
      else if (numScalars == 3)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                        int numscalars, int n,
                                        const vtkIdType *iX, const F *fX,
                                        const vtkIdType *iY, const F *fY,
                                        const vtkIdType *iZ, const F *fZ,
                                        const int useNearest[3]))summation) = \
                           vtkPermuteNearestSummation3);
          default:
            summation = 0;
          }
        }
      else if (numScalars == 4)
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                        int numscalars, int n,
                                        const vtkIdType *iX, const F *fX,
                                        const vtkIdType *iY, const F *fY,
                                        const vtkIdType *iZ, const F *fZ,
                                        const int useNearest[3]))summation) = \
                           vtkPermuteNearestSummation4);
          default:
            summation = 0;
          }
        }
      else
        {
        switch (scalarType)
          {
          vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                        int numscalars, int n,
                                        const vtkIdType *iX, const F *fX,
                                        const vtkIdType *iY, const F *fY,
                                        const vtkIdType *iZ, const F *fZ,
                                        const int useNearest[3]))summation) = \
                           vtkPermuteNearestSummation);
          default:
            summation = 0;
          }
        }
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      switch (scalarType)
        {
        vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                      int numscalars, int n,
                                      const vtkIdType *iX, const F *fX,
                                      const vtkIdType *iY, const F *fY,
                                      const vtkIdType *iZ, const F *fZ,
                                      const int useNearest[3]))summation) = \
                         vtkPermuteTrilinearSummation);
        default:
          summation = 0;
        }
      break;
    case VTK_RESLICE_CUBIC:
      switch (scalarType)
        {
        vtkTemplateAliasMacro(*((void (**)(VTK_TT *&out, const VTK_TT *in,
                                      int numscalars, int n,
                                      const vtkIdType *iX, const F *fX,
                                      const vtkIdType *iY, const F *fY,
                                      const vtkIdType *iZ, const F *fZ,
                                      const int useNearest[3]))summation) = \
                         vtkPermuteTricubicSummation);
        default:
          summation = 0;
        }
      break;
    default:
      summation = 0;
    }
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteNearestTable(vtkImageReslice *self, const int outExt[6],
                            const int inExt[6], const vtkIdType inInc[3],
                            int clipExt[6], vtkIdType **traversal,
                            F **vtkNotUsed(constants),
                            int useNearestNeighbor[3], F newmat[4][4])
{
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

    // this is just for symmetry with Linear and Cubic
    useNearestNeighbor[j] = 1;

    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      int inId = vtkResliceRound((newmat[k][3] + i*newmat[k][j]));
      inId -= inExt[2*k];
      if (self->GetMirror())
        {
        inId = vtkInterpolateMirror(inId, inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId = vtkInterpolateWrap(inId, inExtK);
        region = 1;
        }
      else
        {
        if (inId < 0 || inId >= inExtK)
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
           }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;
            }
          }
        }
      traversal[j][i] = inId*inInc[k];
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}
  
//----------------------------------------------------------------------------
template <class F>
void vtkPermuteLinearTable(vtkImageReslice *self, const int outExt[6],
                           const int inExt[6], const vtkIdType inInc[3],
                           int clipExt[6], vtkIdType **traversal,
                           F **constants,
                           int useNearestNeighbor[3], F newmat[4][4])
{
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
    useNearestNeighbor[j] = (f1 == 0 && f2 == 0);
    
    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];
      F f;
      int trunc = vtkResliceFloor(point, f);
      constants[j][2*i] = 1 - f;
      constants[j][2*i+1] = f;
      
      int fIsNotZero = (f != 0);
      int inId0 = trunc - inExt[2*k];
      int inId1 = inId0 + fIsNotZero;

      if (self->GetMirror())
        {
        inId0 = vtkInterpolateMirror(inId0, inExtK);
        inId1 = vtkInterpolateMirror(inId1, inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId0 = vtkInterpolateWrap(inId0, inExtK);
        inId1 = vtkInterpolateWrap(inId1, inExtK);
        region = 1;
        }
      else if (self->GetBorder())
        {
        if (vtkInterpolateBorder(inId0, inId1, inExtK, f))
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
          }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;           
            }
          }
        }
      else // not self->GetBorder()
        {
        if (inId0 < 0 || inId1 >= inExtK)
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
          }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;           
            }
          }
        }
      traversal[j][2*i] = inId0*inInc[k];
      traversal[j][2*i+1] = inId1*inInc[k];
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
    }
}

//----------------------------------------------------------------------------
template <class F>
void vtkPermuteCubicTable(vtkImageReslice *self, const int outExt[6],
                          const int inExt[6], const vtkIdType inInc[3],
                          int clipExt[6], vtkIdType **traversal,
                          F **constants,
                          int useNearestNeighbor[3], F newmat[4][4])
{
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
    useNearestNeighbor[j] = (f1 == 0 && f2 == 0);

    int inExtK = inExt[2*k+1] - inExt[2*k] + 1;

    int region = 0;
    for (int i = outExt[2*j]; i <= outExt[2*j+1]; i++)
      {
      F point = newmat[k][3] + i*newmat[k][j];
      F f;
      int trunc = vtkResliceFloor(point, f);
      int fIsNotZero = (f != 0);
      int inId[4];
      inId[1] = trunc - inExt[2*k];
      inId[0] = inId[1] - 1;
      inId[2] = inId[1] + 1;
      inId[3] = inId[1] + 2;

      int low = 1 - fIsNotZero;
      int high = 1 + 2*fIsNotZero;
      if (self->GetMirror())
        {
        inId[0] = vtkInterpolateMirror(inId[0], inExtK);
        inId[1] = vtkInterpolateMirror(inId[1], inExtK);
        inId[2] = vtkInterpolateMirror(inId[2], inExtK);
        inId[3] = vtkInterpolateMirror(inId[3], inExtK);
        region = 1;
        }
      else if (self->GetWrap())
        {
        inId[0] = vtkInterpolateWrap(inId[0], inExtK);
        inId[1] = vtkInterpolateWrap(inId[1], inExtK);
        inId[2] = vtkInterpolateWrap(inId[2], inExtK);
        inId[3] = vtkInterpolateWrap(inId[3], inExtK);
        region = 1;
        }      
      else if (self->GetBorder())
        {
        if (vtkInterpolateBorderCheck(inId[1], inId[2], inExtK, f))
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
          }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;           
            }
          }
        int tmpExt = inExtK - 1;
        inId[0] = (inId[0])*(inId[0] >= 0);
        inId[1] = (inId[1])*(inId[1] >= 0);
        inId[2] = tmpExt - (tmpExt - inId[2])*(tmpExt - inId[2] >= 0);
        inId[3] = tmpExt - (tmpExt - inId[3])*(tmpExt - inId[3] >= 0);
        low  = 1 - fIsNotZero;
        high = 1 + 2*fIsNotZero;
        }
      else // not self->GetBorder()
        {
        if (inId[1] < 0 || inId[1] + fIsNotZero >= inExtK)
          {
          if (region == 1)
            { // leaving the input extent
            region = 2;
            clipExt[2*j+1] = i - 1;
            }
          }
        else 
          {
          if (region == 0)
            { // entering the input extent
            region = 1;
            clipExt[2*j] = i;           
            }
          }
        low  = 1 - (inId[0] >= 0)*fIsNotZero;
        high = 1 + (1 + (inId[3] < inExtK))*fIsNotZero;
        }
      vtkTricubicInterpCoeffs(&constants[j][4*i], low, high, f);

      // set default values
      int l;
      for (l = 0; l < 4; l++)
        {
        traversal[j][4*i+l] = inId[1]*inInc[k];
        }
      for (l = low; l <= high; l++)
        { 
        traversal[j][4*i+l] = inId[l]*inInc[k];
        }
      }
    if (region == 0)
      { // never entered input extent!
      clipExt[2*j] = clipExt[2*j+1] + 1;
      }
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
                                     int outExt[6], int id, F newmat[4][4])
{
  vtkIdType outInc[3];
  int scalarSize, numscalars;
  vtkIdType inInc[3];
  int inExt[6], clipExt[6];
  vtkIdType *traversal[3];
  F *constants[3];
  int useNearestNeighbor[3];
  int i;
  
  // find maximum input range
  inData->GetExtent(inExt);

  // Get Increments to march through data 
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outInc[0], outInc[1], outInc[2]);
  scalarSize = outData->GetScalarSize();
  numscalars = inData->GetNumberOfScalarComponents();

  for (i = 0; i < 3; i++)
    {
    clipExt[2*i] = outExt[2*i];
    clipExt[2*i+1] = outExt[2*i+1];
    }

  int interpolationMode = self->GetInterpolationMode();
  if (vtkCanUseNearestNeighbor(newmat, outExt))
    {
    interpolationMode = VTK_RESLICE_NEAREST;
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
    }

  // allocate the interpolation tables
  for (i = 0; i < 3; i++)
    {
    int outExtI = outExt[2*i+1] - outExt[2*i] + 1; 

    traversal[i] = new vtkIdType[outExtI*step];
    traversal[i] -= step*outExt[2*i];
    constants[i] = new F[outExtI*step];
    constants[i] -= step*outExt[2*i];
    }

  // fill in the interpolation tables
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      vtkPermuteNearestTable(self, outExt, inExt, inInc, clipExt,
                             traversal, constants, 
                             useNearestNeighbor, newmat);
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      vtkPermuteLinearTable(self, outExt, inExt, inInc, clipExt,
                            traversal, constants, 
                            useNearestNeighbor, newmat);
      break;
    case VTK_RESLICE_CUBIC:
      vtkPermuteCubicTable(self, outExt, inExt, inInc, clipExt,
                           traversal, constants, 
                           useNearestNeighbor, newmat);
      break;
    }

  // get type-specific functions
  void (*summation)(void *&out, const void *in, int numscalars, int n,
                    const vtkIdType *iX, const F *fX,
                    const vtkIdType *iY, const F *fY,
                    const vtkIdType *iZ, const F *fZ,
                    const int useNearestNeighbor[3]);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);
  vtkGetResliceSummationFunc(self, &summation, interpolationMode);
  vtkGetSetPixelsFunc(self, &setpixels);

  // set color for area outside of input volume extent
  void *background;
  vtkAllocBackgroundPixel(self, &background, numscalars);

  // get the stencil
  vtkImageStencilData *stencil = self->GetStencil();

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

      if (id == 0) 
        { // track progress if we are main thread
        if (!(count%target)) 
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }

      // do extent check
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
          idY < clipExt[2] || idY > clipExt[3])
        { // just clear, we're completely outside
        setpixels(outPtr, background, numscalars, outExt[1] - outExt[0] + 1);
        }
      else
        {
        // clear pixels to left of input extent
        setpixels(outPtr, background, numscalars, clipExt[0] - outExt[0]);

        int iter = 0;
        int idXmin, idXmax;
        while (vtkResliceGetNextExtent(stencil, idXmin, idXmax,
                                       clipExt[0], clipExt[1], idY, idZ,
                                       outPtr, background, numscalars,
                                       setpixels, iter))
          {
          int idX0 = idXmin*step;

          summation(outPtr, inPtr, numscalars, idXmax - idXmin + 1,
                    &traversal[0][idX0], &constants[0][idX0],
                    &traversal[1][idY0], &constants[1][idY0],
                    &traversal[2][idZ0], &constants[2][idZ0],
                    useNearestNeighbor);
          }

        // clear pixels to right of input extent
        setpixels(outPtr, background, numscalars, outExt[1] - clipExt[1]);
        }

      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outInc[1]*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outInc[2]*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);

  for (i = 0; i < 3; i++)
    {
    traversal[i] += step*outExt[2*i];
    constants[i] += step*outExt[2*i];
    delete [] traversal[i];
    delete [] constants[i];
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
         (inSpacing[i] != 1.0 || inOrigin[i] != 0.0))) 
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
  int outExt[6], int id)
{

  vtkDebugMacro(<< "Execute: inData = " << inData[0][0]
                      << ", outData = " << outData[0]);

  // this filter expects that input is the same type as output.
  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData[0][0]->GetScalarType()
                  << ", must match out ScalarType "
                  << outData[0]->GetScalarType());
    return;
    }

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
                                outExt, id);
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

    double newmat[4][4];
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
                               outExt, id, newmat);
      }
    else
      {
      vtkOptimizedExecute(this, inData[0][0], inPtr, outData[0], outPtr,
                          outExt, id, newmat, newtrans);
      }
    }
  else
    {
    vtkImageResliceExecute(this, inData[0][0], inPtr, outData[0], outPtr,
                           outExt, id);
    }
}
