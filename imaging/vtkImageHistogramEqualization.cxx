/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHistogramEqualization.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
#include "vtkImageHistogramEqualization.h"
#include <math.h>
#include <stdlib.h>


//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageHistogramEqualization::vtkImageHistogramEqualization()
{
  this->AveragingRadius = 1;
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}

//----------------------------------------------------------------------------
void vtkImageHistogramEqualization::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
  os << indent << "AveragingRadius : (" << this->AveragingRadius << ")\n";
}

//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageHistogramEqualizationExecute(vtkImageHistogramEqualization *self,
				   vtkImageRegion *inRegion, T *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  T  *outPtr0,*outPtr1,value,average,maximum,minimum;
  int avgradius,avgwindow,defmin;

  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);
  avgradius      = self->GetAveragingRadius ();
  avgwindow      = 2 * avgradius + 1;

  maximum = 0;
  defmin = sizeof(minimum);
  minimum = (T)pow(2.0,double(8*defmin -1)) - 1;
  inPtr1  = inPtr ;
  outPtr1 = outPtr;
  // find max and min  ...
     for (idx1 = min1; idx1 <= max1; ++idx1){
    	 inPtr0  = inPtr1;
    	 outPtr0  = outPtr1;
    	 for (idx0 = min0; idx0 <= max0; ++idx0){
	     if (*inPtr0 > maximum) maximum = *inPtr0;
	     if (*inPtr0 < minimum) minimum = *inPtr0;
		 *outPtr0 = 0;
      	          inPtr0  += inInc0;
      	          outPtr0  += inInc0;
	 }
    	 inPtr1  += inInc1;
    	 outPtr1  += inInc1;
   }
   // get the original histogram ...
   int  orig_numberofbins = (int)(maximum+1);
   long *histogram = new long[orig_numberofbins];
   int idx;
   for ( idx =0; idx < orig_numberofbins; idx++) histogram[idx] = 0;
   inPtr1 = inPtr;
   for (idx1 = min1; idx1 <= max1; ++idx1){
       inPtr0 = inPtr1;
       for (idx0 = min0; idx0 <= max0; ++idx0){
            histogram[(int)(*inPtr0)]++;
	    inPtr0 += outInc0;
       }
       inPtr1 += outInc1;
   }
   // generate an equalized histogram ...
	int optimal_freq = (max0+1)*(max1+1)/orig_numberofbins;
	T *left  = new T[orig_numberofbins];
	T *right = new T[orig_numberofbins];
	for( idx =0; idx < orig_numberofbins; idx++) {	
		left[idx] = right[idx] = 0;
	}
        int total = 0;
	int current = 0;
	for ( idx =0; idx < orig_numberofbins; idx++) {
	    left[idx] = (T)current;
	    total += histogram[idx];
	    while( total > optimal_freq) {
		 total -= optimal_freq;
		 current++;
	    }
	    right[idx] = (T)current;
	}
        double sum;
	T *kptr0, *kptr1;
	int shiftptr = (outInc0*avgradius)+(outInc1*avgradius);
	double kernelsize = (avgwindow*avgwindow);
	int kdy,kdx;
	outPtr1   = outPtr;
	inPtr1    = inPtr;

	for (idx1 = (min1+avgradius); idx1 <= (max1-avgradius); ++idx1){
    	    outPtr0 = outPtr1;
	    inPtr0 = inPtr1;
    	    for (idx0 = (min0+avgradius); idx0 <= (max0-avgradius); ++idx0){
		if (left[(int)(*inPtr0)] == right[(int)(*inPtr0)])
                    *outPtr0 = left[(int)(*inPtr0)];
		else {
		     // average in (radius*2+1)*(radius*2+1)
		     sum = 0.0;
		     kptr1 = inPtr0 - shiftptr;
		     for (kdy=0;kdy<avgwindow;kdy++) {
			 kptr0 = kptr1;
		         for (kdx=0;kdx<avgwindow;kdx++) {
			     sum += (double)(*kptr0);
			     kptr0 += outInc0;
		         }
			 kptr1 += outInc1;
                     }
		     average = (T)(sum/kernelsize);
       if(average>right[(int)(*inPtr0)]) value = right[(int)(*inPtr0)];
       else if(average<left[(int)(*inPtr0)])value=left[(int)(*inPtr0)];
		     else value = average;
		     *outPtr0 = value;
		}   		
		outPtr0 += outInc0;
		inPtr0  += outInc0;
	    }
	    outPtr1 += outInc1;
	    inPtr1  += outInc1;
	}
	delete [] left;
	delete [] right;
	delete [] histogram;

}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageHistogramEqualization::Execute(vtkImageRegion *inRegion, 
				  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
                  << ", must match out ScalarType " << outRegion->GetScalarType());
    return;
    }
  
  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageHistogramEqualizationExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageHistogramEqualizationExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageHistogramEqualizationExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageHistogramEqualizationExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageHistogramEqualizationExecute(this, 
			  inRegion, (unsigned char *)(inPtr), 
			  outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a region that holds the boundary of this filters
// input, and changes the region to hold the boundary of this filters
// output.
void 
vtkImageHistogramEqualization::ComputeOutputImageInformation(vtkImageRegion *inRegion,
						 vtkImageRegion *outRegion)
{
  int extent[4];
  inRegion->GetImageExtent(2, extent);
  outRegion->SetImageExtent(2, extent);
}





//----------------------------------------------------------------------------
// Description:
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the 
// extent of the output region.  After this method finishes, "region" should 
// have the extent of the required input region.
void vtkImageHistogramEqualization::ComputeRequiredInputRegionExtent(
						    vtkImageRegion *outRegion,
			                            vtkImageRegion *inRegion)
{
  int imageExtent[4];
  
  outRegion->GetImageExtent(2, imageExtent);
  inRegion->SetExtent(2, imageExtent);
}

//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// We might as well create both real and imaginary components.
void vtkImageHistogramEqualization::InterceptCacheUpdate(vtkImageRegion *region)
{
  int imageExtent[4];
  
  region->GetImageExtent(2, imageExtent);
  region->SetExtent(2, imageExtent);
}





