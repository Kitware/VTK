// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageStencil
 * @brief   combine images via a cookie-cutter operation
 *
 * vtkImageStencil will combine two images together using a stencil.
 * The stencil should be provided in the form of a vtkImageStencilData,
 */

#ifndef vtkImageStencil_h
#define vtkImageStencil_h

#include "vtkImagingStencilModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImageStencilData;

class VTKIMAGINGSTENCIL_EXPORT vtkImageStencil : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageStencil* New();
  vtkTypeMacro(vtkImageStencil, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the stencil to use.  The stencil can be created
   * from a vtkImplicitFunction or a vtkPolyData. This
   * function does not setup a pipeline connection.
   */
  virtual void SetStencilData(vtkImageStencilData* stencil);
  vtkImageStencilData* GetStencil();
  ///@}

  /**
   * Specify the stencil to use. This sets up a pipeline connection.
   */
  void SetStencilConnection(vtkAlgorithmOutput* outputPort)
  {
    this->SetInputConnection(2, outputPort);
  }

  ///@{
  /**
   * Reverse the stencil.
   */
  vtkSetMacro(ReverseStencil, vtkTypeBool);
  vtkBooleanMacro(ReverseStencil, vtkTypeBool);
  vtkGetMacro(ReverseStencil, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set the second input.  This image will be used for the 'outside' of the
   * stencil.  If not set, the output voxels will be filled with
   * BackgroundValue instead.
   */
  virtual void SetBackgroundInputData(vtkImageData* input);
  vtkImageData* GetBackgroundInput();
  ///@}

  ///@{
  /**
   * Set the default output value to use when the second input is not set.
   */
  void SetBackgroundValue(double val) { this->SetBackgroundColor(val, val, val, val); }
  double GetBackgroundValue() { return this->BackgroundColor[0]; }
  ///@}

  ///@{
  /**
   * Set the default color to use when the second input is not set.
   * This is like SetBackgroundValue, but for multi-component images.
   */
  vtkSetVector4Macro(BackgroundColor, double);
  vtkGetVector4Macro(BackgroundColor, double);
  ///@}

protected:
  vtkImageStencil();
  ~vtkImageStencil() override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int extent[6], int id) override;

  vtkTypeBool ReverseStencil;
  double BackgroundColor[4];

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkImageStencil(const vtkImageStencil&) = delete;
  void operator=(const vtkImageStencil&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
