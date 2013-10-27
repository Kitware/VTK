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

#include "vtkObjectFactory.h"

#include <pthread.h>

static bool vtkSMPToolsInitialized = false;
static int vtkSMPToolsNumberOfThreads = 0;

static std::vector<vtkMultiThreaderIDType> vtkSMPToolsThreadIds;

//static pthread_barrier_t barr;

VTKCOMMONCORE_EXPORT void vtkSMPToolsInitialize()
{
  vtkSMPTools::Initialize();
}

VTKCOMMONCORE_EXPORT int vtkSMPToolsGetNumberOfThreads()
{
  vtkSMPTools::Initialize();

  return vtkSMPToolsNumberOfThreads;
}

VTKCOMMONCORE_EXPORT int vtkSMPToolsGetThreadID()
{
  vtkSMPTools::Initialize();

  vtkMultiThreaderIDType rawID = vtkMultiThreader::GetCurrentThreadID();
  size_t numIDs = vtkSMPToolsThreadIds.size();
  for (size_t i=0; i<numIDs; i++)
    {
    if (vtkSMPToolsThreadIds[i] == rawID)
      {
      return i;
      }
    }
  return -1;
}

VTKCOMMONCORE_EXPORT std::vector<vtkMultiThreaderIDType>& vtkSMPToolsGetThreadIds()
{
  return vtkSMPToolsThreadIds;
}

//--------------------------------------------------------------------------------
void vtkSMPTools::Initialize(int nThreads)
{
  if (vtkSMPToolsInitialized)
    {
    return;
    }
  if (nThreads == 0)
    {
    vtkSMPToolsNumberOfThreads =
      vtkMultiThreader::GetGlobalDefaultNumberOfThreads();
    }
  else
    {
    vtkSMPToolsNumberOfThreads = nThreads;
    }

  vtkSMPToolsInitialized = true;

  vtkSMPToolsThreadIds.resize(vtkSMPToolsNumberOfThreads);
  vtkSMPToolsThreadIds[0] = vtkMultiThreader::GetCurrentThreadID();
}
