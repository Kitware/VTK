/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.h
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
// .NAME vtkImageGaussianSmooth - smooths on a 3D plane.
// .SECTION Description
// vtkImageGaussianSmooth implements Gaussian smoothing over any number of 
// axes. It really consists of multiple decomposed 1D filters.


#ifndef __vtkImageGaussianSmooth_h
#define __vtkImageGaussianSmooth_h


#include "vtkImageDecomposedFilter.h"
#include "vtkImageGaussianSmooth1D.h"

class VTK_EXPORT vtkImageGaussianSmooth : public vtkImageDecomposedFilter
{
public:
  vtkImageGaussianSmooth();
  static vtkImageGaussianSmooth *New() {return new vtkImageGaussianSmooth;};
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkImageGaussianSmooth";};

  void SetFilteredAxes(int num, int *axes);

  // Description:
  // Each axis can have a separate radius factor which determines
  // the cuttoff of the kernel.  The Kernel will have radius =
  // (RadiusFactor * StanardDeviation) pixels.
  void SetRadiusFactors(int num, float *factors);
  vtkImageSetMacro(RadiusFactors, float);
  void SetRadiusFactor(float f) {this->SetRadiusFactors(f, f, f, f);}

  // Description:
  // Each axis can have a separate standard deviation.
  void SetStandardDeviations(int num, float *stds);
  vtkImageSetMacro(StandardDeviations, float);

  // Description:
  // For legacy compatability.
  // Repeats the deviations.
  void SetStandardDeviation(int num, float *stds);
  vtkImageSetMacro(StandardDeviation, float);
  
  // Description:
  // Each axis can have a stride to shrink the image.
  void SetStrides(int num, int *strides);
  vtkImageSetMacro(Strides, int);
  void SetStride(int s) {this->SetStrides(s, s, s, s);}

  
protected:
  int Strides[4];
  float RadiusFactors[4];
  float StandardDeviations[4];

  void InitializeParameters();
};

#endif



