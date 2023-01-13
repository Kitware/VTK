// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridResponders.h"

#include "vtkCellAttribute.h"
#include "vtkCellGridQuery.h"
#include "vtkCellGridResponderBase.h"
#include "vtkCellMetadata.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

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
  bool didFind = false;
  std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellGridResponderBase>>::const_iterator it2;
  for (const auto& cellTypeToken : cellType->InheritanceHierarchy())
  {
    if (cellTypeToken == "vtkObject"_token)
    {
      break;
    }
    it2 = it->second.find(cellTypeToken);
    if (it2 == it->second.end())
    {
      continue;
    }
    if (!it2->second)
    {
      continue;
    }
    didFind = true;
    break;
  }
  if (!didFind)
  {
    vtkIndent indent;
    this->PrintSelf(std::cout, indent);
    vtkErrorMacro("No responder for " << query->GetClassName() << " for "
                                      << cellType->GetClassName() << " found.");
    return false;
  }

  bool result = it2->second->EvaluateQuery(query, cellType, this);
  return result;
}

vtkSmartPointer<vtkCellAttributeCalculator> vtkCellGridResponders::AttributeCalculator(
  vtkStringToken calculatorType, vtkCellMetadata* cellType, vtkCellAttribute* cellAttribute) const
{
  if (!cellType || !cellAttribute)
  {
    vtkErrorMacro("Null cell metadata or attribute.");
    return nullptr;
  }
  auto cit = this->Calculators.find(calculatorType);
  if (cit == this->Calculators.end())
  {
    vtkErrorMacro("No such calculator type " << calculatorType.Data() << ".");
    return nullptr;
  }
  std::unordered_map<vtkStringToken,
    std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellAttributeCalculator>>>::const_iterator
    mit;
  std::unordered_map<vtkStringToken, vtkSmartPointer<vtkCellAttributeCalculator>>::const_iterator
    ait;
  bool didFind = false;
  vtkStringToken tags = cellAttribute->GetAttributeType();
  for (const auto& cellTypeToken : cellType->InheritanceHierarchy())
  {
    if (cellTypeToken == "vtkObject"_token)
    {
      break;
    }
    mit = cit->second.find(cellTypeToken);
    if (mit == cit->second.end())
    {
      continue;
    }
    ait = mit->second.find(tags);
    if (ait == mit->second.end() || !ait->second)
    {
      continue;
    }
    didFind = true;
    break;
  }
  if (!didFind)
  {
    vtkErrorMacro("No calculator support for " << cellType->GetClassName() << " cells and "
                                               << cellAttribute->GetAttributeType().Data() << ".");
    return nullptr;
  }
  return ait->second->Prepare<vtkCellAttributeCalculator>(cellType, cellAttribute);
}

VTK_ABI_NAMESPACE_END
