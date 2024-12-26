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
class vtkDoubleArray;

class VTKIMAGINGCORE_EXPORT vtkImageConstantPad : public vtkImagePadFilter
{
public:
  static vtkImageConstantPad* New();
  vtkTypeMacro(vtkImageConstantPad, vtkImagePadFilter);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the pad value.
   *
   * @note if the ComponentConstants array is set, this value is ignored.
   */
  vtkSetMacro(Constant, double);
  vtkGetMacro(Constant, double);
  ///@}

  ///@{
  /**
   * Set/Get the pad values for each component.
   *
   * @note If this array is set, the Constant value is ignored.
   */
  virtual void SetComponentConstants(vtkDoubleArray* values);
  vtkGetObjectMacro(ComponentConstants, vtkDoubleArray);
  ///@}

protected:
  vtkImageConstantPad();
  ~vtkImageConstantPad() override;

  double Constant;
  vtkDoubleArray* ComponentConstants;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData, int ext[6],
    int id) override;

private:
  vtkImageConstantPad(const vtkImageConstantPad&) = delete;
  void operator=(const vtkImageConstantPad&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
