/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTimeStamp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTimeStamp.h"

#include "vtkObjectFactory.h"
#include "vtkWindows.h"

// We use the Schwarz Counter idiom to make sure that GlobalTimeStamp
// is initialized before any other class uses it.

#if VTK_SIZEOF_VOID_P == 8
# include "vtkAtomicInt64.h" // For global mtime
static vtkAtomicInt64* GlobalTimeStamp;
#else
# include "vtkAtomicInt32.h" // For global mtime
static vtkAtomicInt32* GlobalTimeStamp;
#endif

static unsigned int vtkTimeStampCounter;

vtkTimeStampInitialize::vtkTimeStampInitialize()
{
  if (0 == vtkTimeStampCounter++)
    {
    // Use 32 bit atomic int on 32 bit systems, 64 bit on 64 bit systems.
    // The assumption is that atomic operations will be safer when in the
    // type for integer operations.
#if VTK_SIZEOF_VOID_P == 8
    GlobalTimeStamp = new vtkAtomicInt64(0);
#else
    GlobalTimeStamp = new vtkAtomicInt32(0);
#endif
    }
}

vtkTimeStampInitialize::~vtkTimeStampInitialize()
{
  if (0 == --vtkTimeStampCounter)
    {
    delete GlobalTimeStamp;
    GlobalTimeStamp = 0;
    }
}

//-------------------------------------------------------------------------
vtkTimeStamp* vtkTimeStamp::New()
{
  // If the factory was unable to create the object, then create it here.
  return new vtkTimeStamp;
}

//-------------------------------------------------------------------------
void vtkTimeStamp::Modified()
{
  this->ModifiedTime = (unsigned long)GlobalTimeStamp->Increment();
}
