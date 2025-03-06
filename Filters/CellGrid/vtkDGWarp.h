// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGWarp
 * @brief   Respond to a "warp" query on one particular type of cell.
 *
 * Deform the shape attribute by another (vector-valued) attribute.
 */

#ifndef vtkDGWarp_h
#define vtkDGWarp_h

#include "vtkCellGridResponder.h"
#include "vtkCellGridWarp.h" // for inheritance

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;

class VTKFILTERSCELLGRID_EXPORT vtkDGWarp : public vtkCellGridResponder<vtkCellGridWarp::Query>
{
public:
  static vtkDGWarp* New();
  vtkTypeMacro(vtkDGWarp, vtkCellGridResponder<vtkCellGridWarp::Query>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Query(vtkCellGridWarp::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGWarp() = default;
  ~vtkDGWarp() override = default;

private:
  vtkDGWarp(const vtkDGWarp&) = delete;
  void operator=(const vtkDGWarp&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGWarp_h
// VTK-HeaderTest-Exclude: vtkDGWarp.h
