/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResample.cxx
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
#include "vtkImageResample.h"
#include "vtkImageCache.h"


//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageResample::vtkImageResample()
{
  int idx;
  vtkImageResample1D *filter;

  for (idx = 0; idx < 4; ++idx)
    {
    filter = vtkImageResample1D::New();
    this->Filters[idx] = filter;
    this->MagnificationFactors[idx] = 1.0;
    this->OutputSpacing[idx] = 0.0;
    filter->SetMagnificationFactor(this->MagnificationFactors[idx]);
    filter->SetFilteredAxis(idx);
    }
  // Let the superclass set some superclass variables of the filters.
  this->InitializeFilters();
}

//----------------------------------------------------------------------------
// Description:
// This method sets up multiple magnify filters (one for each axis).
void vtkImageResample::SetAxisOutputSpacing(int num, float spacing)
{
  if (num < 0 || num > 3)
    {
    vtkErrorMacro("SetAxisOutputSpacing: Bad axis " << num);
    return;
    }

  if (this->OutputSpacing[num] == spacing)
    {
    return;
    }
  
  this->OutputSpacing[num] = spacing;
  ((vtkImageResample1D *)this->Filters[num])->SetOutputSpacing(spacing);
  // subfilter handles modified.
}

//----------------------------------------------------------------------------
// Description:
// This method sets up multiple magnify filters (one for each axis).
void vtkImageResample::SetAxisMagnificationFactor(int num, float factor)
{
  if (num < 0 || num > 3)
    {
    vtkErrorMacro("SetAxisMagnificationFactor: Bad axis " << num);
    return;
    }

  if (this->MagnificationFactors[num] == factor)
    {
    return;
    }
  
  this->MagnificationFactors[num] = factor;
  ((vtkImageResample1D *)this->Filters[num])->SetMagnificationFactor(factor);
  // subfilter handles modified.
}







