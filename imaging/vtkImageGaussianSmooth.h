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
// .NAME vtkImageGaussianSmooth - Performs a 1 dimensional convilution.
// .SECTION Description
// vtkImageGaussianSmooth implements a 1d convolution along any axis.  
// It is used in higher level filter which decompose their convolution 
// (i.e. 2d Gaussian smoothing)


#ifndef __vtkImageGaussianSmooth_h
#define __vtkImageGaussianSmooth_h


#include "vtkImageFilter.h"

class VTK_EXPORT vtkImageGaussianSmooth : public vtkImageFilter
{
public:
  vtkImageGaussianSmooth();
  ~vtkImageGaussianSmooth();
  static vtkImageGaussianSmooth *New() {return new vtkImageGaussianSmooth;};
  const char *GetClassName() {return "vtkImageGaussianSmooth";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkSetVector3Macro(StandardDeviations, float);
  void SetStandardDeviation(float std) {SetStandardDeviations(std,std,std);}
  void SetStandardDeviations(float a,float b) {SetStandardDeviations(a,b,0.0);}
  vtkGetVector3Macro(StandardDeviations, float);
  // for compatability with old scripts
  void SetStandardDeviation(float a,float b) {SetStandardDeviations(a,b,0.0);}
  void SetStandardDeviation(float a,float b,float c) 
  {SetStandardDeviations(a,b,c);}

  vtkSetVector3Macro(RadiusFactors, float);
  void SetRadiusFactors(float f, float f2) {SetRadiusFactors(f,f2,1.5);}
  void SetRadiusFactor(float f) {SetRadiusFactors(f, f, f);}
  vtkGetVector3Macro(RadiusFactors, float);

  vtkSetMacro(Dimensionality, int);
  vtkGetMacro(Dimensionality, int);

protected:
  int Dimensionality;
  float StandardDeviations[3];
  float RadiusFactors[3];
  int Radius;
  float *Kernel;
  float *TempKernel;
  
  void ComputeKernel(float std, float factor);
  void ExecuteImageInformation();
  void ComputeRequiredInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteAxis(int axis, vtkImageData *inData, int inExt[6],
		   vtkImageData *outData, int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, 
		       vtkImageData *outData, int outExt[6], int id);
  
};

#endif










