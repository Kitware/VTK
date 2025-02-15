// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * \class vtkImageSSIM
 * \brief This filter implements an algorithm based on SSIM for image comparison.
 *
 * The SSIM (Structural Similarity Index Measure) is a method for measuring
 * the similarity between two images. The SSIM index was originally proposed in:
 * "Zhou Wang, Alan C. Bovik, Hamid R. Sheikh, Eero P. Simoncelli, 'Image Quality
 * Assessment: From Error Visibility to Structural Similarity', IEEE Transactions
 * on Image Processing, 13(4), 600-612, April 2004."
 *
 * This filter takes 2 images as inputs. It will compare the arrays returned by `GetScalars` in the
 * input point data. The 2 arrays need te have the same underlying type, and have the same number of
 * components. It outputs a heatmap of SSIM values between -1 and 1, measuring the structural
 * similarity between 2 corresponding tiles in the images. Input values are weighted by a Gaussian
 * kernel whose standard deviation is defined as `PatchRadius / 3`.
 *
 * The range of the input arrays is important to know for regularizations purposes. 2
 * constants `c1` and `c2` are set by the filter to avoid divisions by zero without distorting the
 * SSIM computation too much. If `L` is the range of an input channel, then `c1 = 0.0001 * L * L`,
 * and `c2 = 0.0009 * L * L`, as advised by the original paper. The range can be automatically
 * computed, or set by the user sungi `SetInputToLab`, `SetInputToRGB`, `SetInputToGrayscale` or
 * `SetInputRange`.
 */

#ifndef vtkImageSSIM_h
#define vtkImageSSIM_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkSmartPointer.h"      // For smart pointer
#include "vtkThreadedImageAlgorithm.h"

#include <array>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkImageSSIMThreadData;
class vtkImageSSIMSMPThreadLocal;
class VTKIMAGINGCORE_EXPORT vtkImageSSIM : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageSSIM* New();
  vtkTypeMacro(vtkImageSSIM, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the Image to compare the input to.
   */
  void SetImageConnection(vtkAlgorithmOutput* output) { this->SetInputConnection(1, output); }
  void SetImageData(vtkDataObject* image) { this->SetInputData(1, image); }
  ///@}

  /**
   * Assume the input is in Lab format. This will set appropriate constants c1 and c2 for each
   * input channel
   */
  void SetInputToLab();

  /**
   * Assume the input is in RGB format, using integers from 0 to 255.
   * This will set appropriate constants c1 and c2 for each input channel
   */
  void SetInputToRGB();

  /**
   * Assume the input is in RGBA format, using integers from 0 to 255.
   * This will set appropriate constants c1 and c2 for each input channel
   */
  void SetInputToRGBA();

  /**
   * Assume the input is in grayscale, using integers from 0 to 255.
   * This will set appropriate constants c1 and c2
   */
  void SetInputToGrayscale();

  /**
   * The c1 and c2 constant will be computed automatically based on the range of each individual
   * components Please note the resulting SSIM can be NaN in specific cases.
   */
  void SetInputToAuto();

  /**
   * Setup the range of each components of the input scalars. If the range has not been set, or if
   * the number of components in the input does not match the number of provided ranges,
   * it will be automatically be set using the input array. The range is used to set the constants
   * c1 and c2.
   */
  void SetInputRange(std::vector<int>& range);

  ///@{
  /**
   * When turned ON, negative outputs of the SSIM computation are clamped to zero.
   * It is turned OFF by default.
   */
  vtkSetMacro(ClampNegativeValues, bool);
  vtkGetMacro(ClampNegativeValues, bool);
  vtkBooleanMacro(ClampNegativeValues, bool);
  ///@}

  ///@{
  /**
   * This sets the size of the tile used to compute the SSIM on each patch. A pixel x are within the
   * patch centered around a pixel p if ||x - p|| <= Radius.
   * It is set to 3 by default.
   */
  vtkSetMacro(PatchRadius, double);
  vtkGetMacro(PatchRadius, double);
  ///@}

  /**
   * Compute error metrics of a provided scalars.
   * Error is defined as the maximum of all individual values within the used method.
   * Errors are computed using Minkownski and Wasserstein distances.
   * Method used are euclidean (tight) or manhattan / earth's mover (loose)
   */
  static void ComputeErrorMetrics(vtkDoubleArray* scalars, double& tight, double& loose);

protected:
  vtkImageSSIM();
  ~vtkImageSSIM() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  using Superclass::AllocateOutputData;
  void AllocateOutputData(vtkImageData* output, vtkInformation* outInfo, int* uExtent) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int threadId) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  void SetInputToAdditiveChar(unsigned int size);
  void GrowExtent(int* uExt, int* wholeExtent);

  int PatchRadius = 6;
  bool ClampNegativeValues = false;

  enum
  {
    MODE_LAB,
    MODE_RGB,
    MODE_RGBA,
    MODE_GRAYSCALE,
    MODE_AUTO,
    MODE_INPUT_RANGE
  };
  int Mode = MODE_AUTO;

  /**
   * Regularization constants. They are set depending on the range of the input data.
   */
  std::vector<std::array<double, 2>> C;

  vtkImageSSIM(const vtkImageSSIM&) = delete;
  void operator=(const vtkImageSSIM&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
