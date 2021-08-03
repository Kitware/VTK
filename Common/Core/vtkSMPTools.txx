/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPTools.h"

//------------------------------------------------------------------------------
template <typename T>
void vtkSMPTools::LocalScope(Config const& config, T&& lambda)
{
  const int oldThreadNumber = vtkSMPTools::GetEstimatedNumberOfThreads();
  vtkSMPTools::Initialize(config.MaxNumberOfThreads);
  const char* oldBackend = vtk::detail::smp::vtkSMPToolsAPI::GetInstance().GetBackend();
  vtkSMPTools::SetBackend(config.Backend.c_str());
  try
  {
    lambda();
  }
  catch (...)
  {
    vtkSMPTools::Initialize(oldThreadNumber);
    vtkSMPTools::SetBackend(oldBackend);
    throw;
  }
  vtkSMPTools::Initialize(oldThreadNumber);
  vtkSMPTools::SetBackend(oldBackend);
}
