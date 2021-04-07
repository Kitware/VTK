/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIYUtilities.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDIYUtilities.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"

#include <map>
#include <vector>

//------------------------------------------------------------------------------
template <class DataSetT>
std::vector<DataSetT*> vtkDIYUtilities::GetDataSets(vtkDataObject* dobj)
{
  std::vector<DataSetT*> datasets;
  if (auto cd = vtkCompositeDataSet::SafeDownCast(dobj))
  {
    auto iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto ds = DataSetT::SafeDownCast(iter->GetCurrentDataObject()))
      {
        datasets.push_back(ds);
      }
    }
    iter->Delete();
  }
  else if (auto ds = DataSetT::SafeDownCast(dobj))
  {
    datasets.push_back(ds);
  }

  return datasets;
}

//------------------------------------------------------------------------------
template <class DummyT>
void vtkDIYUtilities::Link(diy::Master& master, const diy::Assigner& assigner,
  const std::vector<std::map<int, DummyT>>& linksMap)
{
  for (int localId = 0; localId < static_cast<int>(linksMap.size()); ++localId)
  {
    const auto& links = linksMap[localId];
    auto l = new diy::Link();
    for (const auto& pair : links)
    {
      int nid = pair.first;
      l->add_neighbor(diy::BlockID(nid, assigner.rank(nid)));
    }
    master.replace_link(localId, l);
  }
}
