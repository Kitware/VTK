/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCast.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCast.h"
#include "vtkObjectFactory.h"
#include "vtkImageProgressIterator.h"


vtkCxxRevisionMacro(vtkImageCast, "1.43");
vtkStandardNewMacro(vtkImageCast);

//----------------------------------------------------------------------------
vtkImageCast::vtkImageCast()
{
  this->OutputScalarType = VTK_FLOAT;
  this->ClampOverflow = 0;
}


//----------------------------------------------------------------------------
// Just change the Image type.
void vtkImageCast::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
                                      vtkImageData *outData)
{
  outData->SetScalarType(this->OutputScalarType);
}

//----------------------------------------------------------------------------
// The update method first checks to see is a cast is necessary.
void vtkImageCast::UpdateData(vtkDataObject *data)
{
  
  if (! this->GetInput() || ! this->GetOutput())
    {
    vtkErrorMacro("Update: Input or output is not set.");
    return;
    }
  
  // call the superclass update which will cause an execute.
  this->vtkImageToImageFilter::UpdateData(data);
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
    vtkTemplateMacro7(vtkImageCastExecute, self, 
                      inData, outData, outExt, id,
                      static_cast<T *>(0), static_cast<VTK_TT *>(0));
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
void vtkImageCast::ThreadedExecute(vtkImageData *inData, 
                                   vtkImageData *outData,
                                   int outExt[6], int id)
{
  vtkDebugMacro(<< "Execute: inData = " << inData 
                << ", outData = " << outData);
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageCastExecute, this, inData, 
                      outData, outExt, id, static_cast<VTK_TT *>(0));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

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

