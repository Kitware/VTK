// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageIdealLowPass
 * @brief   Simple frequency domain band pass.
 *
 * This filter only works on an image after it has been converted to
 * frequency domain by a vtkImageFFT filter.  A vtkImageRFFT filter
 * can be used to convert the output back into the spatial domain.
 * vtkImageIdealLowPass just sets a portion of the image to zero.  The result
 * is an image with a lot of ringing.  Input and Output must be doubles.
 * Dimensionality is set when the axes are set.  Defaults to 2D on X and Y
 * axes.
 *
 * @sa
 * vtkImageButterworthLowPass vtkImageIdealHighPass vtkImageFFT vtkImageRFFT
 */

#ifndef vtkImageIdealLowPass_h
#define vtkImageIdealLowPass_h

#include "vtkImagingFourierModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGFOURIER_EXPORT vtkImageIdealLowPass : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageIdealLowPass* New();
  vtkTypeMacro(vtkImageIdealLowPass, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the cutoff frequency for each axis.
   * The values are specified in the order X, Y, Z, Time.
   * Units: Cycles per world unit (as defined by the data spacing).
   */
  vtkSetVector3Macro(CutOff, double);
  void SetCutOff(double v) { this->SetCutOff(v, v, v); }
  void SetXCutOff(double cutOff);
  void SetYCutOff(double cutOff);
  void SetZCutOff(double cutOff);
  vtkGetVector3Macro(CutOff, double);
  double GetXCutOff() { return this->CutOff[0]; }
  double GetYCutOff() { return this->CutOff[1]; }
  double GetZCutOff() { return this->CutOff[2]; }
  ///@}

protected:
  vtkImageIdealLowPass();
  ~vtkImageIdealLowPass() override = default;

  double CutOff[3];

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageIdealLowPass(const vtkImageIdealLowPass&) = delete;
  void operator=(const vtkImageIdealLowPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
