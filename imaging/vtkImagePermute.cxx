/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePermute.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageCache.h"
#include "vtkImagePermute.h"

//----------------------------------------------------------------------------
vtkImagePermute::vtkImagePermute()
{
  this->FilteredAxes[0] = 0;
  this->FilteredAxes[1] = 1;
  this->FilteredAxes[2] = 2;
}

//----------------------------------------------------------------------------
void vtkImagePermute::ExecuteImageInformation() 
{
  int idx, axis;
  int ext[6];
  float spacing[3];
  float origin[3];
  float *inOrigin;
  float *inSpacing;
  int *inExt;
  
  inExt = this->Input->GetWholeExtent();
  inSpacing = this->Input->GetSpacing();
  inOrigin = this->Input->GetOrigin();
  
  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->FilteredAxes[idx];
    origin[idx] = inOrigin[axis];
    spacing[idx] = inSpacing[axis];
    ext[idx*2] = inExt[axis*2];
    ext[idx*2+1] = inExt[axis*2+1];
    }
  
  this->Output->SetWholeExtent(ext);
  this->Output->SetSpacing(spacing);
  this->Output->SetOrigin(origin);
}


//----------------------------------------------------------------------------
void vtkImagePermute::ComputeRequiredInputUpdateExtent(int inExt[6], 
						       int outExt[6])
{
  int idx, axis;

  for (idx = 0; idx < 3; ++idx)
    {
    axis = this->FilteredAxes[idx];
    inExt[axis*2] = outExt[idx*2];
    inExt[axis*2+1] = outExt[idx*2+1];
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImagePermuteExecute(vtkImagePermute *self,
				   vtkImageData *inData, T *inPtr,
				   vtkImageData *outData, T *outPtr,
				   int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inInc[3];
  int inInc0, inInc1, inInc2;
  int outIncX, outIncY, outIncZ;
  T *inPtr0, *inPtr1, *inPtr2;
  int scalarSize;
  unsigned long count = 0;
  unsigned long target;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0]; 
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outIncX = inData->GetNumberOfScalarComponents();
  scalarSize = sizeof(T)*outIncX;
  
  // adjust the increments for the permute
  int *fe = self->GetFilteredAxes();
  inInc[0] = inInc0;
  inInc[1] = inInc1;
  inInc[2] = inInc2;
  inInc0 = inInc[fe[0]];
  inInc1 = inInc[fe[1]];
  inInc2 = inInc[fe[2]];
    
  // Loop through ouput pixels
  inPtr2 = inPtr;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtr1 = inPtr2;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      inPtr0 = inPtr1;
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// Pixel operation
	memcpy((void *)outPtr,(void *)inPtr0,scalarSize);
	outPtr += outIncX;
	inPtr0 += inInc0;
	}
      outPtr += outIncY;
      inPtr1 += inInc1;
      }
    outPtr += outIncZ;
    inPtr2 += inInc2;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImagePermute::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  int inExt[6];
  
  this->ComputeRequiredInputUpdateExtent(inExt,outExt);
  
  void *inPtr = inData->GetScalarPointerForExtent(inExt);
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
    case VTK_FLOAT:
      vtkImagePermuteExecute(this, inData, (float *)(inPtr), 
			     outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_INT:
      vtkImagePermuteExecute(this, inData, (int *)(inPtr), 
			     outData, (int *)(outPtr),outExt, id);
      break;
    case VTK_SHORT:
      vtkImagePermuteExecute(this, inData, (short *)(inPtr), 
			     outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePermuteExecute(this, inData, (unsigned short *)(inPtr), 
			     outData, (unsigned short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePermuteExecute(this, inData, (unsigned char *)(inPtr), 
			     outData, (unsigned char *)(outPtr),outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}














