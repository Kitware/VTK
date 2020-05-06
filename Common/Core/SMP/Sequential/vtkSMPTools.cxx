/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMPTools.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMPTools.h"

#include "vtkSMP.h"

// Simple implementation that runs everything sequentially.

const char* vtkSMPTools::GetBackend()
{
  return VTK_SMP_BACKEND;
}

//------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int) {}

int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  return 1;
}
