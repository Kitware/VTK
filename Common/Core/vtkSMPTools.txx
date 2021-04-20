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
void vtkSMPTools::ScopeWithMaxThread(int numThreads, T&& lambda)
{
  const int oldThreadNumber = vtkSMPTools::GetEstimatedNumberOfThreads();
  vtkSMPTools::Initialize(numThreads);
  try
  {
    lambda();
  }
  catch (...)
  {
    vtkSMPTools::Initialize(oldThreadNumber);
    throw;
  }
  vtkSMPTools::Initialize(oldThreadNumber);
}

//------------------------------------------------------------------------------
template <typename T>
void vtkSMPTools::ScopeWithMaxThread(T&& lambda)
{
  vtkSMPTools::ScopeWithMaxThread(0, lambda);
}
