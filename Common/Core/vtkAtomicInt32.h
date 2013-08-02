/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAtomicInt32.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAtomicInt32 - 32 bit integer with atomic operations
// .SECTION Description
// vtkAtomicInt32 can be used to represent a 32 bit integer and provides
// a number of platform-independent atomic integer operations. Atomic
// operations are guaranteed to occur without interruption by other
// threads and therefore can be used to manipulate integers in a
// thread-safe way. Note that there is no guarantee that the value of
// the integer will not be changed by another thread during the execution of
// these functions. Just that the operation will happen atomically. This
// means that if n threads call Increment() on an atomic integer, it is
// guaranteed that its value will be incremented n times.

#ifndef __vtkAtomicInt32_h
#define __vtkAtomicInt32_h

#include "vtkCommonCoreModule.h" // For export macro

struct vtkAtomicInt32Internal;

class VTKCOMMONCORE_EXPORT vtkAtomicInt32
{
public:
  vtkAtomicInt32()
    {
    this->Initialize(0);
    }

  vtkAtomicInt32(const int val)
    {
    this->Initialize(val);
    }

  vtkAtomicInt32(const vtkAtomicInt32& from)
    {
    this->Initialize(from.Get());
    }

  virtual ~vtkAtomicInt32();

  // Description:
  // Sets the value of the integer to the given argument.
  void Set(int value);

  // Description:
  // Returns the integer value.
  int Get() const;

  // Description:
  // Atomically increment the integer value. Returns the
  // result.
  int Increment();

  // Description:
  // Atomically adds the argument to the integer. Returns
  // the result of the addition.
  int Add(int val);

  // Description:
  // Atomically decrement the integer value. Returns the
  // result.
  int Decrement();

  // Description:
  // Atomically subtracts the argument from the integer. Returns
  // the result of the subtraction.
  int Subtract(int val)
  {
    return this->Add(-val);
  }

private:
  vtkAtomicInt32Internal* Internal;

  void Initialize(const int val);
};

#endif
// VTK-HeaderTest-Exclude: vtkAtomicInt32.h
