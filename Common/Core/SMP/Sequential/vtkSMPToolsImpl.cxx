/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPToolsImpl.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "SMP/Common/vtkSMPToolsImpl.h"
#include "SMP/Sequential/vtkSMPToolsImpl.txx"

namespace vtk
{
namespace detail
{
namespace smp
{

//------------------------------------------------------------------------------
template <>
void vtkSMPToolsImpl<BackendType::Sequential>::Initialize(int)
{
}

//------------------------------------------------------------------------------
template <>
int vtkSMPToolsImpl<BackendType::Sequential>::GetEstimatedNumberOfThreads()
{
  return 1;
}

} // namespace smp
} // namespace detail
} // namespace vtk
