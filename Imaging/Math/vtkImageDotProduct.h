// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGMATH_EXPORT vtkImageDotProduct : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageDotProduct* New();
  vtkTypeMacro(vtkImageDotProduct, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(vtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(vtkDataObject* in) { this->SetInputData(1, in); }

protected:
  vtkImageDotProduct();
  ~vtkImageDotProduct() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int threadId) override;

private:
  vtkImageDotProduct(const vtkImageDotProduct&) = delete;
  void operator=(const vtkImageDotProduct&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
