/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSmooth.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageGaussianSmooth - Performs a gaussian convolution.
// .SECTION Description
// vtkImageGaussianSmooth implements a convolution of the input image
// with a gaussian. Supports from one to three dimensional convolutions.

#ifndef __vtkImageGaussianSmooth_h
#define __vtkImageGaussianSmooth_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageGaussianSmooth : public vtkImageToImageFilter
{
public:
  vtkTypeRevisionMacro(vtkImageGaussianSmooth,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates an instance of vtkImageGaussianSmmoth with the following
  // defaults: Dimensioonality 3, StandardDeviations( 2, 2, 2), 
  // Radius Factors ( 1.5, 1.5, 1.5)
  static vtkImageGaussianSmooth *New();
  
  
  // Description:
  // Sets/Gets the Standard deviation of the gaussian in pixel units.
  vtkSetVector3Macro(StandardDeviations, float);
  void SetStandardDeviation(float std)
        {this->SetStandardDeviations(std,std,std);}
  void SetStandardDeviations(float a,float b)
        {this->SetStandardDeviations(a,b,0.0);}
  vtkGetVector3Macro(StandardDeviations, float);

  // Description:
  // Sets/Gets the Standard deviation of the gaussian in pixel units.
  // These methods are provided for compatibility with old scripts
  void SetStandardDeviation(float a,float b)
        {this->SetStandardDeviations(a,b,0.0);}
  void SetStandardDeviation(float a,float b,float c) 
        {this->SetStandardDeviations(a,b,c);}

  // Description:
  // Sets/Gets the Radius Factors of the gaussian in pixel units.
  // The radius factors determine how far out the gaussian kernel will 
  // go before being clamped to zero.
  vtkSetVector3Macro(RadiusFactors, float);
  void SetRadiusFactors(float f, float f2) {this->SetRadiusFactors(f,f2,1.5);}
  void SetRadiusFactor(float f) {this->SetRadiusFactors(f, f, f);}
  vtkGetVector3Macro(RadiusFactors, float);

  // Description:
  // Set/Get the dimensionality of this filter. This determines whether
  // a one, two, or three dimensional gaussian is performed.
  vtkSetMacro(Dimensionality, int);
  vtkGetMacro(Dimensionality, int);

protected:
  vtkImageGaussianSmooth();
  ~vtkImageGaussianSmooth();

  int Dimensionality;
  float StandardDeviations[3];
  float RadiusFactors[3];
  
  void ComputeKernel(double *kernel, int min, int max, double std);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteAxis(int axis, vtkImageData *inData, int inExt[6],
                   vtkImageData *outData, int outExt[6],
                   int *pcycle, int target, int *pcount, int total);
  void ThreadedExecute(vtkImageData *inData, 
                       vtkImageData *outData, int outExt[6], int id);
  
private:
  vtkImageGaussianSmooth(const vtkImageGaussianSmooth&);  // Not implemented.
  void operator=(const vtkImageGaussianSmooth&);  // Not implemented.
};

#endif










