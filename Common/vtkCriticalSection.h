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
// .NAME vtkCriticalSection - Critical section locking class
// .SECTION Description
// vtkCriticalSection allows the locking of variables which are accessed 
// through different threads.  This header file also defines 
// vtkSimpleCriticalSection which is not a subclass of vtkObject.
// The API is identical to that of vtkMutexLock, and the behavior is
// identical as well, except on Windows 9x/NT platforms. The only difference
// on these platforms is that vtkMutexLock is more flexible, in that
// it works across processes as well as across threads, but also costs
// more, in that it evokes a 600-cycle x86 ring transition. The 
// vtkCriticalSection provides a higher-performance equivalent (on 
// Windows) but won't work across processes. Since it is unclear how,
// in vtk, an object at the vtk level can be shared across processes
// in the first place, one should use vtkCriticalSection unless one has
// a very good reason to use vtkMutexLock. If higher-performance equivalents
// for non-Windows platforms (Irix, SunOS, etc) are discovered, they
// should replace the implementations in this class

#ifndef __vtkCriticalSection_h
#define __vtkCriticalSection_h

#include "vtkObject.h"

//BTX

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
class VTK_COMMON_EXPORT vtkSimpleCriticalSection
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

  // Default vtkObject API
  static vtkSimpleCriticalSection *New();
  void Delete()
    {
    delete this;
    }

  void Init();

  // Description:
  // Lock the vtkCriticalSection
  void Lock();

  // Description:
  // Unlock the vtkCriticalSection
  void Unlock();

protected:
  vtkCritSecType   CritSec;
};

//ETX

class VTK_COMMON_EXPORT vtkCriticalSection : public vtkObject
{
public:
  static vtkCriticalSection *New();

  vtkTypeMacro(vtkCriticalSection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Lock the vtkCriticalSection
  void Lock();

  // Description:
  // Unlock the vtkCriticalSection
  void Unlock();

protected:
  vtkSimpleCriticalSection SimpleCriticalSection;
  vtkCriticalSection() {}
  ~vtkCriticalSection() {}

private:
  vtkCriticalSection(const vtkCriticalSection&);  // Not implemented.
  void operator=(const vtkCriticalSection&);  // Not implemented.
};


inline void vtkCriticalSection::Lock()
{
  this->SimpleCriticalSection.Lock();
}

inline void vtkCriticalSection::Unlock()
{
  this->SimpleCriticalSection.Unlock();
}

#endif
