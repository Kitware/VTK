// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPassSelectedArrays
 * @brief pass through chosen arrays
 *
 * vtkPassSelectedArrays can be used to pass through chosen arrays. It is
 * intended as a replacement for vtkPassArrays filter with a more standard API
 * that uses `vtkDataArraySelection` to choose arrays to pass through.
 *
 * To enable/disable arrays to pass, get the appropriate `vtkDataArraySelection`
 * instance using `GetArraySelection` or the association specific get methods
 * such as `GetPointDataArraySelection`, `GetCellDataArraySelection` etc. and
 * then enable/disable arrays using `vtkDataArraySelection` API. Using
 * vtkDataArraySelection::SetUnknownArraySetting` one also dictate how arrays
 * not explicitly listed are to be handled.
 *
 */

#ifndef vtkPassSelectedArrays_h
#define vtkPassSelectedArrays_h

#include "vtkDataObject.h"           // for vtkDataObject::FieldAssociations
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h" // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkDataArraySelection;

class VTKFILTERSGENERAL_EXPORT vtkPassSelectedArrays : public vtkPassInputTypeAlgorithm
{
public:
  static vtkPassSelectedArrays* New();
  vtkTypeMacro(vtkPassSelectedArrays, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable this filter. When disabled, this filter passes all input arrays
   * irrespective of the array selections. Default is `true`.
   */
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  ///@}

  /**
   * Returns the vtkDataArraySelection instance associated with a particular
   * array association type (vtkDataObject::FieldAssociations). Returns nullptr
   * if the association type is invalid others the corresponding
   * vtkDataArraySelection instance is returned.
   */
  vtkDataArraySelection* GetArraySelection(int association);

  ///@{
  /**
   * Convenience methods that call `GetArraySelection` with corresponding
   * association type.
   */
  vtkDataArraySelection* GetPointDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  }
  vtkDataArraySelection* GetCellDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  }
  vtkDataArraySelection* GetFieldDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_NONE);
  }
  vtkDataArraySelection* GetVertexDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_VERTICES);
  }
  vtkDataArraySelection* GetEdgeDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_EDGES);
  }
  vtkDataArraySelection* GetRowDataArraySelection()
  {
    return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_ROWS);
  }
  ///@}

protected:
  vtkPassSelectedArrays();
  ~vtkPassSelectedArrays() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int HandleCellGridAttributes(vtkCellGrid* output);

private:
  vtkPassSelectedArrays(const vtkPassSelectedArrays&) = delete;
  void operator=(const vtkPassSelectedArrays&) = delete;

  bool Enabled;
  vtkSmartPointer<vtkDataArraySelection> ArraySelections[vtkDataObject::NUMBER_OF_ASSOCIATIONS];
};

VTK_ABI_NAMESPACE_END
#endif
