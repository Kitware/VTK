/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageShrink3D.h
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
// .NAME vtkImageShrink3D - Subsamples an image.
// .SECTION Description
// vtkImageShrink3D shrinks an image by sub sampling on a 
// uniform grid (integer multiples).  

#ifndef __vtkImageShrink3D_h
#define __vtkImageShrink3D_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageShrink3D : public vtkImageToImageFilter
{
public:
  static vtkImageShrink3D *New();
  vtkTypeRevisionMacro(vtkImageShrink3D,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the shrink factors
  vtkSetVector3Macro(ShrinkFactors,int);
  vtkGetVector3Macro(ShrinkFactors,int);

  // Description:
  // Set/Get the pixel to use as origin.
  vtkSetVector3Macro(Shift,int);
  vtkGetVector3Macro(Shift,int);

  // Description:
  // Choose Mean, Minimum, Maximum, Median or sub sampling.
  // The neighborhood operations are not centered on the sampled pixel.
  // This may cause a half pixel shift in your output image.
  // You can changed "Shift" to get around this.
  // vtkImageGaussianSmooth or vtkImageMean with strides.
  void SetAveraging(int);
  int GetAveraging() {return this->GetMean();};
  vtkBooleanMacro(Averaging,int);
  
  void SetMean(int);
  vtkGetMacro(Mean,int);
  vtkBooleanMacro(Mean,int);
  
  void SetMinimum(int);
  vtkGetMacro(Minimum,int);
  vtkBooleanMacro(Minimum,int);
  
  void SetMaximum(int);
  vtkGetMacro(Maximum,int);
  vtkBooleanMacro(Maximum,int);
  
  void SetMedian(int);
  vtkGetMacro(Median,int);
  vtkBooleanMacro(Median,int);
  
  
protected:
  vtkImageShrink3D();
  ~vtkImageShrink3D() {};

  int ShrinkFactors[3];
  int Shift[3];
  int Mean;
  int Minimum;
  int Maximum;
  int Median;

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ExecuteInformation(){this->vtkImageToImageFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData, 
                       int ext[6], int id);  
private:
  vtkImageShrink3D(const vtkImageShrink3D&);  // Not implemented.
  void operator=(const vtkImageShrink3D&);  // Not implemented.
};

#endif



