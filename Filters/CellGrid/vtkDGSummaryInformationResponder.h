// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGSummaryInformationResponder
 * @brief   Respond to vtkCellGridSummaryInformationQuery for vtkDGCell types.
 *
 * For each DG cell type in the grid this responder iterates over every
 * vtkCellAttribute and computes both pieces of summary information in a
 * single pass:
 * - The polynomial interpolation order (vtkCellAttribute::CellTypeInfo::Order)
 *   is fed into the query's order-range accumulator.
 * - The degree-of-freedom (DOF) count is computed from the connectivity array
 *   (shared DOFs) or the values array (discontinuous DOFs) and accumulated
 *   per attribute.
 */

#ifndef vtkDGSummaryInformationResponder_h
#define vtkDGSummaryInformationResponder_h

#include "vtkCellGridResponder.h"
#include "vtkCellGridSummaryInformationQuery.h"
#include "vtkFiltersCellGridModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;

class VTKFILTERSCELLGRID_EXPORT vtkDGSummaryInformationResponder
  : public vtkCellGridResponder<vtkCellGridSummaryInformationQuery>
{
public:
  static vtkDGSummaryInformationResponder* New();
  vtkTypeMacro(
    vtkDGSummaryInformationResponder, vtkCellGridResponder<vtkCellGridSummaryInformationQuery>);

  bool Query(vtkCellGridSummaryInformationQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGSummaryInformationResponder() = default;
  ~vtkDGSummaryInformationResponder() override = default;

private:
  vtkDGSummaryInformationResponder(const vtkDGSummaryInformationResponder&) = delete;
  void operator=(const vtkDGSummaryInformationResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGSummaryInformationResponder_h
// VTK-HeaderTest-Exclude: vtkDGSummaryInformationResponder.h
