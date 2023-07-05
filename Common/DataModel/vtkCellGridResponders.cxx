// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridResponders.h"

#include "vtkCellGridQuery.h"
#include "vtkCellGridResponderBase.h"
#include "vtkCellMetadata.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridResponders);

void vtkCellGridResponders::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Responders: (" << this->Responders.size() << ")\n";
  vtkIndent i2 = indent.GetNextIndent();
  vtkIndent i3 = i2.GetNextIndent();
  for (const auto& entry : this->Responders)
  {
    os << i2 << "Query type \"" << entry.first.Data() << "\" (" << entry.second.size() << ")\n";
    for (const auto& inner : entry.second)
    {
      os << i3 << "Cell type \"" << inner.first.Data() << "\" → " << inner.second->GetClassName()
         << "\n";
    }
  }
}

bool vtkCellGridResponders::Query(vtkCellMetadata* cellType, vtkCellGridQuery* query)
{
  // TODO: This currently only searches for direct matches,
  //       but we should search for approximate hits.
  if (!cellType || !query)
  {
    return false;
  }
  auto it = this->Responders.find(query->GetClassName());
  if (it == this->Responders.end())
  {
    return false;
  }
  auto it2 = it->second.find(cellType->GetClassName());
  if (it2 == it->second.end())
  {
    return false;
  }
  if (!it2->second)
  {
    return false;
  }
  bool result = it2->second->EvaluateQuery(query, cellType, this);
  return result;
}

VTK_ABI_NAMESPACE_END
