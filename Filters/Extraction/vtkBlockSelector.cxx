/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBlockSelector.h"

#include "vtkArrayDispatch.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkUniformGridAMRDataIterator.h"

#include <set>

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
  class AMRIdsT : public std::set<std::pair<unsigned int, unsigned int> >
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
};

vtkStandardNewMacro(vtkBlockSelector);

//----------------------------------------------------------------------------
vtkBlockSelector::vtkBlockSelector()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkBlockSelector::~vtkBlockSelector()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkBlockSelector::Initialize(vtkSelectionNode* node)
{
  this->Superclass::Initialize(node);

  assert(this->Node->GetContentType() == vtkSelectionNode::BLOCKS);
  vtkDataArray* selectionList = vtkDataArray::SafeDownCast(this->Node->GetSelectionList());
  if (selectionList->GetNumberOfComponents() == 2)
  {
    if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
          selectionList, this->Internals->AMRIds))
    {
      vtkGenericWarningMacro("SelectionList of unexpected type!");
    }
  }
  else if (selectionList->GetNumberOfComponents() == 1)
  {
    if (!vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Integrals>::Execute(
          selectionList, this->Internals->CompositeIds))
    {
      vtkGenericWarningMacro("SelectionList of unexpected type!");
    }
  }
}

//----------------------------------------------------------------------------
bool vtkBlockSelector::ComputeSelectedElements(
  vtkDataObject* vtkNotUsed(input), vtkSignedCharArray* insidednessArray)
{
  insidednessArray->FillValue(1);
  return true;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkSelector::SelectionMode vtkBlockSelector::GetBlockSelection(unsigned int compositeIndex)
{
  auto& internals = (*this->Internals);
  if (internals.CompositeIds.find(compositeIndex) != internals.CompositeIds.end())
  {
    return INCLUDE;
  }
  else
  {
    return compositeIndex == 0 ? EXCLUDE : INHERIT;
  }
}

//----------------------------------------------------------------------------
void vtkBlockSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
