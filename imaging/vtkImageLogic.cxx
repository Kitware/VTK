/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLogic.cxx
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
#include "vtkImageLogic.h"
#include <math.h>



//----------------------------------------------------------------------------
vtkImageLogic::vtkImageLogic()
{
  this->Operation = VTK_AND;
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the one input operations
template <class T>
static void vtkImageLogicExecute1(vtkImageLogic *self,
				  vtkImageData *in1Data, T *in1Ptr,
				  vtkImageData *outData, 
				  unsigned char *outPtr,
				  int outExt[6], int id)
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;
  unsigned char trueValue = self->GetOutputTrueValue();
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
	// Pixel operation
	switch (op)
	  {
	  case VTK_NOT:
	    if ( ! *outPtr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_NOP:
	    if (*outPtr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
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
static void vtkImageLogicExecute2(vtkImageLogic *self,
				  vtkImageData *in1Data, T *in1Ptr,
				  vtkImageData *in2Data, T *in2Ptr,
				  vtkImageData *outData, 
				  unsigned char *outPtr,
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
  unsigned char trueValue = self->GetOutputTrueValue();
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
	  case VTK_AND:
	    if (*in1Ptr && *in2Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_OR:
	    if (*in1Ptr || *in2Ptr)
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_XOR:
	    if (( ! *in1Ptr && *in2Ptr) || (*in1Ptr && ! *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
	  case VTK_NAND:
	    if ( ! (*in1Ptr && *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	    }
	    break;
	  case VTK_NOR:
	    if ( ! (*in1Ptr || *in2Ptr))
	      {
	      *outPtr = trueValue;
	      }
	    else
	      {
	      *outPtr = 0;
	      }
	    break;
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
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageLogic::ThreadedExecute(vtkImageData **inData, 
				    vtkImageData *outData,
				    int outExt[6], int id)
{
  void *in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData[0]->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }

  if (this->Operation == VTK_NOT || this->Operation == VTK_NOP)
    {
    switch (inData[0]->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageLogicExecute1(this, inData[0], (float *)(in1Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_INT:
	vtkImageLogicExecute1(this, inData[0], (int *)(in1Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_SHORT:
	vtkImageLogicExecute1(this, inData[0], (short *)(in1Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageLogicExecute1(this, inData[0], (unsigned short *)(in1Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageLogicExecute1(this, inData[0], (unsigned char *)(in1Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
  else
    {
    void *in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
    
    switch (inData[0]->GetScalarType())
      {
      case VTK_FLOAT:
	vtkImageLogicExecute2(this, inData[0], (float *)(in1Ptr), 
			      inData[1], (float *)(in2Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_INT:
	vtkImageLogicExecute2(this, inData[0], (int *)(in1Ptr), 
			      inData[1], (int *)(in2Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_SHORT:
	vtkImageLogicExecute2(this, inData[0], (short *)(in1Ptr), 
			      inData[1], (short *)(in2Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkImageLogicExecute2(this, inData[0], (unsigned short *)(in1Ptr), 
			      inData[1], (unsigned short *)(in2Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkImageLogicExecute2(this, inData[0], (unsigned char *)(in1Ptr), 
			      inData[1], (unsigned char *)(in2Ptr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
	break;
      default:
	vtkErrorMacro(<< "Execute: Unknown ScalarType");
	return;
      }
    }
}
















