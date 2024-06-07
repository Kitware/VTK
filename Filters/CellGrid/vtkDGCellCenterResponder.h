// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGCellCenterResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkDGCellCenterResponder_h
#define vtkDGCellCenterResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridCellCenters.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGCellCenterResponder
  : public vtkCellGridResponder<vtkCellGridCellCenters::Query>
{
public:
  static vtkDGCellCenterResponder* New();
  vtkTypeMacro(vtkDGCellCenterResponder, vtkCellGridResponder<vtkCellGridCellCenters::Query>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Query(vtkCellGridCellCenters::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGCellCenterResponder() = default;
  ~vtkDGCellCenterResponder() override = default;

  void AllocateOutputVertices(vtkCellGridCellCenters::Query* request);
  void AddCellCenters(vtkCellGridCellCenters::Query* request, vtkDGCell* cellType);
  void GenerateOutputVertices(vtkCellGridCellCenters::Query* request, vtkDGCell* cellType);

private:
  vtkDGCellCenterResponder(const vtkDGCellCenterResponder&) = delete;
  void operator=(const vtkDGCellCenterResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGCellCenterResponder_h
// VTK-HeaderTest-Exclude: vtkDGCellCenterResponder.h
