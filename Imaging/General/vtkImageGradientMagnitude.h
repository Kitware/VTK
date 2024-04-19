// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageGradientMagnitude
 * @brief   Computes magnitude of the gradient.
 *
 *
 * vtkImageGradientMagnitude computes the gradient magnitude of an image.
 * Setting the dimensionality determines whether the gradient is computed on
 * 2D images, or 3D volumes.  The default is two dimensional XY images.
 *
 * @sa
 * vtkImageGradient vtkImageMagnitude
 */

#ifndef vtkImageGradientMagnitude_h
#define vtkImageGradientMagnitude_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageGradientMagnitude : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageGradientMagnitude* New();
  vtkTypeMacro(vtkImageGradientMagnitude, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * If "HandleBoundariesOn" then boundary pixels are duplicated
   * So central differences can get values.
   */
  vtkSetMacro(HandleBoundaries, vtkTypeBool);
  vtkGetMacro(HandleBoundaries, vtkTypeBool);
  vtkBooleanMacro(HandleBoundaries, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...)
   */
  vtkSetClampMacro(Dimensionality, int, 2, 3);
  vtkGetMacro(Dimensionality, int);
  ///@}

protected:
  vtkImageGradientMagnitude();
  ~vtkImageGradientMagnitude() override = default;

  vtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int outExt[6], int id) override;

private:
  vtkImageGradientMagnitude(const vtkImageGradientMagnitude&) = delete;
  void operator=(const vtkImageGradientMagnitude&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
