// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractHistogram
 * @brief   Extract histogram data (binned values) from any
 * dataset
 *
 * vtkExtractHistogram accepts any vtkDataSet as input and produces a
 * vtkTable containing histogram data as output.  The output vtkTable
 * will contains a vtkDoubleArray named "bin_extents" which contains
 * the boundaries between each histogram bin, and a vtkUnsignedLongArray
 * named "bin_values" which will contain the value for each bin.
 */

#ifndef vtkExtractHistogram_h
#define vtkExtractHistogram_h

#include <memory> // for std::unique_ptr

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkFieldData;
class vtkIntArray;
class vtkExtractHistogramInternal;

class VTKFILTERSSTATISTICS_EXPORT vtkExtractHistogram : public vtkTableAlgorithm
{
public:
  static vtkExtractHistogram* New();
  vtkTypeMacro(vtkExtractHistogram, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Controls which input data component should be binned, for input arrays
   * with more-than-one component.  Setting this to the number of components
   * will compute the histogram of the magnitude (L2 norm) of the tuple.
   * Default is 0.
   */
  vtkSetClampMacro(Component, int, 0, VTK_INT_MAX);
  vtkGetMacro(Component, int);
  ///@}

  ///@{
  /**
   * Controls the number of bins N in the output histogram data.
   * Default is 10.
   */
  vtkSetClampMacro(BinCount, int, 1, VTK_INT_MAX);
  vtkGetMacro(BinCount, int);
  ///@}

  ///@{
  /**
   * Get/Set if first and last bins must be centered around the min and max
   * data. This is only used when UseCustomBinRanges is set to false.
   * Default is false.
   */
  vtkSetMacro(CenterBinsAroundMinAndMax, bool);
  vtkGetMacro(CenterBinsAroundMinAndMax, bool);
  vtkBooleanMacro(CenterBinsAroundMinAndMax, bool);
  ///@}

  ///@{
  /**
   * Get/Set custom bin range to use. These are used only when
   * UseCustomBinRanges is set to true.
   * Default is [0, 100].
   */
  vtkSetVector2Macro(CustomBinRanges, double);
  vtkGetVector2Macro(CustomBinRanges, double);
  ///@}

  ///@{
  /**
   * When set to true, CustomBinRanges will  be used instead of using the full
   * range for the selected array.
   * Default is false.
   */
  vtkSetMacro(UseCustomBinRanges, bool);
  vtkGetMacro(UseCustomBinRanges, bool);
  vtkBooleanMacro(UseCustomBinRanges, bool);
  ///@}

  ///@{
  /**
   * This option controls whether the algorithm calculates averages
   * of variables other than the primary variable that fall into each
   * bin.
   * Default is false.
   */
  vtkSetMacro(CalculateAverages, bool);
  vtkGetMacro(CalculateAverages, bool);
  vtkBooleanMacro(CalculateAverages, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the bin extents array.
   * Default is "bin_extents".
   */
  vtkSetStringMacro(BinExtentsArrayName);
  vtkGetStringMacro(BinExtentsArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the name of the bin values array.
   * Default is "bin_values".
   */
  vtkSetStringMacro(BinValuesArrayName);
  vtkGetStringMacro(BinValuesArrayName);
  ///@}

  ///@{
  /**
   * If this option is set then the bin values will be normalized so that the sum of the bin values
   * adds up to 1.0.
   * Default is false.
   */
  vtkSetMacro(Normalize, bool);
  vtkBooleanMacro(Normalize, bool);
  vtkGetMacro(Normalize, bool);
  ///@}

  ///@{
  /**
   * If this option is set to true then the output table will contain an additional column with
   * accumulated bin values.
   * Default is false.
   */
  vtkSetMacro(Accumulation, bool);
  vtkBooleanMacro(Accumulation, bool);
  vtkGetMacro(Accumulation, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the optional bin accumulation array.
   * Default is "bin_accumulation".
   */
  vtkSetStringMacro(BinAccumulationArrayName);
  vtkGetStringMacro(BinAccumulationArrayName);
  ///@}

  ///@{
  /**
   * Get the bin range which was used to create the bin extents.
   */
  vtkGetVector2Macro(BinRange, double);
  ///@}

protected:
  vtkExtractHistogram();
  ~vtkExtractHistogram() override;

  /**
   * Returns the data range for the input array to process.
   * This method is not called with this->UseCustomBinRanges is true.
   * Returns true is range could be determined correctly, otherwise returns
   * false and range is set to {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN}. When returning
   * true, the actual data range is returned (without any extra padding).
   */
  virtual bool GetInputArrayRange(vtkInformationVector** inputVector, double range[2]);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Initialize the bin_extents using the data range for the selected
  // array.
  virtual bool InitializeBinExtents(vtkInformationVector** inputVector, vtkDoubleArray* binExtents);

  void BinAnArray(
    vtkDataArray* dataArray, vtkIntArray* binValues, double min, double max, vtkFieldData* field);

  void FillBinExtents(vtkDoubleArray* binExtents);

  void NormalizeBins(vtkTable* outputData);

  void AccumulateBins(vtkTable* outputData);

  double CustomBinRanges[2] = { 0, 100 };
  double BinRange[2] = { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX };
  bool CenterBinsAroundMinAndMax = false;
  bool UseCustomBinRanges = false;
  int Component = 0;
  int BinCount = 10;
  bool CalculateAverages = false;
  char* BinExtentsArrayName = nullptr;
  char* BinValuesArrayName = nullptr;
  char* BinAccumulationArrayName = nullptr;
  bool Normalize = false;
  bool Accumulation = false;

  std::unique_ptr<vtkExtractHistogramInternal> Internal;

private:
  void operator=(const vtkExtractHistogram&) = delete;
  vtkExtractHistogram(const vtkExtractHistogram&) = delete;

  int GetInputFieldAssociation();
  vtkFieldData* GetInputFieldData(vtkDataObject* input);
};

VTK_ABI_NAMESPACE_END
#endif
