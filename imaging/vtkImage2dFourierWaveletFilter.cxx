/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dFourierWaveletFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImage2dFourierWaveletFilter.hh"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImage2dFourierWaveletFilter::vtkImage2dFourierWaveletFilter()
{
  this->SetAxes3d(VTK_IMAGE_COMPONENT_AXIS,VTK_IMAGE_X_AXIS,VTK_IMAGE_Y_AXIS);
  this->SetOutputDataType(VTK_IMAGE_FLOAT);
  this->Spacing = 2;
  this->Wavelets = NULL;
}


//----------------------------------------------------------------------------
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// The whole image is generated when any region is requested.
void
vtkImage2dFourierWaveletFilter::InterceptCacheUpdate(vtkImageRegion *region)
{
  int bounds[6];

  this->UpdateImageInformation(region);
  region->GetImageBounds3d(bounds);
  region->SetBounds3d(bounds);
}





//----------------------------------------------------------------------------
// Description:
// Just sets the ImageBounds to be the bounds of the OutRegion.
void vtkImage2dFourierWaveletFilter::ComputeOutputImageInformation(
		    vtkImageRegion *inRegion, vtkImageRegion *outRegion)
{
  int imageBounds[6];
  int waveletBounds[6];
  float aspectRatio[3];
  int imageMin, imageMax, imageSize;
  int waveletMin, waveletMax, waveletSize;
  int idx;
  
  inRegion->GetImageBounds3d(imageBounds);
  this->Wavelets->GetBounds3d(waveletBounds);
  imageBounds[0] = waveletBounds[0];
  imageBounds[1] = waveletBounds[1];
  for (idx = 1; idx < 3; ++idx)
    {
    imageMin = imageBounds[idx*2];
    imageMax = imageBounds[idx*2+1];
    imageSize = imageMax - imageMin + 1;
    waveletMin = waveletBounds[idx*2];
    waveletMax = waveletBounds[idx*2+1];
    waveletSize = waveletMax - waveletMin + 1;
    // no boundary handling
    if (imageSize < waveletSize)
      {
      vtkErrorMacro(<< "ComputeOutputImageInformation: "
                    << "Wavelet too big for image");
      return;
      }
    imageBounds[idx*2+1] = imageMin + (imageSize-waveletSize) / this->Spacing;
    }
  outRegion->SetImageBounds3d(imageBounds);

  // Compute aspect ratio (Components?)
  inRegion->GetAspectRatio3d(aspectRatio);
  aspectRatio[0] = 0.0;
  for (idx = 1; idx < 3; ++idx)
    {
    aspectRatio[idx] *= this->Spacing;
    }
  
  outRegion->SetAspectRatio3d(aspectRatio);
}


  
//----------------------------------------------------------------------------
void vtkImage2dFourierWaveletFilter::ComputeRequiredInputRegionBounds(
					      vtkImageRegion *outRegion, 
					      vtkImageRegion *inRegion)
{
  int bounds[6];
  
  outRegion = outRegion;
  inRegion->GetImageBounds3d(bounds);
  // Only take the first component, but the whole image in other dimensions.
  bounds[1] = bounds[0];
  inRegion->SetBounds3d(bounds);
}




