// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageButterworthLowPass
 * @brief   Frequency domain Low pass.
 *
 * This filter only works on an image after it has been converted to
 * frequency domain by a vtkImageFFT filter.  A vtkImageRFFT filter
 * can be used to convert the output back into the spatial domain.
 * vtkImageButterworthLowPass the high frequency components are
 * attenuated.  Input and output are in doubles, with two components
 * (complex numbers).
 * out(i, j) = (1 + pow(CutOff/Freq(i,j), 2*Order));
 *
 * @sa
 * vtkImageButterworthHighPass vtkImageFFT vtkImageRFFT
 */

#ifndef vtkImageButterworthLowPass_h
#define vtkImageButterworthLowPass_h

#include "vtkImagingFourierModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGFOURIER_EXPORT vtkImageButterworthLowPass : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageButterworthLowPass* New();
  vtkTypeMacro(vtkImageButterworthLowPass, vtkThreadedImageAlgorithm);
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

  ///@{
  /**
   * The order determines sharpness of the cutoff curve.
   */
  vtkSetMacro(Order, int);
  vtkGetMacro(Order, int);
  ///@}

protected:
  vtkImageButterworthLowPass();
  ~vtkImageButterworthLowPass() override = default;

  int Order;
  double CutOff[3];

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageButterworthLowPass(const vtkImageButterworthLowPass&) = delete;
  void operator=(const vtkImageButterworthLowPass&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
