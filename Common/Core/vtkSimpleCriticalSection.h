/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSimpleCriticalSection
 * @brief   Critical section locking class
 *
 * vtkCriticalSection allows the locking of variables which are accessed
 * through different threads.  This header file also defines
 * vtkSimpleCriticalSection which is not a subclass of vtkObject.
 * The API is identical to that of vtkMutexLock, and the behavior is
 * identical as well, except on Windows 9x/NT platforms. The only difference
 * on these platforms is that vtkMutexLock is more flexible, in that
 * it works across processes as well as across threads, but also costs
 * more, in that it evokes a 600-cycle x86 ring transition. The
 * vtkCriticalSection provides a higher-performance equivalent (on
 * Windows) but won't work across processes. Since it is unclear how,
 * in vtk, an object at the vtk level can be shared across processes
 * in the first place, one should use vtkCriticalSection unless one has
 * a very good reason to use vtkMutexLock. If higher-performance equivalents
 * for non-Windows platforms (Irix, SunOS, etc) are discovered, they
 * should replace the implementations in this class
*/

#ifndef vtkSimpleCriticalSection_h
#define vtkSimpleCriticalSection_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkSystemIncludes.h"

#ifdef VTK_USE_SPROC
#include <abi_mutex.h> // Needed for sproc implementation of mutex
typedef abilock_t vtkCritSecType;
#endif

#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
#include <pthread.h> // Needed for pthreads implementation of mutex
typedef pthread_mutex_t vtkCritSecType;
#endif

#ifdef VTK_USE_WIN32_THREADS
# include "vtkWindows.h" // Needed for win32 implementation of mutex
typedef CRITICAL_SECTION vtkCritSecType;
#endif

#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#ifndef VTK_USE_WIN32_THREADS
typedef int vtkCritSecType;
#endif
#endif
#endif

// Critical Section object that is not a vtkObject.
class VTKCOMMONCORE_EXPORT vtkSimpleCriticalSection
{
public:
  // Default cstor
  vtkSimpleCriticalSection()
  {
    this->Init();
  }
  // Construct object locked if isLocked is different from 0
  vtkSimpleCriticalSection(int isLocked)
  {
    this->Init();
    if(isLocked)
    {
      this->Lock();
    }
  }
  // Destructor
  virtual ~vtkSimpleCriticalSection();

  void Init();

  /**
   * Lock the vtkCriticalSection
   */
  void Lock();

  /**
   * Unlock the vtkCriticalSection
   */
  void Unlock();

protected:
  vtkCritSecType   CritSec;

private:
  vtkSimpleCriticalSection(const vtkSimpleCriticalSection& other) VTK_DELETE_FUNCTION;
  vtkSimpleCriticalSection& operator=(const vtkSimpleCriticalSection& rhs) VTK_DELETE_FUNCTION;
};

#endif
// VTK-HeaderTest-Exclude: vtkSimpleCriticalSection.h
