// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageLuminance
 * @brief   Computes the luminance of the input
 *
 * vtkImageLuminance calculates luminance from an rgb input.
 */

#ifndef vtkImageLuminance_h
#define vtkImageLuminance_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageLuminance : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLuminance* New();
  vtkTypeMacro(vtkImageLuminance, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageLuminance();
  ~vtkImageLuminance() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int outExt[6], int id) override;

private:
  vtkImageLuminance(const vtkImageLuminance&) = delete;
  void operator=(const vtkImageLuminance&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
