// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkBlockSelector.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkStringArray.h"

#include <set>
#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkBlockSelector::vtkInternals
{
public:
  // This functor is only needed for vtkArrayDispatch to correctly fill it up.
  // Otherwise, it would simply be a set.
  class CompositeIdsT : public std::set<unsigned int>
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      using T = vtk::GetAPIType<ArrayType>;
      const auto range = vtk::DataArrayValueRange<1>(array);
      std::for_each(range.cbegin(), range.cend(),
        [&](const T val) { this->insert(static_cast<unsigned int>(val)); });
    }
  };

  // This functor is only needed for vtkArrayDispatch to correctly fill it up.
  // otherwise, it'd simply be a set.
  class AMRIdsT : public std::set<std::pair<unsigned int, unsigned int>>
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      const auto tuples = vtk::DataArrayTupleRange<2>(array);
      for (const auto tuple : tuples)
      {
        this->insert(
          std::make_pair(static_cast<unsigned int>(tuple[0]), static_cast<unsigned int>(tuple[1])));
      }
    }
  };

  CompositeIdsT CompositeIds;
  AMRIdsT AMRIds;

  // note: here `selectors` are path-queries used by vtkDataAssembly and **not**
  // `vtkSelector`.
  std::vector<std::string> Selectors;
  std::string AssemblyName = vtkDataAssemblyUtilities::HierarchyName();
};

vtkStandardNewMacro(vtkBlockSelector);

//------------------------------------------------------------------------------
vtkBlockSelector::vtkBlockSelector()
{
  this->Internals = new vtkInternals;
}

//------------------------------------------------------------------------------
vtkBlockSelector::~vtkBlockSelector()
{
  delete this->Internals;
}

//------------------------------------------------------------------------------
void vtkBlockSelector::Initialize(vtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  auto& internals = (*this->Internals);
  internals = vtkInternals(); // reset values.

  const auto contentType = this->Node->GetContentType();
  if (contentType == vtkSelectionNode::BLOCKS)
  {
    vtkDataArray* selectionList = vtkDataArray::SafeDownCast(this->Node->GetSelectionList());
    if (selectionList->GetNumberOfComponents() == 2)
    {
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            selectionList, internals.AMRIds))
      {
        vtkGenericWarningMacro("SelectionList of unexpected type!");
      }
    }
    else if (selectionList->GetNumberOfComponents() == 1)
    {
      if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
            selectionList, internals.CompositeIds))
      {
        vtkGenericWarningMacro("SelectionList of unexpected type!");
      }
    }
  }
  else if (contentType == vtkSelectionNode::BLOCK_SELECTORS)
  {
    if (auto selectionList = vtkStringArray::SafeDownCast(this->Node->GetSelectionList()))
    {
      for (vtkIdType cc = 0, max = selectionList->GetNumberOfValues(); cc < max; ++cc)
      {
        internals.Selectors.push_back(selectionList->GetValue(cc));
      }
      // if selectionList has a name, we use that as an way to pick which
      // assembly to use.
      if (selectionList->GetName() && selectionList->GetName()[0] != '\0')
      {
        internals.AssemblyName = selectionList->GetName();
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkBlockSelector::Execute(vtkDataObject* input, vtkDataObject* output)
{
  auto& internals = (*this->Internals);
  auto inputCD = vtkCompositeDataSet::SafeDownCast(input);
  if (input && this->Node->GetContentType() == vtkSelectionNode::BLOCK_SELECTORS)
  {
    internals.CompositeIds.clear();

    // convert selectors to composite indices.
    if (auto assembly =
          vtkDataAssemblyUtilities::GetDataAssembly(internals.AssemblyName.c_str(), inputCD))
    {
      const auto compositeIds = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
        internals.Selectors, assembly, vtkPartitionedDataSetCollection::SafeDownCast(inputCD));
      // note the vtkPartitionedDataSetCollection is not needed unless were
      // using a vtkDataAssembly which doesn't represent a hierarchy. Such a
      // vtkDataAssembly is currently only supported by
      // vtkPartitionedDataSetCollection.
      internals.CompositeIds.insert(compositeIds.begin(), compositeIds.end());
    }
  }
  this->Superclass::Execute(input, output);
}

//------------------------------------------------------------------------------
bool vtkBlockSelector::ComputeSelectedElements(
  vtkDataObject* vtkNotUsed(input), vtkSignedCharArray* insidednessArray)
{
  insidednessArray->FillValue(1);
  return true;
}

//------------------------------------------------------------------------------
vtkSelector::SelectionMode vtkBlockSelector::GetAMRBlockSelection(
  unsigned int level, unsigned int index)
{
  auto& internals = (*this->Internals);
  if (internals.AMRIds.find(std::make_pair(level, index)) != internals.AMRIds.end())
  {
    return INCLUDE;
  }
  else
  {
    return INHERIT;
  }
}

//------------------------------------------------------------------------------
vtkSelector::SelectionMode vtkBlockSelector::GetBlockSelection(
  unsigned int compositeIndex, bool isDataObjectTree)
{
  auto& internals = (*this->Internals);
  if (internals.CompositeIds.find(compositeIndex) != internals.CompositeIds.end())
  {
    return INCLUDE;
  }
  else
  {
    if (isDataObjectTree)
    {
      return compositeIndex == 0 ? EXCLUDE : INHERIT;
    }
    else
    {
      return EXCLUDE;
    }
  }
}

//------------------------------------------------------------------------------
void vtkBlockSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
