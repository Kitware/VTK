/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBSplineCoefficients.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageBSplineCoefficients.h"
#include "vtkImageBSplineInternals.h"

#include "vtkMath.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTemplateAliasMacro.h"

#include <cstddef>

vtkStandardNewMacro(vtkImageBSplineCoefficients);

//----------------------------------------------------------------------------
vtkImageBSplineCoefficients::vtkImageBSplineCoefficients()
{
  this->SplineDegree = 3;
  this->BorderMode = VTK_IMAGE_BORDER_CLAMP;
  this->OutputScalarType = VTK_FLOAT;
  this->Bypass = 0;
  this->DataWasPassed = 0;
  this->Iteration = 0;
}

//----------------------------------------------------------------------------
vtkImageBSplineCoefficients::~vtkImageBSplineCoefficients()
{
}

//----------------------------------------------------------------------------
void vtkImageBSplineCoefficients::AllocateOutputData(
  vtkImageData *vtkNotUsed(output), vtkInformation *vtkNotUsed(outInfo), int *vtkNotUsed(uExtent))
{
  // turn into a no-op, we allocate our output manually
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageBSplineCoefficients::AllocateOutputData(
  vtkDataObject *output, vtkInformation *vtkNotUsed(outInfo))
{
  // turn into a no-op, we allocate our output manually
  vtkImageData *out = vtkImageData::SafeDownCast(output);
  return out;
}

//----------------------------------------------------------------------------
int vtkImageBSplineCoefficients::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->Bypass)
  {
    // directly pass the scalars to the output
    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
    return 1;
  }
  else if (this->DataWasPassed)
  {
    // force reallocation of the scalars
    outData->GetPointData()->SetScalars(NULL);
    this->DataWasPassed = 0;
  }

  // Allocate the output data
  outData->SetExtent(outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(outInfo);

  if (outData->GetScalarType() != VTK_FLOAT &&
      outData->GetScalarType() != VTK_DOUBLE)
  {
    vtkErrorMacro(<< "Execute: output data must be be type float or double.");
    return 0;
  }

  // copy the data to the output
  int ie[6], oe[6];
  inData->GetExtent(ie);
  outData->GetExtent(oe);
  if (ie[0] == oe[0] && ie[1] == oe[1] && ie[2] == oe[2] &&
      ie[3] == oe[3] && ie[4] == oe[4] && ie[5] == oe[5])
  {
    outData->GetPointData()->GetScalars()->DeepCopy(
      inData->GetPointData()->GetScalars());
  }
  else
  {
    vtkErrorMacro(<< "Execute: input and output extents do not match: "
                  << "(" << ie[0] << "," << ie[1] << "," << ie[2] << ","
                  << ie[3] << "," << ie[4] << "," << ie[5] << ") vs. "
                  << "(" << oe[0] << "," << oe[1] << "," << oe[2] << ","
                  << oe[3] << "," << oe[4] << "," << oe[5] << ")");
    return 0;
  }

  // if spline degree is < 2, no operation is required
  if (this->SplineDegree < 2)
  {
    return 1;
  }

  // We are about to call superclass' RequestData which allocates output
  // based on the update extent. However, we want the output to be the
  // whole extent. So we temprarily override the update extent to be
  // the whole extent.
  int extentcache[6];
  memcpy(extentcache, outInfo->Get(
           vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),
         6*sizeof(int));
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  // execute over the three directions
  for (int i = 0; i < 3; i++)
  {
    this->Iteration = i;

    // ensure that iteration axis is not split during threaded execution
    this->SplitPathLength = 0;
    for (int axis = 2; axis >= 0; --axis)
    {
      if (axis != i)
      {
        this->SplitPath[this->SplitPathLength++] = axis;
      }
    }

    if (ie[2*i+1] > ie[2*i])
    {
      if (!this->vtkThreadedImageAlgorithm::RequestData(
            request, &outputVector, outputVector))
      {
        return 0;
      }
    }
  }

  // Restore update extent
  outInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               extentcache,
               6);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageBSplineCoefficients::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int numComponents = 1;
  int scalarType = VTK_FLOAT;

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

  if (this->Bypass)
  {
    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, scalarType, numComponents);
  }
  else if (this->OutputScalarType == VTK_DOUBLE)
  {
    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, VTK_DOUBLE, numComponents);
  }
  else
  {
    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, VTK_FLOAT, numComponents);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkImageBSplineCoefficients::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int extent[6];

  if (this->Bypass)
  {
    // in bypass mode, just pass the update extent
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  }
  else
  {
    // the whole input extent is required every time
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void vtkImageBSplineCoefficientsExecute(
  vtkImageBSplineCoefficients* self,
  vtkImageData* inData, vtkImageData* outData, T *inPtr, T *outPtr,
  int extent[6], int axis, int threadId)
{
  // change the order so the inner loop is the chosen axis
  static int permute[3][3] = {
    { 0, 1, 2 }, { 1, 0, 2 }, { 2, 0, 1 } };

  int borderMode = self->GetBorderMode();

  int inExtent[6];
  inData->GetExtent(inExtent);
  int inMin0 = inExtent[2*permute[axis][0]];
  int inMax0 = inExtent[2*permute[axis][0] + 1];

  int outMin0 = extent[2*permute[axis][0]];
  int outMax0 = extent[2*permute[axis][0] + 1];
  int outMin1 = extent[2*permute[axis][1]];
  int outMax1 = extent[2*permute[axis][1] + 1];
  int outMin2 = extent[2*permute[axis][2]];
  int outMax2 = extent[2*permute[axis][2] + 1];

  vtkIdType inInc[6];
  inData->GetIncrements(inInc);
  vtkIdType inInc0 = inInc[permute[axis][0]];
  vtkIdType inInc1 = inInc[permute[axis][1]];
  vtkIdType inInc2 = inInc[permute[axis][2]];

  vtkIdType outInc[6];
  outData->GetIncrements(outInc);
  vtkIdType outInc0 = outInc[permute[axis][0]];
  vtkIdType outInc1 = outInc[permute[axis][1]];
  vtkIdType outInc2 = outInc[permute[axis][2]];

  int numscalars = outData->GetNumberOfScalarComponents();

  // for progress reporting
  unsigned long count = 0;
  unsigned long target = static_cast<unsigned long>(
    0.02*(outMax2-outMin2+1)*(outMax1-outMin1+1));
  target++;

  // Get the poles for the spline
  double poles[4];
  long numPoles;
  vtkImageBSplineInternals::GetPoleValues(
    poles, numPoles, self->GetSplineDegree());

  // allocate workspace for one row
  double* image = new double[inMax0 - inMin0 + 1];

  // loop over all the extra axes
  T *inPtr2 = inPtr - (outMin0 - inMin0)*inInc0;
  T *outPtr2 = outPtr;
  for (int idx2 = outMin2; idx2 <= outMax2; idx2++)
  {
    T *inPtr1 = inPtr2;
    T *outPtr1 = outPtr2;
    for (int idx1 = outMin1; !self->AbortExecute && idx1 <= outMax1; idx1++)
    {
      if (threadId == 0 && count % target == 0)
      {
        self->UpdateProgress((axis + count/(50.0*target))/3.0);
      }
      count++;

      // loop over components
      for (int idxC = 0; idxC < numscalars; idxC++)
      {
        T *inPtr0 = inPtr1 + idxC;
        T *outPtr0 = outPtr1 + idxC;

        double *imagePtr = image;
        for (int jdx0 = inMin0; jdx0 <= inMax0; jdx0++)
        {
          *imagePtr++ = static_cast<double>(*inPtr0);
          inPtr0 += inInc0;
        }

        // Call the code that generates the b-spline knots,
        vtkImageBSplineInternals::ConvertToInterpolationCoefficients(
          image, inMax0 - inMin0 + 1, borderMode, poles, numPoles,
          VTK_DBL_EPSILON);

        // Copy to output
        imagePtr = image + (outMin0 - inMin0);
        for (int idx0 = outMin0; idx0 <= outMax0; idx0++)
        {
          *outPtr0 = *imagePtr++;
          outPtr0 += outInc0;
        }
      }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
    }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
  }

  delete [] image;
}

//----------------------------------------------------------------------------
// This is called three times (once per dimension)
void vtkImageBSplineCoefficients::ThreadedExecute(
  vtkImageData *inData, vtkImageData *outData, int outExt[6], int threadId)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  if (outData->GetScalarType() == VTK_FLOAT)
  {
    vtkImageBSplineCoefficientsExecute(
      this, inData, outData,
      static_cast<float*>(inPtr), static_cast<float*>(outPtr),
      outExt, this->Iteration, threadId);
  }
  else if (outData->GetScalarType() == VTK_DOUBLE)
  {
    vtkImageBSplineCoefficientsExecute(
      this, inData, outData,
      static_cast<double*>(inPtr), static_cast<double*>(outPtr),
      outExt, this->Iteration, threadId);
  }
}

