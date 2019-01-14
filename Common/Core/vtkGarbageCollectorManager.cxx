/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGarbageCollectorManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGarbageCollectorManager.h"

#include "vtkGarbageCollector.h"

// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkGarbageCollectorManagerCount;

vtkGarbageCollectorManager::vtkGarbageCollectorManager()
{
  if(++vtkGarbageCollectorManagerCount == 1)
  {
    vtkGarbageCollector::ClassInitialize();
  }
}

vtkGarbageCollectorManager::~vtkGarbageCollectorManager()
{
  if(--vtkGarbageCollectorManagerCount == 0)
  {
    vtkGarbageCollector::ClassFinalize();
  }
}
