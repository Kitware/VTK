// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGChangeBasisResponder
 * @brief   Apply a change of basis to an attribute of a vtkDGCell.
 *
 * This responder copies existing cells but applies the query's change of basis
 * to the value-array of the specified attribute.
 */

#ifndef vtkDGChangeBasisResponder_h
#define vtkDGChangeBasisResponder_h

#include "vtkCellGridChangeBasis.h" // for inheritance
#include "vtkCellGridResponder.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDGCell;
class vtkDataArray;

class VTKFILTERSCELLGRID_EXPORT vtkDGChangeBasisResponder
  : public vtkCellGridResponder<vtkCellGridChangeBasis::Query>
{
public:
  static vtkDGChangeBasisResponder* New();
  vtkTypeMacro(vtkDGChangeBasisResponder, vtkCellGridResponder<vtkCellGridChangeBasis::Query>);

  bool Query(vtkCellGridChangeBasis::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGChangeBasisResponder() = default;
  ~vtkDGChangeBasisResponder() override = default;

  bool ChangeBasis(
    vtkCellGridChangeBasis::Query* query, vtkDGCell* cellType, vtkCellGridResponders* caches);

private:
  vtkDGChangeBasisResponder(const vtkDGChangeBasisResponder&) = delete;
  void operator=(const vtkDGChangeBasisResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGChangeBasisResponder_h
// VTK-HeaderTest-Exclude: vtkDGChangeBasisResponder.h
