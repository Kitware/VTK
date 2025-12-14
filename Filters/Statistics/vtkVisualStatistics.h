// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2010 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkVisualStatistics
 * @brief   A class to provide normality estimation and binned histogram data to render.
 *
 *
 * This filter simply adds a fixed-bin histogram approximation to the moment-based
 * descriptive statistics. It does not add new test or assessment data.
 */

#ifndef vtkVisualStatistics_h
#define vtkVisualStatistics_h

#include "vtkDescriptiveStatistics.h"
#include "vtkFiltersStatisticsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkStatisticalModel;
class vtkStringArray;
class vtkTable;
class vtkVariant;
class vtkDoubleArray;

class VTKFILTERSSTATISTICS_EXPORT vtkVisualStatistics : public vtkDescriptiveStatistics
{
public:
  vtkTypeMacro(vtkVisualStatistics, vtkDescriptiveStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkVisualStatistics* New();

  /// Set the numeric range to use for binning the histogram of the \a field.
  ///
  /// The \a field name is the name of a column in the table, while \a lo
  /// and \a hi are the minimum and maximum values the histogram should span.
  /// The interval [lo, hi] is **closed**, not half-open; values of "hi" that
  /// appear in data will not be counted as out-of-bounds.
  ///
  /// If the range [lo, hi] is invalid, no histogram will be computed
  /// for \a field. This can be caused by NaN values for \a lo or \a hi
  /// or if \a lo > \a hi.
  virtual void SetFieldRange(const char* field, double lo, double hi);
  virtual void SetFieldRange(const std::string& field, double lo, double hi);

  /// Clear all field ranges from this filter.
  virtual void ResetFieldRanges() { this->FieldRanges.clear(); }

  /**
   * Set the number of bins to use for histograms.
   * The default value is 512.
   *
   * Note that the number of histogram values returned is always NumberOfBins+3:
   *
   * + The first bin holds the number of values below the lower bound of the
   *   range for the given field.
   * + The penultimate bin holds the number of values above the upport bound
   *   of the range for the given field. (Note that the histogram interval
   *   is **closed** and *not* half-open, so if you specify [a,b] as the range,
   *   occurrences of b will *not* be included in the penultimate histogram bin.)
   * + For integer-valued arrays, the final bin entry is unused.
   * + For floating-point arrays, the final bin holds the number of NaN values present.
   *
   * Thus the resulting histograms are organized like so:
   *
   * +-------------------+---------------------------------+--------------------+-----+
   * | Out of bounds low | In bounds counts × NumberOfBins | Out of bounds high | NaN |
   * +-------------------+---------------------------------+--------------------+-----+
   */
  vtkSetClampMacro(NumberOfBins, int, 1, VTK_INT_MAX - 3);
  vtkGetMacro(NumberOfBins, int);

  /// Fetch a histogram array given a \a fieldName.
  ///
  /// If this method returns false, no such field exists.
  /// Otherwise, \a histogram will be set to the array of counts.
  vtkDataArray* GetHistogramForField(const std::string& fieldName);

  /// Provide a string that can be used to recreate an instance of this algorithm.
  void AppendAlgorithmParameters(std::string& algorithmParameters) const override;

  /// Implement the inverse of AppendAlgorithmParameters(): given parameters, update this algorithm.
  std::size_t ConsumeNextAlgorithmParameter(
    vtkStringToken parameterName, const std::string& algorithmParameters) override;

protected:
  vtkVisualStatistics();
  ~vtkVisualStatistics() override;

  /**
   * Execute the calculations required by the Learn option, given some input Data
   * NB: input parameters are unused.
   */
  void Learn(vtkTable*, vtkTable*, vtkStatisticalModel*) override;

  /**
   * Execute the calculations required by the Derive option.
   */
  void Derive(vtkStatisticalModel*) override;

  /**
   * Given a collection of models, calculate aggregate model
   */
  bool Aggregate(vtkDataObjectCollection*, vtkStatisticalModel*) override;

private:
  std::map<std::string, std::pair<double, double>> FieldRanges;
  int NumberOfBins{ 512 };

  vtkVisualStatistics(const vtkVisualStatistics&) = delete;
  void operator=(const vtkVisualStatistics&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
