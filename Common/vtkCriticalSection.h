/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.h
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

  vtkTypeMacro(vtkCriticalSection,vtkObject);
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
