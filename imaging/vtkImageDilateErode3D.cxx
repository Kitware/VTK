/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDilateErode3D.cxx
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
#include "vtkImageDilateErode3D.h"


//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImageDilateErode3D fitler.
// By default zero values are dilated.
vtkImageDilateErode3D::vtkImageDilateErode3D()
{
  this->DilateValue = 0.0;
  this->ErodeValue = 255.0;
  this->HandleBoundaries = 1;
  this->Mask = NULL;
  this->KernelSize[0] = 1;
  this->KernelSize[1] = 1;
  this->KernelSize[2] = 1;
  this->KernelSize[3] = 1;

  this->SetFilteredAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS, VTK_IMAGE_Z_AXIS);
}


//----------------------------------------------------------------------------
void vtkImageDilateErode3D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSpatialFilter::PrintSelf(os,indent);
  os << indent << "Dilate Value: " << this->DilateValue << "\n";
  os << indent << "Erode Value: " << this->ErodeValue << "\n";
}

//----------------------------------------------------------------------------
void vtkImageDilateErode3D::SetFilteredAxes(int axis0, int axis1, int axis2)
{
  int axes[3];
  
  axes[0] = axis0;
  axes[1] = axis1;
  axes[2] = axis2;
  this->vtkImageSpatialFilter::SetFilteredAxes(3, axes);
}
  
//----------------------------------------------------------------------------
// Description:
// This method sets the size of the neighborhood.  It also sets the 
// default middle of the neighborhood and computes the eliptical foot print.
void vtkImageDilateErode3D::SetKernelSize(int size0, int size1, int size2)
{
  int modified = 0;
  
  if (this->KernelSize[0] != size0)
    {
    modified = 1;
    this->KernelSize[0] = size0;
    this->KernelMiddle[0] = size0 / 2;
    }
  if (this->KernelSize[1] != size1)
    {
    modified = 1;
    this->KernelSize[1] = size1;
    this->KernelMiddle[1] = size1 / 2;
    }
  if (this->KernelSize[2] != size2)
    {
    modified = 1;
    this->KernelSize[2] = size2;
    this->KernelMiddle[2] = size2 / 2;
    }

  if (modified)
    {
    this->Modified();
    this->ComputeMask();
    }
}

//----------------------------------------------------------------------------
void vtkImageDilateErode3D::ComputeMask()
{
  double f0, f1, f2;
  double radius0, radius1, radius2;
  int inc0, inc1, inc2;
  int idx0, idx1, idx2;
  unsigned char *ptr0, *ptr1, *ptr2;

  // create the eliptical mask
  if (this->Mask)
    {
    this->Mask->Delete();
    }
  this->Mask = vtkImageRegion::New();
  this->Mask->SetScalarType(VTK_UNSIGNED_CHAR);
  this->Mask->SetExtent(0, this->KernelSize[0]-1, 
			0, this->KernelSize[1]-1, 
			0, this->KernelSize[2]-1);
  this->Mask->AllocateScalars();
  if ( ! this->Mask->AreScalarsAllocated())
    {
    this->Mask->Delete();
    this->Mask = NULL;
    vtkErrorMacro(<< "SetKernelSize: Allocation of mask failed.");
    return;
    }
  
  radius0 = (double)(this->KernelSize[0]) / 2.0;
  radius1 = (double)(this->KernelSize[1]) / 2.0;
  radius2 = (double)(this->KernelSize[2]) / 2.0;
  
  this->Mask->GetIncrements(inc0, inc1, inc2);
  ptr2 = (unsigned char *)(this->Mask->GetScalarPointer());
  for (idx2 = 0; idx2 < this->KernelSize[2]; ++idx2)
    {
    ptr1 = ptr2;
    for (idx1 = 0; idx1 < this->KernelSize[1]; ++idx1)
      {
      ptr0 = ptr1;
      for (idx0 = 0; idx0 < this->KernelSize[0]; ++idx0)
	{
	// convert xyz to values in the range of [-1,1]
	f0 = ((double)(idx0) - radius0 + 0.5) / (radius0);
	f1 = ((double)(idx1) - radius1 + 0.5) / (radius1);
	f2 = ((double)(idx2) - radius2 + 0.5) / (radius2);
	
	if (f0*f0 + f1*f1 + f2*f2 <= 1.0)
	  {
	  *ptr0 = 255;
	  }
	else
	  {
	  *ptr0 = 0;
	  }
	
	ptr0 += inc0;
	}
      ptr1 += inc1;
      }
    ptr2 += inc2;
    }
}


