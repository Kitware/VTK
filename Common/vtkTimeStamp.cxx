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
//
// Initialize static member
//
#include "vtkTimeStamp.h"

#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"
#include "vtkWindows.h"
#if defined(__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1040)
  #include <libkern/OSAtomic.h>
#endif

//-------------------------------------------------------------------------
vtkTimeStamp* vtkTimeStamp::New()
{
  // If the factory was unable to create the object, then create it here.
  return new vtkTimeStamp;
}

//-------------------------------------------------------------------------
void vtkTimeStamp::Modified()
{
// Windows optimization
#if defined(WIN32) || defined(_WIN32)
  static LONG vtkTimeStampTime = 0;
  this->ModifiedTime = (unsigned long)InterlockedIncrement(&vtkTimeStampTime);

// Mac optimization (64 bit)
#elif defined(__APPLE__) && __LP64__
  // "ModifiedTime" is "unsigned long", a type that changess sizes
  // depending on architecture.  The atomic increment is safe, since it
  // operates on a variable of the exact type needed.  The cast does not
  // change the size, but does change signedness, which is not ideal.
  static volatile int64_t vtkTimeStampTime = 0;
  this->ModifiedTime = (unsigned long)OSAtomicIncrement64Barrier(&vtkTimeStampTime);

// Mac optimization (32 bit, 10.4 or later)
#elif defined(__APPLE__) && (MAC_OS_X_VERSION_MIN_REQUIRED >= 1040)
  static volatile int32_t vtkTimeStampTime = 0;
  this->ModifiedTime = (unsigned long)OSAtomicIncrement32Barrier(&vtkTimeStampTime);

// General case
#else
  static unsigned long vtkTimeStampTime = 0;
  static vtkSimpleCriticalSection TimeStampCritSec;
  
  TimeStampCritSec.Lock();
  this->ModifiedTime = ++vtkTimeStampTime;
  TimeStampCritSec.Unlock();
#endif
}
