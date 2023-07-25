// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositeDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkDataObject.h"

#include <vector>

#ifndef vtkCompositeDataSet_txx
#define vtkCompositeDataSet_txx

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class DataSetT>
std::vector<DataSetT*> vtkCompositeDataSet::GetDataSets(vtkDataObject* dobj, bool preserveNull)
{
  std::vector<DataSetT*> datasets;
  if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    auto iter = cd->NewIterator();
    iter->SetSkipEmptyNodes(preserveNull ? 0 : 1);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = DataSetT::SafeDownCast(iter->GetCurrentDataObject()))
      {
        datasets.push_back(ds);
      }
      else if (preserveNull)
      {
        datasets.push_back(nullptr);
      }
    }
    iter->Delete();
  }
  else if (auto ds = DataSetT::SafeDownCast(dobj))
  {
    datasets.push_back(ds);
  }
  else if (preserveNull)
  {
    datasets.push_back(nullptr);
  }

  return datasets;
}

VTK_ABI_NAMESPACE_END
#endif
