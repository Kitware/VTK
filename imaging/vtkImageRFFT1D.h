/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRFFT1D.h
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
// .NAME vtkImageRFFT1D - Performs a 1d reverse fast Fourier transform.
// .SECTION Description
// vtkImageRFFT1D implements a 1d reverse Fourier transform.  It
// takes an frequency domain image with two components (real, imaginary) 
// and changes it to spatial domain image also with two channels.
// Input channels always must be 0 = real and 1 = imaginary.


#ifndef __vtkImageRFFT1D_h
#define __vtkImageRFFT1D_h


#include "vtkImageFourierFilter.h"

class VTK_EXPORT vtkImageRFFT1D : public vtkImageFourierFilter
{
public:
  vtkImageRFFT1D();
  static vtkImageRFFT1D *New() {return new vtkImageRFFT1D;};
  const char *GetClassName() {return "vtkImageRFFT1D";};

  void SetAxes(int axis);
  void InterceptCacheUpdate(vtkImageRegion *region);
  
protected:
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion, 
					vtkImageRegion *inRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif










