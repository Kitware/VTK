/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageClip.cxx
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
#include "vtkImageCache.h"
#include "vtkImageRegion.h"
#include "vtkImageClip.h"


//----------------------------------------------------------------------------
vtkImageClip::vtkImageClip()
{
  int idx;

  this->Initialized = 0;
  this->Input = NULL;
  this->Automatic = 0;
  for (idx = 0; idx < 4; ++idx)
    {
    this->OutputWholeExtent[idx*2]  = -VTK_LARGE_INTEGER;
    this->OutputWholeExtent[idx*2+1] = VTK_LARGE_INTEGER;
    }
  this->NumberOfExecutionAxes = 5;
}


//----------------------------------------------------------------------------
void vtkImageClip::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkImageFilter::PrintSelf(os,indent);

  if (this->Automatic)
    {
    os << indent << "Automatic: On\n";
    }
  else
    {
    os << indent << "Automatic: Off\n";
    }

  os << indent << "OutputWholeExtent: (" << this->OutputWholeExtent[0]
     << "," << this->OutputWholeExtent[1];
  for (idx = 1; idx < 4; ++idx)
    {
    os << indent << ", " << this->OutputWholeExtent[idx * 2]
       << "," << this->OutputWholeExtent[idx*2 + 1];
    }
  os << ")\n";
}
  
