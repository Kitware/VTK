// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMapToRGBA
 * @brief   map the input image through a lookup table
 *
 * This filter has been replaced by vtkImageMapToColors, which provided
 * additional features.  Use vtkImageMapToColors instead.
 *
 * @sa
 * vtkLookupTable
 */

#ifndef vtkImageMapToRGBA_h
#define vtkImageMapToRGBA_h

#include "vtkImageMapToColors.h"
#include "vtkImagingColorModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageMapToRGBA : public vtkImageMapToColors
{
public:
  static vtkImageMapToRGBA* New();
  vtkTypeMacro(vtkImageMapToRGBA, vtkImageMapToColors);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageMapToRGBA() = default;
  ~vtkImageMapToRGBA() override = default;

private:
  vtkImageMapToRGBA(const vtkImageMapToRGBA&) = delete;
  void operator=(const vtkImageMapToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
