/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.h
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
// .NAME vtkImageNonMaximumSuppression - Performs non-maximum suppression
// .SECTION Description
// vtkImageNonMaximumSuppression Sets to zero any pixel that is not a peak.
// If a pixel has a neighbor along the vector that has larger magnitude, the
// smaller pixel is set to zero.  The filter takes two inputs: a magnitude
// and a vector.  Output is magnitude information and is always in floats.
// Typically this filter is used with vtkImageGradient and
// vtkImageGradientMagnitude as inputs.


#ifndef __vtkImageNonMaximumSuppression_h
#define __vtkImageNonMaximumSuppression_h

#define VTK_IMAGE_NON_MAXIMUM_SUPPRESSION_MAGNITUDE_INPUT 0
#define VTK_IMAGE_NON_MAXIMUM_SUPPRESSION_VECTOR_INPUT 1

#include "vtkImageTwoInputFilter.h"

class VTK_IMAGING_EXPORT vtkImageNonMaximumSuppression : public vtkImageTwoInputFilter
{
public:
  static vtkImageNonMaximumSuppression *New();
  vtkTypeRevisionMacro(vtkImageNonMaximumSuppression,vtkImageTwoInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the magnitude and vector inputs.
  void SetMagnitudeInput(vtkImageData *input) {this->SetInput1(input);};
  void SetVectorInput(vtkImageData *input) {this->SetInput2(input);};
  
  // Description:
  // If "HandleBoundariesOn" then boundary pixels are duplicated
  // So central differences can get values.
  vtkSetMacro(HandleBoundaries, int);
  vtkGetMacro(HandleBoundaries, int);
  vtkBooleanMacro(HandleBoundaries, int);

  // Description:
  // Determines how the input is interpreted (set of 2d slices or a 3D volume)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageNonMaximumSuppression();
  ~vtkImageNonMaximumSuppression() {};

  int HandleBoundaries;
  int Dimensionality;
  
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);
  virtual void ComputeInputUpdateExtent(int inExt[6], int outExt[6],
                                        int whichInput);
  void ExecuteInformation(){this->vtkImageTwoInputFilter::ExecuteInformation();};
  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
  
private:
  vtkImageNonMaximumSuppression(const vtkImageNonMaximumSuppression&);  // Not implemented.
  void operator=(const vtkImageNonMaximumSuppression&);  // Not implemented.
};

#endif



