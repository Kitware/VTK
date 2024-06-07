// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGCellSourceResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkDGCellSourceResponder_h
#define vtkDGCellSourceResponder_h

#include "vtkFiltersCellGridModule.h" // for export macro

#include "vtkCellGridCellSource.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellMetadata;
class vtkDGCell;

class VTKFILTERSCELLGRID_EXPORT vtkDGCellSourceResponder
  : public vtkCellGridResponder<vtkCellGridCellSource::Query>
{
public:
  static vtkDGCellSourceResponder* New();
  vtkTypeMacro(vtkDGCellSourceResponder, vtkCellGridResponder<vtkCellGridCellSource::Query>);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  bool Query(vtkCellGridCellSource::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGCellSourceResponder() = default;
  ~vtkDGCellSourceResponder() override = default;

  void CreateCellAttribute(vtkDGCell* dgCell, vtkStringToken cellTypeToken,
    const std::string& fieldName, vtkStringToken space, int numberOfComponents,
    vtkStringToken functionSpace, vtkStringToken basis, int order, vtkIdType numberOfValues,
    int basisSize = 1, vtkStringToken dofSharing = vtkStringToken{});

private:
  vtkDGCellSourceResponder(const vtkDGCellSourceResponder&) = delete;
  void operator=(const vtkDGCellSourceResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGCellSourceResponder_h
// VTK-HeaderTest-Exclude: vtkDGCellSourceResponder.h