//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter on any region,
// whether it needs boundary checking or not.
// If the filter needs to be faster, the function could be duplicated
// for strictly center (no boundary ) processing.
template <class T>
static void vtkImageDilateErode3DExecute(vtkImageDilateErode3D *self,
					 vtkImageRegion *inRegion, T *inPtr, 
					 vtkImageRegion *outRegion, T *outPtr,
					 int boundaryFlag)
{
  T erodeValue, dilateValue;
  int *kernelMiddle, *kernelSize;
  // For looping though output (and input) pixels.
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int outIdx0, outIdx1, outIdx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  T *outPtr0, *outPtr1, *outPtr2;
  // For looping through hood pixels
  int hoodMin0, hoodMax0, hoodMin1, hoodMax1, hoodMin2, hoodMax2;
  int hoodIdx0, hoodIdx1, hoodIdx2;
  T *hoodPtr0, *hoodPtr1, *hoodPtr2;
  // For looping through the mask.
  unsigned char *maskPtr, *maskPtr0, *maskPtr1, *maskPtr2;
  int maskInc0, maskInc1, maskInc2;
  vtkImageRegion *mask;
  // The extent of the whole input image
  int inImageMin0, inImageMin1, inImageMin2;
  int inImageMax0, inImageMax1, inImageMax2;
  
  
  // Get information to march through data
  inRegion->GetIncrements(inInc0, inInc1, inInc2); 
  inRegion->GetWholeExtent(inImageMin0, inImageMax0, inImageMin1,
			   inImageMax1, inImageMin2, inImageMax2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2); 
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  
  // Get ivars of this object (easier than making friends)
  erodeValue = (T)(self->GetErodeValue());
  dilateValue = (T)(self->GetDilateValue());
  kernelSize = self->KernelSize;
  kernelMiddle = self->KernelMiddle;
  hoodMin0 = - kernelMiddle[0];
  hoodMin1 = - kernelMiddle[1];
  hoodMin2 = - kernelMiddle[2];
  hoodMax0 = hoodMin0 + kernelSize[0] - 1;
  hoodMax1 = hoodMin1 + kernelSize[1] - 1;
  hoodMax2 = hoodMin2 + kernelSize[2] - 1;

  // Setup mask info
  mask = self->GetMask();
  maskPtr = (unsigned char *)(mask->GetScalarPointer());
  mask->GetIncrements(maskInc0, maskInc1, maskInc2);
  
  // in and out should be marching through corresponding pixels.
  inPtr = (T *)(inRegion->GetScalarPointer(outMin0, outMin1, outMin2));
  
  // loop through pixels of output
  outPtr2 = outPtr;
  inPtr2 = inPtr;
  for (outIdx2 = outMin2; outIdx2 <= outMax2; ++outIdx2)
    {
    outPtr1 = outPtr2;
    inPtr1 = inPtr2;
    for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
      {
      outPtr0 = outPtr1;
      inPtr0 = inPtr1;
      for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
	{

	// Default behavior (copy input pixel)
	*outPtr0 = *inPtr0;
	if (*inPtr0 == erodeValue)
	  {
	  // loop through neighborhood pixels
	  // as sort of a hack to handle boundaries, 
	  // input pointer will be marching through data that does not exist.
	  hoodPtr2 = inPtr0 - kernelMiddle[0] * inInc0 
	    - kernelMiddle[1] * inInc1 - kernelMiddle[2] * inInc2;
	  maskPtr2 = maskPtr;
	  for (hoodIdx2 = hoodMin2; hoodIdx2 <= hoodMax2; ++hoodIdx2)
	    {
	    hoodPtr1 = hoodPtr2;
	    maskPtr1 = maskPtr2;
	    for (hoodIdx1 = hoodMin1; hoodIdx1 <= hoodMax1; ++hoodIdx1)
	      {
	      hoodPtr0 = hoodPtr1;
	      maskPtr0 = maskPtr1;
	      for (hoodIdx0 = hoodMin0; hoodIdx0 <= hoodMax0; ++hoodIdx0)
		{
		// A quick but rather expensive way to handle boundaries
		if ( ! boundaryFlag ||
		    (outIdx0 + hoodIdx0 >= inImageMin0 &&
		     outIdx0 + hoodIdx0 <= inImageMax0 &&
		     outIdx1 + hoodIdx1 >= inImageMin1 &&
		     outIdx1 + hoodIdx1 <= inImageMax1 &&
		     outIdx2 + hoodIdx2 >= inImageMin2 &&
		     outIdx2 + hoodIdx2 <= inImageMax2))
		  {
		  if (*hoodPtr0 == dilateValue && *maskPtr0)
		    {
		    *outPtr0 = dilateValue;
		    }
		  }

		hoodPtr0 += inInc0;
		maskPtr0 += maskInc0;
		}
	      hoodPtr1 += inInc1;
	      maskPtr1 += maskInc1;
	      }
	    hoodPtr2 += inInc2;
	    maskPtr2 += maskInc2;
	    }
	  }
	
	inPtr0 += inInc0;
	outPtr0 += outInc0;
	}
      inPtr1 += inInc1;
      outPtr1 += outInc1;
      }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
    }
}

