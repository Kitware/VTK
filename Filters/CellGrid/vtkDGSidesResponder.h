// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGSidesResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkDGSidesResponder_h
#define vtkDGSidesResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridResponder.h"
#include "vtkCellGridSidesQuery.h" // for inheritance

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGSidesResponders;

class VTKFILTERSCELLGRID_EXPORT vtkDGSidesResponder
  : public vtkCellGridResponder<vtkCellGridSidesQuery>
{
public:
  static vtkDGSidesResponder* New();
  vtkTypeMacro(vtkDGSidesResponder, vtkCellGridResponder<vtkCellGridSidesQuery>);

  bool Query(vtkCellGridSidesQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGSidesResponder() = default;
  ~vtkDGSidesResponder() override = default;

private:
  vtkDGSidesResponder(const vtkDGSidesResponder&) = delete;
  void operator=(const vtkDGSidesResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGSidesResponder_h
// VTK-HeaderTest-Exclude: vtkDGSidesResponder.h
