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
#include <set>
#include <vector>

// clang-format off
#include "vtk_diy2.h" // needed for DIY
#include VTK_DIY2(diy/link.hpp)
#include VTK_DIY2(diy/master.hpp)
#include VTK_DIY2(diy/mpi.hpp)
#include VTK_DIY2(diy/reduce-operations.hpp)
// clang-format on

//------------------------------------------------------------------------------
template <class DataSetT>
std::vector<DataSetT*> vtkDIYUtilities::GetDataSets(vtkDataObject* dobj)
{
  VTK_LEGACY_REPLACED_BODY(
    vtkDIYUtilities::GetDataSets, "VTK 9.1", vtkCompositeDataSet::GetDataSets);
  return vtkCompositeDataSet::GetDataSets<DataSetT>(dobj);
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

#endif
