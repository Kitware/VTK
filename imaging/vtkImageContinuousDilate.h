/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageContinuousDilate.h
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
// .NAME vtkImageContinuousDilate - Maximum of neighborhood.
// .SECTION Description
// vtkImageContinuousDilate implements a continous dilation by replacing
// a pixel with the maximum of its neighborhood.  This filter is implemented 
// as a decomposible neighborhood, so the neighborhood is rectagle for 2D
// or a box for 3D.


#ifndef __vtkImageContinuousDilate_h
#define __vtkImageContinuousDilate_h


#include "vtkImageDecomposedFilter.h"
#include "vtkImageContinuousDilate1D.h"

class VTK_EXPORT vtkImageContinuousDilate : public vtkImageDecomposedFilter
{
public:
  vtkImageContinuousDilate();
  static vtkImageContinuousDilate *New(){return new vtkImageContinuousDilate;};
  const char *GetClassName() {return "vtkImageContinuousDilate";};

  // Description:
  // The Kernel size can be specified for each axis individually
  void SetKernelSize(int sx, int sy, int sz, int st);
  void SetKernelSize(int s) {this->SetKernelSize(s, s, s, s);}
  void SetXKernelSize(int s);
  void SetYKernelSize(int s);
  void SetZKernelSize(int s);
  void SetTimeKernelSize(int s);
  
  // Description:
  // Each axis can have a stride to shrink the image.
  void SetStrides(int sx, int sy, int sz, int st);
  void SetStride(int s) {this->SetStrides(s, s, s, s);}
  void SetXStride(int s);
  void SetYStride(int s);
  void SetZStride(int s);
  void SetTimeStride(int s);
  
  
protected:
  int KernelSize[VTK_IMAGE_DIMENSIONS];
  int Strides[VTK_IMAGE_DIMENSIONS];
};

#endif



