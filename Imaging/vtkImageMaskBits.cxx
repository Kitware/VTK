/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMaskBits.cxx
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
#include "vtkImageMaskBits.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkCxxRevisionMacro(vtkImageMaskBits, "1.12");
vtkStandardNewMacro(vtkImageMaskBits);

vtkImageMaskBits::vtkImageMaskBits()
{
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
static void vtkImageMaskBitsExecute(vtkImageMaskBits *self,
                                             vtkImageData *inData, T *inPtr,
                                             vtkImageData *outData, T *outPtr,
                                             int outExt[6], int id)
{
  int idxX, idxY, idxZ, idxC;
  int maxX, maxY, maxZ, maxC;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  unsigned int *masks;
  int operation;
  
  // find the region to loop over
  maxC = inData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  masks = self->GetMasks();
  operation = self->GetOperation();
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!id) 
       {
       if (!(count%target))
         {
         self->UpdateProgress(count/(50.0*target));
         }
       count++;
       }
      switch (operation)
         {
         case VTK_AND:
           for (idxX = 0; idxX <= maxX; idxX++)
             {
             for (idxC = 0; idxC < maxC; idxC++)
               {
               // Pixel operation
               *outPtr++ = *inPtr++ & (T) masks[idxC];
               }
             }
           break;
         case VTK_OR:
           for (idxX = 0; idxX <= maxX; idxX++)
             {
             for (idxC = 0; idxC < maxC; idxC++)
               {
               // Pixel operation
               *outPtr++ = *inPtr++ | (T) masks[idxC];
               }
             }
           break;
         case VTK_XOR:
           for (idxX = 0; idxX <= maxX; idxX++)
             {
             for (idxC = 0; idxC < maxC; idxC++)
               {
               // Pixel operation
               *outPtr++ = *inPtr++ ^ (T) masks[idxC];
               }
             }
           break;
         case VTK_NAND:
           for (idxX = 0; idxX <= maxX; idxX++)
             {
             for (idxC = 0; idxC < maxC; idxC++)
               {
               // Pixel operation
               *outPtr++ = ~(*inPtr++ & (T) masks[idxC]);
               }
             }
           break;
         case VTK_NOR:
           for (idxX = 0; idxX <= maxX; idxX++)
             {
             for (idxC = 0; idxC < maxC; idxC++)
               {
               // Pixel operation
               *outPtr++ = ~(*inPtr++ | (T) masks[idxC]);
               }
             }
           break;
      }    
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The output data
// must match input type.  This method does handle boundary conditions.
void vtkImageMaskBits::ThreadedExecute(vtkImageData *inData, 
                                        vtkImageData *outData,
                                        int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  switch (inData->GetScalarType())
    {
    case VTK_INT:
      vtkImageMaskBitsExecute(this, 
                               inData, (int *)(inPtr), 
                               outData, (int *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageMaskBitsExecute(this, 
                               inData, (unsigned int *)(inPtr), 
                               outData, (unsigned int *)(outPtr), outExt, id);
      break;
    case VTK_LONG:
      vtkImageMaskBitsExecute(this, 
                               inData, (long *)(inPtr), 
                               outData, (long *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageMaskBitsExecute(this, 
                               inData, (unsigned long *)(inPtr), 
                               outData, (unsigned long *)(outPtr), outExt, id);
      break;
    case VTK_SHORT:
      vtkImageMaskBitsExecute(this, 
                               inData, (short *)(inPtr), 
                               outData, (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMaskBitsExecute(this, 
                               inData, (unsigned short *)(inPtr), 
                               outData, (unsigned short *)(outPtr), 
                               outExt, id);
      break;
    case VTK_CHAR:
      vtkImageMaskBitsExecute(this, 
                               inData, (char *)(inPtr), 
                               outData, (char *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMaskBitsExecute(this, 
                               inData, (unsigned char *)(inPtr), 
                               outData, (unsigned char *)(outPtr), outExt, id);
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
  os << indent << "Masks: (" << this->Masks[0] << ", " << this->Masks[1] << ", "
     << this->Masks[2] << ", " << this->Masks[3] << ")" << endl;
}








