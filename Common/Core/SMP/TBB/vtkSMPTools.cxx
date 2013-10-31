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

#include <tbb/task_scheduler_init.h>

struct vtkSMPToolsInit
{
  tbb::task_scheduler_init Init;

  vtkSMPToolsInit(int numThreads) : Init(numThreads)
    {
    }
};

static bool vtkSMPToolsInitialized = 0;
static vtkSimpleCriticalSection vtkSMPToolsCS;

//--------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int numThreads)
{
  vtkSMPToolsCS.Lock();
  if (!vtkSMPToolsInitialized)
    {
    // If numThreads == 0 (default num. threads), don't create a task_scheduler_init
    // and let TBB do the default thing.
    if (numThreads != 0)
      {
      static vtkSMPToolsInit aInit(numThreads);
      }
    vtkSMPToolsInitialized = true;
    }
  vtkSMPToolsCS.Unlock();
}
