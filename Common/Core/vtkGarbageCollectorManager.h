/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGarbageCollectorManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGarbageCollectorManager - Manages the vtkGarbageCollector singleton.
// .SECTION Description
// vtkGarbageCollectorManager should be included in any translation unit
// that will use vtkGarbageCollector or that implements the singleton
// pattern.  It makes sure that the vtkGarbageCollector singleton is created
// before and destroyed after it is used.

#ifndef __vtkGarbageCollectorManager_h
#define __vtkGarbageCollectorManager_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#include "vtkDebugLeaksManager.h" // DebugLeaks is around longer than
                                  // the garbage collector.

class VTKCOMMONCORE_EXPORT vtkGarbageCollectorManager
{
public:
  vtkGarbageCollectorManager();
  ~vtkGarbageCollectorManager();
};

// This instance will show up in any translation unit that uses
// vtkGarbageCollector or that has a singleton.  It will make sure
// vtkGarbageCollector is initialized before it is used finalized when
// it is done being used.
static vtkGarbageCollectorManager vtkGarbageCollectorManagerInstance;

#endif
// VTK-HeaderTest-Exclude: vtkGarbageCollectorManager.h
