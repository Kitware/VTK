/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth1D.h
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
// .NAME vtkImageGaussianSmooth1D - smooths on one axis.
// .SECTION Description
// vtkImageGaussianSmooth1D implements a 1d Gaussian smoothing
// on one axis.


#ifndef __vtkImageGaussianSmooth1D_h
#define __vtkImageGaussianSmooth1D_h


#include "vtkImageConvolution1D.h"

class VTK_EXPORT vtkImageGaussianSmooth1D : public vtkImageConvolution1D
{
public:
  vtkImageGaussianSmooth1D();
  vtkImageGaussianSmooth1D *New() {return new vtkImageGaussianSmooth1D;};
  char *GetClassName() {return "vtkImageGaussianSmooth1D";};

  // Description:
  // Set/Get the standard deviation of the Gaussian.
  void SetStandardDeviation(float Std);
  vtkGetMacro(StandardDeviation,float);
  // Description:
  // Set/Get the radius of the kernel in units of standard deviations.
  void SetRadiusFactor(float radius);
  vtkGetMacro(RadiusFactor,float);
  // Description:
  // Get the cutoff in units of pixels.
  vtkGetMacro(Radius,int)

protected:
  float StandardDeviation;
  float RadiusFactor;
  int Radius;
  
  void ComputeKernel();
};

#endif



