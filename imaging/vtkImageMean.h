/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMean.h
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
// .NAME vtkImageMean - smooths on a 3D plane.
// .SECTION Description
// vtkImageMean averages over a neighborhood.  The stride can be set 
// to shrink the image in integer multiples.
// It is a decomposed filter so it really consists of
// multiple 1D filters.


#ifndef __vtkImageMean_h
#define __vtkImageMean_h


#include "vtkImageDecomposedFilter.h"
#include "vtkImageMean1D.h"

class VTK_EXPORT vtkImageMean : public vtkImageDecomposedFilter
{
public:
  vtkImageMean();
  char *GetClassName() {return "vtkImageMean";};

  void SetDimensionality(int num);

  // Description:
  // Set/Get the size of the neighborhood to apply mean.
  void SetKernelSize(int num, int *size);
  vtkImageSetMacro(KernelSize, int);
  void GetKernelSize(int num, int *size);
  vtkImageGetMacro(KernelSize, int);

  // Description:
  // Set/Get the stride which shrinks the images.
  void SetStrides(int num, int *size);
  vtkImageSetMacro(Strides, int);
  void GetStrides(int num, int *size);
  vtkImageGetMacro(Strides, int);

protected:
  int KernelSize[VTK_IMAGE_DIMENSIONS];
  int Strides[VTK_IMAGE_DIMENSIONS];
};

#endif



