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
/**
 * @class   vtkImageNonMaximumSuppression
 * @brief   Performs non-maximum suppression
 *
 * vtkImageNonMaximumSuppression Sets to zero any pixel that is not a peak.
 * If a pixel has a neighbor along the vector that has larger magnitude, the
 * smaller pixel is set to zero.  The filter takes two inputs: a magnitude
 * and a vector.  Output is magnitude information and is always in doubles.
 * Typically this filter is used with vtkImageGradient and
 * vtkImageGradientMagnitude as inputs.
*/

#ifndef vtkImageNonMaximumSuppression_h
#define vtkImageNonMaximumSuppression_h

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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the magnitude and vector inputs.
   */
  void SetMagnitudeInputData(vtkImageData *input) {this->SetInputData(0,input);};
  void SetVectorInputData(vtkImageData *input) {this->SetInputData(1,input);};
  //@}

  //@{
  /**
   * If "HandleBoundariesOn" then boundary pixels are duplicated
   * So central differences can get values.
   */
  vtkSetMacro(HandleBoundaries, vtkTypeBool);
  vtkGetMacro(HandleBoundaries, vtkTypeBool);
  vtkBooleanMacro(HandleBoundaries, vtkTypeBool);
  //@}

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices or a 3D volume)
   */
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  //@}

protected:
  vtkImageNonMaximumSuppression();
  ~vtkImageNonMaximumSuppression() override {}

  vtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *) override;

  int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId) override;

private:
  vtkImageNonMaximumSuppression(const vtkImageNonMaximumSuppression&) = delete;
  void operator=(const vtkImageNonMaximumSuppression&) = delete;
};

#endif



