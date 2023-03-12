/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRGBToYIQ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRGBToYIQ
 * @brief   Converts RGB components to YIQ.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as YIQ.
 * Output type must be the same as input type. Only signed types should be
 * used because the YIQ color space uses negative numbers.
 */

#ifndef vtkImageRGBToYIQ_h
#define vtkImageRGBToYIQ_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageRGBToYIQ : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageRGBToYIQ* New();
  vtkTypeMacro(vtkImageRGBToYIQ, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Maximum value of pixel intensity allowed. Default is 255.0.
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);

protected:
  vtkImageRGBToYIQ();
  ~vtkImageRGBToYIQ() override = default;

  double Maximum;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageRGBToYIQ(const vtkImageRGBToYIQ&) = delete;
  void operator=(const vtkImageRGBToYIQ&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