//----------------------------------------------------------------------------
// Description:
// Initializes the wavelets
void vtkImage2dFourierWaveletFilter::InitializeWavelets(int dim)
{
  int idx1, idx2;
  int waveletIdx, f1, f2;
  
  // Free any previous wavelets
  if (this->Wavelets)
    {
    this->Wavelets->Delete();
    }
  // Allocate new region for the wavelets
  this->Wavelets = new vtkImageRegion;
  this->Wavelets->SetDataType(VTK_IMAGE_FLOAT);
  this->Wavelets->SetAxes3d(VTK_IMAGE_COMPONENT_AXIS,
			    VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->Wavelets->SetBounds3d(0,(dim*dim)-1, 0,dim-1, 0,dim-1);
  this->Wavelets->Allocate();
  if ( ! this->Wavelets->IsAllocated())
    {
    this->Wavelets->Delete();
    this->Wavelets = NULL;
    vtkErrorMacro(<< "InitializeWavelets: could not allocate region.");
    return;
    }

  // loop through all Fourier "wavelets" (try to get low freq first)
  waveletIdx = 0;
  for (idx1 = 0; idx1 < 2 * dim; ++idx1)
    {
    for(idx2 = 0; idx2 <= idx1; ++idx2)
      {
      f1 = idx1 - idx2;
      f2 = idx2;
      // Since we are parsing them at a diagonal, check if valid.
      if (f1 >= 0 && f1 < dim && f2 >= 0 && f2 < dim)
	{
	if (waveletIdx >= dim * dim)
	  {
	  // We must have found them all
	  return;
	  }
	// Set the Wavelet.
	this->ComputeWaveletReal(f1, f2, waveletIdx);
	if (this->TestWaveletOrthogonality(waveletIdx))
	  {
	  vtkDebugMacro(<< "InitializeWavelets: Keeping Real ("
	                << f1 << ", " << f2 << ").");
	  ++waveletIdx;
	  }
	if (waveletIdx >= dim * dim)
	  {
	  // We must have found them all
	  return;
	  }
	this->ComputeWaveletImaginary(f1, f2, waveletIdx);
	if (this->TestWaveletOrthogonality(waveletIdx))
	  {
	  vtkDebugMacro(<< "InitializeWavelets: Keeping Imaginary ("
	                << f1 << ", " << f2 << ").");
	  ++waveletIdx;
	  }
	}
      }
    }
}

//----------------------------------------------------------------------------
// Description:
// Computes the real component for a wavelet.
void vtkImage2dFourierWaveletFilter::ComputeWaveletReal(int f1, int f2, int w0)
{
  float *waveletPtr1, *waveletPtr2;
  int waveletInc0, waveletInc1, waveletInc2;
  int waveletBounds[6], waveletSize1, waveletSize2;
  float r1, i1, r2, i2;
  int idx1, idx2;
  
  this->Wavelets->GetIncrements3d(waveletInc0, waveletInc1, waveletInc2);
  this->Wavelets->GetBounds3d(waveletBounds);
  waveletSize1 = waveletBounds[3] - waveletBounds[2] + 1;
  waveletSize2 = waveletBounds[5] - waveletBounds[4] + 1;
  
  waveletPtr2 = (float *)(this->Wavelets->GetVoidPointer3d(w0, 0, 0));
  for (idx2 = 0; idx2 < waveletSize2; ++idx2)
    {
    r2 = cos(6.2831853 * (float)(f2 * idx2) / (float)(waveletSize2));
    i2 = sin(6.2831853 * (float)(f2 * idx2) / (float)(waveletSize2));
    waveletPtr1 = waveletPtr2;
    for (idx1 = 0; idx1 < waveletSize1; ++idx1)
      {
      r1 = cos(6.2831853 * (float)(f1 * idx1) / (float)(waveletSize1));
      i1 = sin(6.2831853 * (float)(f1 * idx1) / (float)(waveletSize1));
      // Complex multiply (real part)
      *waveletPtr1 = r1 * r2 - i1 * i2;
      
      // Hack to keep lowres about the same range
      if (f1 == 0 && f2 == 0)
	{
	*waveletPtr1 /= waveletSize1 * waveletSize2;
	}

      waveletPtr1 += waveletInc1;
      }
    waveletPtr2 += waveletInc2;
    }
}

//----------------------------------------------------------------------------
// Description:
// Computes the imaginary component for a wavelet.
void vtkImage2dFourierWaveletFilter::ComputeWaveletImaginary(int f1, int f2,
							     int w0)
{
  float *waveletPtr1, *waveletPtr2;
  int waveletInc0, waveletInc1, waveletInc2;
  int waveletBounds[6], waveletSize1, waveletSize2;
  float r1, i1, r2, i2;
  int idx1, idx2;
  
  this->Wavelets->GetIncrements3d(waveletInc0, waveletInc1, waveletInc2);
  this->Wavelets->GetBounds3d(waveletBounds);
  waveletSize1 = waveletBounds[3] - waveletBounds[2] + 1;
  waveletSize2 = waveletBounds[5] - waveletBounds[4] + 1;
  
  waveletPtr2 = (float *)(this->Wavelets->GetVoidPointer3d(w0, 0, 0));
  for (idx2 = 0; idx2 < waveletSize2; ++idx2)
    {
    r2 = cos(6.2831853 * (float)(f2 * idx2) / (float)(waveletSize2));
    i2 = sin(6.2831853 * (float)(f2 * idx2) / (float)(waveletSize2));
    waveletPtr1 = waveletPtr2;
    for (idx1 = 0; idx1 < waveletSize1; ++idx1)
      {
      r1 = cos(6.2831853 * (float)(f1 * idx1) / (float)(waveletSize1));
      i1 = sin(6.2831853 * (float)(f1 * idx1) / (float)(waveletSize1));
      // Complex multiply (real part)
      *waveletPtr1 = r1 * i2 + i1 * r2;

      waveletPtr1 += waveletInc1;
      }
    waveletPtr2 += waveletInc2;
    }
}

//----------------------------------------------------------------------------
// Description:
// Checks if wavelet is orthogonal to all previous wavelets.
int vtkImage2dFourierWaveletFilter::TestWaveletOrthogonality(int waveletIdx)
{
  int idx0, idx1, idx2;
  int min0, max0, min1, max1, min2, max2;
  int inc0, inc1, inc2;
  float *waveletPtr0, *waveletPtr1, *waveletPtr2;
  float *otherPtr0, *otherPtr1, *otherPtr2;
  double dot, mag;

  
  this->Wavelets->GetBounds3d(min0, max0, min1, max1, min2, max2);
  this->Wavelets->GetIncrements3d(inc0, inc1, inc2);
  
  // loop through all the wavelets before the one we are testing.
  waveletPtr0 =
    (float *)(this->Wavelets->GetVoidPointer3d(waveletIdx,min1,min2));
  otherPtr0 = (float *)(this->Wavelets->GetVoidPointer3d());
  for (idx0 = min0; idx0 < waveletIdx; ++idx0)
    {
    // Compute magnitude of wavelet and dot of wavelet and other.
    // Of course it is wasteful to comput magnitude multiple times.
    mag = dot = 0.0;
    otherPtr1 = otherPtr0;
    waveletPtr1 = waveletPtr0;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      otherPtr2 = otherPtr1;
      waveletPtr2 = waveletPtr1;
      for (idx2 = min2; idx2 <= max2; ++idx2)
	{
	mag += *waveletPtr2 * *waveletPtr2;
	dot += *waveletPtr2 * *otherPtr2;
	
	waveletPtr2 += inc2;
	otherPtr2 += inc2;
	}
      waveletPtr1 += inc1;
      otherPtr1 += inc1;
      }
    
    if (dot > 0.001 || dot < -0.001 || mag < 0.001)
      {
      return 0;
      }

    otherPtr0 += inc0;
    }
  
  return 1;
}

  


