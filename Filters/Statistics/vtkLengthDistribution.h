// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLengthDistribution
 * @brief   Sample the distribution of representative "cell lengths"
 *  of a mesh.
 *
 * vtkLengthDistribution chooses a subset of N cells and, for each one, chooses
 * two random connectivity entries of the cell. Then it computes the distance
 * between the corresponding points and inserts the distance into an ordered set.
 * The result is a cumulative distribution function (CDF) of lengths which are
 * representative of the length scales present in the dataset.
 * Quantiles (other than the extremal values) of this distribution should be
 * relatively stable estimates of length scales compared to moment-based
 * estimates that may be skewed by outlier elements.
 *
 * This filter produces a vtkTable as its result, with N rows containing
 * monotonically increasing length values. Only polydata and unstructured
 * grids are accepted; other dataset types will produce an empty table
 * and a warning.
 */

#ifndef vtkLengthDistribution_h
#define vtkLengthDistribution_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkDataArray;

class VTKFILTERSSTATISTICS_EXPORT vtkLengthDistribution : public vtkTableAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkLengthDistribution, vtkTableAlgorithm);
  static vtkLengthDistribution* New();

  /// Set/get the size of the distribution.
  /// The output table will be no larger than this but may be
  /// smaller if the input dataset has fewer cells.
  /// The default is to generate 100,000 samples.
  vtkGetMacro(SampleSize, vtkIdType);
  vtkSetMacro(SampleSize, vtkIdType);

  /// Set/get whether to sort the table rows or not.
  ///
  /// By default, the cell lengths are sorted so that
  /// the table can be used as a CDF. If you are working
  /// with large samples and do not need the sampled
  /// set of lengths sorted, turn this off.
  vtkGetMacro(SortSample, bool);
  vtkSetMacro(SortSample, bool);
  vtkBooleanMacro(SortSample, bool);

  /// Return the length scale at a particular quantile.
  ///
  /// This method must only be invoked after the filter
  /// has been run (i.e., the output is up-to-date with
  /// the filter inputs and parameters). It is a convenience
  /// method that fetches the cell-length column from the
  /// output table and returns the value at or immediately
  /// below the requested quantile. If the filter is
  /// configured not to sort data, this method will throw
  /// an exception since the output table is not a CDF.
  ///
  /// By default, the method returns the median length.
  double GetLengthQuantile(double qq = 0.5);

protected:
  ~vtkLengthDistribution() override = default;
  vtkLengthDistribution() = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkIdType SampleSize = 100000;
  bool SortSample = true;

private:
  vtkLengthDistribution(const vtkLengthDistribution&) = delete;
  void operator=(const vtkLengthDistribution&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif // vtkLengthDistribution_h
