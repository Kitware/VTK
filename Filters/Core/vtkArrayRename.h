// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkArrayRename
 * @brief   Rename data arrays.
 *
 *  This class takes any vtkDataObject as input, shallow copies its content to
 *  the output and renames its data arrays.
 *
 *  For each attributes type, array name should be unique.
 *
 *  Supported attributes type are the following: POINT, CELL, FIELD, VERTEX, EDGE and ROW.
 */

#ifndef vtkArrayRename_h
#define vtkArrayRename_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include "vtkDataObject.h" // for AttributeTypes enum

#include <map>    // for std::map
#include <string> // for std::string

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkArrayRename : public vtkPassInputTypeAlgorithm
{
public:
  static vtkArrayRename* New();
  vtkTypeMacro(vtkArrayRename, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set / Get array name mapping for specified attribute type.
   */
  ///@{
  /**
   * Get number of arrays for specified attribute type.
   */
  int GetNumberOfArrays(int attributeType);
  /**
   * Get the input array name from \p idx in \p attributeType field data.
   */
  const char* GetArrayOriginalName(int attributeType, int idx);
  /**
   * Get the new array name from \p idx in \p attributeType field data.
   */
  const char* GetArrayNewName(int attributeType, int idx);
  /**
   * Set the new array name from \p idx in \p attributeType field data.
   */
  void SetArrayName(int attributeType, int idx, const char* newName);
  /**
   * Set the new array name from \p inputName in \p attributeType field data.
   */
  void SetArrayName(int attributeType, const char* inputName, const char* newName);
  /**
   * Clear mapping for the specified attribute type.
   */
  void ClearMapping(int attributeType);
  ///@}

  /**
   * Get / Set array name mapping for PointData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfPointArrays() { return this->GetNumberOfArrays(vtkDataObject::POINT); }
  const char* GetPointArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::POINT, idx);
  }
  const char* GetPointArrayNewName(int idx)
  {
    return this->GetArrayNewName(vtkDataObject::POINT, idx);
  }
  void SetPointArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::POINT, idx, newName);
  }
  void SetPointArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::POINT, inputName, newName);
  }
  void ClearPointMapping() { this->ClearMapping(vtkDataObject::POINT); }
  ///@}

  /**
   * Get / Set array name mapping for CellData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfCellArrays() { return this->GetNumberOfArrays(vtkDataObject::CELL); }
  const char* GetCellArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::CELL, idx);
  }
  const char* GetCellArrayNewName(int idx)
  {
    return this->GetArrayNewName(vtkDataObject::CELL, idx);
  }
  void SetCellArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::CELL, idx, newName);
  }
  void SetCellArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::CELL, inputName, newName);
  }
  void ClearCellMapping() { this->ClearMapping(vtkDataObject::CELL); }
  ///@}

  /**
   * Get / Set array name mapping for FieldData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfFieldArrays() { return this->GetNumberOfArrays(vtkDataObject::FIELD); }
  const char* GetFieldArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::FIELD, idx);
  }
  const char* GetFieldArrayNewName(int idx)
  {
    return this->GetArrayNewName(vtkDataObject::FIELD, idx);
  }
  void SetFieldArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::FIELD, idx, newName);
  }
  void SetFieldArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::FIELD, inputName, newName);
  }
  void ClearFieldMapping() { this->ClearMapping(vtkDataObject::FIELD); }
  ///@}

  /**
   * Get / Set array name mapping for VertexData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfVertexArrays() { return this->GetNumberOfArrays(vtkDataObject::VERTEX); }
  const char* GetVertexArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::VERTEX, idx);
  }
  const char* GetVertexArrayNewName(int idx)
  {
    return this->GetArrayNewName(vtkDataObject::VERTEX, idx);
  }
  void SetVertexArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::VERTEX, idx, newName);
  }
  void SetVertexArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::VERTEX, inputName, newName);
  }
  void ClearVertexMapping() { this->ClearMapping(vtkDataObject::VERTEX); }
  ///@}

  /**
   * Get / Set array name mapping for EdgeData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfEdgeArrays() { return this->GetNumberOfArrays(vtkDataObject::EDGE); }
  const char* GetEdgeArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::EDGE, idx);
  }
  const char* GetEdgeArrayNewName(int idx)
  {
    return this->GetArrayNewName(vtkDataObject::EDGE, idx);
  }
  void SetEdgeArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::EDGE, idx, newName);
  }
  void SetEdgeArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::EDGE, inputName, newName);
  }
  void ClearEdgeMapping() { this->ClearMapping(vtkDataObject::EDGE); }
  ///@}

  /**
   * Get / Set array name mapping for RowData.
   * Forwarded to the corresponding generic method:
   * @sa GetNumberOfArrays, GetArrayOriginalName, GetArrayNewName, SetArrayName
   */
  ///@{
  int GetNumberOfRowArrays() { return this->GetNumberOfArrays(vtkDataObject::ROW); }
  const char* GetRowArrayOriginalName(int idx)
  {
    return this->GetArrayOriginalName(vtkDataObject::ROW, idx);
  }
  const char* GetRowArrayNewName(int idx) { return this->GetArrayNewName(vtkDataObject::ROW, idx); }
  void SetRowArrayName(int idx, const char* newName)
  {
    this->SetArrayName(vtkDataObject::ROW, idx, newName);
  }
  void SetRowArrayName(const char* inputName, const char* newName)
  {
    this->SetArrayName(vtkDataObject::ROW, inputName, newName);
  }
  void ClearRowMapping() { this->ClearMapping(vtkDataObject::ROW); }
  ///@}

  /**
   * Clear name mappings for all attribute types.
   */
  void ClearAll();

protected:
  vtkArrayRename() = default;
  ~vtkArrayRename() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkArrayRename(const vtkArrayRename&) = delete;
  void operator=(const vtkArrayRename&) = delete;

  std::map<int, std::map<std::string, std::string>> ArrayMapping;
};

VTK_ABI_NAMESPACE_END
#endif // vtkArrayRename_h
