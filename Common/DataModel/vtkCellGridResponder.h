// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridResponder
 * @brief   Respond to a query on one particular type of cell.
 *
 * This is pure virtual base class that all responder types must inherit.
 */

#ifndef vtkCellGridResponder_h
#define vtkCellGridResponder_h

#include "vtkCellGridResponderBase.h"

VTK_ABI_NAMESPACE_BEGIN
template <typename QueryClass>
class VTK_ALWAYS_EXPORT vtkCellGridResponder : public vtkCellGridResponderBase
{
public:
  vtkTypeMacro(vtkCellGridResponder<QueryClass>, vtkCellGridResponderBase);
  void PrintSelf(ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
  }

  bool EvaluateQuery(
    vtkCellGridQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) override
  {
    auto* qq = dynamic_cast<QueryClass*>(query);
    if (qq)
    {
      return this->Query(qq, cellType, caches);
    }
    return false;
  }

  virtual bool Query(
    QueryClass* query, vtkCellMetadata* cellType, vtkCellGridResponders* caches) = 0;

protected:
  vtkCellGridResponder() = default;
  ~vtkCellGridResponder() override = default;

private:
  vtkCellGridResponder(const vtkCellGridResponder&) = delete;
  void operator=(const vtkCellGridResponder&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridResponder_h
// VTK-HeaderTest-Exclude: vtkCellGridResponder.h
