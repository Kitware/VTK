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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageAppendComponents.h"



//----------------------------------------------------------------------------
vtkImageAppendComponents::vtkImageAppendComponents()
{
  this->SetExecutionAxes(VTK_IMAGE_COMPONENT_AXIS,
			 VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}

//----------------------------------------------------------------------------
// Description:
// This method tells the ouput it will have more components
void vtkImageAppendComponents::ExecuteImageInformation(vtkImageCache *in1,
						       vtkImageCache *in2,
						       vtkImageCache *out)
{
  out->SetNumberOfScalarComponents(in1->GetNumberOfScalarComponents() +
				   in2->GetNumberOfScalarComponents());
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAppendComponentsExecute(vtkImageAppendComponents *self,
				    vtkImageRegion *in1Region, T *in1Ptr,
				    vtkImageRegion *in2Region, T *in2Ptr,
				    vtkImageRegion *outRegion, T *outPtr)
{
  int minC1, maxC1, minC2, maxC2;
  int min0, max0, min1, max1;
  int idxC, idx0, idx1;
  int in1IncC, in1Inc0, in1Inc1;
  int in2IncC, in2Inc0, in2Inc1;
  int outIncC, outInc0, outInc1;
  T *in1Ptr0, *in1Ptr1;
  T *in2Ptr0, *in2Ptr1;
  T *outPtr0, *outPtr1;
  T *outPtrC;
  T *inPtrC;
  
  self = self;
  
  // Get information to march through data 
  in1Region->GetIncrements(in1IncC, in1Inc0, in1Inc1);
  in2Region->GetIncrements(in2IncC, in2Inc0, in2Inc1);
  in1Region->GetExtent(minC1, maxC1, min0, max0, min1, max1);
  in2Region->GetExtent(minC2, maxC2);
  outRegion->GetIncrements(outIncC, outInc0, outInc1);

  // We should have error checking here.
  
  // Loop through pixels
  in1Ptr1 = in1Ptr;
  in2Ptr1 = in2Ptr;
  outPtr1 = outPtr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    outPtr0 = outPtr1;
    in1Ptr0 = in1Ptr1;
    in2Ptr0 = in2Ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      outPtrC = outPtr0;
      // copy input1 components
      inPtrC = in1Ptr0;
      for (idxC = minC1; idxC <= maxC1; ++idxC)
	{
	*outPtrC = *inPtrC;
	inPtrC += in1IncC;
	outPtrC += outIncC;
	}
      // copy input2 components
      inPtrC = in2Ptr0;
      for (idxC = minC2; idxC <= maxC2; ++idxC)
	{
	*outPtrC = *inPtrC;
	inPtrC += in2IncC;
	outPtrC += outIncC;
	}
      
      outPtr0 += outInc0;
      in1Ptr0 += in1Inc0;
      in2Ptr0 += in2Inc0;
      }
    outPtr1 += outInc1;
    in1Ptr1 += in1Inc1;
    in2Ptr1 += in2Inc1;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageAppendComponents::Execute(vtkImageRegion *inRegion1, 
				 vtkImageRegion *inRegion2, 
				 vtkImageRegion *outRegion)
{
  void *inPtr1 = inRegion1->GetScalarPointer();
  void *inPtr2 = inRegion2->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  // this filter expects that inputs are the same type as output.
  if (inRegion1->GetScalarType() != outRegion->GetScalarType() ||
      inRegion2->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarTypes, " 
         << inRegion1->GetScalarType() << " and " << inRegion2->GetScalarType()
         << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion1->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageAppendComponentsExecute(this, 
			  inRegion1, (float *)(inPtr1), 
			  inRegion2, (float *)(inPtr2), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageAppendComponentsExecute(this, 
			  inRegion1, (int *)(inPtr1), 
			  inRegion2, (int *)(inPtr2), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageAppendComponentsExecute(this, 
			  inRegion1, (short *)(inPtr1), 
			  inRegion2, (short *)(inPtr2), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageAppendComponentsExecute(this, 
			  inRegion1, (unsigned short *)(inPtr1), 
			  inRegion2, (unsigned short *)(inPtr2), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageAppendComponentsExecute(this, 
			  inRegion1, (unsigned char *)(inPtr1), 
			  inRegion2, (unsigned char *)(inPtr2), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
















