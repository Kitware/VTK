/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExecutionSchedulerManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExecutionSchedulerManager.h"
#include "vtkExecutionScheduler.h"

// Must NOT be initialized.  Default initialization to zero is required.
unsigned int vtkExecutionSchedulerManager::Count;

vtkExecutionSchedulerManager::vtkExecutionSchedulerManager()
{
if(++vtkExecutionSchedulerManager::Count == 1)
    {
    vtkExecutionScheduler::ClassInitialize();
    }
}

vtkExecutionSchedulerManager::~vtkExecutionSchedulerManager()
{
if(--vtkExecutionSchedulerManager::Count == 0)
    {
    vtkExecutionScheduler::ClassFinalize();
    }
}
