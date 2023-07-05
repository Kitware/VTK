// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMask
 * @brief   Combines a mask and an image.
 *
 * vtkImageMask combines a mask with an image.  Non zero mask
 * implies the output pixel will be the same as the image.
 * If a mask pixel is zero,  then the output pixel
 * is set to "MaskedValue".  The filter also has the option to pass
 * the mask through a boolean not operation before processing the image.
 * This reverses the passed and replaced pixels.
 * The two inputs should have the same "WholeExtent".
 * The mask input should be unsigned char, and the image scalar type
 * is the same as the output scalar type.
 */

#ifndef vtkImageMask_h
#define vtkImageMask_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageMask : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMask* New();
  vtkTypeMacro(vtkImageMask, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * SetGet the value of the output pixel replaced by mask.
   */
  void SetMaskedOutputValue(int num, double* v);
  void SetMaskedOutputValue(double v) { this->SetMaskedOutputValue(1, &v); }
  void SetMaskedOutputValue(double v1, double v2)
  {
    double v[2];
    v[0] = v1;
    v[1] = v2;
    this->SetMaskedOutputValue(2, v);
  }
  void SetMaskedOutputValue(double v1, double v2, double v3)
  {
    double v[3];
    v[0] = v1;
    v[1] = v2;
    v[2] = v3;
    this->SetMaskedOutputValue(3, v);
  }
  double* GetMaskedOutputValue() { return this->MaskedOutputValue; }
  int GetMaskedOutputValueLength() { return this->MaskedOutputValueLength; }

  ///@{
  /**
   * Set/Get the alpha blending value for the mask
   * The input image is assumed to be at alpha = 1.0
   * and the mask image uses this alpha to blend using
   * an over operator.
   */
  vtkSetClampMacro(MaskAlpha, double, 0.0, 1.0);
  vtkGetMacro(MaskAlpha, double);
  ///@}

  /**
   * Set the input to be masked.
   */
  void SetImageInputData(vtkImageData* in);

  /**
   * Set the mask to be used.
   */
  void SetMaskInputData(vtkImageData* in);

  ///@{
  /**
   * When Not Mask is on, the mask is passed through a boolean not
   * before it is used to mask the image.  The effect is to pass the
   * pixels where the input mask is zero, and replace the pixels
   * where the input value is non zero.
   */
  vtkSetMacro(NotMask, vtkTypeBool);
  vtkGetMacro(NotMask, vtkTypeBool);
  vtkBooleanMacro(NotMask, vtkTypeBool);
  ///@}

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(vtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(vtkDataObject* in) { this->SetInputData(1, in); }

protected:
  vtkImageMask();
  ~vtkImageMask() override;

  double* MaskedOutputValue;
  int MaskedOutputValueLength;
  vtkTypeBool NotMask;
  double MaskAlpha;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int threadId) override;

private:
  vtkImageMask(const vtkImageMask&) = delete;
  void operator=(const vtkImageMask&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
