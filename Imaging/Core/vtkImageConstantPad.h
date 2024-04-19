// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageConstantPad
 * @brief   Makes image larger by padding with constant.
 *
 * vtkImageConstantPad changes the image extent of its input.
 * Any pixels outside of the original image extent are filled with
 * a constant value (default is 0.0).
 *
 * @sa
 * vtkImageWrapPad vtkImageMirrorPad
 */

#ifndef vtkImageConstantPad_h
#define vtkImageConstantPad_h

#include "vtkImagePadFilter.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageConstantPad : public vtkImagePadFilter
{
public:
  static vtkImageConstantPad* New();
  vtkTypeMacro(vtkImageConstantPad, vtkImagePadFilter);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the pad value.
   */
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);
  ///@}

protected:
  vtkImageConstantPad();
  ~vtkImageConstantPad() override = default;

  double Constant;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData, int ext[6],
    int id) override;

private:
  vtkImageConstantPad(const vtkImageConstantPad&) = delete;
  void operator=(const vtkImageConstantPad&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
