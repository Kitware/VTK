// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridBoundsQuery
 * @brief   Compute the geometric bounds of a cell-grid.
 *
 * If no cells are present, invalid bounds will be returned
 * (i.e., bds[1] < bds[0] after calling `GetBounds(bds)`).
 */

#ifndef vtkCellGridBoundsQuery_h
#define vtkCellGridBoundsQuery_h

#include "vtkCellGridQuery.h"

#include <array> // For Bounds ivar.

VTK_ABI_NAMESPACE_BEGIN
class vtkBoundingBox;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridBoundsQuery : public vtkCellGridQuery
{
public:
  static vtkCellGridBoundsQuery* New();
  vtkTypeMacro(vtkCellGridBoundsQuery, vtkCellGridQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Initialize() override;
  void GetBounds(double* bds) VTK_SIZEHINT(6);
  void AddBounds(vtkBoundingBox& bbox);

protected:
  vtkCellGridBoundsQuery() = default;
  ~vtkCellGridBoundsQuery() override = default;

  std::array<double, 6> Bounds;

private:
  vtkCellGridBoundsQuery(const vtkCellGridBoundsQuery&) = delete;
  void operator=(const vtkCellGridBoundsQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridBoundsQuery_h
