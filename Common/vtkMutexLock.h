/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutexLock.h
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
// .NAME vtkMutexLock - mutual exclusion locking class
// .SECTION Description
// vtkMutexLock allows the locking of variables which are accessed 
// through different threads.  This header file also defines 
// vtkSimpleMutexLock which is not a subclass of vtkObject.

#ifndef __vtkMutexVariable_h
#define __vtkMutexVariable_h


#include "vtkObject.h"

//BTX

#ifdef VTK_USE_SPROC
#include <abi_mutex.h>
typedef abilock_t vtkMutexType;
#endif

#ifdef VTK_USE_PTHREADS
#include <pthread.h>
typedef pthread_mutex_t vtkMutexType;
#endif
 
#ifdef VTK_USE_WIN32_THREADS
#include <winbase.h>
typedef HANDLE vtkMutexType;
#endif

#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#ifndef VTK_USE_WIN32_THREADS
typedef int vtkMutexType;
#endif
#endif
#endif

// Mutex lock that is not a vtkObject.
class VTK_COMMON_EXPORT vtkSimpleMutexLock
{
public:
  // left public purposely
  vtkSimpleMutexLock();
  virtual ~vtkSimpleMutexLock();

  static vtkSimpleMutexLock *New();

  virtual const char *GetClassName() {return "vtkSimpleMutexLock";};
  virtual int IsA(const char *name);
  static vtkSimpleMutexLock *SafeDownCast(vtkSimpleMutexLock *o);

  void Delete() {delete this;}
  
  // Description:
  // Lock the vtkMutexLock
  void Lock( void );

  // Description:
  // Unlock the vtkMutexLock
  void Unlock( void );

protected:
  vtkMutexType   MutexLock;
};

//ETX

class VTK_COMMON_EXPORT vtkMutexLock : public vtkObject
{
public:
  static vtkMutexLock *New();

  vtkTypeRevisionMacro(vtkMutexLock,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Lock the vtkMutexLock
  void Lock( void );

  // Description:
  // Unlock the vtkMutexLock
  void Unlock( void );

protected:
  vtkSimpleMutexLock   SimpleMutexLock;
  vtkMutexLock() {};
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
