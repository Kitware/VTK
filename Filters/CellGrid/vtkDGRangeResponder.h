// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGRangeResponder
 * @brief   Compute the range of a cell-attribute over any vtkDGCell.
 *
 * This simply computes the range of the underlying scalar arrays;
 * it does not attempt to account for true minima/maximum induced
 * by higher-order curvature.
 */

#ifndef vtkDGRangeResponder_h
#define vtkDGRangeResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridRangeQuery.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDataArray;
class vtkTypeInt64Array;

class VTKFILTERSCELLGRID_EXPORT vtkDGRangeResponder
  : public vtkCellGridResponder<vtkCellGridRangeQuery>
{
public:
  static vtkDGRangeResponder* New();
  vtkTypeMacro(vtkDGRangeResponder, vtkCellGridResponder<vtkCellGridRangeQuery>);

  bool Query(vtkCellGridRangeQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGRangeResponder() = default;
  ~vtkDGRangeResponder() override = default;

private:
  // Update the range of a discontinuous Lagrange-polynomial attribute.
  template <bool FiniteRange>
  bool DiscontinuousLagrange(
    vtkCellAttribute* attribute, vtkDataArray* values, vtkCellGridRangeQuery* request);

  // Update the range of a continuous Lagrange-polynomial attribute.
  template <bool FiniteRange>
  bool ContinuousLagrange(vtkCellAttribute* attribute, vtkTypeInt64Array* conn,
    vtkDataArray* values, vtkCellGridRangeQuery* request);

  vtkDGRangeResponder(const vtkDGRangeResponder&) = delete;
  void operator=(const vtkDGRangeResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGRangeResponder_h
// VTK-HeaderTest-Exclude: vtkDGRangeResponder.h
