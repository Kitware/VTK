/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAppendComponents.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include "vtkImageAppendComponents.h"



//----------------------------------------------------------------------------
// Description:
// This method tells the ouput it will have more components
void vtkImageAppendComponents::ExecuteImageInformation()
{
  int idx1, num;

  num = 0;
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (this->Inputs[idx1] != NULL)
      {
      num += this->Inputs[idx1]->GetNumberOfScalarComponents();
      }
    }
  this->Output->SetNumberOfScalarComponents(num);
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
					    vtkImageData *inData, 
					    T *inPtrP, int inComp,
					    vtkImageData *outData, 
					    T *outPtrP, int outComp,
					    int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  T *outPtr, *inPtr;
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)(outData->GetNumberOfScalarComponents()*
			   (maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  inIncX = inData->GetNumberOfScalarComponents();
  outIncX = outData->GetNumberOfScalarComponents();
  

  outPtr = outPtrP + outComp;
  inPtr = inPtrP + inComp;
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) 
	  {
	  self->UpdateProgress(count/(50.0*target) 
			       + (maxZ+1)*(maxY+1)*outComp);
	  }
	count++;
	}
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// Pixel operation
	*outPtr = *inPtr;
	outPtr += outIncX;
	inPtr += inIncX;
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppendComponents::ThreadedExecute(vtkImageData **inData, 
					       vtkImageData *outData,
					       int outExt[6], int id)
{
  int idx1, inComp, outComp, num;
  void *inPtr;
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  outComp = -1;
  for (idx1 = 0; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {
      inPtr = inData[idx1]->GetScalarPointerForExtent(outExt);
      num = inData[idx1]->GetNumberOfScalarComponents();
      // inefficient to have this loop here (could be inner loop)
      for (inComp = 0; inComp < num; ++inComp)
	{
	++outComp;
	// this filter expects that input is the same type as output.
	if (inData[idx1]->GetScalarType() != outData->GetScalarType())
	  {
	  vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << 
	  inData[idx1]->GetScalarType() << 
	  "), must match output ScalarType (" << outData->GetScalarType() 
	  << ")");
	  return;
	  }
	
	switch (inData[idx1]->GetScalarType())
	  {
	  case VTK_FLOAT:
	    vtkImageAppendComponentsExecute(this, inData[idx1],
				    (float *)(inPtr), inComp,
				    outData, (float *)(outPtr), 
				    outComp, outExt, id);
	    break;
	  case VTK_INT:
	    vtkImageAppendComponentsExecute(this, inData[idx1], 
				    (int *)(inPtr), inComp,
				    outData, (int *)(outPtr),
				    outComp, outExt, id);
	    break;
	  case VTK_SHORT:
	    vtkImageAppendComponentsExecute(this, inData[idx1], 
				    (short *)(inPtr), inComp,
				    outData, (short *)(outPtr), 
				    outComp, outExt, id);
	    break;
	  case VTK_UNSIGNED_SHORT:
	    vtkImageAppendComponentsExecute(this, inData[idx1], 
				    (unsigned short *)(inPtr), inComp,
				    outData, (unsigned short *)(outPtr), 
				    outComp, outExt, id);
	    break;
	  case VTK_UNSIGNED_CHAR:
	    vtkImageAppendComponentsExecute(this, inData[idx1], 
				    (unsigned char *)(inPtr), inComp,
				    outData, (unsigned char *)(outPtr),
				    outComp, outExt, id);
	    break;
	  default:
	    vtkErrorMacro(<< "Execute: Unknown ScalarType");
	    return;
	  }
	}
      }
    }
  
}
















