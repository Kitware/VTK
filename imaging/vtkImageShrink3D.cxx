/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.cxx
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
#include <math.h>
#include "vtkImageCache.h"
#include "vtkImageShrink3D.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageShrink3D::vtkImageShrink3D()
{
  this->ShrinkFactors[0] = this->ShrinkFactors[1] = this->ShrinkFactors[2] = 1;
  this->Shift[0] = this->Shift[1] = this->Shift[2] = 0;
  this->Averaging = 1;
}

//----------------------------------------------------------------------------
void vtkImageShrink3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  os << indent << "ShrinkFactors: (" << this->ShrinkFactors[0] << ", "
     << this->ShrinkFactors[1] << ", " << this->ShrinkFactors[2] << ")\n";
  os << indent << "Shift: (" << this->Shift[0] << ", "
     << this->Shift[1] << ", " << this->Shift[2] << ")\n";

  os << indent << "Averaging: " << (this->Averaging ? "On\n" : "Off\n");

}

//----------------------------------------------------------------------------
// Description:
// This method computes the Region of input necessary to generate outRegion.
void vtkImageShrink3D::ComputeRequiredInputUpdateExtent(int inExt[6], 
							int outExt[6])
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    // For Min.
    inExt[idx*2] = outExt[idx*2] * this->ShrinkFactors[idx] 
      + this->Shift[idx];
    // For Max.
    inExt[idx*2+1] = outExt[idx*2+1] * this->ShrinkFactors[idx]
      + this->Shift[idx];
    // If we are averaging, we need a little more
    if (this->Averaging)
      {
      inExt[idx*2+1] += this->ShrinkFactors[idx] - 1;
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// Computes any global image information associated with regions.
// Any problems with roundoff or negative numbers ???
void vtkImageShrink3D::ExecuteImageInformation()
{
  int idx;
  int wholeExtent[6];
  float spacing[3];
  
  this->Input->GetWholeExtent(wholeExtent);
  this->Input->GetSpacing(spacing);

  for (idx = 0; idx < 3; ++idx)
    {
    // Scale the output extent
    wholeExtent[2*idx] = 
      (int)(ceil((float)(wholeExtent[2*idx] - this->Shift[idx]) 
		 / (float)(this->ShrinkFactors[idx])));
    wholeExtent[2*idx+1] = (int)(floor(
     (float)(wholeExtent[2*idx+1]-this->Shift[idx]-this->ShrinkFactors[idx]+1)
         / (float)(this->ShrinkFactors[idx])));
    // Change the data spacing
    spacing[idx] *= (float)(this->ShrinkFactors[idx]);
    }

  this->Output->SetWholeExtent(wholeExtent);
  this->Output->SetSpacing(spacing);
}



//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
template <class T>
static void vtkImageShrink3DExecute(vtkImageShrink3D *self,
				    vtkImageData *inData, T *inPtr,
				    vtkImageData *outData, T *outPtr,
				    int outExt[6], int id)
{
  int outIdx0, outIdx1, outIdx2, inIdx0, inIdx1, inIdx2;
  int inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  int outInc0, outInc1, outInc2;
  int tmpInc0, tmpInc1, tmpInc2;
  T *tmpPtr0, *tmpPtr1, *tmpPtr2;
  int rowLength;
  int factor0, factor1, factor2;
  int averaging;
  float sum, norm;
  unsigned long count = 0;
  unsigned long target;
  int idxC, maxC, maxX;
  T *outPtr2;

  averaging = self->GetAveraging();
  self->GetShrinkFactors(factor0, factor1, factor2);
  
  // Get information to march through data 
  inData->GetIncrements(inInc0, inInc1, inInc2);
  tmpInc0 = inInc0 * factor0;
  tmpInc1 = inInc1 * factor1;
  tmpInc2 = inInc2 * factor2;
  outData->GetContinuousIncrements(outExt,outInc0, outInc1, outInc2);
  norm = 1.0 / (float)(factor0 * factor1 * factor2);
  rowLength = (outExt[1] - outExt[0]+1)*outData->GetNumberOfScalarComponents();
  target = (unsigned long)((outExt[5] - outExt[4] + 1)*
    (outExt[3] - outExt[2] + 1)/50.0);
  target++;
  maxX = outExt[1] - outExt[0];
  maxC = inData->GetNumberOfScalarComponents();

  // Loop through output pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    tmpPtr2 = inPtr + idxC;
    outPtr2 = outPtr + idxC;
    for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
      {
    tmpPtr1 = tmpPtr2;
    for (outIdx1 = outExt[2]; outIdx1 <= outExt[3]; ++outIdx1)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      tmpPtr0 = tmpPtr1;
      for (outIdx0 = 0; outIdx0 <= maxX; ++outIdx0)
	{
	// Copy pixel from this location
	if ( ! averaging)
	  {
	  *outPtr2 = *tmpPtr0;
	  }
	else
	  {
	  sum = 0.0;
	  // Loop through neighborhood pixels
	  inPtr2 = tmpPtr0;
	  for (inIdx2 = 0; inIdx2 < factor2; ++inIdx2)
	    {
	    inPtr1 = inPtr2;
	    for (inIdx1 = 0; inIdx1 < factor1; ++inIdx1)
	      {
	      inPtr0 = inPtr1;
	      for (inIdx0 = 0; inIdx0 < factor0; ++inIdx0)
		{
		sum += (float)(*inPtr0);
		inPtr0 += inInc0;
		}
	      inPtr1 += inInc1;
	      }
	    inPtr2 += inInc2;
	    }
	  *outPtr2 = (T)(sum * norm);
	  }
	tmpPtr0 += tmpInc0;
	outPtr2 += maxC;
	}
      tmpPtr1 += tmpInc1;
      outPtr2 += outInc1;
      }
    tmpPtr2 += tmpInc2;
    outPtr2 += outInc2;
    }
  }
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input data to fill the output data.
// It can handle any type data, but the two datas must have the same 
// data type.
void vtkImageShrink3D::ThreadedExecute(vtkImageData *inData, 
				       vtkImageData *outData,
				       int outExt[6], int id)
{
  int inExt[6];
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);

  this->ComputeRequiredInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);

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
      vtkImageShrink3DExecute(this, 
			      inData, (float *)(inPtr), 
			      outData, (float *)(outPtr), outExt, id);
      break;
    case VTK_INT:
      vtkImageShrink3DExecute(this, 
			      inData, (int *)(inPtr), 
			      outData, (int *)(outPtr), outExt, id);
      break;
    case VTK_SHORT:
      vtkImageShrink3DExecute(this, 
			      inData, (short *)(inPtr), 
			      outData, (short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageShrink3DExecute(this, 
			      inData, (unsigned short *)(inPtr), 
			      outData, (unsigned short *)(outPtr), outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageShrink3DExecute(this, 
			      inData, (unsigned char *)(inPtr), 
			      outData, (unsigned char *)(outPtr), outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}














