/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDifference.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkImageDifference.h"
#include "stdlib.h"

// Description:
// Construct object to extract all of the input data.
vtkImageDifference::vtkImageDifference()
{
  this->Error = 0;
  this->ThresholdedError = 0.0;
  this->Threshold = 51;
  this->AllowShift = 1;
}



// simple macro for calculating error
#define vtkImageDifferenceComputeError(c1,c2) \
  r1 = abs(((int)(c1)[0] - (int)(c2)[0])); \
  g1 = abs(((int)(c1)[1] - (int)(c2)[1])); \
  b1 = abs(((int)(c1)[2] - (int)(c2)[2]));\
  if ((r1+g1+b1) < (tr+tg+tb)) { tr = r1; tg = g1; tb = b1; }


//----------------------------------------------------------------------------
void vtkImageDifference::ThreadedExecute(vtkImageData **inData, 
					 vtkImageData *outData,
					 int outExt[6], int id)
{
  unsigned char *in1Ptr0, *in1Ptr1, *in1Ptr2;
  unsigned char *in2Ptr0, *in2Ptr1, *in2Ptr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  int min0, max0, min1, max1, min2, max2;
  int idx0, idx1, idx2;
  int in1Inc0, in1Inc1, in1Inc2;
  int in2Inc0, in2Inc1, in2Inc2;
  int outInc0, outInc1, outInc2;
  int tr, tg, tb, r1, g1, b1;

  id = id;
  this->Error = 0;
  this->ThresholdedError = 0;
  
  if (inData[0] == NULL || inData[1] == NULL || outData == NULL)
    {
    vtkErrorMacro(<< "Execute: Missing data");
    this->Error = 1;
    this->ThresholdedError = 1;
    return;
    }

  if (inData[0]->GetNumberOfScalarComponents() != 3 ||
      inData[1]->GetNumberOfScalarComponents() != 3 ||
      outData->GetNumberOfScalarComponents() != 3)
    {
    vtkErrorMacro(<< "Execute: Expecting 3 components (RGB)");
    this->Error = 1;
    this->ThresholdedError = 1;
    return;
    }
    
  // this filter expects that input is the same type as output.
  if (inData[0]->GetScalarType() != VTK_UNSIGNED_CHAR || 
      inData[1]->GetScalarType() != VTK_UNSIGNED_CHAR || 
      outData->GetScalarType() != VTK_UNSIGNED_CHAR)
      {
      vtkErrorMacro(<< "Execute: All ScalarTypes must be unsigned char");
      this->Error = 1;
      this->ThresholdedError = 1;
      return;
      }
  
  in1Ptr2 = (unsigned char *) inData[0]->GetScalarPointerForExtent(outExt);  
  in2Ptr2 = (unsigned char *) inData[1]->GetScalarPointerForExtent(outExt);  
  outPtr2 = (unsigned char *) outData->GetScalarPointerForExtent(outExt);  

  inData[0]->GetIncrements(in1Inc0, in1Inc1, in1Inc2);
  inData[1]->GetIncrements(in2Inc0, in2Inc1, in2Inc2);
  outData->GetIncrements(outInc0, outInc1, outInc2);
  
  min0 = outExt[0];  max0 = outExt[1];
  min1 = outExt[2];  max1 = outExt[3];
  min2 = outExt[4];  max2 = outExt[5];
  
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    in1Ptr1 = in1Ptr2;
    in2Ptr1 = in2Ptr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      in1Ptr0 = in1Ptr1;
      in2Ptr0 = in2Ptr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
	{

	tr = 1000;
	tg = 1000;
	tb = 1000;
      
	/* check the exact match pixel */
	vtkImageDifferenceComputeError(in1Ptr0,in2Ptr0);
	
	/* If AllowShift, then we examine neighboring pixels to 
	   find the least difference.  This feature is used to 
	   allow images to shift slightly between different graphics
	   systems, like between opengl and starbase. */
	
	if (this->AllowShift) 
	  {
	  /* lower row */
	  if (idx1 > min1)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0-in1Inc1,in2Ptr0);
	    if (idx0 > min0)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0-in1Inc0-in1Inc1,in2Ptr0);
	      }
	    if (idx0 < max0)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0+in1Inc0-in1Inc1,in2Ptr0);
	      }
	    }
	  /* middle row (center already considered) */
	  if (idx0 > min0)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0-in1Inc0,in2Ptr0);
	    }
	  if (idx0 < max0)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0+in1Inc0,in2Ptr0);
	    }
	  /* upper row */
	  if (idx1 < max1)
	    {
	    vtkImageDifferenceComputeError(in1Ptr0+in1Inc1,in2Ptr0);
	    if (idx0 > min0)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0-in1Inc0+in1Inc1,in2Ptr0);
	      }
	    if (idx0 < max0)
	      {
	      vtkImageDifferenceComputeError(in1Ptr0+in1Inc0+in1Inc1,in2Ptr0);
	      }
	    }
	  }
	
	this->Error = this->Error + (tr + tg + tb)/(3.0*255);
	tr -= this->Threshold;
	if (tr < 0) tr = 0;
	tg -= this->Threshold;
	if (tg < 0) tg = 0;
	tb -= this->Threshold;
	if (tb < 0) tb = 0;
	*outPtr0++ = (unsigned char)tr;
	*outPtr0++ = (unsigned char)tg;
	*outPtr0++ = (unsigned char)tb;
	this->ThresholdedError += (tr + tg + tb)/(3.0*255.0);

	in1Ptr0 += in1Inc0;
	in2Ptr0 += in2Inc0;
	}
      outPtr1 += outInc1;
      in1Ptr1 += in1Inc1;
      in2Ptr1 += in2Inc1;
      }
    outPtr2 += outInc2;
    in1Ptr2 += in1Inc2;
    in2Ptr2 += in2Inc2;
    }
}

//----------------------------------------------------------------------------
// Make sure both the inputs are the same size.
void vtkImageDifference::ExecuteImageInformation()
{
  int *in1Ext, *in2Ext;
  
  // Make sure the Input has been set.
  // we require that input 1 be set.
  if ( ! this->Inputs[0] || ! this->Inputs[1])
    {
    vtkErrorMacro(<< "ExecuteImageInformation: Input is not set.");
    this->Error = 1;
    this->ThresholdedError = 1;
    return;
    }
  
  in1Ext = this->Inputs[0]->GetWholeExtent();
  in2Ext = this->Inputs[1]->GetWholeExtent();

  if (in1Ext[0] != in2Ext[0] || in1Ext[1] != in2Ext[1] || 
      in1Ext[2] != in2Ext[2] || in1Ext[3] != in2Ext[3] || 
      in1Ext[4] != in2Ext[4] || in1Ext[5] != in2Ext[5])
    {
    vtkErrorMacro("ExecuteImageInformation: Input are not the same size.");
    this->Error = 1;
    this->ThresholdedError = 1;
    return;
    }
  
  this->Output->SetWholeExtent(in1Ext);
}


void vtkImageDifference::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageTwoInputFilter::PrintSelf(os,indent);
  
  os << indent << "Error: " << this->Error << "\n";
  os << indent << "ThresholdedError: " << this->ThresholdedError << "\n";
  os << indent << "Threshold: " << this->Threshold << "\n";
  os << indent << "AllowShift: " << this->AllowShift << "\n";
}


