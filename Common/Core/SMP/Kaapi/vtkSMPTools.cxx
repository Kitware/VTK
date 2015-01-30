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

#include "vtkCriticalSection.h"

#include <kaapic.h>

VTKCOMMONCORE_EXPORT void vtkSMPToolsInitialize()
{
  vtkSMPTools::Initialize(0);
}

struct vtkSMPToolsInit
{
  vtkSMPToolsInit()
    {
      kaapic_init(KAAPIC_START_ONLY_MAIN);
    }

  ~vtkSMPToolsInit()
    {
      kaapic_finalize();
    }
};

static bool vtkSMPToolsInitialized = 0;
static vtkSimpleCriticalSection vtkSMPToolsCS;

//--------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int)
{
  vtkSMPToolsCS.Lock();
  if (!vtkSMPToolsInitialized)
    {
    static vtkSMPToolsInit aInit;
    vtkSMPToolsInitialized = true;
    }
  vtkSMPToolsCS.Unlock();
}

int vtkSMPTools::GetEstimatedNumberOfThreads()
{
  return kaapic_get_concurrency();
}
