// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDGTransformResponder
 * @brief   Apply a transform to an attribute of a vtkDGCell.
 *
 * This responder copies existing cells but applies the query's transform
 * to the value-array of the named attribute (or the shape attribute if
 * no attribute is named).
 */

#ifndef vtkDGTransformResponder_h
#define vtkDGTransformResponder_h

#include "vtkCellGridResponder.h"
#include "vtkCellGridTransform.h" // for inheritance

VTK_ABI_NAMESPACE_BEGIN
class vtkCellAttribute;
class vtkCellMetadata;
class vtkDGCell;
class vtkDataArray;

class VTKFILTERSCELLGRID_EXPORT vtkDGTransformResponder
  : public vtkCellGridResponder<vtkCellGridTransform::Query>
{
public:
  static vtkDGTransformResponder* New();
  vtkTypeMacro(vtkDGTransformResponder, vtkCellGridResponder<vtkCellGridTransform::Query>);

  bool Query(vtkCellGridTransform::Query* query, vtkCellMetadata* cellType,
    vtkCellGridResponders* caches) override;

protected:
  vtkDGTransformResponder() = default;
  ~vtkDGTransformResponder() override = default;

private:
  vtkDGTransformResponder(const vtkDGTransformResponder&) = delete;
  void operator=(const vtkDGTransformResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkDGTransformResponder_h
// VTK-HeaderTest-Exclude: vtkDGTransformResponder.h
