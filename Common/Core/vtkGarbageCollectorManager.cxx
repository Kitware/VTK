// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGarbageCollectorManager.h"

#include "vtkGarbageCollector.h"

// Must NOT be initialized.  Default initialization to zero is
// necessary.
VTK_ABI_NAMESPACE_BEGIN
static unsigned int vtkGarbageCollectorManagerCount;

vtkGarbageCollectorManager::vtkGarbageCollectorManager()
{
  if (++vtkGarbageCollectorManagerCount == 1)
  {
    vtkGarbageCollector::ClassInitialize();
  }
}

vtkGarbageCollectorManager::~vtkGarbageCollectorManager()
{
  if (--vtkGarbageCollectorManagerCount == 0)
  {
    vtkGarbageCollector::ClassFinalize();
  }
}
VTK_ABI_NAMESPACE_END
