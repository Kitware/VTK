/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCriticalSection.cxx
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
#include "vtkCriticalSection.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkCriticalSection* vtkCriticalSection::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkCriticalSection");
  if(ret)
    {
    return (vtkCriticalSection*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkCriticalSection;
}

// New for the SimpleCriticalSection
vtkSimpleCriticalSection *vtkSimpleCriticalSection::New()
{
  return new vtkSimpleCriticalSection;
}

void vtkSimpleCriticalSection::Init()
{
#ifdef VTK_USE_SPROC
  init_lock( &this->CritSec );
#endif
  
#ifdef VTK_USE_WIN32_THREADS
  //this->MutexLock = CreateMutex( NULL, FALSE, NULL ); 
  InitializeCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
#ifdef VTK_HP_PTHREADS
  pthread_mutex_init(&(this->CritSec), pthread_mutexattr_default);
#else
  pthread_mutex_init(&(this->CritSec), NULL);
#endif
#endif
}


// Destruct the vtkMutexVariable
vtkSimpleCriticalSection::~vtkSimpleCriticalSection()
{
#ifdef VTK_USE_WIN32_THREADS
  //CloseHandle(this->MutexLock);
  DeleteCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_destroy( &this->CritSec);
#endif
}

// Lock the vtkCriticalSection
void vtkSimpleCriticalSection::Lock()
{
#ifdef VTK_USE_SPROC
  spin_lock( &this->CritSec );
#endif

#ifdef VTK_USE_WIN32_THREADS
  //WaitForSingleObject( this->MutexLock, INFINITE );
  EnterCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_lock( &this->CritSec);
#endif
}

// Unlock the vtkCriticalSection
void vtkSimpleCriticalSection::Unlock()
{
#ifdef VTK_USE_SPROC
  release_lock( &this->CritSec );
#endif

#ifdef VTK_USE_WIN32_THREADS
  //ReleaseMutex( this->MutexLock );
  LeaveCriticalSection(&this->CritSec);
#endif

#ifdef VTK_USE_PTHREADS
  pthread_mutex_unlock( &this->CritSec);
#endif
}

int vtkSimpleCriticalSection::IsA(const char *type)
{
  if ( !strcmp(this->vtkSimpleCriticalSection::GetClassName(),type) )
    {
    return 1;
    }
  return 0;
}

vtkSimpleCriticalSection *vtkSimpleCriticalSection::SafeDownCast(vtkSimpleCriticalSection *o)
{
  return (vtkSimpleCriticalSection *)o;
}
  
void vtkCriticalSection::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os, indent);
}

