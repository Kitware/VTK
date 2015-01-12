/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutexLock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMutexLock - mutual exclusion locking class
// .SECTION Description
// vtkMutexLock allows the locking of variables which are accessed
// through different threads.  This header file also defines
// vtkSimpleMutexLock which is not a subclass of vtkObject.

#ifndef vtkMutexLock_h
#define vtkMutexLock_h


#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

//BTX

#ifdef VTK_USE_SPROC
#include <abi_mutex.h> // Needed for SPROC implementation of mutex
typedef abilock_t vtkMutexType;
#endif

#if defined(VTK_USE_PTHREADS) || defined(VTK_HP_PTHREADS)
#include <pthread.h> // Needed for PTHREAD implementation of mutex
typedef pthread_mutex_t vtkMutexType;
#endif

#ifdef VTK_USE_WIN32_THREADS
typedef vtkWindowsHANDLE vtkMutexType;
#endif

#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#ifndef VTK_USE_WIN32_THREADS
typedef int vtkMutexType;
#endif
#endif
#endif

// Mutex lock that is not a vtkObject.
class VTKCOMMONCORE_EXPORT vtkSimpleMutexLock
{
public:
  // left public purposely
  vtkSimpleMutexLock();
  virtual ~vtkSimpleMutexLock();

  static vtkSimpleMutexLock *New();

  void Delete() {delete this;}

  // Description:
  // Lock the vtkMutexLock
  void Lock( void );

  // Description:
  // Unlock the vtkMutexLock
  void Unlock( void );

protected:
  friend class vtkSimpleConditionVariable;
  vtkMutexType   MutexLock;

private:
  vtkSimpleMutexLock(const vtkSimpleMutexLock& other); // no copy constructor
  vtkSimpleMutexLock& operator=(const vtkSimpleMutexLock& rhs); // no copy assignment
};

//ETX

class VTKCOMMONCORE_EXPORT vtkMutexLock : public vtkObject
{
public:
  static vtkMutexLock *New();

  vtkTypeMacro(vtkMutexLock,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Lock the vtkMutexLock
  void Lock( void );

  // Description:
  // Unlock the vtkMutexLock
  void Unlock( void );

protected:
  //BTX
  friend class vtkConditionVariable; // needs to get at SimpleMutexLock.
  //ETX

  vtkSimpleMutexLock   SimpleMutexLock;
  vtkMutexLock() {}
private:
  vtkMutexLock(const vtkMutexLock&);  // Not implemented.
  void operator=(const vtkMutexLock&);  // Not implemented.
};


inline void vtkMutexLock::Lock( void )
{
  this->SimpleMutexLock.Lock();
}

inline void vtkMutexLock::Unlock( void )
{
  this->SimpleMutexLock.Unlock();
}

#endif