//----------------------------------------------------------------------------
void vtkImageBSplineCoefficients::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "SplineDegree: " << this->SplineDegree << "\n";
  os << "BorderMode: " << this->GetBorderModeAsString() << "\n";
  os << "OutputScalarType: " << this->GetOutputScalarTypeAsString() << "\n";
  os << "Bypass: " << (this->Bypass ? "On\n" : "Off\n" );
}

//----------------------------------------------------------------------------
const char *vtkImageBSplineCoefficients::GetBorderModeAsString()
{
  switch (this->BorderMode)
  {
    case VTK_IMAGE_BORDER_CLAMP:
      return "Clamp";
    case VTK_IMAGE_BORDER_MIRROR:
      return "Mirror";
    case VTK_IMAGE_BORDER_REPEAT:
      return "Repeat";
    default:
      break;
  }

  return "Unknown";
}

//----------------------------------------------------------------------------
const char *vtkImageBSplineCoefficients::GetOutputScalarTypeAsString()
{
  return vtkImageScalarTypeNameMacro(this->OutputScalarType);
}

//----------------------------------------------------------------------------
int vtkImageBSplineCoefficients::CheckBounds(const double point[3])
{
  const double *bounds = this->GetOutput()->GetBounds();
  for (int i = 0; i < 3; i++)
  {
    double a = bounds[0];
    double b = bounds[1];
    if ((b - a) > 1e-16 && (point[i] < a || point[i] > b))
    {
      return 0;
    }
    bounds++;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkImageBSplineCoefficients::Evaluate(const double p[3], double *val)
{
  vtkImageData *output = this->GetOutput();
  int extent[6];
  double spacing[3], origin[3];
  output->GetExtent(extent);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  int width = extent[1] - extent[0] + 1;
  int height = extent[3] - extent[2] + 1;
  int slices = extent[5] - extent[4] + 1;
  double x = (p[0] - origin[0])/spacing[0] - extent[0];
  double y = (p[1] - origin[1])/spacing[1] - extent[2];
  double z = (p[2] - origin[2])/spacing[2] - extent[4];

  if (width < 1 || height < 1 || slices < 1)
  {
    vtkErrorMacro("Called Evaluate() on empty extent");
    return;
  }

  int numscalars = output->GetNumberOfScalarComponents();
  int scalarType = output->GetScalarType();

  if (scalarType == VTK_FLOAT)
  {
    float *coeffs = static_cast<float *>(output->GetScalarPointer());
    float value4[4];
    float *value = value4;
    if (numscalars > 4)
    {
      value = new float[numscalars];
    }

    vtkImageBSplineInternals::InterpolatedValue(
      coeffs, value, width, height, slices, numscalars, x, y, z,
      this->SplineDegree, this->BorderMode);

    for (int i = 0; i < numscalars; i++)
    {
      val[i] = value[i];
    }

    if (value != value4)
    {
      delete [] value;
    }
  }
  else if (scalarType == VTK_DOUBLE)
  {
    double *coeffs = static_cast<double *>(output->GetScalarPointer());
    vtkImageBSplineInternals::InterpolatedValue(
      coeffs, val, width, height, slices, numscalars, x, y, z,
      this->SplineDegree, this->BorderMode);
  }
  else
  {
    vtkErrorMacro("Called Evaluate(), but data is not float or double.");
  }
}

//----------------------------------------------------------------------------
double vtkImageBSplineCoefficients::Evaluate(double x, double y, double z)
{
  vtkImageData *output = this->GetOutput();
  int extent[6];
  double spacing[3], origin[3];
  output->GetExtent(extent);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  int width = extent[1] - extent[0] + 1;
  int height = extent[3] - extent[2] + 1;
  int slices = extent[5] - extent[4] + 1;
  x = (x - origin[0])/spacing[0] - extent[0];
  y = (y - origin[1])/spacing[1] - extent[2];
  z = (z - origin[2])/spacing[2] - extent[4];

  if (width < 1 || height < 1 || slices < 1)
  {
    vtkErrorMacro("Called Evaluate() on empty extent");
    return 0.0;
  }

  int numscalars = output->GetNumberOfScalarComponents();
  int scalarType = output->GetScalarType();

  if (scalarType == VTK_FLOAT)
  {
    float *coeffs = static_cast<float *>(output->GetScalarPointer());
    float value4[4];
    float *value = value4;
    if (numscalars > 4)
    {
      value = new float[numscalars];
    }

    vtkImageBSplineInternals::InterpolatedValue(
      coeffs, value, width, height, slices, numscalars, x, y, z,
      this->SplineDegree, this->BorderMode);

    if (value != value4)
    {
      value4[0] = value[0];
      delete [] value;
      value = value4;
    }

    return value[0];
  }
  else if (scalarType == VTK_DOUBLE)
  {
    double *coeffs = static_cast<double *>(output->GetScalarPointer());
    double value4[4];
    double *value = value4;
    if (numscalars > 4)
    {
      value = new double[numscalars];
    }

    vtkImageBSplineInternals::InterpolatedValue(
      coeffs, value, width, height, slices, numscalars, x, y, z,
      this->SplineDegree, this->BorderMode);

    if (value != value4)
    {
      value4[0] = value[0];
      delete [] value;
      value = value4;
    }

    return value[0];
  }
  else
  {
    vtkErrorMacro("Called Evaluate(), but data is not float or double.");
  }

  return 0;
}
