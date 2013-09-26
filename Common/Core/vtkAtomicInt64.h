/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt64.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAtomicInt64 - 64 bit integer with atomic operations
// .SECTION Description
// vtkAtomicInt64 can be used to represent a 32 bit integer and provides
// a number of platform-independent atomic integer operations. Atomic
// operations are guaranteed to occur without interruption by other
// threads and therefore can be used to manipulate integers in a
// thread-safe way. Note that there is no guarantee that the value of
// the integer will not be changed by another thread during the execution of
// these functions. Just that the operation will happen atomically. This
// means that if n threads call Increment() on an atomic integer, it is
// guaranteed that its value will be incremented n times.

#ifndef __vtkAtomicInt64_h
#define __vtkAtomicInt64_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkType.h" // For vtkTypeInt64

#include <cassert>

struct vtkAtomicInt64Internal;

class VTKCOMMONCORE_EXPORT vtkAtomicInt64
{
public:
  vtkAtomicInt64()
    {
    this->Initialize(0);
    }

  vtkAtomicInt64(const vtkTypeInt64 val)
    {
    this->Initialize(val);
    }

  vtkAtomicInt64(const vtkAtomicInt64& from)
    {
    this->Initialize(from.Get());
    }

  virtual ~vtkAtomicInt64();

  // Description:
  // Sets the value of the integer to the given argument.
  void Set(vtkTypeInt64 value);

  // Description:
  // Returns the integer value.
  vtkTypeInt64 Get() const;

  // Description:
  // Atomically increment the integer value. Returns the
  // result.
  vtkTypeInt64 Increment();

  // Description:
  // Atomically adds the argument to the integer. Returns
  // the result of the addition.
  vtkTypeInt64 Add(vtkTypeInt64 val);

  // Description:
  // Atomically decrement the integer value. Returns the
  // result.
  vtkTypeInt64 Decrement();

  // Description:
  // Atomically subtracts the argument from the integer. Returns
  // the result of the subtraction.
  vtkTypeInt64 Subtract(vtkTypeInt64 val)
  {
    return this->Add(-val);
  }

private:
  vtkAtomicInt64Internal* Internal;

  void Initialize(const vtkTypeInt64 val);
};

#endif
// VTK-HeaderTest-Exclude: vtkAtomicInt64.h
