// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkThresholdPoints
 * @brief   extracts points whose scalar value satisfies threshold criterion
 *
 * vtkThresholdPoints is a filter that extracts points from a dataset that
 * satisfy a threshold criterion. The criterion can take three forms:
 * 1) greater than a particular value; 2) less than a particular value; or
 * 3) between a particular value. The output of the filter is polygonal data.
 *
 * @sa
 * vtkThreshold vtkSelectEnclosedPoints vtkExtractEnclosedPoints
 */

#ifndef vtkThresholdPoints_h
#define vtkThresholdPoints_h

#include "vtkDeprecation.h"
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT VTK_MARSHALAUTO vtkThresholdPoints : public vtkPolyDataAlgorithm
{
public:
  static vtkThresholdPoints* New();
  vtkTypeMacro(vtkThresholdPoints, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Possible values for the threshold function:
   * - THRESHOLD_BETWEEN - Keep values between the lower and upper thresholds.
   * - THRESHOLD_LOWER - Keep values below the lower threshold.
   * - THRESHOLD_UPPER - Keep values above the upper threshold.
   */
  enum ThresholdType
  {
    THRESHOLD_BETWEEN = 0,
    THRESHOLD_LOWER,
    THRESHOLD_UPPER
  };

  ///@{
  /**
   * Set/Get the threshold method. Default: THRESHOLD_BETWEEN.
   */
  void SetThresholdFunction(int function);
  int GetThresholdFunction();
  ///@}

  /**
   * Criterion is cells whose scalars are less or equal to lower threshold.
   */
  VTK_DEPRECATED_IN_9_5_0("Use 'SetLowerThreshold' and 'SetThresholdFunction' instead.")
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are greater or equal to upper threshold.
   */
  VTK_DEPRECATED_IN_9_5_0("Use 'SetUpperThreshold' and 'SetThresholdFunction' instead.")
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  VTK_DEPRECATED_IN_9_5_0(
    "Use 'SetLowerThreshold', 'SetUpperThreshold' and 'SetThresholdFunction' instead.")
  void ThresholdBetween(double lower, double upper);

  ///@{
  /**
   * Set/get the upper and lower thresholds. The default values are set to +infinity and -infinity,
   * respectively.
   */
  vtkSetMacro(UpperThreshold, double);
  vtkGetMacro(UpperThreshold, double);
  vtkSetMacro(LowerThreshold, double);
  vtkGetMacro(LowerThreshold, double);
  ///@}

  ///@{
  /**
   * Set/Get the component to threshold. Set this to a value greater than the number of
   * components in the selected data array to threshold by magnitude.
   */
  vtkSetMacro(InputArrayComponent, int);
  vtkGetMacro(InputArrayComponent, int);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkThresholdPoints();
  ~vtkThresholdPoints() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  double LowerThreshold = -std::numeric_limits<double>::infinity();
  double UpperThreshold = std::numeric_limits<double>::infinity();
  int InputArrayComponent = 0;
  int OutputPointsPrecision = DEFAULT_PRECISION;

  int (vtkThresholdPoints::*ThresholdFunction)(double s) = &vtkThresholdPoints::Between;

  int Lower(double s) { return (s <= this->LowerThreshold ? 1 : 0); }
  int Upper(double s) { return (s >= this->UpperThreshold ? 1 : 0); }
  int Between(double s)
  {
    return (s >= this->LowerThreshold ? (s <= this->UpperThreshold ? 1 : 0) : 0);
  }

private:
  vtkThresholdPoints(const vtkThresholdPoints&) = delete;
  void operator=(const vtkThresholdPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
