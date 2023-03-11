/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageYIQToRGB.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRGBToYIQ
 * @brief   Converts YIQ components to RGB.
 *
 * For each pixel with Y, I, and Q components this
 * filter output the color coded as RGB.
 * Output type must be the same as input type. Only signed types should be
 * used because the YIQ color space uses negative numbers.
 */

#ifndef vtkImageYIQToRGB_h
#define vtkImageYIQToRGB_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageYIQToRGB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageYIQToRGB* New();
  vtkTypeMacro(vtkImageYIQToRGB, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Maximum value of pixel intensity allowed. Default is 255.0.
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);

protected:
  vtkImageYIQToRGB();
  ~vtkImageYIQToRGB() override = default;

  double Maximum;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageYIQToRGB(const vtkImageYIQToRGB&) = delete;
  void operator=(const vtkImageYIQToRGB&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
