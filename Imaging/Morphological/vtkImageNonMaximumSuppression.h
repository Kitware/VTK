/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageNonMaximumSuppression.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// and a vector.  Output is magnitude information and is always in doubles.
// Typically this filter is used with vtkImageGradient and
// vtkImageGradientMagnitude as inputs.


#ifndef __vtkImageNonMaximumSuppression_h
#define __vtkImageNonMaximumSuppression_h

#define VTK_IMAGE_NON_MAXIMUM_SUPPRESSION_MAGNITUDE_INPUT 0
#define VTK_IMAGE_NON_MAXIMUM_SUPPRESSION_VECTOR_INPUT 1

#include "vtkImagingMorphologicalModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"
#include "vtkImageData.h" // makes things a bit easier

class VTKIMAGINGMORPHOLOGICAL_EXPORT vtkImageNonMaximumSuppression : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageNonMaximumSuppression *New();
  vtkTypeMacro(vtkImageNonMaximumSuppression,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the magnitude and vector inputs.
  void SetMagnitudeInputData(vtkImageData *input) {this->SetInputData(0,input);};
  void SetVectorInputData(vtkImageData *input) {this->SetInputData(1,input);};

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
  ~vtkImageNonMaximumSuppression() {}

  int HandleBoundaries;
  int Dimensionality;

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId);

private:
  vtkImageNonMaximumSuppression(const vtkImageNonMaximumSuppression&);  // Not implemented.
  void operator=(const vtkImageNonMaximumSuppression&);  // Not implemented.
};

#endif



