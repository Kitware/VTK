// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
VTK_ABI_NAMESPACE_BEGIN
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

VTK_ABI_NAMESPACE_END
#endif
