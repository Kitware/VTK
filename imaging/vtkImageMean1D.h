/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMean1D.h
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
// .NAME vtkImageMean1D - Mean of a neighborhood.
// .SECTION Description
// vtkImageMean1D replaces eache pixel with the mean of its neighborhood.
// It is meant to be used by the decomposible mean filter.
// It implements stride which will reduce the resolution of the image.
// Start is center of the first neighborhood which will be mapped to
// the pixel (0, 0, ...).  The input can be any type, but the output
// is always the same type as the input.  Boundaries are always handled.
// KernelCenter is set automatically.  If KernelCenter is set manually,
// it should be in the range [0, KernelSize).


#ifndef __vtkImageMean1D_h
#define __vtkImageMean1D_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageMean1D : public vtkImageFilter
{
public:
  vtkImageMean1D();
  static vtkImageMean1D *New() {return new vtkImageMean1D;};
  char *GetClassName() {return "vtkImageMean1D";};

  // Description:
  // Set/Get the stride which will reduce the resolution of the output.
  vtkSetMacro(Stride,int);
  vtkGetMacro(Stride,int);

  void SetKernelSize(int size);
  // Description:
  // Get the center of the kernel (neighborhood).
  vtkGetMacro(KernelSize,int);

  void SetKernelMiddle(int middle);
  // Description:
  // Set/Get the center of the kernel (neighborhood).
  vtkGetMacro(KernelMiddle,int);

protected:
  int KernelSize;
  int KernelMiddle;
  int Stride;
  

  void ComputeOutputImageInformation(vtkImageRegion *inRegion,
				     vtkImageRegion *outRegion);
  void ComputeRequiredInputRegionExtent(vtkImageRegion *outRegion,
					vtkImageRegion *inRegion);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);  
};

#endif



