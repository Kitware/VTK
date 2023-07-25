// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageCheckerboard
 * @brief   show two images at once using a checkboard pattern
 *
 *  vtkImageCheckerboard displays two images as one using a checkerboard
 *  pattern. This filter can be used to compare two images. The
 *  checkerboard pattern is controlled by the NumberOfDivisions
 *  ivar. This controls the number of checkerboard divisions in the whole
 *  extent of the image.
 */

#ifndef vtkImageCheckerboard_h
#define vtkImageCheckerboard_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageCheckerboard : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageCheckerboard* New();
  vtkTypeMacro(vtkImageCheckerboard, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the number of divisions along each axis.
   */
  vtkSetVector3Macro(NumberOfDivisions, int);
  vtkGetVectorMacro(NumberOfDivisions, int, 3);
  ///@}

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(vtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(vtkDataObject* in) { this->SetInputData(1, in); }

protected:
  vtkImageCheckerboard();
  ~vtkImageCheckerboard() override = default;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int threadId) override;
  int NumberOfDivisions[3];

private:
  vtkImageCheckerboard(const vtkImageCheckerboard&) = delete;
  void operator=(const vtkImageCheckerboard&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
