// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageRGBToHSI
 * @brief   Converts RGB components to HSI.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as hue, saturation and intensity.
 * Output type must be the same as input type.
 */

#ifndef vtkImageRGBToHSI_h
#define vtkImageRGBToHSI_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToHSI : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToHSI* New();
  vtkTypeMacro(vtkImageRGBToHSI, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
   * defaults to 255 instead of 2PI, because unsigned char is expected as
   * input.  Maximum also specifies the maximum of the Saturation.
   */
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);
  ///@}

protected:
  vtkImageRGBToHSI();
  ~vtkImageRGBToHSI() override = default;

  double Maximum;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageRGBToHSI(const vtkImageRGBToHSI&) = delete;
  void operator=(const vtkImageRGBToHSI&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
