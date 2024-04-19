// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDebugLeaksManager.h"
#include "vtkDebugLeaks.h"

// Global optimization performed by MSVC breaks the initialization
// order across translation units enforced by this manager.  Disable
// them for this object file.
#if defined(_MSC_VER)
#pragma optimize("g", off)
#endif

// Must NOT be initialized.  Default initialization to zero is
// necessary.
VTK_ABI_NAMESPACE_BEGIN
static unsigned int vtkDebugLeaksManagerCount;

vtkDebugLeaksManager::vtkDebugLeaksManager()
{
  if (++vtkDebugLeaksManagerCount == 1)
  {
    vtkDebugLeaks::ClassInitialize();
  }
}

vtkDebugLeaksManager::~vtkDebugLeaksManager()
{
  if (--vtkDebugLeaksManagerCount == 0)
  {
    vtkDebugLeaks::ClassFinalize();
  }
}

#if defined(_MSC_VER)
#pragma optimize("g", on)
#endif
VTK_ABI_NAMESPACE_END
