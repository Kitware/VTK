// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageSobel2D
 * @brief   Computes a vector field using sobel functions.
 *
 * vtkImageSobel2D computes a vector field from a scalar field by using
 * Sobel functions.  The number of vector components is 2 because
 * the input is an image.  Output is always doubles.
 */

#ifndef vtkImageSobel2D_h
#define vtkImageSobel2D_h

#include "vtkImageSpatialAlgorithm.h"
#include "vtkImagingGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageSobel2D : public vtkImageSpatialAlgorithm
{
public:
  static vtkImageSobel2D* New();
  vtkTypeMacro(vtkImageSobel2D, vtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageSobel2D();
  ~vtkImageSobel2D() override = default;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkImageSobel2D(const vtkImageSobel2D&) = delete;
  void operator=(const vtkImageSobel2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
