// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGElevationResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkDGElevationResponder_h
#define vtkDGElevationResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridElevationQuery.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;

class VTKFILTERSCELLGRID_EXPORT vtkDGElevationResponder
  : public vtkCellGridResponder<vtkCellGridElevationQuery>
{
public:
  static vtkDGElevationResponder* New();
  vtkTypeMacro(vtkDGElevationResponder, vtkCellGridResponder<vtkCellGridElevationQuery>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Query(vtkCellGridElevationQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGElevationResponder() = default;
  ~vtkDGElevationResponder() override = default;

private:
  vtkDGElevationResponder(const vtkDGElevationResponder&) = delete;
  void operator=(const vtkDGElevationResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGElevationResponder_h
// VTK-HeaderTest-Exclude: vtkDGElevationResponder.h
