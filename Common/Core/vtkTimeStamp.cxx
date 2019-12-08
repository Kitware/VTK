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

#include <atomic>

//-------------------------------------------------------------------------
vtkTimeStamp* vtkTimeStamp::New()
{
  // If the factory was unable to create the object, then create it here.
  return new vtkTimeStamp;
}

//-------------------------------------------------------------------------
void vtkTimeStamp::Modified()
{
  // Here because of a static destruction error? You're not the first. After
  // discussion of the tradeoffs, the cost of adding a Schwarz counter on this
  // static to ensure it gets destructed is unlikely to be worth the cost over
  // just leaking it.
  //
  // Solutions and their tradeoffs:
  //
  //  - Schwarz counter: each VTK class now has a static initializer function
  //    that increments and integer. This cannot be inlined or optimized away.
  //    Adds latency to ParaView startup.
  //  - Separate library for this static. This adds another library to VTK
  //    which are already legion. It could not be folded into a kit because
  //    that would bring you back to the same problem you have today.
  //  - Leak a heap allocation for it. It's 24 bytes, leaked exactly once, and
  //    is easily suppressed in Valgrind.
  //
  // The last solution has been decided to have the smallest downside of these.
  //
  //  static const vtkAtomicUIntXX* GlobalTimeStamp = new vtkAtomicUIntXX(0);
  //
  // Good luck!
#if defined(VTK_USE_64BIT_TIMESTAMPS) || VTK_SIZEOF_VOID_P == 8
  static std::atomic<uint64_t> GlobalTimeStamp(0U);
#else
  static std::atomic<uint32_t> GlobalTimeStamp(0U);
#endif
  this->ModifiedTime = (vtkMTimeType)++GlobalTimeStamp;
}
