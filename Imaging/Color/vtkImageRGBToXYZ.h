// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageRGBToXYZ
 * @brief   Converts XYZ components to LAB.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded in the XYZ color space.
 * Output type must be the same as input type.
 */

#ifndef vtkImageRGBToXYZ_h
#define vtkImageRGBToXYZ_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToXYZ : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToXYZ* New();
  vtkTypeMacro(vtkImageRGBToXYZ, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageRGBToXYZ();
  ~vtkImageRGBToXYZ() override = default;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageRGBToXYZ(const vtkImageRGBToXYZ&) = delete;
  void operator=(const vtkImageRGBToXYZ&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
