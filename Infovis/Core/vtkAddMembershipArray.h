// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkAddMembershipArray
 * @brief   Add an array to the output indicating
 * membership within an input selection.
 *
 *
 * This filter takes an input selection, vtkDataSetAttribute
 * information, and data object and adds a bit array to the output
 * vtkDataSetAttributes indicating whether each index was selected or not.
 */

#ifndef vtkAddMembershipArray_h
#define vtkAddMembershipArray_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;

class VTKINFOVISCORE_EXPORT vtkAddMembershipArray : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAddMembershipArray* New();
  vtkTypeMacro(vtkAddMembershipArray, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5
  };

  ///@{
  /**
   * The field type to add the membership array to.
   */
  vtkGetMacro(FieldType, int);
  vtkSetClampMacro(FieldType, int, 0, 5);
  ///@}

  ///@{
  /**
   * The name of the array added to the output vtkDataSetAttributes
   * indicating membership. Defaults to "membership".
   */
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);
  ///@}

  vtkSetStringMacro(InputArrayName);
  vtkGetStringMacro(InputArrayName);

  void SetInputValues(vtkAbstractArray*);
  vtkGetObjectMacro(InputValues, vtkAbstractArray);

protected:
  vtkAddMembershipArray();
  ~vtkAddMembershipArray() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FieldType;
  char* OutputArrayName;
  char* InputArrayName;

  vtkAbstractArray* InputValues;

private:
  vtkAddMembershipArray(const vtkAddMembershipArray&) = delete;
  void operator=(const vtkAddMembershipArray&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
