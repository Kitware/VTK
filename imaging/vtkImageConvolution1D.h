/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageConvolution1D.h
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
// .NAME vtkImageConvolution1D - Performs a 1 dimensional convilution.
// .SECTION Description
// vtkImageConvolution1D implements a 1d convolution along any axis.  
// It is used in higher level filter which decompose their convolution 
// (i.e. 2d Gaussian smoothing)


#ifndef __vtkImageConvolution1D_h
#define __vtkImageConvolution1D_h


#include "vtkImageSpatialFilter.h"
#include "vtkImageRegion.h"

class VTK_EXPORT vtkImageConvolution1D : public vtkImageSpatialFilter
{
public:
  vtkImageConvolution1D();
  ~vtkImageConvolution1D();
  char *GetClassName() {return "vtkImageConvolution1D";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void SetKernel(float *kernel, int size);
  
  // Description:
  // Set/Get whether to rescale boundary-truncated kernel
  vtkSetMacro(BoundaryRescale,int);
  vtkGetMacro(BoundaryRescale,int);
  vtkBooleanMacro(BoundaryRescale,int);

  // Description:
  // This filter handles different strides to shrink the output.
  void SetStride(int stride){ this->Strides[0] = stride; this->Modified();};
  
  // users shouldn't access these directly but templated functions need to
  float *Kernel;
  float KernelArea;
  int   BoundaryRescale;  // Kernel is rescaled at boundaries

protected:
  void Execute(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  void ExecuteCenter(vtkImageRegion *inRegion, vtkImageRegion *outRegion);
  
};

#endif