//----------------------------------------------------------------------------
// Set the OutputWholeExtent (used internally also)
void vtkImageClip::SetOutputWholeExtent(int dim, int *extent)
{
  int idx;
  int modified = 0;
  
  if (dim > 4)
    {
    vtkWarningMacro("GetOutputWholeExtent: dim > 4");
    dim = 4;
    }

  dim = dim * 2;
  for (idx = 0; idx < dim; ++idx)
    {
    if (this->OutputWholeExtent[idx] != extent[idx])
      {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
      }
    }

  this->Initialized = 1;
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageClip::SetOutputAxisWholeExtent(int axis, int min, int max)
{
  if (axis < 0 || axis > 3)
    {
    vtkErrorMacro("SetOutputAxisWholeExtent: bad axis " << axis);
    return;
    }

  if (this->OutputWholeExtent[axis*2] != min ||
      this->OutputWholeExtent[axis*2+1] != max)
    {
    this->OutputWholeExtent[axis*2] = min;
    this->OutputWholeExtent[axis*2+1] = max;
    this->Modified();
    }
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Get the OutputWholeExtent.
void vtkImageClip::GetOutputWholeExtent(int dim, int *extent)
{
  int idx;
  
  if (dim > 4)
    {
    vtkWarningMacro("GetOutputWholeExtent: dim > 4");
    dim = 4;
    }
  
  if (this->Automatic)
    {
    this->ComputeOutputWholeExtent();
    }
  
  dim = dim * 2;
  for (idx = 0; idx < dim; ++idx)
    {
    extent[idx] = this->OutputWholeExtent[idx];
    }
}

//----------------------------------------------------------------------------
// Change the WholeExtent
void vtkImageClip::ExecuteImageInformation(vtkImageCache *in, 
					   vtkImageCache *out)
{
  int idx, extent[8];
  
  in->GetWholeExtent(extent);
  if ( ! this->Initialized)
    {
    this->SetOutputWholeExtent(4, extent);
    }
  if ( ! this->Bypass)
    {
    if (this->Automatic)
      {
      this->ComputeOutputWholeExtent();
      }
    
    // Clip the OutputWholeExtent with the input WholeExtent
    for (idx = 0; idx < 4; ++idx)
      {
      if (this->OutputWholeExtent[idx*2] > extent[idx*2])
	{
	extent[idx*2] = this->OutputWholeExtent[idx*2];
	}
      if (this->OutputWholeExtent[idx*2+1] < extent[idx*2+1])
	{
	extent[idx*2+1] = this->OutputWholeExtent[idx*2+1];
	}
      }
    }
  
  out->SetWholeExtent(extent);
}


//----------------------------------------------------------------------------
// Description:
// Sets the output whole extent to be the input whole extent.
void vtkImageClip::ResetOutputWholeExtent()
{
  if ( ! this->Input)
    {
    vtkWarningMacro("ResetOutputWholeExtent: No input");
    return;
    }

  this->Input->UpdateImageInformation();
  this->SetOutputWholeExtent(4, this->Input->GetWholeExtent());
}



//----------------------------------------------------------------------------
// Description:
// This method simply copies by reference the input data to the output.
void vtkImageClip::Update()
{
  // Make sure the Input has been set.
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input is not set.");
    return;
    }

  this->Input->SetUpdateExtent(this->Output->GetUpdateExtent());
  this->Input->Update();
  this->Output->SetScalarData(this->Input->GetScalarData());
  this->Output->SetNumberOfScalarComponents(
		      this->Input->GetNumberOfScalarComponents());
  // release input data
  if (this->Input->ShouldIReleaseData())
    {
    this->Input->ReleaseData();
    }
}

  
//----------------------------------------------------------------------------
// Clips the first axis
template <class T>
static void vtkImageClipAxis(vtkImageRegion *region, T backGround)
{
  int idx0, idx1, idx2, idx3, idx4;
  int inc0, inc1, inc2, inc3, inc4;
  int min0, max0, min1, max1, min2, max2, min3, max3, min4, max4;
  T *ptr0, *ptr1, *ptr2, *ptr3, *ptr4;
  int flag;  // signals loop to stop after non background pixel.
  
  region->GetExtent(min0,max0, min1,max1, min2,max2, min3,max3, min4,max4);
  region->GetIncrements(inc0, inc1, inc2, inc3, inc4);
  // loop to find min
  ptr0 = (T *)(region->GetScalarPointer(min0, min1, min2, min3, min4));
  flag = 1;
  for (idx0 = min0; idx0 <= max0 && flag; ++idx0)
    {
    ptr1 = ptr0;
    for (idx1 = min1; idx1 <= max1 && flag; ++idx1)
      {
      ptr2 = ptr1;
      for (idx2 = min2; idx2 <= max2 && flag; ++idx2)
	{
	ptr3 = ptr2;
	for (idx3 = min3; idx3 <= max3 && flag; ++idx3)
	  {
	  ptr4 = ptr3;
	  for (idx4 = min4; idx4 <= max4 && flag; ++idx4)
	    {
	    if (*ptr4 != backGround)
	      {
	      flag = 0;
	      min0 = idx0;
	      }
	    ptr4 += inc4;
	    }
	  ptr3 += inc3;
	  }
	ptr2 += inc2;
	}
      ptr1 += inc1;
      }
    ptr0 += inc0;
    }
  if (flag)
    { // the whole image is back ground.
    min0 = max0;
    }

  // loop to find max
  ptr0 = (T *)(region->GetScalarPointer(max0, min1, min2, min3, min4));
  flag = 1;
  for (idx0 = max0; idx0 >= min0 && flag; --idx0)
    {
    ptr1 = ptr0;
    for (idx1 = min1; idx1 <= max1 && flag; ++idx1)
      {
      ptr2 = ptr1;
      for (idx2 = min2; idx2 <= max2 && flag; ++idx2)
	{
	ptr3 = ptr2;
	for (idx3 = min3; idx3 <= max3 && flag; ++idx3)
	  {
	  ptr4 = ptr3;
	  for (idx4 = min4; idx4 <= max4 && flag; ++idx4)
	    {
	    if (*ptr4 != backGround)
	      {
	      flag = 0;
	      max0 = idx0;
	      }
	    ptr4 += inc4;
	    }
	  ptr3 += inc3;
	  }
	ptr2 += inc2;
	}
      ptr1 += inc1;
      }
    ptr0 -= inc0;
    }
  if (flag)
    { // the whole image is back ground.
    max0 = min0;
    }

  region->SetExtent(min0, max0);
}




//----------------------------------------------------------------------------
// The automatically computes a new WholeExtent.
template <class T>
static void vtkImageClipCompute(vtkImageRegion *region, T *ptr)
{
  int idx0, idx1, idx2, idx3, idx4, idx;
  int t0, t1, t2, t3, t4;
  int min0, max0, min1, max1, min2, max2, min3, max3, min4, max4;
  T backGround, v[32];
  int vc[32];
  int found, best, count = 0;
  int axes[5];

  // Find back ground pixel.
  for (idx = 0; idx < 32; ++idx)
    {
    vc[idx] = 0;
    }
  // loop though all the corners.
  region->GetExtent(min0,max0, min1,max1, min2,max2, min3,max3, min4,max4);
  for (idx4 = 0, t4 = min4; idx4 < 2; ++idx4, t4 = max4)
    {
    for (idx3 = 0, t3 = min3; idx3 < 2; ++idx3, t3 = max3)
      {
      for (idx2 = 0, t2 = min2; idx2 < 2; ++idx2, t2 = max2)
	{
	for (idx1 = 0, t1 = min1; idx1 < 2; ++idx1, t1 = max1)
	  {
	  for (idx0 = 0, t0 = min0; idx0 < 2; ++idx0, t0 = max0)
	    {
	    ptr = (T *)(region->GetScalarPointer(t0, t1, t2, t3, t4));
	    // see if this is a new pixel value.
	    for (found = 0, idx = 0; idx < count && found == 0; ++idx)
	      {
	      if (*ptr == v[idx])
		{
		found = 1;
		++vc[idx];
		}
	      }
	    if ( ! found)
	      {
	      v[count] = *ptr;
	      vc[count] = 1;
	      ++count;
	      }
	    }
	  }
	}
      }
    }
  // pick the value with the largest count.
  best = 0;
  for (idx = 0; idx < count; ++idx)
    {
    if (vc[idx] > best)
      {
      best = vc[idx];
      backGround = v[idx];
      }
    }
  
  // We have a backGround value, now shrink the axes one by one.
  region->GetAxes(5, axes);
  // do not bother with component axis
  for (idx = 0; idx < 4; ++ idx)
    {
    region->SetAxes(axes[idx]);
    vtkImageClipAxis(region, backGround);
    }
  
  // Restore original coordinate system.
  region->SetAxes(5, axes);
}
    
  
  
//----------------------------------------------------------------------------
void vtkImageClip::ComputeOutputWholeExtent()
{
  vtkImageRegion *region;
  void *ptr;
  
  // Only recompute if necessary
  // Do we really need this, or does pipline handle this?
  if (this->CTime.GetMTime() > this->GetPipelineMTime())
    {
    return;
    }
  
  // Get the entire input region.
  if ( ! this->Input)
    {
    vtkErrorMacro("ComputeOutputWholeExtent: No Input.");
    }
  this->Input->Update();
  region = this->Input->GetScalarRegion();

  // Look through every pixel.
  ptr = region->GetScalarPointer();
  switch (region->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageClipCompute(region, (float *)(ptr));
      break;
    case VTK_INT:
      vtkImageClipCompute(region, (int *)(ptr));
      break;
    case VTK_SHORT:
      vtkImageClipCompute(region, (short *)(ptr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageClipCompute(region, (unsigned short *)(ptr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageClipCompute(region, (unsigned char *)(ptr));
      break;
    default:
      vtkErrorMacro("ComputeOutputWholeExtent: Cannot handle ScalarType.\n");
      return;
    }         
  
  // This ignores the computed component extent.
  this->SetOutputWholeExtent(4, region->GetExtent());
  this->CTime.Modified();
  region->Delete();
}





