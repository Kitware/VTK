// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOStrStreamWrapper
 * @brief   Wrapper for ostrstream.  Internal VTK use only.
 *
 * Provides a wrapper around the C++ ostrstream class so that VTK
 * source files need not include the full C++ streams library.  This
 * is intended to prevent cluttering of the translation unit and speed
 * up compilation.  Experimentation has revealed between 10% and 60%
 * less time for compilation depending on the platform.  This wrapper
 * is used by the macros in vtkSetGet.h.
 */

#ifndef vtkOStrStreamWrapper_h
#define vtkOStrStreamWrapper_h

#include "vtkCommonCoreModule.h"

#ifndef VTK_SYSTEM_INCLUDES_INSIDE
Do_not_include_vtkOStrStreamWrapper_directly_vtkSystemIncludes_includes_it;
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCORE_EXPORT vtkOStrStreamWrapper : public vtkOStreamWrapper
{
public:
  /**
   * Constructor.
   */
  vtkOStrStreamWrapper();

  /**
   * Destructor frees all used memory.
   */
  ~vtkOStrStreamWrapper() override;

  /**
   * Get the string that has been written.  This call transfers
   * ownership of the returned memory to the caller.  Call
   * rdbuf()->freeze(0) to return ownership to the vtkOStrStreamWrapper.
   */
  char* str();

  /**
   * Returns a pointer to this class.  This is a hack so that the old
   * ostrstream's s.rdbuf()->freeze(0) can work.
   */
  vtkOStrStreamWrapper* rdbuf();

  ///@{
  /**
   * Set whether the memory is frozen.  The vtkOStrStreamWrapper will free
   * the memory returned by str() only if it is not frozen.
   */
  void freeze();
  void freeze(int);
  ///@}

protected:
  // The pointer returned by str().
  char* Result;

  // Whether the caller of str() owns the memory.
  int Frozen;

private:
  vtkOStrStreamWrapper(const vtkOStrStreamWrapper& r) = delete;
  vtkOStrStreamWrapper& operator=(const vtkOStrStreamWrapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkOStrStreamWrapper.h