//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// Create an addition resolution level.
// Regions bound only the lores. quadrent of the last resolution level.
template <class T>
void 
vtkImage2dFourierWaveletFilterExecute(vtkImage2dFourierWaveletFilter *self,
				      vtkImageRegion *inRegion, T *inPtr,
				      vtkImageRegion *outRegion, float *outPtr)
{
  vtkImageRegion *wavelets;

  T *inPtr1, *inPtr2;
  T *inTmpPtr1, *inTmpPtr2;
  int inInc0, inInc1, inInc2;
  float *outPtr0, *outPtr1, *outPtr2;
  int outInc0, outInc1, outInc2;
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  float *wavePtr, *wavePtr0, *wavePtr1, *wavePtr2;
  int waveInc0, waveInc1, waveInc2;
  int waveMin0, waveMax0, waveMin1, waveMax1, waveMin2, waveMax2;
  int waveIdx0, waveIdx1, waveIdx2;
  int spacing;
  
  
  // Get information to march through data 
  inRegion->GetIncrements3d(inInc0, inInc1, inInc2);
  outRegion->GetIncrements3d(outInc0, outInc1, outInc2);
  outRegion->GetBounds3d(outMin0,outMax0, outMin1,outMax1, outMin2,outMax2);
  // Get wavelet information
  wavelets = self->GetWavelets();
  wavelets->GetIncrements3d(waveInc0, waveInc1, waveInc2);
  wavelets->GetBounds3d(waveMin0,waveMax0,waveMin1,waveMax1,waveMin2,waveMax2);
  wavePtr = (float *)(wavelets->GetVoidPointer3d());

  spacing = self->GetSpacing();
  
  
  // Sanity check
  if (outMin0 != waveMin0 || outMax0 != waveMax0)
    {
    cerr << "ERROR: Execute: Components do not match.\n";
    return;
    }
  
  // Loop through output pixels
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  // 1st spatial axis (X)
  for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
    {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    // 2nd spatial axis (Y)
    for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
      {
      
      // Initialize output sum to 0.0;
      outPtr0 = outPtr1;
      for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
	{
	*outPtr0 = 0.0;
	outPtr0 += outInc0;
	}
      
      // Compute sums
      // Loop through neighborhood.
      wavePtr2 = wavePtr;
      inTmpPtr2 = inPtr1;
      for (waveIdx2 = waveMin2; waveIdx2 <= waveMax2; ++waveIdx2)
	{
	wavePtr1 = wavePtr2;
	inTmpPtr1 = inTmpPtr2;
	for (waveIdx1 = waveMin1; waveIdx1 <= waveMax1; ++waveIdx1)
	  {

	  // loop through all wavelets
	  wavePtr0 = wavePtr1;
	  outPtr0 = outPtr1;
	  for (waveIdx0 = waveMin0; waveIdx0 <= waveMax0; ++waveIdx0)
	    {
	    *outPtr0 += *wavePtr0 * *inTmpPtr1;
	     
	    wavePtr0 += waveInc0;
	    outPtr0 += outInc0;
	    }
	  
	  wavePtr1 += waveInc1;
	  inTmpPtr1 += inInc1;
	  }
	wavePtr2 += waveInc2;
	inTmpPtr2 += inInc2;
	}
      
      inPtr1 += inInc1 * spacing;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2 * spacing;
    outPtr2 += outInc2;
    }
}

    
//----------------------------------------------------------------------------
// Description:
// Execute can handle any input data type, but the output is always floats.
void vtkImage2dFourierWaveletFilter::Execute3d(vtkImageRegion *inRegion, 
					       vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;
  
  vtkDebugMacro(<< "Execute3d: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that output is float.
  if (outRegion->GetDataType() != VTK_IMAGE_FLOAT)
    {
    vtkErrorMacro(<< "Execute2d: Output must be floats");
    return;
    }

  inPtr = inRegion->GetVoidPointer2d();
  outPtr = outRegion->GetVoidPointer2d();

  switch (inRegion->GetDataType())
    {
    case VTK_IMAGE_FLOAT:
      vtkImage2dFourierWaveletFilterExecute(this,
				    inRegion, (float *)(inPtr), 
				    outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_INT:
      vtkImage2dFourierWaveletFilterExecute(this,
				    inRegion, (int *)(inPtr), 
				    outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_SHORT:
      vtkImage2dFourierWaveletFilterExecute(this,
				    inRegion, (short *)(inPtr), 
				    outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_SHORT:
      vtkImage2dFourierWaveletFilterExecute(this,
				    inRegion, (unsigned short *)(inPtr), 
				    outRegion, (float *)(outPtr));
      break;
    case VTK_IMAGE_UNSIGNED_CHAR:
      vtkImage2dFourierWaveletFilterExecute(this,
				    inRegion, (unsigned char *)(inPtr), 
				    outRegion, (float *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute3d: Unknown DataType");
      return;
    }
}
















