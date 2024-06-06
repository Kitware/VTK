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

bool vtkCellGridResponders::CalculatorForTagSet::Matches(const TagSet& providedTags) const
{
  // We don't care if \a providedTags contains extra key+value data,
  // but \a providedTags *must* have one or more matches for each key
  // in this->MatchingTags.
  for (const auto& tagEntry : this->MatchingTags)
  {
    auto keyIt = providedTags.find(tagEntry.first);
    if (keyIt == providedTags.end())
    {
      return false;
    }
    bool foundMatch = false;
    for (const auto& value : keyIt->second)
    {
      auto valueIt = tagEntry.second.find(value);
      if (valueIt != tagEntry.second.end())
      {
        // We found a match for tagEntry.first in providedTags; skip to the next tagEntry.
        foundMatch = true;
        break;
      }
    }
    if (foundMatch)
    {
      continue;
    }
    // We had a key match but no values matched.
    return false;
  }
  return true;
}

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
  vtkStringToken calculatorType, vtkCellMetadata* cellType, vtkCellAttribute* attrib,
  const TagSet& tags) const
{
  vtkSmartPointer<vtkCellAttributeCalculator> calc;
  auto it = this->CalculatorRegistry.find(calculatorType);
  if (it == this->CalculatorRegistry.end())
  {
    return calc;
  }
  // For now, return the first registered calculator that matches the provided tags.
  // In the future, it may be possible to compute some discriminatory metric
  // to compare all matching CalculatorForTagSet objects.
  for (const auto& entry : it->second)
  {
    if (entry.Matches(tags))
    {
      calc = entry.CalculatorPrototype;
      break;
    }
  }
  if (calc)
  {
    calc = calc->Prepare<vtkCellAttributeCalculator>(cellType, attrib);
  }
  return calc;
}

vtkSmartPointer<vtkObject> vtkCellGridResponders::GetCacheData(std::size_t key)
{
  vtkSmartPointer<vtkObject> value;
  auto it = this->Caches.find(key);
  if (it == this->Caches.end())
  {
    return value;
  }
  return it->second;
}

bool vtkCellGridResponders::SetCacheData(
  std::size_t key, vtkSmartPointer<vtkObject> value, bool overwrite)
{
  auto it = this->Caches.find(key);
  if (it != this->Caches.end())
  {
    if (!overwrite)
    {
      return false;
    }
    else if (!value)
    {
      this->Caches.erase(it);
      return true;
    }
  }
  if (!value)
  {
    return false;
  }
  this->Caches[key] = value;
  return true;
}

VTK_ABI_NAMESPACE_END
