// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGridAMRDataIterator
 * @brief   Deprecated AMR iterator
 *
 * This class is deprecated, please use vtkUniformGridAMRIterator instead.
 * Please note vtkDataObjectTreeIterator inherits vtkDataObjectTreeIterator.
 */

#ifndef vtkUniformGridAMRDataIterator_h
#define vtkUniformGridAMRDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUniformGridAMRIterator.h"

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_6_0(
  "This class is deprecated, please use vtkUniformGridAMRIterator instead and note that it now "
  "inherits vtkDataObjectTreeIterator") VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMRDataIterator
  : public vtkUniformGridAMRIterator
{
public:
  static vtkUniformGridAMRDataIterator* New();
  vtkTypeMacro(vtkUniformGridAMRDataIterator, vtkUniformGridAMRIterator);

protected:
  vtkUniformGridAMRDataIterator();
  ~vtkUniformGridAMRDataIterator() override;

private:
  vtkUniformGridAMRDataIterator(const vtkUniformGridAMRDataIterator&) = delete;
  void operator=(const vtkUniformGridAMRDataIterator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
