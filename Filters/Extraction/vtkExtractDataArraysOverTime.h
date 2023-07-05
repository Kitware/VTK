// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractDataArraysOverTime
 * @brief   extracts array from input dataset over time.
 *
 * vtkExtractDataArraysOverTime extracts array from input dataset over time.
 * The filter extracts attribute arrays, based on the chosen field association
 * (vtkExtractDataArraysOverTime::SetFieldAssociation).
 *
 * vtkExtractDataArraysOverTime::ReportStatisticsOnly determines if each element
 * is individually tracked or only summary statistics for each timestep are
 * tracked.
 *
 * If ReportStatisticsOnly is off, the filter tracks each element in the input
 * over time. It requires that it can identify matching elements from one
 * timestep to another. There are several ways of doing that.
 *
 * \li if vtkExtractDataArraysOverTime::UseGlobalIDs is true, then the filter
 *     will look for array marked as vtkDataSetAttributes::GLOBALIDS in the
 *     input and use that to track the element.
 * \li if vtkExtractDataArraysOverTime::UseGlobalIDs is false or there are no
 *     element ids present, then the filter will look for the array chosen for
 *     processing using `vtkAlgorithm::SetInputArrayToProcess` at index 0.
 * \li if earlier attempts fail, then simply the element id (i.e. index) is used.
 *
 * The output is a vtkMultiBlockDataSet with single level, where leaf nodes can
 * are vtkTable instances.
 *
 * The output is structured as follows:
 *
 * \li if vtkExtractDataArraysOverTime::ReportStatisticsOnly is true, then the
 *     stats are computed per input block (if input is a composite dataset) or on the whole
 *     input dataset and placed as blocks named as <tt>stats block=\<block id\></tt>.
 *     For non-composite input, the single leaf block is output is named as
 *     \c stats.
 *
 * \li if vtkExtractDataArraysOverTime::ReportStatisticsOnly if off, then each
 *    tracked element is placed in a separate output block. The block name is of
 *    the form <tt>id=\<id\> block=\<block id\></tt> where the \em block= suffix is
 *    dropped for non-composite input datasets. If global ids are being used for
 *    tracking then the name is simply <tt>gid=\<global id\></tt>.
 *
 * @sa vtkPExtractDataArraysOverTime
 */

#ifndef vtkExtractDataArraysOverTime_h
#define vtkExtractDataArraysOverTime_h

#include "vtkDataObject.h"              // for vtkDataObject
#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkTable;
class vtkDataSetAttributes;
class vtkDescriptiveStatistics;
class vtkOrderStatistics;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractDataArraysOverTime
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractDataArraysOverTime* New();
  vtkTypeMacro(vtkExtractDataArraysOverTime, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the number of time steps
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  ///@}

  ///@{
  /**
   * FieldAssociation indicates which attributes to extract over time. This filter
   * can extract only one type of attribute arrays. Currently, vtkDataObject::FIELD
   * and vtkDataObject::POINT_THEN_CELL are not supported.
   */
  vtkSetClampMacro(
    FieldAssociation, int, vtkDataObject::POINT, vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES - 1);
  vtkGetMacro(FieldAssociation, int);
  ///@}

  ///@{
  /**
   * Instead of breaking a data into a separate time-history
   * table for each (block,ID)-tuple, you may call
   * ReportStatisticsOnlyOn(). Then a single table per
   * block of the input dataset will report the minimum, maximum,
   * quartiles, and (for numerical arrays) the average and standard
   * deviation of the data over time.

   * The default is off to preserve backwards-compatibility.
   */
  vtkSetMacro(ReportStatisticsOnly, bool);
  vtkGetMacro(ReportStatisticsOnly, bool);
  vtkBooleanMacro(ReportStatisticsOnly, bool);
  ///@}

  ///@{
  /**
   * When ReportStatisticsOnly is false, if UseGlobalIDs is true, then the filter will track
   * elements using their global ids, if present. Default is true.
   */
  vtkSetMacro(UseGlobalIDs, bool);
  vtkGetMacro(UseGlobalIDs, bool);
  ///@}

protected:
  vtkExtractDataArraysOverTime();
  ~vtkExtractDataArraysOverTime() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual void PostExecute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int CurrentTimeIndex;
  int NumberOfTimeSteps;
  int FieldAssociation;
  bool ReportStatisticsOnly;
  bool UseGlobalIDs;
  int Error;
  enum Errors
  {
    NoError,
    MoreThan1Indices
  };

  virtual vtkSmartPointer<vtkDescriptiveStatistics> NewDescriptiveStatistics();
  virtual vtkSmartPointer<vtkOrderStatistics> NewOrderStatistics();

private:
  vtkExtractDataArraysOverTime(const vtkExtractDataArraysOverTime&) = delete;
  void operator=(const vtkExtractDataArraysOverTime&) = delete;

  class vtkInternal;
  friend class vtkInternal;
  vtkInternal* Internal;
};
VTK_ABI_NAMESPACE_END
#endif
