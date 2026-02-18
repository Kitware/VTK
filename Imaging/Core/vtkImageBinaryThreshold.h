// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkImageBinaryThreshold
 *
 * @brief vtkImageBinaryThreshold is an image thresholding algorithm that allows users to replace
 * pixel data by comparing them with 2 thresholds. The configuration of the filter allows access to
 * 3 different thresholding modes: `THRESHOLD_LOWER`, `THRESHOLD_UPPER` or `THRESHOLD_BETWEEN`. To
 * control the output of the filter, `ReplaceIn` and `ReplaceOut` can be set to specify whether to
 * use `InValue` and `OutValue` respectively to replace the input value. For example, if `ReplaceIn`
 * is set to 0, output pixels that are supposed to be `InValue` will not be replaced and will keep
 * it's original input value.
 */

#ifndef vtkImageBinaryThreshold_h
#define vtkImageBinaryThreshold_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKIMAGINGCORE_EXPORT vtkImageBinaryThreshold : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageBinaryThreshold* New();
  vtkTypeMacro(vtkImageBinaryThreshold, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether to replace the pixel in range with InValue
   * Default is false.
   */
  vtkSetMacro(ReplaceIn, bool);
  vtkGetMacro(ReplaceIn, bool);
  vtkBooleanMacro(ReplaceIn, bool);
  ///@}

  ///@{
  /**
   * Set/Get what replaces the in range pixels with this value.
   * Default is 0.0.
   */
  vtkSetMacro(InValue, double);
  vtkGetMacro(InValue, double);
  ///@}

  ///@{
  /**
   * Set/Get whether to replace the pixel out of range with OutValue.
   * Default is false.
   */
  vtkSetMacro(ReplaceOut, bool);
  vtkGetMacro(ReplaceOut, bool);
  vtkBooleanMacro(ReplaceOut, bool);
  ///@}

  ///@{
  /**
   * Set/Get what replaces the in range pixels with this value.
   * Default is 0.0.
   */
  vtkSetMacro(OutValue, double);
  vtkGetMacro(OutValue, double);
  ///@}

  ///@{
  /**
   * Set/Get the Upper threshold.
   * Default is VTK_FLOAT_MAX.
   */
  vtkSetMacro(UpperThreshold, double);
  vtkGetMacro(UpperThreshold, double);
  ///@}

  ///@{
  /**
   * Set/Get the Lower threshold.
   * Default is VTK_FLOAT_MIN.
   */
  vtkSetMacro(LowerThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  ///@}

  /**
   * Describe the behavior of the threshold:
   * - THRESHOLD_BETWEEN: uses both `LowerThreshold` and `UpperThreshold`. In this mode, anything in
   * the thresholds range will be replaced by the `InValue`. Anything out of the thresholds range
   * will be replaced by the `OutValue`.
   * - THRESHOLD_LOWER: uses only the `UpperThreshold`. In this mode, anything below the threshold
   * will be replaced by the `InValue`. Likewise, anything above will be replaced by the `OutValue`.
   * - THRESHOLD_UPPER: uses only the `LowerThreshold`. In this mode, anything above the threshold
   * will be replaced by the `InValue`. Likewise, anything below will be replaced by the `OutValue`.
   */
  enum ThresholdFunction
  {
    THRESHOLD_BETWEEN = 0,
    THRESHOLD_LOWER,
    THRESHOLD_UPPER,
  };

  ///@{
  /**
   * Set/Get the threshold mode.
   * Default is ThresholdMode::Between.
   */
  vtkSetMacro(ThresholdFunction, int);
  vtkGetMacro(ThresholdFunction, int);
  ///@}

  ///@{
  /**
   * Set the desired output scalar type to cast to. Scalar types can be found in `vtkType.h`.
   * If it is set to -1, the output type will be the same as the input type.
   * See vtkType.h for all data type values.
   * Default is -1.
   */
  vtkSetMacro(OutputScalarType, int);
  vtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(VTK_DOUBLE); }
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(VTK_FLOAT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(VTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(VTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(VTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(VTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(VTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(VTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(VTK_CHAR); }
  void SetOutputScalarTypeToSignedChar() { this->SetOutputScalarType(VTK_SIGNED_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(VTK_UNSIGNED_CHAR); }
  ///@}

protected:
  vtkImageBinaryThreshold() = default;
  ~vtkImageBinaryThreshold() override = default;

  /**
   * Set the output scalar type to the given OutputScalarType.
   * If it's value is -1, the scalar type is the same as the input scalar type.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * This method passes input and output data, and executes the filter
   * algorithm to fill the output from the input.
   * It just executes a switch statement to call the correct function for
   * the datas data types.
   */
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageBinaryThreshold(const vtkImageBinaryThreshold&) = delete;
  void operator=(const vtkImageBinaryThreshold&) = delete;

  int ThresholdFunction = ThresholdFunction::THRESHOLD_BETWEEN;
  double UpperThreshold = VTK_FLOAT_MAX;
  double LowerThreshold = VTK_FLOAT_MIN;
  bool ReplaceIn = false;
  double InValue = 0.0;
  bool ReplaceOut = false;
  double OutValue = 0.0;

  int OutputScalarType = -1; // invalid; output same as input
};

VTK_ABI_NAMESPACE_END
#endif // vtkImageBinaryThreshold_h
