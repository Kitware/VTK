// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageRGBToHSV
 * @brief   Converts RGB components to HSV.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as hue, saturation and value.
 * Output type must be the same as input type.
 */

#ifndef vtkImageRGBToHSV_h
#define vtkImageRGBToHSV_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToHSV : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToHSV* New();
  vtkTypeMacro(vtkImageRGBToHSV, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);

protected:
  vtkImageRGBToHSV();
  ~vtkImageRGBToHSV() override = default;

  double Maximum;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageRGBToHSV(const vtkImageRGBToHSV&) = delete;
  void operator=(const vtkImageRGBToHSV&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
