/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCriticalSection - critical section locking class
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
#include <abi_mutex.h>
typedef abilock_t vtkCritSecType;
#endif

#ifdef VTK_USE_PTHREADS
#include <pthread.h>
typedef pthread_mutex_t vtkCritSecType;
#endif
 
#ifdef VTK_USE_WIN32_THREADS
#include <winbase.h>
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
  vtkSimpleCriticalSection()
    {
      this->Init();
    }

  vtkSimpleCriticalSection(int isLocked)
    {
      this->Init();
      if(isLocked)
        {
        this->Lock();
        }
    }

  void Init();

  virtual ~vtkSimpleCriticalSection();

  static vtkSimpleCriticalSection *New();

  // What's the point of these (here and in MutexLock)? This class
  // is not part of the hierarchy!! -CRV
  virtual const char *GetClassName() {return "vtkSimpleCriticalSection";};
  virtual int IsA(const char *name);
  static vtkSimpleCriticalSection *SafeDownCast(vtkSimpleCriticalSection *o);

  void Delete() {delete this;}
  
  // Description:
  // Lock the vtkCriticalSection
  void Lock( void );

  // Description:
  // Unlock the vtkCriticalSection
  void Unlock( void );

protected:
  vtkCritSecType   CritSec;
};

//ETX

class VTK_COMMON_EXPORT vtkCriticalSection : public vtkObject
{
public:
  static vtkCriticalSection *New();

  vtkTypeRevisionMacro(vtkCriticalSection,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Lock the vtkCriticalSection
  void Lock( void );

  // Description:
  // Unlock the vtkCriticalSection
  void Unlock( void );

protected:
  vtkSimpleCriticalSection   SimpleCriticalSection;
  vtkCriticalSection() {};
private:
  vtkCriticalSection(const vtkCriticalSection&);  // Not implemented.
  void operator=(const vtkCriticalSection&);  // Not implemented.
};


inline void vtkCriticalSection::Lock( void )
{
  this->SimpleCriticalSection.Lock();
}

inline void vtkCriticalSection::Unlock( void )
{
  this->SimpleCriticalSection.Unlock();
}

#endif
