/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLuminance.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>

#include "vtkImageLuminance.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageLuminance* vtkImageLuminance::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageLuminance");
  if(ret)
    {
    return (vtkImageLuminance*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageLuminance;
}





//----------------------------------------------------------------------------
// This method overrides information set by parent's ExecuteInformation.
void vtkImageLuminance::ExecuteInformation(vtkImageData *vtkNotUsed(inData), 
					   vtkImageData *outData)
{
  outData->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
// This execute method handles boundaries.
// it handles boundaries. Pixels are just replicated to get values 
// out of extent.
template <class T>
static void vtkImageLuminanceExecute(vtkImageLuminance *self,
					     vtkImageData *inData, T *inPtr,
					     vtkImageData *outData, T *outPtr,
					     int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  float luminance;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

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
      for (idxX = 0; idxX <= maxX; idxX++)
	{
        luminance =  0.30 * *inPtr++;
        luminance += 0.59 * *inPtr++;
	luminance += 0.11 * *inPtr++;
	*outPtr = (T)(luminance);
	outPtr++;
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
void vtkImageLuminance::ThreadedExecute(vtkImageData *inData, 
					vtkImageData *outData,
					int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetNumberOfScalarComponents() != 3)
    {
    vtkErrorMacro(<< "Execute: input must have 3 components, but has " << inData->GetNumberOfScalarComponents());
    return;
    }
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    case VTK_DOUBLE:
      vtkImageLuminanceExecute(this, 
			       inData, (double *)(inPtr), 
			       outData, (double *)(outPtr), outExt, id);
      break;
    case VTK_FLOAT:
      vtkImageLuminanceExecute(this, 
			       inData, (float *)(inPtr), 
			       outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_LONG:
      vtkImageLuminanceExecute(this, 
			       inData, (long *)(inPtr), 
			       outData, (long *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_LONG:
      vtkImageLuminanceExecute(this, 
			       inData, (unsigned long *)(inPtr), 
			       outData, (unsigned long *)(outPtr), 
			       outExt, id);
      break;
    case VTK_INT:
      vtkImageLuminanceExecute(this, 
			       inData, (int *)(inPtr), 
			       outData, (int *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_INT:
      vtkImageLuminanceExecute(this, 
			       inData, (unsigned int *)(inPtr), 
			       outData, (unsigned int *)(outPtr), 
			       outExt, id);
      break;
    case VTK_SHORT:
      vtkImageLuminanceExecute(this, 
			       inData, (short *)(inPtr), 
			       outData, (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageLuminanceExecute(this, 
			       inData, (unsigned short *)(inPtr), 
			       outData, (unsigned short *)(outPtr), 
			       outExt, id);
      break;
    case VTK_CHAR:
      vtkImageLuminanceExecute(this, 
			       inData, (char *)(inPtr), 
			       outData, (char *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageLuminanceExecute(this, 
			       inData, (unsigned char *)(inPtr), 
			       outData, (unsigned char *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}










