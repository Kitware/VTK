/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth1d.cxx
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
#include<math.h>
#include "vtkImageGaussianSmooth1d.h"


//----------------------------------------------------------------------------
// Description:
// This method sets up the Gaussian kernel.
void vtkImageGaussianSmooth1d::SetGaussianStdRadius(float Std,int Radius)
{
  int idx;
  float *kernel;
  float sum;

  vtkDebugMacro(<< "SetGaussianStdRadius: Std = " << Std 
                << ", Radius = " << Radius);
  
  // save the values
  this->Std = Std;
  this->Radius = Radius;
  this->Modified();

  // generate the kernel
  kernel = new float[2 * Radius + 1];
  kernel[Radius] = 1.0;
  sum = 0.5;
  for (idx = 1; idx <= Radius; ++idx)
    sum += kernel[Radius + idx] = 
      exp(- (float)(idx * idx) / (2.0 * Std * Std));

  // normalize
  sum = 0.5 / sum;
  kernel[Radius] *= sum;
  for (idx = 1; idx <= Radius; ++idx)
    kernel[Radius + idx] = kernel[Radius - idx] = 
      kernel[Radius + idx] * sum;

  this->BoundaryRescaleOn();
  
  // set the kernel
  this->SetKernel(kernel, Radius * 2 + 1);

  // free kernel
  delete [] kernel;
}
  


















