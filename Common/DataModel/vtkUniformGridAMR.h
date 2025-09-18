// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * Legacy, empty shell inheriting vtkAMRDataObject
 *
 * @sa vtkAMRDataObject
 */

#ifndef vtkUniformGridAMR_h
#define vtkUniformGridAMR_h

#include "vtkAMRDataObject.h"
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNew.h"                   // for vtkNew

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkUniformGridAMR : public vtkAMRDataObject
{
public:
  static vtkUniformGridAMR* New();
  vtkTypeMacro(vtkUniformGridAMR, vtkAMRDataObject);

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_GRID_AMR; }

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRDataObject::GetData instead")
  static vtkUniformGridAMR* GetData(vtkInformation* info);

  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRDataObject::GetData instead")
  static vtkUniformGridAMR* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkUniformGridAMR();
  ~vtkUniformGridAMR() override;

private:
  vtkUniformGridAMR(const vtkUniformGridAMR&) = delete;
  void operator=(const vtkUniformGridAMR&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
