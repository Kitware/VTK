/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRFFT.cxx
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
#include "vtkImageRFFT.h"

//----------------------------------------------------------------------------
vtkImageRFFT::vtkImageRFFT()
{
  // To avoid compiler warnings
  _vtkImageComplexMultiplyTemp.Real = 0.0;
}



//----------------------------------------------------------------------------
// Description:
// This method sets up multiple RFFT filters
void vtkImageRFFT::SetDimensionality(int num)
{
  int idx, idx2;
  int splitOrder[VTK_IMAGE_DIMENSIONS];
  int *axes;
  
  
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
    this->Filters[idx] = new vtkImageRFFT1D;
    this->Filters[idx]->SetAxes(this->Axes[idx]);
    // Splitting the principle axis will do no good (reverse order).
    axes = this->Filters[idx]->GetAxes();
    for (idx2 = 0; idx2 < VTK_IMAGE_DIMENSIONS; ++idx2)
      {
      splitOrder[idx2] = axes[VTK_IMAGE_DIMENSIONS - idx2 - 1];
      }
    this->Filters[idx]->SetSplitOrder(VTK_IMAGE_DIMENSIONS, splitOrder);
    }
  
  this->Dimensionality = num;
  this->Modified();
}














