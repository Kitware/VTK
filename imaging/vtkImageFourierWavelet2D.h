/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierWavelet2D.h
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
// .NAME vtkImageFourierWavelet2D - Fourier wavelet decomposition.
// .SECTION Description
// vtkImageFourierWavelet2D Fourier components (NxN) are used
// as a Wavelet set.  The set is orthogonal, but they overlap.
// The output is a multispectral image whose spatial dimensions
// are smaller than the original image.
// Channel 0 has a lowres version of the original image.  For a multi
// resolution decomposition, chain these filters (multiple instanceses of 
// this class together).  


#ifndef __vtkImageFourierWavelet2D_h
#define __vtkImageFourierWavelet2D_h

#include <math.h>
#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageFourierWavelet2D : public vtkImageFilter
{
public:
  vtkImageFourierWavelet2D();
  char *GetClassName() {return "vtkImageFourierWavelet2D";};

  void InitializeWavelets(int dim);

  void InterceptCacheUpdate(vtkImageRegion *region);

  // Description:
  // Get the region that holds the wavelets.
  vtkGetObjectMacro(Wavelets, vtkImageRegion);

  // Description:
  // Set/Get the spacing between subsamples
  vtkSetMacro(Spacing, int);
  vtkGetMacro(Spacing, int);

protected:
  vtkImageRegion *Wavelets;
  int Spacing;

  void ComputeWaveletReal(int f1, int f2, int w0);
  void ComputeWaveletImaginary(int f1, int f2, int w0);  
  int  TestWaveletOrthogonality(int waveletIdx);

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion, 
					vtkImageRegion *inRegion);
  
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



