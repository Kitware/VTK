/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMathematics.cxx
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
#include "vtkImageData.h"
#include "vtkImageMathematics.h"
#include <math.h>



//----------------------------------------------------------------------------
vtkImageMathematics::vtkImageMathematics()
{
  this->Operation = VTK_ADD;
}



//----------------------------------------------------------------------------
// Description:
// The output extent is the intersection.
void vtkImageMathematics::ExecuteImageInformation()
{
  int ext[6], *ext2, idx;

  this->Inputs[0]->GetWholeExtent(ext);
  // two input take intersection
  if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT || 
      this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
      this->Operation == VTK_MIN || this->Operation == VTK_MAX) 
    {
    ext2 = this->Inputs[1]->GetWholeExtent();
    for (idx = 0; idx < 3; ++idx)
      {
      if (ext2[idx*2] > ext[idx*2])
	{
	ext[idx*2] = ext2[idx*2];
	}
      if (ext2[idx*2+1] < ext[idx*2+1])
	{
	ext[idx*2+1] = ext2[idx*2+1];
	}
      }
    }
  
  this->Output->SetWholeExtent(ext);
}






//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
static void vtkImageMathematicsExecute1(vtkImageMathematics *self,
					vtkImageData *in1Data, T *in1Ptr,
					vtkImageData *outData, T *outPtr,
					int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operaton
	switch (op)
	  {
	  case VTK_INVERT:
	    *outPtr = (T)(1.0 / *in1Ptr);
	    break;
	  case VTK_SIN:
	    *outPtr = (T)(sin((double)*in1Ptr));
	    break;
	  case VTK_COS:
	    *outPtr = (T)(cos((double)*in1Ptr));
	    break;
	  case VTK_EXP:
	    *outPtr = (T)(exp((double)*in1Ptr));
	    break;
	  case VTK_LOG:
	    *outPtr = (T)(log((double)*in1Ptr));
	    break;
	  case VTK_ABS:
	    *outPtr = (T)(fabs((double)*in1Ptr));
	    break;
	  case VTK_SQR:
	    *outPtr = *in1Ptr * *in1Ptr;
	    break;
	  case VTK_SQRT:
	    *outPtr = (T)(sqrt((double)*in1Ptr));
	    break;
	  }
	outPtr++;
	in1Ptr++;
	}
      outPtr += outIncY;
      in1Ptr += inIncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    }
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageMathematicsExecute2(vtkImageMathematics *self,
					vtkImageData *in1Data, T *in1Ptr,
					vtkImageData *in2Data, T *in2Ptr,
					vtkImageData *outData, T *outPtr,
					int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  int op = self->GetOperation();

  
  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*in1Data->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      for (idxR = 0; idxR < rowLength; idxR++)
	{
	// Pixel operation
	switch (op)
	  {
	  case VTK_ADD:
	    *outPtr = *in1Ptr + *in2Ptr;
	    break;
	  case VTK_SUBTRACT:
	    *outPtr = *in1Ptr - *in2Ptr;
	    break;
	  case VTK_MULTIPLY:
	    *outPtr = *in1Ptr * *in2Ptr;
	    break;
	  case VTK_DIVIDE:
	    if (*in2Ptr)
	      {
	      *outPtr = *in1Ptr / *in2Ptr;
	      }
	    else
	      {
	      *outPtr = (T)(*in1Ptr / 0.00001);
	      }
	    break;
	  case VTK_MIN:
	    if (*in1Ptr < *in2Ptr)
	      {
	      *outPtr = *in1Ptr;
	      }
	    else
	      {
	      *outPtr = *in2Ptr;
	      }
	    break;
	  case VTK_MAX:
	    if (*in1Ptr > *in2Ptr)
	      {
	      *outPtr = *in1Ptr;
	      }
	    else
	      {
	      *outPtr = *in2Ptr;
	      }
	  }
	outPtr++;
	in1Ptr++;
	in2Ptr++;
	}
      outPtr += outIncY;
      in1Ptr += inIncY;
      in2Ptr += in2IncY;
      }
    outPtr += outIncZ;
    in1Ptr += inIncZ;
    in2Ptr += in2IncZ;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageMathematics::ThreadedExecute(vtkImageData **inData, 
					  vtkImageData *outData,
					  int outExt[6], int id)
{
  void *inPtr1 = inData[0]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  

  if (this->Operation == VTK_ADD || this->Operation == VTK_SUBTRACT || 
      this->Operation == VTK_MULTIPLY || this->Operation == VTK_DIVIDE ||
      this->Operation == VTK_MIN || this->Operation == VTK_MAX) 
    {
    void *inPtr2 = inData[1]->GetScalarPointerForExtent(outExt);

    // this filter expects that input is the same type as output.
    if (inData[0]->GetScalarType() != outData->GetScalarType() ||
	inData[1]->GetScalarType() != outData->GetScalarType())
      {
      vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0]->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
      return;
      }
    
    switch (inData[0]->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageMathematicsExecute2(this, 
				    inData[0], (float *)(inPtr1), 
				    inData[1], (float *)(inPtr2), 
				    outData, (float *)(outPtr), 
				    outExt, id);
	break;
      case VTK_INT:
	vtkImageMathematicsExecute2(this, 
				    inData[0], (int *)(inPtr1), 
				    inData[1], (int *)(inPtr2), 
				    outData, (int *)(outPtr), 
				    outExt, id);
	break;
      case VTK_SHORT:
	vtkImageMathematicsExecute2(this, 
				    inData[0], (short *)(inPtr1), 
				    inData[1], (short *)(inPtr2), 
				    outData, (short *)(outPtr), 
				    outExt, id);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageMathematicsExecute2(this, 
				    inData[0], (unsigned short *)(inPtr1), 
				    inData[1], (unsigned short *)(inPtr2), 
				    outData, (unsigned short *)(outPtr), 
				    outExt, id);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageMathematicsExecute2(this, 
				    inData[0], (unsigned char *)(inPtr1), 
				    inData[1], (unsigned char *)(inPtr2), 
				    outData, (unsigned char *)(outPtr), 
				    outExt, id);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
  else
    {
    // this filter expects that input is the same type as output.
    if (inData[0]->GetScalarType() != outData->GetScalarType())
      {
      vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0]->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
      return;
      }
    switch (inData[0]->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageMathematicsExecute1(this, 
				    inData[0], (float *)(inPtr1), 
				    outData, (float *)(outPtr), 
				    outExt, id);
	break;
      case VTK_INT:
	vtkImageMathematicsExecute1(this, 
				    inData[0], (int *)(inPtr1), 
				    outData, (int *)(outPtr), 
				    outExt, id);
	break;
      case VTK_SHORT:
	vtkImageMathematicsExecute1(this, 
				    inData[0], (short *)(inPtr1), 
				    outData, (short *)(outPtr), 
				    outExt, id);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageMathematicsExecute1(this, 
				    inData[0], (unsigned short *)(inPtr1), 
				    outData, (unsigned short *)(outPtr), 
				    outExt, id);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageMathematicsExecute1(this, 
				    inData[0], (unsigned char *)(inPtr1), 
				    outData, (unsigned char *)(outPtr), 
				    outExt, id);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }

    }
}
















