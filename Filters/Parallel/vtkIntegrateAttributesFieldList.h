// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIntegrateAttributesFieldList
 * @brief   Helper class for vtkIntegrateAttributes
 *
 * Helper class for vtkIntegrateAttributes. Override CreateArray to only
 * forward double arrays.
 */
#ifndef vtkIntegrateAttributesFieldList_h
#define vtkIntegrateAttributesFieldList_h

#include "vtkDataSetAttributes.h"     // For vtkFieldList definition
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class vtkIntegrateAttributesFieldList : public vtkDataSetAttributes::FieldList
{
  typedef vtkDataSetAttributes::FieldList Superclass;

public:
  static vtkIntegrateAttributesFieldList* New();
  vtkIntegrateAttributesFieldList(int numInputs = 0);

private:
  vtkIntegrateAttributesFieldList(const vtkIntegrateAttributesFieldList&) = delete;
  void operator=(const vtkIntegrateAttributesFieldList&) = delete;

protected:
  // overridden to only create vtkDoubleArray for numeric arrays.
  vtkSmartPointer<vtkAbstractArray> CreateArray(int type) const override;
};

VTK_ABI_NAMESPACE_END
#endif
