/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSkeleton2D.cxx
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
#include "vtkImageSkeleton2D.h"



//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageSkeleton2D fitler.
vtkImageSkeleton2D::vtkImageSkeleton2D()
{
  this->Prune = 0;
}

//----------------------------------------------------------------------------
void vtkImageSkeleton2D::SetNumberOfIterations(int num)
{
  this->vtkImageIterateFilter::SetNumberOfIterations(num);
}


//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageSkeleton2D::ComputeRequiredInputUpdateExtent(int *inExt, 
							  int *outExt)
{
  int idx;
  int *wholeExtent;
  
  wholeExtent = this->Input->GetWholeExtent();

  inExt[4] = outExt[4];
  inExt[5] = outExt[5];
  
  for (idx = 0; idx < 2; ++idx)
    {
    inExt[idx*2] = outExt[idx*2] - 1;
    inExt[idx*2+1] = outExt[idx*2+1] + 1;
    
    // If the expanded region is out of the IMAGE Extent (grow min)
    if (inExt[idx*2] < wholeExtent[idx*2])
      {
      inExt[idx*2] = wholeExtent[idx*2];
      }
    // If the expanded region is out of the IMAGE Extent (shrink max)      
    if (inExt[idx*2+1] > wholeExtent[idx*2+1])
      {
      // shrink the required region extent
      inExt[idx*2+1] = wholeExtent[idx*2+1];
      }
    }
}