//----------------------------------------------------------------------------
// Description:
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
// This function deals with regions that are in the center of the image and 
// need no boundary checking.
void vtkImageDilateErode3D::ExecuteCenter(vtkImageRegion *inRegion, 
					  vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);

  // Error checking on mask
  if ( ! this->Mask || (this->Mask->GetScalarType() != VTK_UNSIGNED_CHAR))
    {
    vtkErrorMacro(<< "Execute: Bad Mask");
    return;
    }

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
      vtkImageDilateErode3DExecute(this, inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr), 0);
      break;
    case VTK_INT:
      vtkImageDilateErode3DExecute(this, inRegion, (int *)(inPtr), 
				   outRegion, (int *)(outPtr), 0);
      break;
    case VTK_SHORT:
      vtkImageDilateErode3DExecute(this, inRegion, (short *)(inPtr), 
				   outRegion, (short *)(outPtr), 0);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDilateErode3DExecute(this, inRegion, (unsigned short *)(inPtr), 
				   outRegion, (unsigned short *)(outPtr), 0);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDilateErode3DExecute(this, inRegion, (unsigned char *)(inPtr), 
				   outRegion, (unsigned char *)(outPtr), 0);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method contains the first switch statement that calls the correct
// templated function for the input and output region types.
// It hanldes image boundaries, so the image does not shrink.
void vtkImageDilateErode3D::Execute(vtkImageRegion *inRegion, 
				    vtkImageRegion *outRegion)
{
  void *inPtr = inRegion->GetScalarPointer();
  void *outPtr = outRegion->GetScalarPointer();
  
  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);

  // Error checking on mask
  if ( ! this->Mask)
    {
    this->ComputeMask();
    }
  if (this->Mask->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro(<< "Execute: Bad Mask");
    return;
    }

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
      vtkImageDilateErode3DExecute(this, inRegion, (float *)(inPtr), 
				   outRegion, (float *)(outPtr), 1);
      break;
    case VTK_INT:
      vtkImageDilateErode3DExecute(this, inRegion, (int *)(inPtr), 
				   outRegion, (int *)(outPtr), 1);
      break;
    case VTK_SHORT:
      vtkImageDilateErode3DExecute(this, inRegion, (short *)(inPtr), 
				   outRegion, (short *)(outPtr), 1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageDilateErode3DExecute(this, inRegion, (unsigned short *)(inPtr), 
				   outRegion, (unsigned short *)(outPtr), 1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageDilateErode3DExecute(this, inRegion, (unsigned char *)(inPtr), 
				   outRegion, (unsigned char *)(outPtr), 1);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
