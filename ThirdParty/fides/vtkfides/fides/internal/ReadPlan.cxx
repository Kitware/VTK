//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/internal/ReadPlan.h>

#include <fides/DataSource.h>
#include <fides/Keys.h>

#include <stdexcept>

namespace fides
{
namespace datamodel
{

namespace
{

template <typename T>
void HashCombine(size_t& seed, const T& v)
{
  seed ^= std::hash<T>{}(v) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

/// Reconstruct a \c MetaData suitable for one \c ReadVariable call from
/// a \c ReadRequest + the original snapshot. Anything in the snapshot
/// that isn't a selection key (e.g. fusion-specific keys) flows through
/// unchanged; the per-key fields override or remove from the snapshot.
fides::metadata::MetaData BuildSelections(const ReadRequest& req,
                                          const fides::metadata::MetaData& snapshot)
{
  fides::metadata::MetaData out = snapshot;

  if (req.Selection.HasBlockSelection)
  {
    fides::metadata::Vector<size_t> v;
    v.Data = req.Selection.BlockSelection;
    out.Set(fides::keys::BLOCK_SELECTION(), v);
  }
  else
  {
    out.Remove(fides::keys::BLOCK_SELECTION());
  }

  if (req.Selection.HasStepSelection)
  {
    out.Set(fides::keys::STEP_SELECTION(), fides::metadata::Index(req.Selection.StepSelection));
  }
  else
  {
    out.Remove(fides::keys::STEP_SELECTION());
  }

  if (req.Selection.HasGroupSelection)
  {
    out.Set(fides::keys::GROUP_SELECTION(), fides::metadata::String(req.Selection.GroupSelection));
  }
  else
  {
    out.Remove(fides::keys::GROUP_SELECTION());
  }

  if (req.MultiBlock)
  {
    out.Set(fides::keys::READ_AS_MULTIBLOCK(), fides::metadata::Bool(true));
  }
  else
  {
    out.Remove(fides::keys::READ_AS_MULTIBLOCK());
  }

  return out;
}

}

SelectionKey SelectionKey::From(const fides::metadata::MetaData& selections)
{
  SelectionKey k;
  if (selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    k.BlockSelection =
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data;
    k.HasBlockSelection = true;
  }
  if (selections.Has(fides::keys::STEP_SELECTION()))
  {
    k.StepSelection = selections.Get<fides::metadata::Index>(fides::keys::STEP_SELECTION()).Data;
    k.HasStepSelection = true;
  }
  if (selections.Has(fides::keys::GROUP_SELECTION()))
  {
    k.GroupSelection = selections.Get<fides::metadata::String>(fides::keys::GROUP_SELECTION()).Data;
    k.HasGroupSelection = true;
  }
  return k;
}

bool SelectionKey::operator==(const SelectionKey& o) const
{
  return this->HasBlockSelection == o.HasBlockSelection &&
    this->BlockSelection == o.BlockSelection && this->HasStepSelection == o.HasStepSelection &&
    this->StepSelection == o.StepSelection && this->HasGroupSelection == o.HasGroupSelection &&
    this->GroupSelection == o.GroupSelection;
}

bool ReadRequest::operator==(const ReadRequest& o) const
{
  return this->DataSourceName == o.DataSourceName && this->VariableName == o.VariableName &&
    this->IsVector == o.IsVector && this->MultiBlock == o.MultiBlock &&
    this->Selection == o.Selection;
}

size_t ReadRequestHash::operator()(const ReadRequest& req) const
{
  size_t seed = 0;
  HashCombine(seed, req.DataSourceName);
  HashCombine(seed, req.VariableName);
  HashCombine(seed, static_cast<int>(req.IsVector));
  HashCombine(seed, req.MultiBlock);
  HashCombine(seed, req.Selection.HasBlockSelection);
  for (size_t b : req.Selection.BlockSelection)
  {
    HashCombine(seed, b);
  }
  HashCombine(seed, req.Selection.HasStepSelection);
  HashCombine(seed, req.Selection.StepSelection);
  HashCombine(seed, req.Selection.HasGroupSelection);
  HashCombine(seed, req.Selection.GroupSelection);
  return seed;
}

ReadResultMap ReadPlan::Execute(const std::vector<ReadRequest>& requests,
                                const std::unordered_map<std::string, std::string>& paths,
                                DataSourcesType& sources,
                                const fides::metadata::MetaData& selections)
{
  ReadResultMap results;
  for (const auto& req : requests)
  {
    if (results.count(req))
    {
      continue;
    }
    auto it = sources.find(req.DataSourceName);
    if (it == sources.end() || !it->second)
    {
      throw std::runtime_error("ReadPlan: unknown data source '" + req.DataSourceName + "'.");
    }
    auto& ds = *it->second;
    ds.OpenSource(paths, req.DataSourceName);

    auto perRequestSelections = BuildSelections(req, selections);

    if (req.MultiBlock)
    {
      results[req] = ds.ReadMultiBlockVariable(req.VariableName, perRequestSelections);
    }
    else
    {
      results[req] = ds.ReadVariable(req.VariableName, perRequestSelections, req.IsVector);
    }
  }
  return results;
}

}
}
