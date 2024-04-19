// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageWrapPad
 * @brief   Makes an image larger by wrapping existing data.
 *
 * vtkImageWrapPad performs a modulo operation on the output pixel index
 * to determine the source input index.  The new image extent of the
 * output has to be specified.  Input has to be the same scalar type as
 * output.
 */

#ifndef vtkImageWrapPad_h
#define vtkImageWrapPad_h

#include "vtkImagePadFilter.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;

class VTKIMAGINGCORE_EXPORT vtkImageWrapPad : public vtkImagePadFilter
{
public:
  static vtkImageWrapPad* New();
  vtkTypeMacro(vtkImageWrapPad, vtkImagePadFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageWrapPad() = default;
  ~vtkImageWrapPad() override = default;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wholeExtent[6]) override;
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData, int ext[6],
    int id) override;

private:
  vtkImageWrapPad(const vtkImageWrapPad&) = delete;
  void operator=(const vtkImageWrapPad&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
