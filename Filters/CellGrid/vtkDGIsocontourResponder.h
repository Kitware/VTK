// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGIsocontourResponder
 * @brief   Find the vtkDGCell cells that may hold an isocontour.
 *
 * This responder answers vtkCellGridIsocontour::Query for one cell type: it
 * reports the cells whose Bernstein-Bezier coefficients of the scalar
 * cell-attribute are not all on one side of the isovalue. The field lies within
 * the convex hull of its Bezier coefficients, so the other cells cannot hold the
 * isocontour. A cell type whose attribute is not in the Bezier basis is left out.
 */

#ifndef vtkDGIsocontourResponder_h
#define vtkDGIsocontourResponder_h

#include "vtkCellGridIsocontour.h" // For inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;

class VTKFILTERSCELLGRID_EXPORT vtkDGIsocontourResponder
  : public vtkCellGridResponder<vtkCellGridIsocontour::Query>
{
public:
  static vtkDGIsocontourResponder* New();
  vtkTypeMacro(vtkDGIsocontourResponder, vtkCellGridResponder<vtkCellGridIsocontour::Query>);

  bool Query(vtkCellGridIsocontour::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGIsocontourResponder() = default;
  ~vtkDGIsocontourResponder() override = default;

private:
  vtkDGIsocontourResponder(const vtkDGIsocontourResponder&) = delete;
  void operator=(const vtkDGIsocontourResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGIsocontourResponder_h
