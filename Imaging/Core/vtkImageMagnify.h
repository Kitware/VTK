// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageMagnify
 * @brief   magnify an image by an integer value
 *
 * vtkImageMagnify maps each pixel of the input onto a nxmx... region
 * of the output.  Location (0,0,...) remains in the same place. The
 * magnification occurs via pixel replication, or if Interpolate is on,
 * by bilinear interpolation. Initially, interpolation is off and magnification
 * factors are set to 1 in all directions.
 */

#ifndef vtkImageMagnify_h
#define vtkImageMagnify_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImageMagnify : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageMagnify* New();
  vtkTypeMacro(vtkImageMagnify, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the integer magnification factors in the i-j-k directions.
   * Initially, factors are set to 1 in all directions.
   */
  vtkSetVector3Macro(MagnificationFactors, int);
  vtkGetVector3Macro(MagnificationFactors, int);
  ///@}

  ///@{
  /**
   * Turn interpolation on and off (pixel replication is used when off).
   * Initially, interpolation is off.
   */
  vtkSetMacro(Interpolate, vtkTypeBool);
  vtkGetMacro(Interpolate, vtkTypeBool);
  vtkBooleanMacro(Interpolate, vtkTypeBool);
  ///@}

protected:
  vtkImageMagnify();
  ~vtkImageMagnify() override = default;

  int MagnificationFactors[3];
  vtkTypeBool Interpolate;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

  void InternalRequestUpdateExtent(int* inExt, int* outExt);

private:
  vtkImageMagnify(const vtkImageMagnify&) = delete;
  void operator=(const vtkImageMagnify&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
