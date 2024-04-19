// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkThresholdTable
 * @brief   Thresholds table rows.
 *
 *
 * vtkThresholdTable uses minimum and/or maximum values to threshold
 * table rows based on the values in a particular column.
 * The column to threshold is specified using SetInputArrayToProcess(0, ...).
 */

#ifndef vtkThresholdTable_h
#define vtkThresholdTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include "vtkVariant.h" // For vtkVariant arguments

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkThresholdTable : public vtkTableAlgorithm
{
public:
  static vtkThresholdTable* New();
  vtkTypeMacro(vtkThresholdTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    ACCEPT_LESS_THAN = 0,
    ACCEPT_GREATER_THAN = 1,
    ACCEPT_BETWEEN = 2,
    ACCEPT_OUTSIDE = 3
  };

  ///@{
  /**
   * The mode of the threshold filter.  Options are:
   * ACCEPT_LESS_THAN (0) accepts rows with values < MaxValue;
   * ACCEPT_GREATER_THAN (1) accepts rows with values > MinValue;
   * ACCEPT_BETWEEN (2) accepts rows with values > MinValue and < MaxValue;
   * ACCEPT_OUTSIDE (3) accepts rows with values < MinValue or > MaxValue.
   */
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);
  ///@}

  ///@{
  /**
   * The minimum value for the threshold.
   * This may be any data type stored in a vtkVariant.
   */
  virtual void SetMinValue(vtkVariant v)
  {
    this->MinValue = v;
    this->Modified();
  }
  virtual vtkVariant GetMinValue() { return this->MinValue; }
  ///@}

  ///@{
  /**
   * The maximum value for the threshold.
   * This may be any data type stored in a vtkVariant.
   */
  virtual void SetMaxValue(vtkVariant v)
  {
    this->MaxValue = v;
    this->Modified();
  }
  virtual vtkVariant GetMaxValue() { return this->MaxValue; }
  ///@}

  /**
   * Criterion is rows whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(vtkVariant lower, vtkVariant upper);

  /**
   * The minimum value for the threshold as a double.
   */
  void SetMinValue(double v) { this->SetMinValue(vtkVariant(v)); }

  /**
   * The maximum value for the threshold as a double.
   */
  void SetMaxValue(double v) { this->SetMaxValue(vtkVariant(v)); }

  /**
   * Criterion is rows whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper)
  {
    this->ThresholdBetween(vtkVariant(lower), vtkVariant(upper));
  }

  /**
   * Return true if value, converted to double, matches the criteria of the current Mode.
   */
  bool IsValueAcceptable(vtkVariant value);

protected:
  vtkThresholdTable();
  ~vtkThresholdTable() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkVariant MinValue;
  vtkVariant MaxValue;
  int Mode;

private:
  vtkThresholdTable(const vtkThresholdTable&) = delete;
  void operator=(const vtkThresholdTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
