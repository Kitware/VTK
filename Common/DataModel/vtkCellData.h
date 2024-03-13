// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellData
 * @brief   represent and manipulate cell attribute data
 *
 * vtkCellData is a class that is used to represent and manipulate
 * cell attribute data (e.g., scalars, vectors, normals, texture
 * coordinates, etc.) Special methods are provided to work with filter
 * objects, such as passing data through filter, copying data from one
 * cell to another, and interpolating data given cell interpolation weights.
 *
 * By default, `GhostTypesToSkip` is set to `DUPLICATECELL | HIDDENCELL | REFINEDCELL`.
 * See `vtkDataSetAttributes` for the definition of those constants.
 */

#ifndef vtkCellData_h
#define vtkCellData_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDataSetAttributes.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkCellData : public vtkDataSetAttributes
{
public:
  static vtkCellData* New();
  static vtkCellData* ExtendedNew();

  vtkTypeMacro(vtkCellData, vtkDataSetAttributes);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCellData(); // make sure constructor and destructor are protected
  ~vtkCellData() override = default;

private:
  vtkCellData(const vtkCellData&) = delete;
  void operator=(const vtkCellData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
