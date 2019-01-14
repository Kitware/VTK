/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCorrelation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageCorrelation
 * @brief   Correlation imageof the two inputs.
 *
 * vtkImageCorrelation finds the correlation between two data sets.
 * SetDimensionality determines
 * whether the Correlation will be 3D, 2D or 1D.
 * The default is a 2D Correlation.  The Output type will be double.
 * The output size will match the size of the first input.
 * The second input is considered the correlation kernel.
*/

#ifndef vtkImageCorrelation_h
#define vtkImageCorrelation_h



#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageCorrelation : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageCorrelation *New();
  vtkTypeMacro(vtkImageCorrelation,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...).
   * The default is 2.
   */
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  //@}

  /**
   * Set the input image.
   */
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }

  /**
   * Set the correlation kernel.
   */
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

protected:
  vtkImageCorrelation();
  ~vtkImageCorrelation() override {}

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
  vtkImageCorrelation(const vtkImageCorrelation&) = delete;
  void operator=(const vtkImageCorrelation&) = delete;
};

#endif



