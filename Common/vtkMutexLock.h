/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutexLock.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
 
#ifdef _WIN32
#include <winbase.h>
typedef HANDLE vtkMutexType;
#endif

#ifndef VTK_USE_SPROC
#ifndef VTK_USE_PTHREADS
#ifndef _WIN32
typedef int vtkMutexType;
#endif
#endif
#endif

// Mutex lock that is not a vtkObject.
class VTK_EXPORT vtkSimpleMutexLock
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

class VTK_EXPORT vtkMutexLock : public vtkObject
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
  vtkSimpleMutexLock   SimpleMutexLock;
  vtkMutexLock() {};
  vtkMutexLock(const vtkMutexLock&);
  void operator=(const vtkMutexLock&);
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
