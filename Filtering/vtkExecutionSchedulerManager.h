/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutionSchedulerManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExecutionSchedulerManager - Manage the instantiation and deletion
// of the vtkExecutionScheduler singleton.
//
// vtkExecutionSchedulerManager is included in the header of
// vtkExecutionScheduler, or any of its subclasses.
// It makes sure that the singleton is created after the vtkDebugLeaks singleton,
// and destroyed before vtkDebugLeaks is cleaned up.

// .Section See Also
// vtkExecutionScheduler

#ifndef __vtkExecutionSchedulerManager_h
#define __vtkExecutionSchedulerManager_h

#include "vtkSystemIncludes.h"
#include "vtkDebugLeaksManager.h" // DebugLeaks exists longer than us.

class VTK_FILTERING_EXPORT vtkExecutionSchedulerManager
{
public:
  vtkExecutionSchedulerManager();
  ~vtkExecutionSchedulerManager();
private:
  static unsigned int Count;
};

// Description: This instance will show up in any translation unit that uses
// vtkExecutionScheduler, or that has a singleton.  It will make sure
// that it is initialized before it is used, and is one of the last
// static objects to be destroyed.
static vtkExecutionSchedulerManager vtkExecutionSchedulerManagerInstance;

#endif
