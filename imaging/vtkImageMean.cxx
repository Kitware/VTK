/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMean.cxx
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
#include "vtkImageMean.h"

//----------------------------------------------------------------------------
vtkImageMean::vtkImageMean()
{
  int idx;
  
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->Strides[idx] = 1;
    this->KernelSize[idx] = 1;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method sets up multiple smoothing filters
void vtkImageMean::SetDimensionality(int num)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetDimensionality: " << num << " is too many fitlers.");
    return;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    if (this->Filters[idx])
      {
      this->Filters[idx]->Delete();
      }
    this->Filters[idx] = vtkImageMean1D::New();
    this->Filters[idx]->SetAxes(this->Axes[idx]);
    ((vtkImageMean1D *)
     (this->Filters[idx]))->SetKernelSize(this->KernelSize[idx]);
    ((vtkImageMean1D *)(this->Filters[idx]))->SetStride(this->Strides[idx]);
    }
  
  this->Dimensionality = num;
  this->Modified();
}


//----------------------------------------------------------------------------
void vtkImageMean::SetStrides(int num, int *Strides)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetStrides: not that many dimensions.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->Strides[idx] = Strides[idx];
    // Set the filter if it has been created.
    if (this->Filters[idx])
      {
      ((vtkImageMean1D *)(this->Filters[idx]))->SetStride(Strides[idx]);
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageMean::GetStrides(int num, int *Strides)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "GetStrides: not that many dimensions.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    Strides[idx] = this->Strides[idx];
    }
}



//----------------------------------------------------------------------------
void vtkImageMean::SetKernelSize(int num, int *size)
{
  int idx;
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "SetKernelSize: not that many dimensions.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    this->KernelSize[idx] = size[idx];
    // Set the filter if it has been created.
    if (this->Filters[idx])
      {
      ((vtkImageMean1D *)(this->Filters[idx]))->SetKernelSize(size[idx]);
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageMean::GetKernelSize(int num, int *size)
{
  int idx;

  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "GetKernelSize: not that many dimensions.");
    num = VTK_IMAGE_DIMENSIONS;
    }
  
  for (idx = 0; idx < num; ++idx)
    {
    size[idx] = this->KernelSize[idx];
    }
}

