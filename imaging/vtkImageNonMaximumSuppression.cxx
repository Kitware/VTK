/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageCache.h"
#include "vtkImageNonMaximumSuppression.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageNonMaximumSuppression fitler.
vtkImageNonMaximumSuppression::vtkImageNonMaximumSuppression()
{
  this->Dimensionality= 2;
  this->HandleBoundaries = 1;
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the image extent of this filters
// input, and changes the region to hold the image extent of this filters
// output.
void vtkImageNonMaximumSuppression::ExecuteImageInformation()
{
  int extent[6];
  int idx;
  
  this->Inputs[0]->GetWholeExtent(extent);
  if ( ! this->HandleBoundaries)
    {
    // shrink output image extent.
    for (idx = 0; idx < this->Dimensionality; ++idx)
      {
      extent[idx*2] += 1;
      extent[idx*2+1] -= 1;
      }
    }

  this->Output->SetNumberOfScalarComponents
    (this->Inputs[0]->GetNumberOfScalarComponents());
  
  this->Output->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the input extent necessary to generate the output.
void vtkImageNonMaximumSuppression::
ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6],
				 int whichInput)
{
  int *wholeExtent;
  int idx;

  wholeExtent = this->Inputs[0]->GetWholeExtent();
  memcpy(inExt,outExt,6*sizeof(int));
  if (whichInput == 1)
    {
    return;
    }
  
  // grow input image extent for input 0
  for (idx = 0; idx < this->Dimensionality; ++idx)
    {
    inExt[idx*2] -= 1;
    inExt[idx*2+1] += 1;
    if (this->HandleBoundaries)
      {
      // we must clip extent with whole extent if we hanlde boundaries.
      if (inExt[idx*2] < wholeExtent[idx*2])
	{
	inExt[idx*2] = wholeExtent[idx*2];
	}
      if (inExt[idx*2 + 1] > wholeExtent[idx*2 + 1])
	{
	inExt[idx*2 + 1] = wholeExtent[idx*2 + 1];
	}
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
// Handles the two input operations
template <class T>
static void vtkImageNonMaximumSuppressionExecute(vtkImageNonMaximumSuppression *self,
						 vtkImageData *in1Data, 
						 T *in1Ptr,
						 vtkImageData *in2Data, 
						 T *in2Ptr,
						 vtkImageData *outData, 
						 T *outPtr,
						 int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int in2IncX, in2IncY, in2IncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int useZMin, useZMax, useYMin, useYMax, useXMin, useXMax;
  float d, normalizeFactor, vector[3], *ratio;
  int neighborA, neighborB;
  int *wholeExtent, *inIncs;
  int axesNum;
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get the dimensionality of the gradient.
  axesNum = self->GetDimensionality();
  // get some other info we need
  inIncs = in1Data->GetIncrements(); 
  wholeExtent = in1Data->GetExtent(); 
  
  // Get increments to march through data 
  in1Data->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  in2Data->GetContinuousIncrements(outExt, in2IncX, in2IncY, in2IncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  
  // Gradient is computed with data spacing (world coordinates)
  ratio = in2Data->GetSpacing();
  
  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    useZMin = ((idxZ + outExt[4]) <= wholeExtent[4]) ? 0 : -inIncs[2];
    useZMax = ((idxZ + outExt[4]) >= wholeExtent[5]) ? 0 : inIncs[2];
    for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
      {
      useYMin = ((idxY + outExt[2]) <= wholeExtent[2]) ? 0 : -inIncs[1];
      useYMax = ((idxY + outExt[2]) >= wholeExtent[3]) ? 0 : inIncs[1];
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
	useXMin = ((idxX + outExt[0]) <= wholeExtent[0]) ? 0 : -inIncs[0];
	useXMax = ((idxX + outExt[0]) >= wholeExtent[1]) ? 0 : inIncs[0];

	// calculate the neighbors
	d = vector[0] = *in2Ptr * ratio[0];
	normalizeFactor = (d * d);
	d = vector[1] = in2Ptr[1] * ratio[1];
	normalizeFactor += (d * d);
	if (axesNum == 3)
	  {
	  d = vector[2] = in2Ptr[2] * ratio[2];
	  normalizeFactor += (d * d);
	  }
	if (normalizeFactor)
	  {
	  normalizeFactor = 1.0 / sqrt(normalizeFactor);
	  }
	// Vector points positive along this idx?
	// (can point along multiple axes)
	d = vector[0] * normalizeFactor;  
	
	if (d > 0.5)  
	  {
	  neighborA = useXMax;
	  neighborB = useXMin;
	  }
	else if (d < -0.5)
	  {
	  neighborB = useXMax;
	  neighborA = useXMin;
	  }
	else
	  {
	  neighborA = 0;
	  neighborB = 0;
	  }
	d = vector[1] * normalizeFactor;  
	if (d > 0.5)  
	  {
	  neighborA += useYMax;
	  neighborB += useYMin;
	  }
	else if (d < -0.5)
	  {
	  neighborB += useYMax;
	  neighborA += useYMin;
	  }
	if (axesNum == 3)
	  {
	  d = vector[2] * normalizeFactor;  
	  if (d > 0.5)  
	    {
	    neighborA += useZMax;
	    neighborB += useZMin;
	    }
	  else if (d < -0.5)
	    {
	    neighborB += useZMax;
	    neighborA += useZMin;
	    }
	  }
	
	// now process the components
	for (idxC = 0; idxC < maxC; idxC++)
	  {
	  // Pixel operation
	  // Set Output Magnitude
	  if (in1Ptr[neighborA] > *in1Ptr || in1Ptr[neighborB] > *in1Ptr)
	    {
	    *outPtr = 0;
	    }
	  else
	    {
	    *outPtr = *in1Ptr;
	    // also check for them being equal is neighbor with larger ptr
	    if ((neighborA > neighborB)&&(in1Ptr[neighborA] == *in1Ptr))
	      {
	      *outPtr = 0;
	      }
	    else if ((neighborB > neighborA)&&(in1Ptr[neighborB] == *in1Ptr))
	      {
	      *outPtr = 0;
	      }
	    }
	  outPtr++;
	  in1Ptr++;
	  }
	in2Ptr += axesNum;
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
void vtkImageNonMaximumSuppression::ThreadedExecute(vtkImageData **inData, 
						    vtkImageData *outData,
						    int outExt[6], int id)
{
  void *in1Ptr = inData[0]->GetScalarPointerForExtent(outExt);
  void *in2Ptr = inData[1]->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != outData->GetScalarType() ||
      inData[1]->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << 
    inData[0]->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData[0]->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageNonMaximumSuppressionExecute(this, inData[0], 
					   (float *)(in1Ptr), 
					   inData[1], (float *)(in2Ptr), 
					   outData, (float *)(outPtr), 
					   outExt, id);
      break;
    case VTK_INT:
      vtkImageNonMaximumSuppressionExecute(this, inData[0], (int *)(in1Ptr), 
					   inData[1], (int *)(in2Ptr), 
					   outData, (int *)(outPtr), 
					   outExt, id);
      break;
    case VTK_SHORT:
      vtkImageNonMaximumSuppressionExecute(this, inData[0], 
					   (short *)(in1Ptr), 
					   inData[1], (short *)(in2Ptr), 
					   outData, (short *)(outPtr), 
					   outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageNonMaximumSuppressionExecute(this, inData[0], 
					   (unsigned short *)(in1Ptr), 
					   inData[1], 
					   (unsigned short *)(in2Ptr), 
					   outData, 
					   (unsigned short *)(outPtr), 
					   outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageNonMaximumSuppressionExecute(this, inData[0], 
					   (unsigned char *)(in1Ptr), 
					   inData[1], 
					   (unsigned char *)(in2Ptr), 
					   outData, 
					   (unsigned char *)(outPtr), 
					   outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


void vtkImageNonMaximumSuppression::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);

  os << indent << "Dimensionality: " << this->Dimensionality << "\n";

  os << indent << "HandleBoundaries: " << (this->HandleBoundaries ? "On\n" : "Off\n");
}


