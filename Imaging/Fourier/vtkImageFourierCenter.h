// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageFourierCenter
 * @brief   Shifts constant frequency to center for
 * display.
 *
 * Is used for displaying images in frequency space.  FFT converts spatial
 * images into frequency space, but puts the zero frequency at the origin.
 * This filter shifts the zero frequency to the center of the image.
 * Input and output are assumed to be doubles.
 */

#ifndef vtkImageFourierCenter_h
#define vtkImageFourierCenter_h

#include "vtkImageDecomposeFilter.h"
#include "vtkImagingFourierModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGFOURIER_EXPORT vtkImageFourierCenter : public vtkImageDecomposeFilter
{
public:
  static vtkImageFourierCenter* New();
  vtkTypeMacro(vtkImageFourierCenter, vtkImageDecomposeFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageFourierCenter();
  ~vtkImageFourierCenter() override = default;

  int IterativeRequestUpdateExtent(vtkInformation* in, vtkInformation* out) override;

  void ThreadedRequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector,
    vtkImageData*** inDataVec, vtkImageData** outDataVec, int outExt[6], int threadId) override;

private:
  vtkImageFourierCenter(const vtkImageFourierCenter&) = delete;
  void operator=(const vtkImageFourierCenter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
