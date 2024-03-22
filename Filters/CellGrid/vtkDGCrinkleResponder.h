// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGCrinkleResponder
 * @brief   Compute the sides on the outside surface of a collection of DG cells.
 *
 */

#ifndef vtkDGCrinkleResponder_h
#define vtkDGCrinkleResponder_h

#include "vtkFiltersCellGridModule.h" // For export macro.

#include "vtkCellGridCrinkleQuery.h" // For inheritance.
#include "vtkCellGridResponder.h"
#include "vtkDGCell.h" // For Shape enum.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGCrinkleResponders;

class VTKFILTERSCELLGRID_EXPORT vtkDGCrinkleResponder
  : public vtkCellGridResponder<vtkCellGridCrinkleQuery>
{
public:
  static vtkDGCrinkleResponder* New();
  vtkTypeMacro(vtkDGCrinkleResponder, vtkCellGridResponder<vtkCellGridCrinkleQuery>);

  bool Query(vtkCellGridCrinkleQuery* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGCrinkleResponder() = default;
  ~vtkDGCrinkleResponder() override = default;

  bool HashCrinkle(vtkCellGridCrinkleQuery* query, vtkDGCell* cellType);
  bool SummarizeCrinkle(vtkCellGridCrinkleQuery* query, vtkDGCell* cellType);
  bool GenerateSideSets(vtkCellGridCrinkleQuery* query, vtkDGCell* cellType);
  static bool ProcessCrinkleOfInput(
    vtkCellGridCrinkleQuery* query, vtkDGCell::Shape sideShape, vtkDGCell::Shape cellShape);

  /// Called by HashCrinkle to recursively hash sides of sides of a cell.
  ///
  /// This is only used when processing entries of vtkDGCell::GetSideSpecs()
  /// (and not when processing vtkDGCell::GetCellSpec()).
  void HashCrinkleOfSide(vtkCellGridCrinkleQuery* query, vtkDGCell* cellType,
    vtkDGCell::Shape sourceShape, std::vector<vtkIdType>& side,
    const std::vector<vtkIdType>& sidesOfSide, vtkIdType cellId,
    const std::vector<vtkTypeInt64>& entry, std::set<int>& sidesVisited, vtkDataArray* ngm);

private:
  vtkDGCrinkleResponder(const vtkDGCrinkleResponder&) = delete;
  void operator=(const vtkDGCrinkleResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGCrinkleResponder_h
// VTK-HeaderTest-Exclude: vtkDGCrinkleResponder.h
