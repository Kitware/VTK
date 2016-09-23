/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMaskBits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageMaskBits.h"

#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkObjectFactory.h"

#include <cmath>

vtkStandardNewMacro(vtkImageMaskBits);

vtkImageMaskBits::vtkImageMaskBits()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Operation = VTK_AND;
  this->Masks[0] = 0xffffffff;
  this->Masks[1] = 0xffffffff;
  this->Masks[2] = 0xffffffff;
  this->Masks[3] = 0xffffffff;
}



//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values
// out of extent.
template <class T>
void vtkImageMaskBitsExecute(vtkImageMaskBits *self,
                             vtkImageData *inData,
                             vtkImageData *outData,
                             int outExt[6], int id, T *)
{
  vtkImageIterator<T> inIt(inData, outExt);
  vtkImageProgressIterator<T> outIt(outData, outExt, self, id);
  int idxC, maxC;
  unsigned int *masks;
  int operation;

  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  masks = self->GetMasks();
  operation = self->GetOperation();

  // Loop through output pixels
  while (!outIt.IsAtEnd())
  {
    T* inSI = inIt.BeginSpan();
    T* outSI = outIt.BeginSpan();
    T* outSIEnd = outIt.EndSpan();
    switch (operation)
    {
      case VTK_AND:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ & static_cast<T>(masks[idxC]);
          }
        }
        break;
      case VTK_OR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ | static_cast<T>(masks[idxC]);
          }
        }
        break;
      case VTK_XOR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = *inSI++ ^ static_cast<T>(masks[idxC]);
          }
        }
        break;
      case VTK_NAND:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = ~(*inSI++ & static_cast<T>(masks[idxC]));
          }
        }
        break;
      case VTK_NOR:
        while (outSI != outSIEnd)
        {
          for (idxC = 0; idxC < maxC; idxC++)
          {
            // Pixel operation
            *outSI++ = ~(*inSI++ | static_cast<T>(masks[idxC]));
          }
        }
        break;
    }
    inIt.NextSpan();
    outIt.NextSpan();
  }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageMaskBits::ThreadedExecute (vtkImageData *inData,
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
  {
    vtkErrorMacro(<< "Execute: input ScalarType, "
                  << inData->GetScalarType()
                  << ", must match out ScalarType "
                  << outData->GetScalarType());
    return;
  }

  switch (inData->GetScalarType())
  {
    case VTK_INT:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<int *>(0));
      break;
    case VTK_UNSIGNED_INT:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<unsigned int *>(0));
      break;
    case VTK_LONG:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<long *>(0));
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<unsigned long *>(0));
      break;
    case VTK_SHORT:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<short *>(0));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<unsigned short *>(0));
      break;
    case VTK_CHAR:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<char *>(0));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMaskBitsExecute(this, inData, outData, outExt, id,
                              static_cast<unsigned char *>(0));
      break;
    default:
      vtkErrorMacro(<< "Execute: ScalarType can only be [unsigned] char, [unsigned] short, "
      << "[unsigned] int, or [unsigned] long.");
      return;
  }
}


void vtkImageMaskBits::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Operation: " << this->Operation << "\n";
  os << indent << "Masks: ("
     << this->Masks[0] << ", " << this->Masks[1] << ", "
     << this->Masks[2] << ", " << this->Masks[3] << ")" << endl;
}
