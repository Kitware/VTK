// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractSelectedArraysOverTime
 * @brief   extracts a selection over time.
 *
 * vtkExtractSelectedArraysOverTime extracts a selection over time.
 * This is combination of two filters, an vtkExtractSelection filter followed by
 * vtkExtractDataArraysOverTime, to do its work.
 *
 * The filter has two inputs - 0th input is the temporal data to extracted,
 * while the second input is the selection (vtkSelection) to extract. Based on
 * the type of the selection, this filter setups up properties on the internal
 * vtkExtractDataArraysOverTime instance to produce a reasonable extract.
 *
 * The output is a vtkMultiBlockDataSet. See vtkExtractDataArraysOverTime for
 * details on how the output is structured.
 */

#ifndef vtkExtractSelectedArraysOverTime_h
#define vtkExtractSelectedArraysOverTime_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer.

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkExtractDataArraysOverTime;
class vtkExtractSelection;
class vtkSelection;
class vtkTable;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedArraysOverTime
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractSelectedArraysOverTime* New();
  vtkTypeMacro(vtkExtractSelectedArraysOverTime, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the number of time steps
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  ///@}

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  ///@{
  /**
   * Set/get the vtkExtractSelection instance used to obtain
   * array values at each time step. By default, vtkExtractSelection is used.
   */
  virtual void SetSelectionExtractor(vtkExtractSelection*);
  vtkExtractSelection* GetSelectionExtractor();
  ///@}

  ///@{
  /**
   * Instead of breaking a selection into a separate time-history
   * table for each (block,ID)-tuple, you may call
   * ReportStatisticsOnlyOn(). Then a single table per
   * block of the input dataset will report the minimum, maximum,
   * quartiles, and (for numerical arrays) the average and standard
   * deviation of the selection over time.

   * The default is off to preserve backwards-compatibility.
   */
  vtkSetMacro(ReportStatisticsOnly, bool);
  vtkGetMacro(ReportStatisticsOnly, bool);
  vtkBooleanMacro(ReportStatisticsOnly, bool);
  ///@}

protected:
  vtkExtractSelectedArraysOverTime();
  ~vtkExtractSelectedArraysOverTime() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual void PostExecute(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  /**
   * Determines the FieldType and ContentType for the selection. If the
   * selection is a vtkSelection::SELECTIONS selection, then this method ensures
   * that all child nodes have the same field type and content type otherwise,
   * it returns 0.
   */
  int DetermineSelectionType(vtkSelection*);

  int NumberOfTimeSteps;
  int FieldType;
  int ContentType;
  bool ReportStatisticsOnly;
  int Error;

  enum Errors
  {
    NoError,
    MoreThan1Indices
  };

  vtkSmartPointer<vtkExtractSelection> SelectionExtractor;
  vtkSmartPointer<vtkExtractDataArraysOverTime> ArraysExtractor;

private:
  vtkExtractSelectedArraysOverTime(const vtkExtractSelectedArraysOverTime&) = delete;
  void operator=(const vtkExtractSelectedArraysOverTime&) = delete;

  /**
   * Applies the `SelectionExtractor` to extract the dataset to track
   * and return it. This should be called for each time iteration.
   */
  vtkSmartPointer<vtkDataObject> Extract(vtkInformationVector** inputV, vtkInformation* outInfo);

  bool IsExecuting;
};

VTK_ABI_NAMESPACE_END
#endif
