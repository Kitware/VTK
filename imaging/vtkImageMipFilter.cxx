/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMipFilter.cxx
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
#include "vtkImageMipFilter.h"



//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageMipFilter::vtkImageMipFilter()
{
  this->ProjectionRange[0] = 0;
  this->ProjectionRange[1] = 0;
  this->MinMaxIP = 1;
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS,VTK_IMAGE_Z_AXIS);
}



//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageMipFilterExecute(vtkImageMipFilter *self,
				   vtkImageRegion *inRegion, T *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1,idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1;
  int prorange[2], minmaxip;
  
  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1, inInc2);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);
  self->GetProjectionRange(prorange[0],prorange[1]);
  minmaxip = self->GetMinMaxIP();

	//  Loop first through projection range then along the other two axes
	inPtr1  = inPtr ;
        outPtr1 = outPtr;

    if ( minmaxip == 1) {
	for (idx1 = min1; idx1 <= max1; ++idx1){
    	    outPtr0 = outPtr1;
    	    inPtr0  = inPtr1;
    	    for (idx0 = min0; idx0 <= max0; ++idx0){
		*outPtr0 = 0;
		inPtr2  = inPtr0;
		for (idx2 = prorange[0];idx2 < prorange[1];idx2++){
		    if (*inPtr2 > *outPtr0) *outPtr0 = *inPtr2;
      	               inPtr2 += inInc2;
		}
		outPtr0 += outInc0;
      	        inPtr0  += inInc0;
	    }
	    outPtr1 += outInc1;
    	    inPtr1  += inInc1;
	}
   }
   else if ( minmaxip == 0) {
	for (idx1 = min1; idx1 <= max1; ++idx1){
    	    outPtr0 = outPtr1;
    	    inPtr0  = inPtr1;
    	    for (idx0 = min0; idx0 <= max0; ++idx0){
		// need to find optimum minimum !!! interesting
		*outPtr0 = 32767;
		inPtr2  = inPtr0;
		for (idx2 = prorange[0];idx2 < prorange[1];idx2++){
		    if (*inPtr2 < *outPtr0) *outPtr0 = *inPtr2;
      	               inPtr2 += inInc2;
		}
		outPtr0 += outInc0;
      	        inPtr0  += inInc0;
	    }
	    outPtr1 += outInc1;
    	    inPtr1  += inInc1;
        }
   }
   else {
	cerr << "Not Valid value for MinMaxIP, must be either 0 or 1" << endl;
        return;
   }




}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageMipFilter::Execute(vtkImageRegion *inRegion, 
				  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetDataType() != outRegion->GetDataType())
    {
    vtkErrorMacro(<< "Execute: input DataType, " << inRegion->GetDataType()
                  << ", must match out DataType " << outRegion->GetDataType());
    return;
    }
  
  switch (inRegion->GetDataType())
    {
    case VTK_FLOAT:
      vtkImageMipFilterExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageMipFilterExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageMipFilterExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageMipFilterExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageMipFilterExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown DataType");
      return;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the boundary of this filters
// input, and changes the region to hold the boundary of this filters
// output.
void 
vtkImageMipFilter::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						 vtkImageRegion *outRegion)
{
  int extent[6];


  // reduce extent from 3 to 2 D.
  inRegion->GetImageExtent(extent, 3);
  extent[4] = 0; extent[5] =0;
  outRegion->SetImageExtent(extent, 3);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageMipFilter::ComputeRequiredInputRegionExtent(
                                                    vtkImageRegion *outRegion, 
			                            vtkImageRegion *inRegion)
{
  int extent[6];
  int imageExtent[6];
  
  outRegion->GetExtent(extent, 3);
  
  inRegion->GetImageExtent(imageExtent, 3);

  extent[4] = this->ProjectionRange[0];
  extent[5] = this->ProjectionRange[1];

  inRegion->SetExtent(extent, 3);
}













