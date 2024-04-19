// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendCompositeDataLeaves
 * @brief   appends one or more composite datasets with the same structure together into a single
 * output composite dataset
 *
 * vtkAppendCompositeDataLeaves is a filter that takes input composite datasets with the same
 * structure: (1) the same number of entries and -- if any children are composites -- the
 * same constraint holds from them; and (2) the same type of dataset at each position.
 * It then creates an output dataset with the same structure whose leaves contain all the
 * cells from the datasets at the corresponding leaves of the input datasets.
 *
 * Currently, this filter only supports "appending" of a few types for the leaf
 * nodes and the logic used for each supported data type is as follows:
 *
 * \li vtkUnstructuredGrid - appends all unstructured grids from the leaf
 *     location on all inputs into a single unstructured grid for the
 *     corresponding location in the output composite dataset. PointData and
 *     CellData arrays are extracted and appended only if they are available in
 *     all datasets.(For example, if one dataset has scalars but another does
 *     not, scalars will not be appended.)
 *
 * \li vtkPolyData - appends all polydatas from the leaf location on all inputs
 *     into a single polydata for the corresponding location in the output
 *     composite dataset. PointData and CellData arrays are extracted and
 *     appended only if they are available in all datasets.(For example, if one
 *     dataset has scalars but another does not, scalars will not be appended.)
 *
 * \li vtkImageData/vtkUniformGrid - simply passes the first non-null
 *     grid for a particular location to corresponding location in the output.
 *
 * \li vtkTable - simply passes the first non-null vtkTable for a particular
 *     location to the corresponding location in the output.
 *
 * Other types of leaf datasets will be ignored and their positions in the
 * output dataset will be nullptr pointers.
 *
 * @sa
 * vtkAppendPolyData vtkAppendFilter
 */

#ifndef vtkAppendCompositeDataLeaves_h
#define vtkAppendCompositeDataLeaves_h

#include "vtkCompositeDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCompositeDataIterator;
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkAppendCompositeDataLeaves : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkAppendCompositeDataLeaves* New();

  vtkTypeMacro(vtkAppendCompositeDataLeaves, vtkCompositeDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get whether the field data of each dataset in the composite dataset is copied to the
   * output. If AppendFieldData is non-zero, then field data arrays from all the inputs are added to
   * the output. If there are duplicates, the array on the first input encountered is taken.
   */
  vtkSetMacro(AppendFieldData, vtkTypeBool);
  vtkGetMacro(AppendFieldData, vtkTypeBool);
  vtkBooleanMacro(AppendFieldData, vtkTypeBool);
  ///@}

protected:
  vtkAppendCompositeDataLeaves();
  ~vtkAppendCompositeDataLeaves() override;

  /**
   * Since vtkCompositeDataSet is an abstract class and we output the same types as the input,
   * we must override the default implementation.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Iterates over the datasets and appends corresponding notes.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * The input is repeatable, so we override the default implementation.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * When leaf nodes are unstructured grids, this uses a vtkAppendFilter to merge them.
   */
  virtual void AppendUnstructuredGrids(vtkInformationVector* inputVector, int i, int numInputs,
    vtkCompositeDataIterator* iter, vtkCompositeDataSet* output);

  /**
   * When leaf nodes are polydata, this uses a vtkAppendPolyData to merge them.
   */
  virtual void AppendPolyData(vtkInformationVector* inputVector, int i, int numInputs,
    vtkCompositeDataIterator* iter, vtkCompositeDataSet* output);

  /**
   * Both AppendUnstructuredGrids and AppendPolyData call AppendFieldDataArrays. If
   * AppendFieldData is non-zero, then field data arrays from all the inputs are added
   * to the output. If there are duplicates, the array on the first input encountered
   * is taken.
   */
  virtual void AppendFieldDataArrays(vtkInformationVector* inputVector, int i, int numInputs,
    vtkCompositeDataIterator* iter, vtkDataSet* dset);

  vtkTypeBool AppendFieldData;

private:
  vtkAppendCompositeDataLeaves(const vtkAppendCompositeDataLeaves&) = delete;
  void operator=(const vtkAppendCompositeDataLeaves&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkAppendCompositeDataLeaves_h
