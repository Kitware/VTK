/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousDilate.cxx
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
#include "vtkImageContinuousDilate.h"

//----------------------------------------------------------------------------
vtkImageContinuousDilate::vtkImageContinuousDilate()
{
  int idx;
  vtkImageContinuousDilate1D *filter;

  for (idx = 0; idx < 4; ++idx)
    {
    filter = vtkImageContinuousDilate1D::New();
    this->Filters[idx] = filter;
    this->Strides[idx] = 1;
    this->KernelSize[idx] = 1;
    filter->SetFilteredAxis(idx);
    filter->SetStride(this->Strides[idx]);
    filter->SetKernelSize(this->KernelSize[idx]);
    }
  // Let the superclass set some superclass variables of the filters.
  this->InitializeFilters();
}


//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetStrides(int sx, int sy, 
					int sz, int st)
{
  this->Strides[0] = sx;
  this->Strides[1] = sy;
  this->Strides[2] = sz;
  this->Strides[3] = st;
  ((vtkImageContinuousDilate1D *)this->Filters[0])->SetStride(sx);
  ((vtkImageContinuousDilate1D *)this->Filters[1])->SetStride(sy);
  ((vtkImageContinuousDilate1D *)this->Filters[2])->SetStride(sz);
  ((vtkImageContinuousDilate1D *)this->Filters[3])->SetStride(st);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetXStride(int s)
{
  this->Strides[0] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[0])->SetStride(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetYStride(int s)
{
  this->Strides[1] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[1])->SetStride(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetZStride(int s)
{
  this->Strides[2] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[2])->SetStride(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetTimeStride(int s)
{
  this->Strides[3] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[3])->SetStride(s);
  // Modified handled by sub calls
}


//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetKernelSize(int sx, int sy, int sz, int st)
{
  this->KernelSize[0] = sx;
  this->KernelSize[1] = sy;
  this->KernelSize[2] = sz;
  this->KernelSize[3] = st;
  ((vtkImageContinuousDilate1D *)this->Filters[0])->SetKernelSize(sx);
  ((vtkImageContinuousDilate1D *)this->Filters[1])->SetKernelSize(sy);
  ((vtkImageContinuousDilate1D *)this->Filters[2])->SetKernelSize(sz);
  ((vtkImageContinuousDilate1D *)this->Filters[3])->SetKernelSize(st);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetXKernelSize(int s)
{
  this->KernelSize[0] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[0])->SetKernelSize(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetYKernelSize(int s)
{
  this->KernelSize[1] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[1])->SetKernelSize(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetZKernelSize(int s)
{
  this->KernelSize[2] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[2])->SetKernelSize(s);
  // Modified handled by sub calls
}
//----------------------------------------------------------------------------
void vtkImageContinuousDilate::SetTimeKernelSize(int s)
{
  this->KernelSize[3] = s;
  ((vtkImageContinuousDilate1D *)this->Filters[3])->SetKernelSize(s);
  // Modified handled by sub calls
}




