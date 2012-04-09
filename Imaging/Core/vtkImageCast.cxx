/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCast.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCast.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageCast);

//----------------------------------------------------------------------------
vtkImageCast::vtkImageCast()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->OutputScalarType = VTK_FLOAT;
  this->ClampOverflow = 0;
}


//----------------------------------------------------------------------------
// Just change the Image type.
int vtkImageCast::RequestInformation (
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkImageCastExecute(vtkImageCast *self,
                         vtkImageData *inData,
                         vtkImageData *outData,
                         int outExt[6], int id, IT *, OT *)
{
  vtkImageIterator<IT> inIt(inData, outExt);
  vtkImageProgressIterator<OT> outIt(outData, outExt, self, id);

  double typeMin, typeMax, val;
  int clamp;

  // for preventing overflow
  typeMin = outData->GetScalarTypeMin();
  typeMax = outData->GetScalarTypeMax();
  clamp = self->GetClampOverflow();

  // Loop through ouput pixels
  while (!outIt.IsAtEnd())
    {
    IT* inSI = inIt.BeginSpan();
    OT* outSI = outIt.BeginSpan();
    OT* outSIEnd = outIt.EndSpan();
    if (clamp)
      {
      while (outSI != outSIEnd)
        {
        // Pixel operation
        val = static_cast<double>(*inSI);
        if (val > typeMax)
          {
          val = typeMax;
          }
        if (val < typeMin)
          {
          val = typeMin;
          }
        *outSI = static_cast<OT>(val);
        ++outSI;
        ++inSI;
        }
      }
    else
      {
      while (outSI != outSIEnd)
        {
        // now process the components
        *outSI = static_cast<OT>(*inSI);
        ++outSI;
        ++inSI;
        }
      }
    inIt.NextSpan();
    outIt.NextSpan();
    }
}



//----------------------------------------------------------------------------
template <class T>
void vtkImageCastExecute(vtkImageCast *self,
                         vtkImageData *inData,
                         vtkImageData *outData, int outExt[6], int id,
                         T *)
{
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageCastExecute(self,
                                         inData, outData, outExt, id,
                                         static_cast<T *>(0),
                                         static_cast<VTK_TT *>(0)));
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageCast::ThreadedExecute (vtkImageData *inData,
                                   vtkImageData *outData,
                                   int outExt[6], int id)
{
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCastExecute(this, inData,
                          outData, outExt, id,
                          static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageCast::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "ClampOverflow: ";
  if (this->ClampOverflow)
    {
    os << "On\n";
    }
  else
    {
    os << "Off\n";
    }
}

