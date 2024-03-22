// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGSidesResponder
 * @brief   Compute the sides on the outside surface of a collection of DG cells.
 *
 */

#ifndef vtkDGSidesResponder_h
#define vtkDGSidesResponder_h

#include "vtkFiltersCellGridModule.h" // For export macro.

#include "vtkCellGridResponder.h"
#include "vtkCellGridSidesQuery.h" // For inheritance.
#include "vtkDGCell.h"             // For Shape enum.

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

  bool HashSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType);
  bool SummarizeSides(vtkCellGridSidesQuery* query, vtkDGCell* cellType);
  bool GenerateSideSets(vtkCellGridSidesQuery* query, vtkDGCell* cellType);
  static bool ProcessSidesOfInput(
    vtkCellGridSidesQuery* query, vtkDGCell::Shape sideShape, vtkDGCell::Shape cellShape);

  /// Called by HashSides to recursively hash sides of sides of a cell.
  ///
  /// This is only used when processing entries of vtkDGCell::GetSideSpecs()
  /// (and not when processing vtkDGCell::GetCellSpec()).
  void HashSidesOfSide(vtkCellGridSidesQuery* query, vtkDGCell* cellType,
    vtkDGCell::Shape sourceShape, std::vector<vtkIdType>& side,
    const std::vector<vtkIdType>& sidesOfSide, vtkIdType cellId,
    const std::vector<vtkTypeInt64>& entry, std::set<int>& sidesVisited, vtkDataArray* ngm);

private:
  vtkDGSidesResponder(const vtkDGSidesResponder&) = delete;
  void operator=(const vtkDGSidesResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGSidesResponder_h
// VTK-HeaderTest-Exclude: vtkDGSidesResponder.h
