/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageHistogram.cxx
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
#include "vtkImageHistogram.h"
#include <math.h>
#include <stdlib.h>


//----------------------------------------------------------------------------
// Description:
// Constructor sets default values
vtkImageHistogram::vtkImageHistogram()
{
  this->NumberOfBins   = 256;
  this->OffsetLevel  = 0;
  this->OffsetOff();

  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
}

//----------------------------------------------------------------------------
void vtkImageHistogram::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
  os << indent << "NumberOfBins : (" << this->GetNumberOfBins() << ")\n";
  os << indent << "OffsetLevel : (" << this->GetOffsetLevel() << ")\n";
  os << indent << "Offset : (" << this->GetOffset() << ")\n";

}

//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class T>
void vtkImageHistogramExecute(vtkImageHistogram *self,
				   vtkImageRegion *inRegion, T *inPtr,
				   vtkImageRegion *outRegion, T *outPtr)
{
  int min0, max0, min1, max1;
  int idx0, idx1;
  int inInc0, inInc1;
  int outInc0, outInc1;
  T *inPtr0, *inPtr1;
  T *outPtr0,*outPtr1,value,maximum,minimum;
  int numberofbins,defmin;
  double scale;

  // Get information to march through data 
  inRegion->GetIncrements(inInc0, inInc1);
  outRegion->GetIncrements(outInc0, outInc1);
  outRegion->GetExtent(min0, max0, min1, max1);
  numberofbins = self->GetNumberOfBins();
 
   //  Find min & max of intensity value - number of levels ...
   maximum = 0;
   defmin = sizeof(minimum);
   minimum = (T)pow(2.0,double(8*defmin -1)) - 1;
   inPtr1  = inPtr ;
   outPtr1 = outPtr;
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
   int  orig_numberofbins = (int)(maximum - minimum+1);
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
   // zero out background ...
   int offsetlevel = self->GetOffsetLevel();
   if ( self->GetOffset()) {
      for ( idx =0; idx < offsetlevel; idx++) histogram[0] = 0;
      if (offsetlevel >= orig_numberofbins) offsetlevel= (orig_numberofbins-1);
      orig_numberofbins -= offsetlevel;
   }
   else offsetlevel = 0;

   // subsampled with summing to a desired number of levels or bins
   if (numberofbins <= 0 ) numberofbins = 1;
   double finbinsize = ((double)orig_numberofbins/(double)numberofbins);
   int binsize  = (int)(finbinsize + 0.5);
   if ( binsize <= 0 ) binsize = 1;
   int usedbins = orig_numberofbins / binsize;

   long *histogram2 = new long[usedbins];
   int jdx;
   double sum_binvalue;
   for ( idx =0; idx < usedbins; idx++) histogram2[idx] = 0;
   for ( idx =0; idx < usedbins; idx++) {
	sum_binvalue = 0.0;
	for (jdx =idx*binsize; jdx< (idx+1)*binsize; jdx++) {
	    sum_binvalue += (double)histogram[jdx + offsetlevel];
	}
	histogram2[idx] = (long)(sum_binvalue);
   }

   // keep the range that we can display ...
   long *histogram3 = new long[(max0+1)];
   for ( idx =0; idx < (max0+1); idx++) histogram3[idx] = 0;
   if ( (max0+1) > usedbins) 
      for ( idx =0; idx < usedbins; idx++) histogram3[idx] = histogram2[idx];
   else 
      for ( idx =0; idx < (max0+1); idx++) histogram3[idx] = histogram2[idx];

   // find max & min of histogram3...
   long min_hist;
   long max_hist = 0;
   defmin = sizeof(min_hist);
   min_hist = (long)pow(2.0,double(8*defmin -1)) - 1;
   for ( idx =0; idx < (max0+1); idx++) {
       if ( histogram3[idx] < min_hist) min_hist =  histogram3[idx];
       if ( histogram3[idx] > max_hist) max_hist =  histogram3[idx];
   }
   // scale histogram value to fit the y-image dimension
   scale = (double)(max1)/(double)(max_hist - min_hist);
   for ( idx =0; idx < (max0+1); idx++) {
	histogram3[idx] = (long)((double)(histogram3[idx] - min_hist) * scale) ;
   }
   

   // put the histogram in an image ...
   defmin = sizeof(value);
   value = (T)pow(2.0,double(8*defmin -1)) - 1;
   outPtr1 = outPtr;
   for (idx1 = max1; idx1 >= min1; --idx1){
       outPtr0 = outPtr1;
       for (idx0 = min0; idx0 <= max0; ++idx0){
            if ( (long)idx1 < histogram3[idx0] ) *outPtr0 = value;
	    else *outPtr0 =0;
	    outPtr0 += outInc0;
        }
	outPtr1 += outInc1;
   }
	delete [] histogram;
	delete [] histogram2;
	delete [] histogram3;
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageHistogram::Execute(vtkImageRegion *inRegion, 
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
      vtkImageHistogramExecute(this, 
			  inRegion, (float *)(inPtr), 
			  outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageHistogramExecute(this, 
			  inRegion, (int *)(inPtr), 
			  outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageHistogramExecute(this, 
			  inRegion, (short *)(inPtr), 
			  outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageHistogramExecute(this, 
			  inRegion, (unsigned short *)(inPtr), 
			  outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageHistogramExecute(this, 
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
vtkImageHistogram::ComputeOutputImageInformation(vtkImageRegion *inRegion,
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
void vtkImageHistogram::ComputeRequiredInputRegionExtent(
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
void vtkImageHistogram::InterceptCacheUpdate(vtkImageRegion *region)
{
  int imageExtent[4];
  
  region->GetImageExtent(2, imageExtent);
  region->SetExtent(2, imageExtent);
}





