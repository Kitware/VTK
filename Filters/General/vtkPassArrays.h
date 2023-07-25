// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov
/**
 * @class   vtkPassArrays
 * @brief   Passes a subset of arrays to the output
 *
 *
 * This filter preserves all the topology of the input, but only a subset of
 * arrays are passed to the output. Add an array to be passed to the output
 * data object with AddArray(). If RemoveArrays is on, the specified arrays will
 * be the ones that are removed instead of the ones that are kept.
 *
 * Arrays with special attributes (scalars, pedigree ids, etc.) will retain those
 * attributes in the output.
 *
 * By default, only those field types with at least one array specified through
 * AddArray will be processed. If instead UseFieldTypes
 * is turned on, you explicitly set which field types to process with AddFieldType.
 *
 * By default, ghost arrays will be passed unless RemoveArrays is selected
 * and those arrays are specifically chosen to be removed.
 *
 * Example 1:
 *
 * <pre>
 * passArray->AddArray(vtkDataObject::POINT, "velocity");
 * </pre>
 *
 * The output will have only that one array "velocity" in the
 * point data, but cell and field data will be untouched.
 *
 * Example 2:
 *
 * <pre>
 * passArray->AddArray(vtkDataObject::POINT, "velocity");
 * passArray->UseFieldTypesOn();
 * passArray->AddFieldType(vtkDataObject::POINT);
 * passArray->AddFieldType(vtkDataObject::CELL);
 * </pre>
 *
 * The point data would still contain the single array, but the cell data
 * would be cleared since you did not specify any arrays to pass. Field data would
 * still be untouched.
 *
 * @section Note
 *
 * vtkPassArrays has been replaced by `vtkPassSelectedArrays`. It is recommended
 * that newer code uses `vtkPassSelectedArrays` instead of this filter.
 * `vtkPassSelectedArrays` uses `vtkDataArraySelection` to select arrays and
 * hence provides a more typical API. `vtkPassArrays` may be deprecated in
 * future releases.
 */

#ifndef vtkPassArrays_h
#define vtkPassArrays_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkPassArrays : public vtkDataObjectAlgorithm
{
public:
  static vtkPassArrays* New();
  vtkTypeMacro(vtkPassArrays, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Adds an array to pass through.
   * fieldType where the array that should be passed (point data, cell data, etc.).
   * It should be one of the constants defined in the vtkDataObject::AttributeTypes
   * enumeration.
   */
  virtual void AddArray(int fieldType, const char* name);

  virtual void AddPointDataArray(const char* name);
  virtual void AddCellDataArray(const char* name);
  virtual void AddFieldDataArray(const char* name);

  virtual void RemoveArray(int fieldType, const char* name);

  virtual void RemovePointDataArray(const char* name);
  virtual void RemoveCellDataArray(const char* name);
  virtual void RemoveFieldDataArray(const char* name);

  ///@{
  /**
   * Clear all arrays to pass through.
   */
  virtual void ClearArrays();
  virtual void ClearPointDataArrays();
  virtual void ClearCellDataArrays();
  virtual void ClearFieldDataArrays();
  ///@}

  ///@{
  /**
   * Instead of passing only the specified arrays, remove the specified arrays
   * and keep all other arrays. Default is off.
   */
  vtkSetMacro(RemoveArrays, bool);
  vtkGetMacro(RemoveArrays, bool);
  vtkBooleanMacro(RemoveArrays, bool);
  ///@}

  ///@{
  /**
   * Process only those field types explicitly specified with AddFieldType.
   * Otherwise, processes field types associated with at least one specified
   * array. Default is off.
   */
  vtkSetMacro(UseFieldTypes, bool);
  vtkGetMacro(UseFieldTypes, bool);
  vtkBooleanMacro(UseFieldTypes, bool);
  ///@}

  /**
   * Add a field type to process.
   * fieldType where the array that should be passed (point data, cell data, etc.).
   * It should be one of the constants defined in the vtkDataObject::AttributeTypes
   * enumeration.
   * NOTE: These are only used if UseFieldType is turned on.
   */
  virtual void AddFieldType(int fieldType);

  /**
   * Clear all field types to process.
   */
  virtual void ClearFieldTypes();

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

protected:
  vtkPassArrays();
  ~vtkPassArrays() override;

  /**
   * Override to limit types of supported input types to non-composite
   * datasets
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Creates the same output type as the input type.
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool RemoveArrays;
  bool UseFieldTypes;

  class Internals;
  Internals* Implementation;

private:
  vtkPassArrays(const vtkPassArrays&) = delete;
  void operator=(const vtkPassArrays&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
