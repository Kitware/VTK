/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.cxx
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
#include "vtkImageCorrelation.h"



//----------------------------------------------------------------------------
vtkImageCorrelation::vtkImageCorrelation()
{
  this->Dimensionality = 2;
}


//----------------------------------------------------------------------------
// Description:
// Grow the output image 
void vtkImageCorrelation::ExecuteImageInformation()
{
  int extent1[6];
  
  this->Inputs[0]->GetWholeExtent(extent1);
  
  this->Output->SetNumberOfScalarComponents(1);
  
  this->Output->SetWholeExtent(extent1);
  this->Output->SetScalarType(VTK_FLOAT);
}

//----------------------------------------------------------------------------
// Description:
// Grow
void vtkImageCorrelation::ComputeRequiredInputUpdateExtent(int inExt[6], 
							   int outExt[6],
							   int whichInput)
{
  if (whichInput == 1)
    {
    // get the whole image for input 2
    memcpy(inExt,this->Inputs[whichInput]->GetWholeExtent(),6*sizeof(int));
    }
  else
    {
    // try to get all the data required to handle the boundaries
    // but limit to the whole extent
    int idx;
    int *i0WExtent = this->Inputs[0]->GetWholeExtent();
    int *i1WExtent = this->Inputs[1]->GetWholeExtent();
    memcpy(inExt,outExt,6*sizeof(int));
    for (idx = 0; idx < 3; idx++)
      {
      inExt[idx*2+1] = outExt[idx*2+1] +
	(i1WExtent[idx*2+1] - i1WExtent[idx*2]);

      // clip to whole extent
      if (inExt[idx*2+1] > i0WExtent[idx*2+1])
	{
	inExt[idx*2+1] = i0WExtent[idx*2+1];
	}
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageCorrelationExecute(vtkImageCorrelation *self,
				       vtkImageData *in1Data, T *in1Ptr,
				       vtkImageData *in2Data, T *in2Ptr,
				       vtkImageData *outData, float *outPtr,
				       int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int in1IncX, in1IncY, in1IncZ;
  int in1CIncX, in1CIncY, in1CIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int *in2Extent;
  T *in2Ptr2, *in1Ptr2;
  int kIdxX, kIdxY, kIdxZ;
  int xKernMax, yKernMax, zKernMax;
  int maxIX, maxIY, maxIZ;
  int *wExtent;
  
  // find the region to loop over
  maxC = in1Data->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // get some other info we need
  in2Extent = self->GetInput2()->GetWholeExtent(); 
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, in1CIncX, in1CIncY, in1CIncZ);
  in1Data->GetIncrements(in1IncX, in1IncY, in1IncZ);
  in2Data->GetIncrements(in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // how far can we gon with input data
  // this may be farther that the outExt because of 
  // subpieces etc.
  wExtent = in1Data->GetExtent();
  maxIZ = wExtent[5] - outExt[4];
  maxIY = wExtent[3] - outExt[2];
  maxIX = wExtent[1] - outExt[0];
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    // how much of kernel to use
    zKernMax = maxIZ - idxZ;
    if (zKernMax > in2Extent[5]) zKernMax = in2Extent[5];
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      yKernMax = maxIY - idxY;
      if (yKernMax > in2Extent[3]) yKernMax = in2Extent[3];
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// determine the extent of input 1 that contributes to this pixel
	*outPtr = 0.0;
	xKernMax = maxIX - idxX;
	if (xKernMax > in2Extent[1]) xKernMax = in2Extent[1];

	// sumation
	for (kIdxZ = 0; kIdxZ <= zKernMax; kIdxZ++)
	  {
	  for (kIdxY = 0; kIdxY <= yKernMax; kIdxY++)
	    {
	    in1Ptr2 = in1Ptr + kIdxY*in1IncY + kIdxZ*in1IncZ;
	    in2Ptr2 = in2Ptr + kIdxY*in2IncY + kIdxZ*in2IncZ;
	    for (kIdxX = 0; kIdxX <= xKernMax; kIdxX++)
	      {
	      for (idxC = 0; idxC < maxC; idxC++)
		{
		*outPtr = *outPtr + (*in1Ptr2) * (*in2Ptr2);
		in1Ptr2++;
		in2Ptr2++;
		}
	      }
	    }
	  }
	in1Ptr += maxC;
	outPtr++;
	}
      in1Ptr += in1CIncY;
      outPtr += outIncY;
      }
    in1Ptr += in1CIncZ;
    outPtr += outIncZ;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the datas data types.
void vtkImageCorrelation::ThreadedExecute(vtkImageData **inData, 
					  vtkImageData *outData,
					  int outExt[6], int id)
{
  int *in2Extent = this->Inputs[1]->GetWholeExtent();
  void *in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  void *in2Ptr = inData[1]->GetScalarPointerForExtent(in2Extent);
  float *outPtr = (float *)outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != inData[1]->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << 
    inData[0]->GetScalarType() << " and input2 ScalarType " <<
    inData[1]->GetScalarType() << ", should match");
    return;
    }
  
  // input depths must match
  if (inData[0]->GetNumberOfScalarComponents() != 
      inData[1]->GetNumberOfScalarComponents())
    {
    vtkErrorMacro(<< "Execute: input depths must match");
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageCorrelationExecute(this, inData[0], 
				 (float *)(in1Ptr), 
				 inData[1], (float *)(in2Ptr), 
				 outData, outPtr, 
				 outExt, id);
      break;
    case VTK_INT:
      vtkImageCorrelationExecute(this, inData[0], (int *)(in1Ptr), 
				 inData[1], (int *)(in2Ptr), 
				 outData, outPtr, 
				 outExt, id);
      break;
    case VTK_SHORT:
      vtkImageCorrelationExecute(this, inData[0], 
				 (short *)(in1Ptr), 
				 inData[1], (short *)(in2Ptr), 
				 outData, outPtr, 
				 outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCorrelationExecute(this, inData[0], 
				 (unsigned short *)(in1Ptr), 
				 inData[1], 
				 (unsigned short *)(in2Ptr), 
				 outData, outPtr, outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCorrelationExecute(this, inData[0], 
				 (unsigned char *)(in1Ptr), 
				 inData[1], 
				 (unsigned char *)(in2Ptr), 
				 outData, outPtr, outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageCorrelation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
}

