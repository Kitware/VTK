/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOStrStreamWrapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOStrStreamWrapper - Wrapper for ostrstream.  Internal VTK use only.
// .SECTION Description
// Provides a wrapper around the C++ ostrstream class so that VTK
// source files need not include the full C++ streams library.  This
// is intended to prevent cluttering of the translation unit and speed
// up compilation.  Experimentation has revealed between 10% and 60%
// less time for compilation depending on the platform.  This wrapper
// is used by the macros in vtkSetGet.h.

#ifndef __vtkOStrStreamWrapper_h
#define __vtkOStrStreamWrapper_h

#ifndef __VTK_SYSTEM_INCLUDES__INSIDE
Do_not_include_vtkOStrStreamWrapper_directly__vtkSystemIncludes_includes_it;
#endif

class VTKCOMMONCORE_EXPORT vtkOStrStreamWrapper: public vtkOStreamWrapper
{
public:
  // Description:
  // Constructor.
  vtkOStrStreamWrapper();

  // Description:
  // Destructor frees all used memory.
  ~vtkOStrStreamWrapper();

  // Description:
  // Get the string that has been written.  This call transfers
  // ownership of the returned memory to the caller.  Call
  // rdbuf()->freeze(0) to return ownership to the vtkOStrStreamWrapper.
  char* str();

  // Description:
  // Returns a pointer to this class.  This is a hack so that the old
  // ostrstream's s.rdbuf()->freeze(0) can work.
  vtkOStrStreamWrapper* rdbuf();

  // Description:
  // Set whether the memory is frozen.  The vtkOStrStreamWrapper will free
  // the memory returned by str() only if it is not frozen.
  void freeze();
  void freeze(int);
protected:
  // The pointer returned by str().
  char* Result;

  // Whether the caller of str() owns the memory.
  int Frozen;
private:
  vtkOStrStreamWrapper(const vtkOStrStreamWrapper& r); // Not Implemented.
  vtkOStrStreamWrapper& operator=(const vtkOStrStreamWrapper&); // Not Implemented.
};

#endif
// VTK-HeaderTest-Exclude: vtkOStrStreamWrapper.h
