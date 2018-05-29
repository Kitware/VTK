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
#include "vtkAssume.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkSelectionNode.h"
#include "vtkSignedCharArray.h"
#include "vtkUniformGridAMRDataIterator.h"

#include <set>

class vtkBlockSelector::vtkInternals {
public:

  // This functor is only needed for vtkArrayDispatch to correctly fill it up.
  // Otherwise, it would simply be a set.
  class CompositeIdsT : public std::set<unsigned int>
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      VTK_ASSUME(array->GetNumberOfComponents() == 1);
      vtkDataArrayAccessor<ArrayType> accessor(array);
      for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
      {
        this->insert(static_cast<unsigned int>(accessor.Get(cc, 0)));
      }
    }
  };

  // this functor are only needed for vtkArrayDispatch to correctly fill it up.
  // otherwise, it'd simply be a set.
  class AMRIdsT : public std::set<std::pair<unsigned int, unsigned int> >
  {
  public:
    template <typename ArrayType>
    void operator()(ArrayType* array)
    {
      VTK_ASSUME(array->GetNumberOfComponents() == 2);
      vtkDataArrayAccessor<ArrayType> accessor(array);
      for (vtkIdType cc = 0, max = array->GetNumberOfTuples(); cc < max; ++cc)
      {
        this->insert(
          std::pair<unsigned int, unsigned int>(static_cast<unsigned int>(accessor.Get(cc, 0)),
            static_cast<unsigned int>(accessor.Get(cc, 1))));
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
void vtkBlockSelector::Initialize(vtkSelectionNode* node, const std::string& insidednessArrayName)
{
  this->Superclass::Initialize(node, insidednessArrayName);

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
bool vtkBlockSelector::ComputeSelectedElementsForBlock(vtkDataObject* vtkNotUsed(input),
    vtkSignedCharArray* insidednessArray, unsigned int compositeIndex,
    unsigned int amrLevel, unsigned int amrIndex)
{

  bool is_selected =
    (this->Internals->CompositeIds.find(compositeIndex) != this->Internals->CompositeIds.end()) ||
    (this->Internals->AMRIds.find(std::pair<unsigned int, unsigned int>(amrLevel, amrIndex)) != this->Internals->AMRIds.end());
  if (this->SkipBlock(compositeIndex, amrLevel, amrIndex) || !is_selected)
  {
    insidednessArray->FillValue(0);
  }
  else
  {
    insidednessArray->FillValue(1);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkBlockSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
