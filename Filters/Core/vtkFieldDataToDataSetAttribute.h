// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFieldDataToDataSetAttribute
 * @brief   map field data to other attribute data
 *
 * vtkFieldDataToDataSetAttribute is a filter that copies field data arrays into
 * another attribute data arrays.
 *
 * This is done at very low memory cost by using the Implicit Array infrastructure.
 *
 * NOTE: It copies only the first component of the first tuple into a vtkConstantArray.
 * vtkStringArray are not supported.
 *
 * @sa
 * vtkFieldData vtkCellData
 */

#ifndef vtkFieldDataToDataSetAttribute_h
#define vtkFieldDataToDataSetAttribute_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include "vtkDataObject.h" // for vtkDataObject::AttributeType enum

#include <set>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkFieldDataToDataSetAttribute : public vtkPassInputTypeAlgorithm
{
public:
  static vtkFieldDataToDataSetAttribute* New();
  vtkTypeMacro(vtkFieldDataToDataSetAttribute, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Activate whether to process all input arrays or only the selected ones.
   * If false, only arrays selected by the user will be considered by this filter.
   * The default is true.
   */
  vtkSetMacro(ProcessAllArrays, bool);
  vtkGetMacro(ProcessAllArrays, bool);
  vtkBooleanMacro(ProcessAllArrays, bool);
  ///@}

  ///@{
  /**
   * Set/Get the output attribute type.
   */
  vtkSetMacro(OutputFieldType, int);
  vtkGetMacro(OutputFieldType, int);
  ///@}

  /**
   * Arrays to be processed.
   */
  ///@{
  /**
   * Adds an array to be processed.
   * This only has an effect if ProcessAllArrays is off.
   * If the name is already present, nothing happens.
   */
  virtual void AddFieldDataArray(const char* name);

  /**
   * Removes an array to be processed.
   * This only has an effect if ProcessAllArrays is off.
   * If the name is not present, nothing happens.
   */
  virtual void RemoveFieldDataArray(const char* name);

  /**
   * Removes all arrays to be processed from the list.
   * This only has an effect if ProcessAllArrays is off.
   */
  virtual void ClearFieldDataArrays();

  /**
   * Get the names of the arrays to process.
   */
  virtual const std::set<std::string>& GetFieldDataArrays();
  ///@}

protected:
  vtkFieldDataToDataSetAttribute() = default;
  ~vtkFieldDataToDataSetAttribute() override = default;

  /**
   * Reimplemented to remove Composite support. This filter
   * relies on the Executive and handles composite block per block.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Reimplemented create data arrays as required.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkFieldDataToDataSetAttribute(const vtkFieldDataToDataSetAttribute&) = delete;
  void operator=(const vtkFieldDataToDataSetAttribute&) = delete;

  bool ProcessAllArrays = true;

  /// see vtkDataObject::AttributeTypes
  int OutputFieldType = vtkDataObject::POINT;

  std::set<std::string> FieldDataArrays;
};

VTK_ABI_NAMESPACE_END
#endif
