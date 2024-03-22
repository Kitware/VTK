// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGRangeResponder
 * @brief   Compute the range of a cell-attribute over any vtkDGCell.
 *
 * This simply computes the range of the underlying scalar arrays;
 * it does not attempt to account for true minima/maximum induced
 * by higher-order curvature.
 *
 * This responder also makes the assumption that the value for the
 * attribute at each collocation point is exactly the product of a
 * single basis function (usually 1.0) and its associated coefficient.
 * This means that when one basis function takes on the value 1.0
 * the other basis functions are zero.
 *
 * Finally, for H(curl) and H(div) function spaces, this responder
 * will use an attribute-calculator to compute values and mid-edge
 * and mid-face points (respectively) rather than obtaining bounds
 * directly from basis coefficients; this is because these function
 * spaces also introduce a dependency on the inverse of the shape
 * function gradient.
 *
 * Note that the computed ranges currently include all cells
 * even if they are blanked and only sides are "visible."
 */

#ifndef vtkDGRangeResponder_h
#define vtkDGRangeResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridRangeQuery.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDGCell;
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
  template <bool DOFSharing, bool FiniteRange>
  bool ConstantRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
    const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
    vtkCellGridRangeQuery* request);

  template <bool DOFSharing, bool FiniteRange>
  bool HGradRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
    const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
    vtkCellGridRangeQuery* request);

  template <bool DOFSharing, bool FiniteRange>
  bool HCurlRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
    const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
    vtkCellGridRangeQuery* request);

  template <bool DOFSharing, bool FiniteRange>
  bool HDivRange(vtkDGCell* dgCell, vtkCellAttribute* attribute,
    const vtkCellAttribute::CellTypeInfo& cellTypeInfo, vtkDataArray* values,
    vtkCellGridRangeQuery* request);

#if 0
  /// Update the range of an H(curl) attribute.
  template<bool FiniteRange, bool DOFSharing>
  bool HCurl(
    vtkCellAttribute* attribute,
    vtkDataArray* conn,
    vtkDataArray* values,
    vtkCellGridRangeQuery* request);

  // Update the range of a discontinuous Lagrange-polynomial attribute.
  template <bool FiniteRange>
  bool DiscontinuousLagrange(
    vtkCellAttribute* attribute, vtkDataArray* values, vtkCellGridRangeQuery* request);

  // Update the range of a continuous Lagrange-polynomial attribute.
  template <bool FiniteRange>
  bool ContinuousLagrange(vtkCellAttribute* attribute, vtkDataArray* conn,
    vtkDataArray* values, vtkCellGridRangeQuery* request);
#endif

  vtkDGRangeResponder(const vtkDGRangeResponder&) = delete;
  void operator=(const vtkDGRangeResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGRangeResponder_h
// VTK-HeaderTest-Exclude: vtkDGRangeResponder.h
