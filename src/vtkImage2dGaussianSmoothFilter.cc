/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2dGaussianSmoothFilter.cc
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
#include<math.h>
#include "vtkImage2dGaussianSmoothFilter.hh"
#include "vtkImageCache.hh"

//----------------------------------------------------------------------------
// Description:
// This method sets up the 2 1d filters that perform the convolution.
vtkImage2dGaussianSmoothFilter::vtkImage2dGaussianSmoothFilter()
{
  // create the filter chain 
  this->Filter1 = new vtkImage1dGaussianSmoothFilter;
  this->Filter2 = new vtkImage1dGaussianSmoothFilter;
  this->Filter2->SetInput(this->Filter1->GetOutput());

  // default values for the smoothing plane 
  this->Axis1 = 0;
  this->Axis2 = 1;
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Delete the sub filters.
vtkImage2dGaussianSmoothFilter::~vtkImage2dGaussianSmoothFilter()
{
  this->Filter1->Delete();
  this->Filter2->Delete();
}


//----------------------------------------------------------------------------
// Description:
// Turn debugging output on. (in sub filters also)
void vtkImage2dGaussianSmoothFilter::DebugOn()
{
  this->vtkObject::DebugOn();
  this->Filter1->DebugOn();
  this->Filter2->DebugOn();
}


//----------------------------------------------------------------------------
// Description:
// Pass modified message to sub filters.
void vtkImage2dGaussianSmoothFilter::Modified()
{
  this->vtkObject::Modified();
  this->Filter1->Modified();
  this->Filter2->Modified();
}


//----------------------------------------------------------------------------
// Description:
// Set the Input of the filter.
void vtkImage2dGaussianSmoothFilter::SetInput(vtkImageSource *input)
{
  this->Input = input;
  this->Modified();

  vtkDebugMacro(<< "SetInput: " << input->GetClassName()
		<< " (" << input << ")");

  // set the input of the first sub filter 
  this->Filter1->SetInput(input);
}



//----------------------------------------------------------------------------
// Description:
// Set the plane of the smoothing.
void vtkImage2dGaussianSmoothFilter::SetAxes(int axis1, int axis2)
{
  vtkDebugMacro(<< "SetAxes: axis1 = " << axis1 << ", axis2 = " << axis2);

  this->Filter1->SetAxis(axis1);
  this->Filter2->SetAxis(axis2);
  this->Modified();
}




//----------------------------------------------------------------------------
// Description:
// This method sets the kernal. Both axes are the same.  
// A future simple extension could make the kernel eliptical.
void vtkImage2dGaussianSmoothFilter::SetGauss(float Std, int Radius)
{
  vtkDebugMacro(<< "SetGauss: Std = " << Std << ", Radius = " << Radius);

  this->Filter1->SetGauss(Std, Radius);
  this->Filter2->SetGauss(Std, Radius);
  this->Modified();
}



//----------------------------------------------------------------------------
// Description:
// This method fills a requested region. 
// It justs feeds the request to the sub filter.
void vtkImage2dGaussianSmoothFilter::GenerateRegion(int *outOffset, 
						    int *outSize)
{
  vtkDebugMacro(<< "GenerateRegion: offset = (" 
                << outOffset[0] << ", " << outOffset[1] << ", " << outOffset[2]
                << "), size = (" 
                << outSize[0] << ", " << outSize[1] << ", " << outSize[2] 
                << ")");

  this->Filter2->GenerateRegion(outOffset, outSize);
}
  
//----------------------------------------------------------------------------
// Description:
// This method sets the cache object of the filter.
// It justs feeds the request to the sub filter.
void vtkImage2dGaussianSmoothFilter::SetCache(vtkImageCache *cache)
{
  vtkDebugMacro(<< "SetCache: (" << cache << ")");

  this->Filter2->SetCache(cache);
}
  
//----------------------------------------------------------------------------
// Description:
// This method returns the cache to make a connection
// It justs feeds the request to the sub filter.
vtkImageSource *vtkImage2dGaussianSmoothFilter::GetOutput()
{
  vtkImageSource *source;

  source = this->Filter2->GetOutput();
  vtkDebugMacro(<< "GetOutput: returning source "
                << source->GetClassName() << " (" << source << ")");

  return source;
}
  
//----------------------------------------------------------------------------
// Description:
// This method returns the l;ast cache of the internal pipline.
vtkImageCache *vtkImage2dGaussianSmoothFilter::GetCache()
{
  vtkImageCache *cache;

  cache = this->Filter2->GetCache();
  vtkDebugMacro(<< "GetOutput: returning cache "
                << cache->GetClassName() << " (" << cache << ")");

  return cache;
}
  


//----------------------------------------------------------------------------
// Description:
// This method returns the largest region that can be requested.
// It justs feeds the request to the sub filter.
void vtkImage2dGaussianSmoothFilter::GetBoundary(int *offset, int *size)
{
  vtkImageSource *source;

  source = this->Filter2->GetOutput();
  source->GetBoundary(offset, size);
  
  vtkDebugMacro(<< "GetBoundary: returning offset = ("
          << offset[0] << ", " << offset[1] << ", " << offset[2]
          << "), size = (" << size[0] << ", " << size[1] << ", " << size[2]
          << ")");  
}
  

//----------------------------------------------------------------------------
// Description:
// This Method returns the MTime of the pipeline before this filter.
// It propagates the message back.
unsigned long int vtkImage2dGaussianSmoothFilter::GetPipelineMTime()
{
  unsigned long int time1, time2;

  // This objects MTime
  time1 = this->GetMTime();

  // Pipeline mtime
  time2 = this->Filter2->GetPipelineMTime();
  
  // Return the larger of the two
  if (time2 > time1)
    time1 = time2;

  return time1;
}


















