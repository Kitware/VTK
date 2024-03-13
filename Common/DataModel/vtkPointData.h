// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointData
 * @brief   represent and manipulate point attribute data
 *
 * vtkPointData is a class that is used to represent and manipulate
 * point attribute data (e.g., scalars, vectors, normals, texture
 * coordinates, etc.) Most of the functionality is handled by
 * vtkDataSetAttributes.
 *
 * By default, `GhostTypesToSkip` is set to `DUPLICATEPOINT | HIDDENPOINT`.
 * See `vtkDataSetAttributes` for the definition of those constants.
 */

#ifndef vtkPointData_h
#define vtkPointData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSetAttributes.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkPointData : public vtkDataSetAttributes
{
public:
  static vtkPointData* New();
  static vtkPointData* ExtendedNew();

  vtkTypeMacro(vtkPointData, vtkDataSetAttributes);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPointData();
  ~vtkPointData() override = default;

private:
  vtkPointData(const vtkPointData&) = delete;
  void operator=(const vtkPointData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
