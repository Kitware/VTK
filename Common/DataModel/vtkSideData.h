// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSideData
 * @brief   represent and manipulate side attribute data
 *
 * vtkSideData holds attribute data on the sides shared between cells: the
 * faces of 3D cells and the edges of 2D cells, indexed by side id. It is the
 * peer of vtkPointData and vtkCellData for the vtkDataObject::SIDE attribute
 * type and the vtkDataObject::FIELD_ASSOCIATION_SIDES field association.
 *
 * @sa
 * vtkUnstructuredGrid vtkUnstructuredGridSideMap
 */

#ifndef vtkSideData_h
#define vtkSideData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSetAttributes.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkSideData : public vtkDataSetAttributes
{
public:
  static vtkSideData* New();
  static vtkSideData* ExtendedNew();

  vtkTypeMacro(vtkSideData, vtkDataSetAttributes);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSideData() = default;
  ~vtkSideData() override = default;

private:
  vtkSideData(const vtkSideData&) = delete;
  void operator=(const vtkSideData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
