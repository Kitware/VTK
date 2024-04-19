// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMergeDataObjectFilter
 * @brief   merge dataset and data object field to create dataset with attribute data
 *
 * vtkMergeDataObjectFilter is a filter that merges the field from a
 * vtkDataObject with a vtkDataSet. The resulting combined dataset can
 * then be processed by other filters (e.g.,
 * vtkFieldDataToAttributeDataFilter) to create attribute data like
 * scalars, vectors, etc.
 *
 * The filter operates as follows. The field data from the
 * vtkDataObject is merged with the input's vtkDataSet and then placed
 * in the output. You can choose to place the field data into the cell
 * data field, the point data field, or the datasets field (i.e., the
 * one inherited from vtkDataSet's superclass vtkDataObject). All this
 * data shuffling occurs via reference counting, therefore memory is
 * not copied.
 *
 * One of the uses of this filter is to allow you to read/generate the
 * structure of a dataset independent of the attributes. So, for
 * example, you could store the dataset geometry/topology in one file,
 * and field data in another. Then use this filter in combination with
 * vtkFieldDataToAttributeData to create a dataset ready for
 * processing in the visualization pipeline.
 */

#ifndef vtkMergeDataObjectFilter_h
#define vtkMergeDataObjectFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkMergeDataObjectFilter : public vtkDataSetAlgorithm
{
public:
  static vtkMergeDataObjectFilter* New();
  vtkTypeMacro(vtkMergeDataObjectFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify the data object to merge with the input dataset.
   */
  void SetDataObjectInputData(vtkDataObject* object);
  vtkDataObject* GetDataObject();
  ///@}

  ///@{
  /**
   * Specify where to place the field data during the merge process.  There
   * are three choices: the field data associated with the vtkDataObject
   * superclass; the point field attribute data; and the cell field attribute
   * data.
   */
  vtkSetMacro(OutputField, int);
  vtkGetMacro(OutputField, int);
  void SetOutputFieldToDataObjectField();
  void SetOutputFieldToPointDataField();
  void SetOutputFieldToCellDataField();
  ///@}

protected:
  vtkMergeDataObjectFilter();
  ~vtkMergeDataObjectFilter() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int OutputField; // which output field

private:
  vtkMergeDataObjectFilter(const vtkMergeDataObjectFilter&) = delete;
  void operator=(const vtkMergeDataObjectFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
