// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGSidesResponder
 * @brief   Compute the sides on the outside surface of a collection of DG cells.
 *
 */

#ifndef vtkDGSidesResponder_h
#define vtkDGSidesResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridResponder.h"
#include "vtkCellGridSidesQuery.h" // for inheritance

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGCell;
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

  bool HashSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType);
  bool GenerateSideSets(vtkCellGridSidesQuery* query, vtkDGCell* cellType);

private:
  vtkDGSidesResponder(const vtkDGSidesResponder&) = delete;
  void operator=(const vtkDGSidesResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGSidesResponder_h
// VTK-HeaderTest-Exclude: vtkDGSidesResponder.h
