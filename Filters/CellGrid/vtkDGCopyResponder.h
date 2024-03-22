// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGCopyResponder
 * @brief   Copy data from one vtkCellGrid to another.
 *
 * This responder can perform a shallow copy, a deep copy, and a structure-only copy.
 */

#ifndef vtkDGCopyResponder_h
#define vtkDGCopyResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridCopyQuery.h" // for inheritance
#include "vtkCellGridResponder.h"
#include "vtkDGCell.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;

class VTKFILTERSCELLGRID_EXPORT vtkDGCopyResponder
  : public vtkCellGridResponder<vtkCellGridCopyQuery>
{
public:
  static vtkDGCopyResponder* New();
  vtkTypeMacro(vtkDGCopyResponder, vtkCellGridResponder<vtkCellGridCopyQuery>);

  bool Query(
    vtkCellGridCopyQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) override;

protected:
  vtkDGCopyResponder() = default;
  ~vtkDGCopyResponder() override = default;

  /// This method is used to copy cell connectivity arrays for one cell type.
  ///
  /// It is called from Query().
  void CopySpecs(vtkCellGridCopyQuery* query, vtkDGCell* sourceMetadata, vtkDGCell* targetMetadata);

  /// Copy a single cell/side specification from \a sourceSpec into \a targetSpec.
  ///
  /// This will either copy arrays by reference, by value, or create new arrays of
  /// the same type as directed by the \a query.
  void CopySpec(
    vtkCellGridCopyQuery* query, vtkDGCell::Source& sourceSpec, vtkDGCell::Source& targetSpec);

private:
  vtkDGCopyResponder(const vtkDGCopyResponder&) = delete;
  void operator=(const vtkDGCopyResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGCopyResponder_h
// VTK-HeaderTest-Exclude: vtkDGCopyResponder.h
