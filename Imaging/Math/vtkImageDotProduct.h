/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDotProduct.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageDotProduct
 * @brief   Dot product of two vector images.
 *
 * vtkImageDotProduct interprets the scalar components of two images
 * as vectors and takes the dot product vector by vector (pixel by pixel).
*/

#ifndef vtkImageDotProduct_h
#define vtkImageDotProduct_h



#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

class VTKIMAGINGMATH_EXPORT vtkImageDotProduct : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageDotProduct *New();
  vtkTypeMacro(vtkImageDotProduct,vtkThreadedImageAlgorithm);

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(vtkDataObject *in) { this->SetInputData(0,in); }
  virtual void SetInput2Data(vtkDataObject *in) { this->SetInputData(1,in); }

protected:
  vtkImageDotProduct();
  ~vtkImageDotProduct() {}

  virtual int RequestInformation (vtkInformation *,
                                  vtkInformationVector **,
                                  vtkInformationVector *);

  virtual void ThreadedRequestData(vtkInformation *request,
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector,
                                   vtkImageData ***inData,
                                   vtkImageData **outData,
                                   int extent[6], int threadId);

private:
  vtkImageDotProduct(const vtkImageDotProduct&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImageDotProduct&) VTK_DELETE_FUNCTION;
};

#endif



// VTK-HeaderTest-Exclude: vtkImageDotProduct.h
