/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage1dConvolutionFilter.hh
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
// .NAME vtkImage1dConvolutionFilter - Filter that performs a convolution with a 1d kernel.
// .SECTION Description
// vtkImage1dConvolutionFilter implements a 1d convolution along any axis.  It is used
// in higher level filter which decompose their convolution 
// (i.e. 2d Gaussian smoothing)


#ifndef __vtkImage1dConvolutionFilter_h
#define __vtkImage1dConvolutionFilter_h


#include "vtkImageFilter.hh"

class vtkImage1dConvolutionFilter : public vtkImageFilter
{
public:
  vtkImage1dConvolutionFilter();
  ~vtkImage1dConvolutionFilter();
  char *GetClassName() {return "vtkImage1dConvolutionFilter";};
  void GetBoundary(int *offset, int *size);

  // Description:
  // Set/Get the convolution kernal (kernel is copied).
  void SetKernel(float *Kernel, int KernelSize);
  vtkGetMacro(KernelSize,int);

  // Description:
  // Set/Get the convolution axis.
  vtkSetMacro(Axis,int);
  vtkGetMacro(Axis,int);

protected:
  float *Kernel;
  int   KernelSize;
  int   KernelOffset;    /* how to center the kernel */
  int   Axis;            /* the axis of the convolution */

  void RequiredRegion(int *outOffset, int *outSize,
		   int *inOffset, int *inSize);
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
};

#endif



