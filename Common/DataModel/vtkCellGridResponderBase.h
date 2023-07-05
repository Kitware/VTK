// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridResponderBase
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkCellGridResponderBase_h
#define vtkCellGridResponderBase_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGridQuery;
class vtkCellMetadata;
class vtkCellGridResponders;

class VTKCOMMONDATAMODEL_EXPORT vtkCellGridResponderBase : public vtkObject
{
public:
  vtkTypeMacro(vtkCellGridResponderBase, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  /// Respond to the \a query for cells of \a cellType, possibly using \a caches.
  virtual bool EvaluateQuery(
    vtkCellGridQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) = 0;

protected:
  vtkCellGridResponderBase() = default;
  ~vtkCellGridResponderBase() override = default;

private:
  vtkCellGridResponderBase(const vtkCellGridResponderBase&) = delete;
  void operator=(const vtkCellGridResponderBase&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridResponderBase_h
