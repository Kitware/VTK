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
// .NAME vtkImageCorrelation - Correlation imageof the two inputs.
// .SECTION Description
// vtkImageCorrelation finds the correlation between two data sets.
// SetDimensionality determines
// whether the Correlation will be 3D, 2D or 1D.
// The default is a 2D Correlation.  The Output type will be double.
// The output size will match the size of the first input.
// The second input is considered the correlation kernel.

#ifndef __vtkImageCorrelation_h
#define __vtkImageCorrelation_h



#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGGENERAL_EXPORT vtkImageCorrelation : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageCorrelation *New();
  vtkTypeMacro(vtkImageCorrelation,vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Determines how the input is interpreted (set of 2d slices ...).
  // The default is 2.
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);

  // Description:
  // Set the input image.
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }

  // Description:
  // Set the correlation kernel.
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

protected:
  vtkImageCorrelation();
  ~vtkImageCorrelation() {}

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
  vtkImageCorrelation(const vtkImageCorrelation&);  // Not implemented.
  void operator=(const vtkImageCorrelation&);  // Not implemented.
};

#endif



