// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkImageBinaryThreshold.h"

#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkImageBinaryThreshold);

namespace
{

template <typename T>
double ClampToImageScalarTypeMinMax(double value, vtkImageData* image)
{
  double clampedValue = std::clamp(value, image->GetScalarTypeMin(), image->GetScalarTypeMax());
  return static_cast<T>(clampedValue);
}

}

//------------------------------------------------------------------------------
int vtkImageBinaryThreshold::RequestInformation(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (this->OutputScalarType == -1)
  {
    vtkInformation* inScalarInfo = vtkDataObject::GetActiveFieldInformation(
      inInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (!inScalarInfo)
    {
      vtkErrorMacro("Missing scalar field on input information!");
      return 0;
    }
    vtkDataObject::SetPointDataActiveScalarInfo(
      outInfo, inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE()), -1);
  }
  else
  {
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  }
  return 1;
}

//------------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkImageThresholdExecute(vtkImageBinaryThreshold* self, vtkImageData* inData,
  vtkImageData* outData, int outExt[6], int id, IT*, OT*)
{
  vtkImageIterator<IT> inIt(inData, outExt);
  vtkImageProgressIterator<OT> outIt(outData, outExt, self, id);

  int thresholdFunction = self->GetThresholdFunction();
  double filterLowerThreshold =
    thresholdFunction == vtkImageBinaryThreshold::ThresholdFunction::THRESHOLD_LOWER
    ? VTK_FLOAT_MIN
    : self->GetLowerThreshold();
  double filterUpperThreshold =
    thresholdFunction == vtkImageBinaryThreshold::ThresholdFunction::THRESHOLD_UPPER
    ? VTK_FLOAT_MAX
    : self->GetUpperThreshold();
  IT lowerThreshold = ::ClampToImageScalarTypeMinMax<IT>(filterLowerThreshold, inData);
  IT upperThreshold = ::ClampToImageScalarTypeMinMax<IT>(filterUpperThreshold, inData);

  OT inValue = ::ClampToImageScalarTypeMinMax<OT>(self->GetInValue(), outData);
  OT outValue = ::ClampToImageScalarTypeMinMax<OT>(self->GetOutValue(), outData);
  int replaceIn = self->GetReplaceIn();
  int replaceOut = self->GetReplaceOut();
  IT temp;

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    IT* inSI = inIt.BeginSpan();
    OT* outSI = outIt.BeginSpan();
    OT* outSIEnd = outIt.EndSpan();
    while (outSI != outSIEnd)
    {
      // Pixel operation
      temp = (*inSI);
      if (lowerThreshold <= temp && temp <= upperThreshold)
      {
        // match
        if (replaceIn)
        {
          *outSI = inValue;
        }
        else
        {
          *outSI = static_cast<OT>(temp);
        }
      }
      else
      {
        // not match
        if (replaceOut)
        {
          *outSI = outValue;
        }
        else
        {
          *outSI = static_cast<OT>(temp);
        }
      }
      ++inSI;
      ++outSI;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}

//------------------------------------------------------------------------------
template <class T>
void vtkImageThresholdExecute1(vtkImageBinaryThreshold* self, vtkImageData* inData,
  vtkImageData* outData, int outExt[6], int id, T*)
{
  switch (outData->GetScalarType())
  {
    vtkTemplateMacro(vtkImageThresholdExecute(
      self, inData, outData, outExt, id, static_cast<T*>(nullptr), static_cast<VTK_TT*>(nullptr)));
    default:
      vtkGenericWarningMacro("Execute: Unknown input ScalarType");
      return;
  }
}

//------------------------------------------------------------------------------
void vtkImageBinaryThreshold::ThreadedRequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* vtkNotUsed(outputVector),
  vtkImageData*** inData, vtkImageData** outData, int outExt[6], int id)
{
  switch (inData[0][0]->GetScalarType())
  {
    vtkTemplateMacro(vtkImageThresholdExecute1(
      this, inData[0][0], outData[0], outExt, id, static_cast<VTK_TT*>(nullptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
  }
}

//------------------------------------------------------------------------------
void vtkImageBinaryThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  std::string thresholdFunctionStr;
  switch (this->ThresholdFunction)
  {
    case ThresholdFunction::THRESHOLD_BETWEEN:
      thresholdFunctionStr = "THRESHOLD_BETWEEN";
      break;
    case ThresholdFunction::THRESHOLD_LOWER:
      thresholdFunctionStr = "THRESHOLD_LOWER";
      break;
    case ThresholdFunction::THRESHOLD_UPPER:
      thresholdFunctionStr = "THRESHOLD_UPPER";
      break;
  }

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "ThresholdFunction: " << thresholdFunctionStr << "\n";
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "ReplaceIn: " << this->ReplaceIn << "\n";
  os << indent << "ReplaceOut: " << this->ReplaceOut << "\n";
}
VTK_ABI_NAMESPACE_END
