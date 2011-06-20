/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageResliceBase.h"

#include "vtkIntArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"
#include "vtkDataSetAttributes.h"
#include "vtkGarbageCollector.h"
#include "vtkTypeTraits.h"
#include "vtkImageResliceDetail.h"

vtkStandardNewMacro(vtkImageResliceBase);
vtkCxxSetObjectMacro(vtkImageResliceBase,ResliceAxes,vtkMatrix4x4);
vtkCxxSetObjectMacro(vtkImageResliceBase,ResliceTransform,vtkAbstractTransform);

typedef void (vtkImageResliceBase::*vtkImageResliceConvertScalarsType)(
    void *outPtr, void *inPtr, int inputType, int inNumComponents,
    int count, int idX, int idY, int idZ, int threadId);

//----------------------------------------------------------------------------
vtkImageResliceBase::vtkImageResliceBase()
{
  // if NULL, the main Input is used
  this->TransformInputSampling = 1;
  this->AutoCropOutput = 0;
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

  this->Wrap = 0; // don't wrap
  this->Mirror = 0; // don't mirror
  this->Border = 1; // apply a border
  this->InterpolationMode = VTK_RESLICE_NEAREST; // no interpolation
  this->InterpolationSizeParameter = 3; // for Lanczos and Kaiser

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
}

//----------------------------------------------------------------------------
vtkImageResliceBase::~vtkImageResliceBase()
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
}

//----------------------------------------------------------------------------
void vtkImageResliceBase::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "Wrap: " << (this->Wrap ? "On\n":"Off\n");
  os << indent << "Mirror: " << (this->Mirror ? "On\n":"Off\n");
  os << indent << "Border: " << (this->Border ? "On\n":"Off\n");
  os << indent << "InterpolationMode: "
     << this->GetInterpolationModeAsString() << "\n";
  os << indent << "InterpolationSizeParameter: " << this->InterpolationSizeParameter << "\n";
  os << indent << "BackgroundColor: " <<
    this->BackgroundColor[0] << " " << this->BackgroundColor[1] << " " <<
    this->BackgroundColor[2] << " " << this->BackgroundColor[3] << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundColor[0] << "\n";
  os << indent << "OutputDimensionality: " <<
    this->OutputDimensionality << "\n";
}

//----------------------------------------------------------------------------
void vtkImageResliceBase::SetOutputSpacing(double x, double y, double z)
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
void vtkImageResliceBase::SetOutputSpacingToDefault()
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
void vtkImageResliceBase::SetOutputOrigin(double x, double y, double z)
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
void vtkImageResliceBase::SetOutputOriginToDefault()
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
void vtkImageResliceBase::SetOutputExtent(int a, int b, int c, int d, int e, int f)
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
void vtkImageResliceBase::SetOutputExtentToDefault()
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
const char *vtkImageResliceBase::GetInterpolationModeAsString()
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
void vtkImageResliceBase::SetResliceAxesDirectionCosines(double x0, double x1,
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
void vtkImageResliceBase::GetResliceAxesDirectionCosines(double xdircos[3],
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
void vtkImageResliceBase::SetResliceAxesOrigin(double x, double y, double z)
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
void vtkImageResliceBase::GetResliceAxesOrigin(double origin[3])
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
unsigned long int vtkImageResliceBase::GetMTime()
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
int vtkImageResliceBase::ConvertScalarInfo(
  int &vtkNotUsed(scalarType), int &vtkNotUsed(numComponents))
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageResliceBase::ConvertScalars(
  void *inPtr, void *outPtr, int inputType, int inputComponents, int count,
  int vtkNotUsed(idX), int vtkNotUsed(idY), int vtkNotUsed(idZ),
  int vtkNotUsed(threadId))
{
  size_t bytecount = count*inputComponents;
  bytecount *= vtkDataArray::GetDataTypeSize(inputType);
  memcpy(outPtr, inPtr, bytecount);
}

//----------------------------------------------------------------------------
int vtkImageResliceBase::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // This is to be implemented by subclasses. This method simply
  // returns the whole extent of the input.

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  int wholeExtent[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wholeExtent, 6);
  this->HitInputExtent = 1;

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageResliceBase::GetAutoCroppedOutputBounds(vtkInformation *inInfo,
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
int vtkImageResliceBase::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
  inInfo->Get(vtkDataObject::SPACING(), inSpacing);
  inInfo->Get(vtkDataObject::ORIGIN(), inOrigin);

  this->InternalRequestInformation(
      request, inputVector, outputVector,
      inWholeExt, inSpacing, inOrigin);

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageResliceBase::InternalRequestInformation(
  vtkInformation *,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector,
  int inWholeExt[6], double inSpacing[3], double inOrigin[3])
{
  int i,j;
  double outSpacing[3], outOrigin[3];
  int outWholeExt[6];
  double maxBounds[6];

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

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

  this->GetIndexMatrix(inInfo, outInfo);

  this->BuildInterpolationTables();

  return 1;
}

//----------------------------------------------------------------------------
// build any tables required for the interpolation
void vtkImageResliceBase::BuildInterpolationTables()
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

vtkMatrix4x4 *vtkImageResliceBase::GetIndexMatrix(vtkInformation *inInfo,
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
void vtkImageResliceBase::ThreadedRequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector,
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

  this->InternalThreadedRequestData(
    request, inputVector, outputVector, inData, outData, outExt, threadId );
}

void vtkImageResliceBase::InternalThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***vtkNotUsed(inData),
  vtkImageData **vtkNotUsed(outData),
  int [6], int vtkNotUsed(threadId))
{
}
