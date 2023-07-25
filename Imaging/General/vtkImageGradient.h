// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageGradient
 * @brief   Computes the gradient vector.
 *
 * vtkImageGradient computes the gradient vector of an image.  The
 * vector results are stored as scalar components. The Dimensionality
 * determines whether to perform a 2d or 3d gradient. The default is
 * two dimensional XY gradient.  OutputScalarType is always
 * double. Gradient is computed using central differences.
 */

#ifndef vtkImageGradient_h
#define vtkImageGradient_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageGradient : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageGradient* New();
  vtkTypeMacro(vtkImageGradient, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...)
   */
  vtkSetClampMacro(Dimensionality, int, 2, 3);
  vtkGetMacro(Dimensionality, int);
  ///@}

  ///@{
  /**
   * Get/Set whether to handle boundaries.  If enabled, boundary
   * pixels are treated as duplicated so that central differencing
   * works for the boundary pixels.  If disabled, the output whole
   * extent of the image is reduced by one pixel.
   */
  vtkSetMacro(HandleBoundaries, vtkTypeBool);
  vtkGetMacro(HandleBoundaries, vtkTypeBool);
  vtkBooleanMacro(HandleBoundaries, vtkTypeBool);
  ///@}

protected:
  vtkImageGradient();
  ~vtkImageGradient() override = default;

  vtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*,
    vtkImageData*** inData, vtkImageData** outData, int outExt[6], int threadId) override;

private:
  vtkImageGradient(const vtkImageGradient&) = delete;
  void operator=(const vtkImageGradient&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
