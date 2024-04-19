// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkOrderStatistics
 * @brief   A class for univariate order statistics
 *
 *
 * Given a selection of columns of interest in an input data table, this
 * class provides the following functionalities, depending on the
 * execution mode it is executed in:
 * * Learn: calculate histogram.
 * * Derive: calculate PDFs and arbitrary quantiles. Provide specific names when 5-point
 *   statistics (minimum, 1st quartile, median, third quartile, maximum) requested.
 * * Assess: given an input data set and a set of q-quantiles, label each datum
 *   either with the quantile interval to which it belongs, or 0 if it is smaller
 *   than smaller quantile, or q if it is larger than largest quantile.
 * * Test: calculate Kolmogorov-Smirnov goodness-of-fit statistic between CDF based on
 *   model quantiles, and empirical CDF
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
 * Updated by Philippe Pebay, Kitware SAS 2012
 */

#ifndef vtkOrderStatistics_h
#define vtkOrderStatistics_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkStatisticsAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiBlockDataSet;
class vtkStringArray;
class vtkTable;
class vtkVariant;

class VTKFILTERSSTATISTICS_EXPORT vtkOrderStatistics : public vtkStatisticsAlgorithm
{
public:
  vtkTypeMacro(vtkOrderStatistics, vtkStatisticsAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkOrderStatistics* New();

  /**
   * The type of quantile definition.
   */
  enum QuantileDefinitionType
  {
    InverseCDF = 0,              // Identical to method 1 of R
    InverseCDFAveragedSteps = 1, // Identical to method 2 of R, ignored for non-numeric types
    NearestObservation = 2       // Identical to method 3 of R
  };

  ///@{
  /**
   * Set/Get the number of quantiles (with uniform spacing).
   */
  vtkSetMacro(NumberOfIntervals, vtkIdType);
  vtkGetMacro(NumberOfIntervals, vtkIdType);
  ///@}

  ///@{
  /**
   * Set the quantile definition.
   */
  vtkSetMacro(QuantileDefinition, QuantileDefinitionType);
  void SetQuantileDefinition(int);
  ///@}

  ///@{
  /**
   * Set/Get whether quantization will be allowed to enforce maximum histogram size.
   */
  vtkSetMacro(Quantize, bool);
  vtkGetMacro(Quantize, bool);
  ///@}

  ///@{
  /**
   * Set/Get the maximum histogram size.
   * This maximum size is enforced only when Quantize is TRUE.
   */
  vtkSetMacro(MaximumHistogramSize, vtkIdType);
  vtkGetMacro(MaximumHistogramSize, vtkIdType);
  ///@}

  /**
   * Get the quantile definition.
   */
  vtkIdType GetQuantileDefinition() { return static_cast<vtkIdType>(this->QuantileDefinition); }

  /**
   * A convenience method (in particular for access from other applications) to
   * set parameter values.
   * Return true if setting of requested parameter name was executed, false otherwise.
   */
  bool SetParameter(const char* parameter, int index, vtkVariant value) override;

  /**
   * Given a collection of models, calculate aggregate model
   * NB: not implemented
   */
  void Aggregate(vtkDataObjectCollection*, vtkMultiBlockDataSet*) override {}

  ///@{
  /**
   * If there is a ghost array in the input, then ghosts matching `GhostsToSkip` mask
   * will be skipped. It is set to 0xff by default (every ghosts types are skipped).
   *
   * @sa
   * vtkDataSetAttributes
   * vtkFieldData
   * vtkPointData
   * vtkCellData
   */
  vtkSetMacro(GhostsToSkip, unsigned char);
  vtkGetMacro(GhostsToSkip, unsigned char);
  ///@}

protected:
  vtkOrderStatistics();
  ~vtkOrderStatistics() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Execute the calculations required by the Learn option.
   */
  void Learn(vtkTable*, vtkTable*, vtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(vtkMultiBlockDataSet*) override;

  /**
   * Execute the calculations required by the Test option.
   */
  void Test(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) override;

  /**
   * Execute the calculations required by the Assess option.
   */
  void Assess(vtkTable* inData, vtkMultiBlockDataSet* inMeta, vtkTable* outData) override
  {
    this->Superclass::Assess(inData, inMeta, outData, 1);
  }

  /**
   * Provide the appropriate assessment functor.
   */
  void SelectAssessFunctor(vtkTable* outData, vtkDataObject* inMeta, vtkStringArray* rowNames,
    AssessFunctor*& dfunc) override;

  vtkIdType NumberOfIntervals;
  QuantileDefinitionType QuantileDefinition;
  bool Quantize;
  vtkIdType MaximumHistogramSize;
  vtkIdType NumberOfGhosts;
  unsigned char GhostsToSkip;

private:
  vtkOrderStatistics(const vtkOrderStatistics&) = delete;
  void operator=(const vtkOrderStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
