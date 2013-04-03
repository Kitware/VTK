/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDebugLeaksManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDebugLeaksManager.h"
#include "vtkDebugLeaks.h"

// Global optimization performed by MSVC breaks the initialization
// order across translation units enforced by this manager.  Disable
// them for this object file.
#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif

// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int vtkDebugLeaksManagerCount;

vtkDebugLeaksManager::vtkDebugLeaksManager()
{
  if(++vtkDebugLeaksManagerCount == 1)
    {
    vtkDebugLeaks::ClassInitialize();
    }
}

vtkDebugLeaksManager::~vtkDebugLeaksManager()
{
  if(--vtkDebugLeaksManagerCount == 0)
    {
    vtkDebugLeaks::ClassFinalize();
    }
}

#if defined(_MSC_VER)
# pragma optimize("g", on)
#endif
