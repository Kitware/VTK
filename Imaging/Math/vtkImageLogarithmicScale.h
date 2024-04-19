// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageLogarithmicScale
 * @brief   Passes each pixel through log function.
 *
 * vtkImageLogarithmicScale passes each pixel through the function
 * c*log(1+x).  It also handles negative values with the function
 * -c*log(1-x).
 */

#ifndef vtkImageLogarithmicScale_h
#define vtkImageLogarithmicScale_h

#include "vtkImagingMathModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGMATH_EXPORT vtkImageLogarithmicScale : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageLogarithmicScale* New();
  vtkTypeMacro(vtkImageLogarithmicScale, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the scale factor for the logarithmic function.
   */
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);
  ///@}

protected:
  vtkImageLogarithmicScale();
  ~vtkImageLogarithmicScale() override = default;

  double Constant;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int outExt[6], int id) override;

private:
  vtkImageLogarithmicScale(const vtkImageLogarithmicScale&) = delete;
  void operator=(const vtkImageLogarithmicScale&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