//----------------------------------------------------------------------------
// Description:
// This method contains the second switch statement that calls the correct
// templated function for the mask types.
// This is my best attempt at skeleton.  The rules are a little hacked up,
// but it is the only way I can think of to get the 
// desired results with a 3x3 kernel.
template <class T>
static void vtkImageSkeleton2DExecute(vtkImageSkeleton2D *self,
			   vtkImageData *inData, T *inPtr, 
			   vtkImageData *outData, int *outExt, T *outPtr)
{
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2, numComps;
  int idx0, idx1, idx2, idxC;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2, *inPtrC;
  T *outPtr0, *outPtr1, *outPtr2;
  int wholeMin0, wholeMax0, wholeMin1, wholeMax1, wholeMin2, wholeMax2;
  int prune = self->GetPrune();
  float n[8];
  int countFaces, countCorners;

  // Get information to march through data
  inData->GetIncrements(inInc0, inInc1, inInc2); 
  outData->GetIncrements(outInc0, outInc1, outInc2); 
  outMin0 = outExt[0];  outMax0 = outExt[1];
  outMin1 = outExt[2];  outMax1 = outExt[3];
  outMin2 = outExt[4];  outMax2 = outExt[5];
  self->GetInput()->GetWholeExtent(wholeMin0, wholeMax0, wholeMin1, wholeMax1,
				   wholeMin2, wholeMax2);
  numComps = inData->GetNumberOfScalarComponents();
  
  inPtrC = inPtr;
  for (idxC = 0; idxC < numComps; ++idxC)
    {
    inPtr2 = inPtrC;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      // erode input
      inPtr1 = inPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
	{
	inPtr0 = inPtr1;
	for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
	  {
	  // Center pixel has to be on.
	  if (*inPtr0)
	    {
	    // neighbors independant of boundaries
	    n[0] = (idx0>wholeMin0) ? *(inPtr0-inInc0) : 0;
	    n[1] = (idx0>wholeMin0)&&(idx1>wholeMin1) 
	      ? *(inPtr0-inInc0-inInc1) : 0;
	    n[2] = (idx1>wholeMin1) ? *(inPtr0-inInc1) : 0;
	    n[3] = (idx1>wholeMin1)&&(idx0<wholeMax0) 
	      ? *(inPtr0-inInc1+inInc0) : 0;
	    n[4] = (idx0<wholeMax0) ? *(inPtr0+inInc0) : 0;
	    n[5] = (idx0<wholeMax0)&&(idx1<wholeMax1) 
	      ? *(inPtr0+inInc0+inInc1) : 0;
	    n[6] = (idx1<wholeMax1) ? *(inPtr0+inInc1) : 0;
	    n[7] = (idx1<wholeMax1)&&(idx0>wholeMin0) 
	      ? *(inPtr0+inInc1-inInc0) : 0;
	    
	    countFaces = (n[0]>1)+(n[2]>1)+(n[4]>1)+(n[6]>1);
	    countCorners = (n[1]>1)+(n[3]>1)+(n[5]>1)+(n[7]>1);
	    
	    // special case
	    if (prune && ((countFaces + countCorners) <= 1))
	      {
	      *inPtr0 = 1;
	      }
	    
	    // one of four face neighbors has to be off
	    if (n[0] == 0 || n[2] == 0 ||
		n[4] == 0 || n[6] == 0)
	      { // remaining pixels need to be connected.
	      // across corners
	      if (1)
		//(n[0] == 0 || n[2] == 0 || n[1] > 0) &&
		//  (n[2] == 0 || n[4] == 0 || n[3] > 0) &&
		//  (n[4] == 0 || n[6] == 0 || n[5] > 0) &&
		//  (n[6] == 0 || n[0] == 0 || n[7] > 0))
		{
		// do not break corner connectivity
		if ((n[1] <= 1 || n[0] > 1 || n[2] > 1) &&
		    (n[3] <= 1 || n[2] > 1 || n[4] > 1) &&
		    (n[5] <= 1 || n[4] > 1 || n[6] > 1) &&
		    (n[7] <= 1 || n[6] > 1 || n[0] > 1))
		  {
		  // opposite faces (special conedition so double thick lines
		  // will not be completely eroded)
		  if ((n[0] == 0 || n[4] == 0 || n[2] > 1 || n[6] > 1) &&
		      (n[2] == 0 || n[6] == 0 || n[0] > 1 || n[4] > 1))
		    {
		    // check to stop pruning (sort of a hack huristic)
		    if (prune || (countFaces > 2) || 
			((countFaces == 2) && (countCorners > 1)))
		      {
		      *inPtr0 = 1;
		      }
		    }
		  }
		}
	      }
	    }
	  inPtr0 += inInc0;
	  }
	inPtr1 += inInc1;
	}
      inPtr2 += inInc2;
      }
    ++inPtrC;
    }


  // copy to output
  for (idxC = 0; idxC < numComps; ++idxC)
    {
    outPtr2 = outPtr;
    inPtr2 = inPtr;
    for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
      {
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
	{
	outPtr0 = outPtr1;
	inPtr0 = inPtr1;
	for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
	  {
	  if (*inPtr0 <= 1)
	    {
	    *outPtr0 = (T)(0.0);
	    }
	  else
	    {
	    *outPtr0 = *inPtr0;
	    }
	  // *outPtr0 = *inPtr0; // for debugging
      
	  inPtr0 += inInc0;
	  outPtr0 += outInc0;      
	  }
	inPtr1 += inInc1;
	outPtr1 += outInc1;      
	}
      inPtr2 += inInc2;
      outPtr2 += outInc2;      
      }
    ++inPtr;
    ++outPtr;
    }
}





//----------------------------------------------------------------------------
// Description:
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
void vtkImageSkeleton2D::ThreadedExecute(vtkImageData *inData, 
					 vtkImageData *outData, 
					 int outExt[6], int id)
{
  void *inPtr;
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  vtkImageData *tempData;
  int inExt[6];

  id = id;

  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
      << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  this->ComputeRequiredInputUpdateExtent(inExt, outExt); 

  // Make a temporary copy of the input data
  tempData = vtkImageData::New();
  tempData->SetScalarType(inData->GetScalarType());
  tempData->SetExtent(inExt);
  tempData->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
  tempData->CopyAndCastFrom(inData, inExt);

  inPtr = tempData->GetScalarPointerForExtent(inExt);
  switch (tempData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageSkeleton2DExecute(this, tempData, (float *)(inPtr), 
				outData, outExt, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageSkeleton2DExecute(this, tempData, (int *)(inPtr), 
				outData, outExt, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageSkeleton2DExecute(this, tempData, (short *)(inPtr), 
				outData, outExt, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageSkeleton2DExecute(this, tempData, (unsigned short *)(inPtr), 
				outData, outExt, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageSkeleton2DExecute(this, tempData, (unsigned char *)(inPtr), 
				outData, outExt, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      tempData->Delete();
      return;
    }

  tempData->Delete();
}



  




