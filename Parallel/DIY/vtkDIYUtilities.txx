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
#ifndef vtkDIYUtilities_txx
#define vtkDIYUtilities_txx

#include "vtkDIYUtilities.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"

#include <map>
#include <vector>

#include "vtk_diy2.h" // needed for DIY
#include VTK_DIY2(diy / link.hpp)
#include VTK_DIY2(diy / master.hpp)
#include VTK_DIY2(diy / mpi.hpp)
#include VTK_DIY2(diy / reduce - operations.hpp)

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
template <class BlockT, class AssignerT, class LinksMapT>
void vtkDIYUtilities::Link(
  diy::Master& master, const AssignerT& assigner, const LinksMapT& linksMap)
{
  struct Functor
  {
    Functor(const diy::Master& master, const LinksMapT& linksMap)
      : Master(master)
      , LinksMap(linksMap)
    {
    }

    void operator()(BlockT*, const diy::ReduceProxy& rp) const
    {
      int myBlockId = rp.gid();
      this->Neighbors[myBlockId];
      if (rp.round() == 0)
      {
        int localId = this->Master.lid(myBlockId);
        const auto& links = this->LinksMap[localId];
        for (int i = 0; i < rp.out_link().size(); ++i)
        {
          const diy::BlockID& blockId = rp.out_link().target(i);
          int connected = links.count(blockId.gid) != 0;
          const auto& dest = rp.out_link().target(i);
          rp.enqueue(dest, &connected, 1);
        };
      }
      else
      {
        std::vector<int>& myNeighbors = this->Neighbors.at(myBlockId);
        for (int i = 0; i < rp.in_link().size(); ++i)
        {
          const auto& src = rp.in_link().target(i);
          int connected;
          rp.dequeue(src, &connected, 1);
          if (connected)
          {
            myNeighbors.push_back(src.gid);
          }
        }
      }
    }
    const diy::Master& Master;
    const LinksMapT& LinksMap;
    mutable std::map<int, std::vector<int>> Neighbors;
  } functor(master, linksMap);

  diy::all_to_all(master, assigner, functor);

  // Update local links.
  for (auto& pair : functor.Neighbors)
  {
    auto l = new diy::Link();
    for (const auto& nid : pair.second)
    {
      l->add_neighbor(diy::BlockID(nid, assigner.rank(nid)));
    }
    master.replace_link(master.lid(pair.first), l);
  }
}

#endif
