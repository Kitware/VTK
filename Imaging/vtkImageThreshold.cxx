/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageThreshold.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageThreshold.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkDataSetAttributes.h"

vtkStandardNewMacro(vtkImageThreshold);

//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageThreshold::vtkImageThreshold()
{
  this->UpperThreshold = VTK_LARGE_FLOAT;
  this->LowerThreshold = -VTK_LARGE_FLOAT;
  this->ReplaceIn = 0;
  this->InValue = 0.0;
  this->ReplaceOut = 0;
  this->OutValue = 0.0;

  this->OutputScalarType = -1; // invalid; output same as input
}

//----------------------------------------------------------------------------
void vtkImageThreshold::SetInValue(double val)
{
  if (val != this->InValue || this->ReplaceIn != 1)
    {
    this->InValue = val;
    this->ReplaceIn = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageThreshold::SetOutValue(double val)
{
  if (val != this->OutValue || this->ReplaceOut != 1)
    {
    this->OutValue = val;
    this->ReplaceOut = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void vtkImageThreshold::ThresholdByUpper(double thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < VTK_LARGE_FLOAT)
    {
    this->LowerThreshold = thresh;
    this->UpperThreshold = VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void vtkImageThreshold::ThresholdByLower(double thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -VTK_LARGE_FLOAT)
    {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void vtkImageThreshold::ThresholdBetween(double lower, double upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkImageThreshold::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  if (this->OutputScalarType == -1)
    {
    // This information already copied from input to output in CopyInformationToPipeline?
    vtkInformation *inScalarInfo = vtkDataObject::GetActiveFieldInformation(inInfo, 
      vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (!inScalarInfo)
      {
      vtkErrorMacro("Missing scalar field on input information!");
      return 0;
      }
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, 
      inScalarInfo->Get( vtkDataObject::FIELD_ARRAY_TYPE() ), -1 );
    }
  else
    {
    vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
    }
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkImageThresholdExecute(vtkImageThreshold *self,
                              vtkImageData *inData,
                              vtkImageData *outData, 
                              int outExt[6], int id, IT *, OT *)
{
  vtkImageIterator<IT> inIt(inData, outExt);
  vtkImageProgressIterator<OT> outIt(outData, outExt, self, id);
  IT  lowerThreshold;
  IT  upperThreshold;
  int replaceIn = self->GetReplaceIn();
  OT  inValue;
  int replaceOut = self->GetReplaceOut();
  OT  outValue;
  IT temp;
  
  // Make sure the thresholds are valid for the input scalar range
  if (static_cast<double>(self->GetLowerThreshold())
      < inData->GetScalarTypeMin())
    {
    lowerThreshold = static_cast<IT>(inData->GetScalarTypeMin());
    }
  else 
    {
    if (static_cast<double>(self->GetLowerThreshold()) >
        inData->GetScalarTypeMax())
      {
      lowerThreshold = static_cast<IT>(inData->GetScalarTypeMax());
      }
    else
      {
      lowerThreshold = static_cast<IT>(self->GetLowerThreshold());
      }
    }
  if (static_cast<double>(self->GetUpperThreshold())
      > inData->GetScalarTypeMax())
    {
    upperThreshold = static_cast<IT>(inData->GetScalarTypeMax());
    }
  else 
    {
    if (static_cast<double>(self->GetUpperThreshold())
        < inData->GetScalarTypeMin())
      {
      upperThreshold = static_cast<IT>(inData->GetScalarTypeMin());
      }
    else
      {
      upperThreshold = static_cast<IT>(self->GetUpperThreshold());
      }
    }
  
  // Make sure the replacement values are within the output scalar range
  if (static_cast<double>(self->GetInValue()) < outData->GetScalarTypeMin())
    {
    inValue = static_cast<OT>(outData->GetScalarTypeMin());
    }
  else 
    {
    if (static_cast<double>(self->GetInValue()) > outData->GetScalarTypeMax())
      {
      inValue = static_cast<OT>(outData->GetScalarTypeMax());
      }
    else
      {
      inValue = static_cast<OT>(self->GetInValue());
      }
    }
  if (static_cast<double>(self->GetOutValue()) > outData->GetScalarTypeMax())
    {
    outValue = static_cast<OT>(outData->GetScalarTypeMax());
    }
  else 
    {
    if (static_cast<double>(self->GetOutValue()) < outData->GetScalarTypeMin())
      {
      outValue = static_cast<OT>(outData->GetScalarTypeMin());
      }
    else
      {
      outValue = static_cast<OT>(self->GetOutValue());
      }
    }
  
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

//----------------------------------------------------------------------------
template <class T>
void vtkImageThresholdExecute1(vtkImageThreshold *self,
                               vtkImageData *inData,
                               vtkImageData *outData,
                               int outExt[6], int id, T *)
{
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageThresholdExecute(self, inData,
                               outData, outExt, id, 
                               static_cast<T *>(0), 
                               static_cast<VTK_TT *>(0)));
    default:
      vtkGenericWarningMacro("Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
// This method is passed a input and output data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageThreshold::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  switch (inData[0][0]->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageThresholdExecute1(this, 
                                inData[0][0], 
                                outData[0], 
                                outExt, 
                                id,
                                static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageThreshold::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "InValue: " << this->InValue << "\n";
  os << indent << "OutValue: " << this->OutValue << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "ReplaceIn: " << this->ReplaceIn << "\n";
  os << indent << "ReplaceOut: " << this->ReplaceOut << "\n";
}
