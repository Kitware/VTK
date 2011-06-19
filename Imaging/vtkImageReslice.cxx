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
#include "vtkImageResliceDetail.h"

typedef void (vtkImageReslice::*vtkImageResliceConvertScalarsType)(
    void *outPtr, void *inPtr, int inputType, int inNumComponents,
    int count, int idX, int idY, int idZ, int threadId);

vtkStandardNewMacro(vtkImageReslice);
vtkCxxSetObjectMacro(vtkImageReslice, InformationInput, vtkImageData);

//----------------------------------------------------------------------------
vtkImageReslice::vtkImageReslice()
{
  // if NULL, the main Input is used
  this->InformationInput = NULL;
  this->OutputDimensionality = 3;

  this->Optimization = 1; // turn off when you're paranoid

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
  this->SetInformationInput(NULL);
}

//----------------------------------------------------------------------------
// build any tables required for the interpolation. Do this in every
// translation unit because the interpolation table is a static array that's
// declaraed in the common vtkImageResliceDetail.h header file.
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
void vtkImageReslice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "Optimization: " << (this->Optimization ? "On\n":"Off\n");
  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "GenerateStencilOutput: " << (this->GenerateStencilOutput ? "On\n":"Off\n");
  os << indent << "StencilOutput: " << this->GetStencilOutput() << "\n";
  os << indent << "OutputDimensionality: "
     << this->OutputDimensionality << "\n";
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
int vtkImageReslice::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

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

  return this->InternalRequestInformation(
      request, inputVector, outputVector,
      inWholeExt, inSpacing, inOrigin);
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
// Some helper functions for 'RequestData'
//----------------------------------------------------------------------------

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
// This function executes the filter for any type of data.  It is much simpler
// in structure than vtkImageResliceOptimizedExecute.
void vtkImageResliceExecute(vtkImageReslice *self,
                            vtkImageData *inData, void *inPtr,
                            vtkImageData *outData, void *outPtr,
                            vtkImageResliceConvertScalarsType convertScalars,
                            int outExt[6], int threadId)
{
  int inComponents, outComponents, numpixels;
  int idX, idY, idZ;
  int startIdX, endIdX, idXmin, idXmax, iter;
  int isInBounds, wasInBounds;
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
  double *floatPtr, *tmpPtr;
  void (*interpolate)(double *outPtr, const void *inPtr,
                      const int inExt[6], const vtkIdType inInc[3],
                      int numscalars, const double point[3], int mode);
  void (*convertpixels)(void *&out, const double *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // the 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = vtkResliceGetMode(self);

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
  outComponents = outData->GetNumberOfScalarComponents();
  inComponents = inData->GetNumberOfScalarComponents();

  // allocate an output row of type double
  floatPtr = new double [inComponents*(outExt[1] - outExt[0] + 1)];

  // allocate a voxel to copy into the background (out-of-bounds) regions
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
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (threadId == 0)
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
                                     outPtr, background, outComponents,
                                     setpixels, iter))
        {
        wasInBounds = 1;
        isInBounds = 1;
        startIdX = idXmin;
        idX = idXmin;
        tmpPtr = floatPtr;

        while (startIdX <= idXmax)
          {
          for (; idX <= idXmax && isInBounds == wasInBounds; idX++)
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
          endIdX = idX - 1 - (isInBounds != wasInBounds);
          numpixels = endIdX - startIdX + 1;

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
  int i, inComponents, outComponents, numpixels;
  int idX, idY, idZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int scalarSize, inputScalarSize;
  int inExt[6];
  vtkIdType inInc[3];
  unsigned long count = 0;
  unsigned long target;
  int iter, startIdX, endIdX, idXmin, idXmax;
  int isInBounds, wasInBounds;
  double temp[3];
  F inOrigin[3], inInvSpacing[3];
  F xAxis[4], yAxis[4], zAxis[4], origin[4];
  F inPoint0[4];
  F inPoint1[4];
  F inPoint[4], f;
  void *background;
  F *floatPtr, *tmpPtr;
  void (*interpolate)(F *outPtr, const void *inPtr,
                      const int inExt[6], const vtkIdType inInc[3],
                      int numscalars, const F point[3], int mode);
  void (*convertpixels)(void *&out, const F *in, int numscalars, int n);
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  int mode = vtkResliceGetMode(self);
  int wrap = (self->GetWrap() || self->GetMirror());

  int perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 ||
      newmat[3][2] != 0 || newmat[3][3] != 1)
    {
    perspective = 1;
    }

  int optimizeNearest = 0;
  if (self->GetInterpolationMode() == VTK_RESLICE_NEAREST &&
      !(wrap || newtrans || perspective || convertScalars) &&
      inData->GetScalarType() == outData->GetScalarType() &&
      self->GetBorder() == 1)
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
  inputScalarSize = inData->GetScalarSize();
  inComponents= inData->GetNumberOfScalarComponents();
  outComponents= outData->GetNumberOfScalarComponents();

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

  // allocate an output row of type double
  floatPtr = 0;
  if (!optimizeNearest)
    {
    floatPtr = new F [inComponents*(outExt[1] - outExt[0] + 1)];
    }

  // set color for area outside of input volume extent
  vtkAllocBackgroundPixel(self, &background, outComponents);

  // Set interpolation method
  vtkGetResliceInterpFunc(self, &interpolate);
  vtkGetConversionFunc(self, &convertpixels);
  vtkGetSetPixelsFunc(self, &setpixels);

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

      if (!threadId)
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
                                     outPtr, background, outComponents,
                                     setpixels, iter))
        {
        if (!optimizeNearest)
          {
          wasInBounds = 1;
          isInBounds = 1;
          startIdX = idXmin;
          idX = idXmin;
          tmpPtr = floatPtr;

          while (startIdX <= idXmax)
            {
            for (; idX <= idXmax && isInBounds == wasInBounds; idX++)
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

              // apply the AbstractTransform if there is one
              vtkResliceApplyTransform(newtrans, inPoint, inOrigin,
                                       inInvSpacing);

              isInBounds = 0;
              if (inPoint[0] >= bounds[0] && inPoint[0] <= bounds[1] &&
                  inPoint[1] >= bounds[2] && inPoint[1] <= bounds[3] &&
                  inPoint[2] >= bounds[4] && inPoint[2] <= bounds[5])
                {
                // do the interpolation
                isInBounds = 1;
                interpolate(tmpPtr, inPtr, inExt, inInc, inComponents,
                            inPoint, mode);
                }
              tmpPtr += inComponents;
              }

            // write a segment to the output
            endIdX = idX - 1 - (isInBounds != wasInBounds);
            numpixels = endIdX - startIdX + 1;

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
                                              inIdZ*inInc[2])*inputScalarSize);
              }

            setpixels(outPtr, inPtrTmp, outComponents, 1);
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
                              int outExt[6], int threadId, F newmat[4][4])
{
  vtkIdType outInc[3];
  int scalarSize, inComponents, outComponents;
  vtkIdType inInc[3];
  int inExt[6], clipExt[6];
  vtkIdType *traversal[3];
  F *constants[3];
  int i;

  // find maximum input range
  inData->GetExtent(inExt);

  // Get Increments to march through data
  inData->GetIncrements(inInc);
  outData->GetContinuousIncrements(outExt, outInc[0], outInc[1], outInc[2]);
  scalarSize = outData->GetScalarSize();
  inComponents = inData->GetNumberOfScalarComponents();
  outComponents = outData->GetNumberOfScalarComponents();

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

  int doConversion = 1;
  if (interpolationMode == VTK_RESLICE_NEAREST &&
      inData->GetScalarType() == outData->GetScalarType() &&
      !convertScalars && inComponents <= 4)
    {
    doConversion = 0;
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
  for (i = 0; i < 3; i++)
    {
    int outExtI = outExt[2*i+1] - outExt[2*i] + 1;

    traversal[i] = new vtkIdType[outExtI*step];
    traversal[i] -= step*outExt[2*i];
    constants[i] = new F[outExtI*step];
    constants[i] -= step*outExt[2*i];
    }

  // this 'mode' species what to do with the 'pad' (out-of-bounds) area
  int mode = vtkResliceGetMode(self);

  // compute the bounds in structured coords
  F bounds[6];
  vtkResliceGetStructuredBounds(self, inExt, bounds);

  // fill in the interpolation tables
  switch (interpolationMode)
    {
    case VTK_RESLICE_NEAREST:
      vtkPermuteNearestTable(outExt, inExt, inInc, clipExt,
                             traversal, constants, &mode, newmat, bounds);
      break;
    case VTK_RESLICE_LINEAR:
    case VTK_RESLICE_RESERVED_2:
      vtkPermuteLinearTable(outExt, inExt, inInc, clipExt,
                            traversal, constants, &mode, newmat, bounds);
      break;
    case VTK_RESLICE_CUBIC:
      vtkPermuteCubicTable(outExt, inExt, inInc, clipExt,
                           traversal, constants, &mode, newmat, bounds);
      break;
    default:
      vtkPermuteGeneralTable(outExt, inExt, inInc, clipExt,
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

  // get temp float space for type conversion
  F *floatPtr = 0;
  if (doConversion)
    {
    floatPtr = new F [inComponents*(outExt[1] - outExt[0] + 1)];
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
      if (idZ < clipExt[4] || idZ > clipExt[5] ||
          idY < clipExt[2] || idY > clipExt[3])
        { // just clear, we're completely outside
        setpixels(outPtr, background, outComponents, outExt[1] - outExt[0] + 1);
        }
      else
        {
        // clear pixels to left of input extent
        setpixels(outPtr, background, outComponents, clipExt[0] - outExt[0]);

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
            void *tmpPtr = floatPtr;

            summation(tmpPtr, inPtr, inComponents, idXmax - idXmin + 1, mode,
                      &traversal[0][idX0], &constants[0][idX0],
                      &traversal[1][idY0], &constants[1][idY0],
                      &traversal[2][idZ0], &constants[2][idZ0]);

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
        static_cast<char *>(outPtr) + outInc[1]*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outInc[2]*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);

  if (doConversion)
    {
    delete [] floatPtr;
    }

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
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageReslice::InternalThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int threadId)
{
  int inExt[6];
  inData[0][0]->GetExtent(inExt);

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
