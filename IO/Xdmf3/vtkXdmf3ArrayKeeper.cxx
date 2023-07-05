// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkXdmf3ArrayKeeper.h"

// clang-format off
#include "vtk_xdmf3.h"
#include VTKXDMF3_HEADER(core/XdmfArray.hpp)
// clang-format on

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkXdmf3ArrayKeeper::vtkXdmf3ArrayKeeper()
{
  generation = 0;
}

//------------------------------------------------------------------------------
vtkXdmf3ArrayKeeper::~vtkXdmf3ArrayKeeper()
{
  this->Release(true);
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::BumpGeneration()
{
  this->generation++;
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::Insert(XdmfArray* val)
{
  this->operator[](val) = this->generation;
}

//------------------------------------------------------------------------------
void vtkXdmf3ArrayKeeper::Release(bool force)
{
  vtkXdmf3ArrayKeeper::iterator it = this->begin();
  // int cnt = 0;
  // int total = 0;
  while (it != this->end())
  {
    // total++;
    vtkXdmf3ArrayKeeper::iterator current = it++;
    if (force || (current->second != this->generation))
    {
      XdmfArray* atCurrent = current->first;
      atCurrent->release();
      this->erase(current);
      // cnt++;
    }
  }
  // cerr << "released " << cnt << "/" << total << " arrays" << endl;
}
VTK_ABI_NAMESPACE_END
