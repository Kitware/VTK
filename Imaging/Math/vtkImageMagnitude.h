// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMagnitude.h
 * @brief   Collapses components with magnitude function.
 *
 * vtkImageMagnitude takes the magnitude of the components.
 */

#ifndef vtkImageMagnitude_h
#define vtkImageMagnitude_h

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGMATH_EXPORT vtkImageMagnitude : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMagnitude* New();
  vtkTypeMacro(vtkImageMagnitude, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageMagnitude();
  ~vtkImageMagnitude() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int outExt[6], int id) override;

private:
  vtkImageMagnitude(const vtkImageMagnitude&) = delete;
  void operator=(const vtkImageMagnitude&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
