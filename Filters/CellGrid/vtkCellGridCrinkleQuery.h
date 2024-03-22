// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellGridCrinkleQuery_h
#define vtkCellGridCrinkleQuery_h

#include "vtkCellGridSidesQuery.h"
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkImplicitFunction.h"      // For ivar.
#include "vtkNew.h"                   // For ivar.

#include <array>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

/**\brief A query for cells/sides on or to one side of an implicit function.
 *
 * This query expects responders to answer the query in three fixed passes:
 * + In the first pass, inputs are traversed and outputs are accumulated according
 *   to the coincident shapes they produce.
 * + In the second pass, outputs are pruned and reindexed according to cell type.
 * + In the third pass, the output grid is populated.
 *
 */
class VTKFILTERSCELLGRID_EXPORT vtkCellGridCrinkleQuery : public vtkCellGridSidesQuery
{
public:
  static vtkCellGridCrinkleQuery* New();
  vtkTypeMacro(vtkCellGridCrinkleQuery, vtkCellGridSidesQuery);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize() override;
  void Finalize() override;

  /// Set/get the implicit function used.
  vtkSetObjectMacro(Function, vtkImplicitFunction);
  vtkGetObjectMacro(Function, vtkImplicitFunction);

protected:
  vtkCellGridCrinkleQuery() = default;
  ~vtkCellGridCrinkleQuery() override = default;

  vtkImplicitFunction* Function;
  int HalfSpace;

private:
  vtkCellGridCrinkleQuery(const vtkCellGridCrinkleQuery&) = delete;
  void operator=(const vtkCellGridCrinkleQuery&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridCrinkleQuery_h
